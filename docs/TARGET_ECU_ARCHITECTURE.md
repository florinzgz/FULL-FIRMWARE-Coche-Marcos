# TARGET ECU ARCHITECTURE
# Multi-Node CAN-Based Vehicle Control System

**Document Version:** 1.0  
**Date:** 2026-01-13  
**Status:** 🎯 DESIGN PROPOSAL - NOT IMPLEMENTED

---

## EXECUTIVE SUMMARY

This document defines the target multi-ECU architecture for the electric vehicle control system. The design transitions from a **monolithic ESP32-S3 single-controller** to a **distributed CAN-based multi-ECU system** with clear separation of concerns, improved fault isolation, and enhanced real-time determinism.

**Design Philosophy:**
- **ESP32-S3:** Gateway + UI + High-Level Logic
- **STM32 ECUs:** Real-Time Actuators + Sensors + Local Fail-Safes

---

## 1. SYSTEM ARCHITECTURE OVERVIEW

### 1.1 ECU NODE TOPOLOGY

```
┌─────────────────────────────────────────────────────────────┐
│                    CAN BUS (500 kbps)                       │
│                  Twisted Pair, 120Ω Term                    │
└────┬────────────────┬────────────────┬─────────────────┬────┘
     │                │                │                 │
     ▼                ▼                ▼                 ▼
┌──────────┐    ┌───────────┐   ┌───────────┐   ┌─────────────┐
│  ESP32   │    │   STM32   │   │  Future   │   │   Future    │
│ Gateway  │    │   Motor   │   │  Sensor   │   │  Lighting   │
│   ECU    │    │    ECU    │   │    ECU    │   │     ECU     │
│          │    │           │   │ (Phase 2) │   │  (Phase 2)  │
│  N16R8   │    │  STM32F4  │   │           │   │             │
└──────────┘    └───────────┘   └───────────┘   └─────────────┘
```

**Phase 1 Nodes (Mandatory):**
- **ESP32-S3 Gateway ECU** (ID: 0x01)
- **STM32 Motor ECU** (ID: 0x10)

**Phase 2+ Nodes (Optional):**
- **STM32 Sensor ECU** (ID: 0x20) - Obstacle detection, future sensors
- **STM32 Lighting ECU** (ID: 0x30) - LED control, future lighting features

---

### 1.2 NODE RESPONSIBILITIES

#### **ESP32-S3 GATEWAY ECU (Node ID: 0x01)**

**Primary Role:** System orchestrator, user interface, high-level logic

| Subsystem | Function | Hardware | Bus |
|-----------|----------|----------|-----|
| **HUD/Display** | TFT rendering, compositing | ST7796S 480x320 SPI | SPI |
| **Touchscreen** | User input | XPT2046 SPI | SPI |
| **Audio** | Safety alerts, feedback | DFPlayer Mini | UART1 |
| **Pedal** | Throttle input | Hall sensor ADC | ADC (GPIO 4) |
| **Key Switch** | Power on/off detection | GPIO inputs | GPIO 40, 41 |
| **Main Relay** | Power hold circuit | GPIO output | GPIO 35 |
| **Spare Relay** | Lights/media power | GPIO output | GPIO 46 |
| **CAN Gateway** | Message routing, arbitration | CAN transceiver | CAN bus |
| **Mode Manager** | Vehicle mode logic (FULL/MINIMAL/SAFE) | Software | - |
| **Safety Arbiter** | High-level safety decisions | Software | - |
| **Data Logger** | Telemetry, diagnostics | Serial + future SD card | UART0 |

**Inputs from CAN:**
- Motor status (currents, temps, PWM states)
- Wheel speeds (4x)
- Steering angle
- Shifter position
- Fault codes

**Outputs to CAN:**
- Motor commands (throttle %, steering angle)
- Relay commands (enable/disable)
- Configuration updates
- Emergency stop

**Local Decisions:**
- UI rendering and layout
- Audio playback triggers
- Telemetry formatting
- Diagnostic mode activation

**Power Requirements:**
- 5V @ 1A (USB-C or buck converter from 12V)
- Separate from motor power (isolated)

**Failure Mode:**
- **ESP32 Crash:** Motors enter fail-safe (stop), display goes dark, vehicle inoperable
- **CAN TX Timeout:** Motor ECU detects silence, enters safe state
- **Boot Loop:** Safe mode activates (limited functionality)

---

#### **STM32 MOTOR ECU (Node ID: 0x10)**

**Primary Role:** Real-time motor control, local sensor acquisition, autonomous fail-safes

| Subsystem | Function | Hardware | Bus |
|-----------|----------|----------|-----|
| **Traction Motors** | 4x4 PWM control | 4x BTS7960 drivers | I2C + PWM |
| **Steering Motor** | Servo PWM control | 1x BTS7960 driver | I2C + PWM |
| **PWM Drivers** | 16-channel PWM generation | 3x PCA9685 (0x40, 0x41, 0x42) | I2C |
| **GPIO Expander** | Motor direction control | MCP23017 (0x20) | I2C |
| **Current Sensors** | Over-current protection | 6x INA226 via TCA9548A (0x70) | I2C |
| **Temperature Sensors** | Thermal protection | 4x DS18B20 | OneWire |
| **Wheel Speed Sensors** | Speed calculation, ABS/TCS | 4x inductive sensors | GPIO ISR |
| **Steering Encoder** | Angle feedback | E6B2-CWZ6C 1200PR | GPIO ISR (quadrature) |
| **Shifter** | Gear position | 5-position via MCP23017 | I2C (shared) |
| **Traction Relay** | 24V motor power | GPIO output | GPIO |
| **Steering Relay** | 12V steering power | GPIO output | GPIO |
| **CAN Interface** | Command RX, status TX | CAN transceiver | CAN bus |

**Inputs from CAN:**
- Throttle demand (0-100%)
- Steering angle setpoint (-45° to +45°)
- Relay enable/disable commands
- Emergency stop command

**Outputs to CAN:**
- Motor currents (6 channels)
- Motor temperatures (4 channels)
- Wheel speeds (4 wheels)
- Steering angle (actual)
- Shifter position (P/R/N/D1/D2)
- Fault flags (over-current, over-temp, CAN timeout)

**Local Control (Autonomous):**
- **Over-current protection:** Reduce PWM if current > threshold
- **Over-temp protection:** Reduce power if temp > 80°C, stop if > 100°C
- **CAN timeout fail-safe:** Disable motors if no ESP32 command for 100ms
- **Emergency stop:** Immediate relay cutoff on CAN command or local fault

**Real-Time Loops:**
- **100 Hz:** Motor PWM update, wheel speed calc, CAN TX
- **10 Hz:** Current sensor read, CAN heartbeat
- **1 Hz:** Temperature sensor read

**Power Requirements:**
- 12V @ 500mA (buck converter from 24V battery)
- Separate power domain from ESP32 (isolated)

**Failure Mode:**
- **STM32 Crash:** Watchdog reset (5s timeout), motors disabled during reset
- **CAN RX Timeout:** Enter fail-safe (motors off, relays open)
- **Sensor Failure:** Continue with degraded data (e.g., disable ABS if no wheel speeds)

---

### 1.3 NODE HARDWARE SPECIFICATIONS

#### **ESP32-S3 Gateway ECU**

| Component | Specification | Notes |
|-----------|--------------|-------|
| **MCU** | ESP32-S3-WROOM-2 N16R8 | Dual-core Xtensa LX7 @ 240 MHz |
| **Flash** | 16MB QIO @ 3.3V | Code, assets, logs |
| **PSRAM** | 8MB QSPI @ 3.3V | Frame buffers, sprites |
| **CAN Transceiver** | TJA1050 or MCP2551 | 3.3V ↔ CAN differential |
| **CAN Controller** | TWAI (built-in ESP32-S3) | ISO 11898-1 compliant |
| **Power** | 5V @ 1A (USB-C or 12V buck) | Isolated from motor power |
| **Enclosure** | IP54 rated (splash-proof) | Dashboard mount |

#### **STM32 Motor ECU**

| Component | Specification | Notes |
|-----------|--------------|-------|
| **MCU** | STM32F405RGT6 or STM32F407VGT6 | ARM Cortex-M4 @ 168 MHz, FPU |
| **Flash** | 1MB (MCU internal) | Firmware only |
| **RAM** | 192 KB (MCU internal) | Real-time buffers |
| **CAN Transceiver** | TJA1050 or MCP2551 | 5V ↔ CAN differential |
| **CAN Controller** | bxCAN (built-in STM32F4) | Dual CAN with hardware filters |
| **I2C Bus** | 400 kHz | 3x PCA9685, MCP23017, TCA9548A, 6x INA226 |
| **GPIO** | 16+ for ISRs, relays | Wheel sensors, encoder, relays |
| **Power** | 12V @ 500mA (24V buck) | Isolated from ESP32 |
| **Enclosure** | IP67 rated (waterproof) | Under-hood mount near motors |

**Why STM32F4?**
- ✅ **Hard real-time:** FreeRTOS with preemptive multitasking
- ✅ **FPU:** Floating-point math for control algorithms (ABS/TCS)
- ✅ **Dual CAN:** bxCAN with hardware message filtering
- ✅ **DMA:** Offload I2C/SPI/UART to DMA for determinism
- ✅ **Automotive-grade:** Extended temperature range (-40°C to +85°C)
- ✅ **Cost-effective:** ~$5-10 per unit

---

## 2. CAN BUS SPECIFICATION

### 2.1 PHYSICAL LAYER

| Parameter | Value | Notes |
|-----------|-------|-------|
| **Bitrate** | 500 kbps | Standard automotive CAN |
| **Topology** | Linear bus | Twisted pair, daisy-chain |
| **Cable** | ISO 11898-2 compliant | Twisted pair, shielded |
| **Termination** | 120Ω resistors | One at each bus end |
| **Max Length** | 40 meters @ 500 kbps | Typical vehicle wiring |
| **Voltage Levels** | CAN_H: 3.5V, CAN_L: 1.5V (dominant) | Differential signaling |
| **Connectors** | JST-XH or Molex | Locking connectors |

### 2.2 DATA LINK LAYER

| Parameter | Value | Notes |
|-----------|-------|-------|
| **Frame Format** | CAN 2.0B (Extended ID) | 29-bit identifiers |
| **Error Detection** | CRC-15, ACK, frame check | Hardware CRC |
| **Arbitration** | CSMA/CD | Priority-based (lower ID wins) |
| **Max Payload** | 8 bytes (CAN 2.0) | Standard DLC |

**CAN ID Allocation:**
```
Priority-based allocation (lower ID = higher priority)

0x000-0x0FF: Emergency / Critical Safety
  0x001: Emergency stop (ESP32 → Motor ECU)
  0x002: Motor ECU fault broadcast (Motor ECU → ESP32)

0x100-0x1FF: Motor Commands (ESP32 → Motor ECU)
  0x100: Throttle demand + steering angle
  0x101: Relay control commands
  0x102: Configuration updates

0x200-0x2FF: Motor Status (Motor ECU → ESP32)
  0x200: Motor currents (6 channels)
  0x201: Motor temperatures (4 channels)
  0x202: Wheel speeds (4 wheels)
  0x203: Steering angle (actual)
  0x204: Shifter position

0x300-0x3FF: Diagnostics / Heartbeat
  0x300: ESP32 heartbeat (10 Hz)
  0x301: Motor ECU heartbeat (10 Hz)

0x400-0x4FF: Future Sensor ECU
0x500-0x5FF: Future Lighting ECU
```

### 2.3 MESSAGE DEFINITIONS

#### **0x100: Motor Commands** (ESP32 → Motor ECU, 100 Hz)

| Byte | Field | Range | Units | Notes |
|------|-------|-------|-------|-------|
| 0 | Throttle Demand | 0-200 | % × 2 | 0-100%, multiply by 2 for 0.5% resolution |
| 1 | Steering Angle | 0-180 | deg + 90 | -90° to +90°, offset by 90 |
| 2 | Gear | 0-4 | enum | 0=P, 1=R, 2=N, 3=D1, 4=D2 |
| 3 | Flags | Bitfield | - | Bit 0: Traction enable, Bit 1: Steering enable |
| 4-7 | Reserved | - | - | Future expansion |

#### **0x200: Motor Currents** (Motor ECU → ESP32, 10 Hz)

| Byte | Field | Range | Units | Notes |
|------|-------|-------|-------|-------|
| 0-1 | Motor FL Current | 0-65535 | mA | Unsigned 16-bit |
| 2-3 | Motor FR Current | 0-65535 | mA | Unsigned 16-bit |
| 4-5 | Motor RL Current | 0-65535 | mA | Unsigned 16-bit |
| 6-7 | Motor RR Current | 0-65535 | mA | Unsigned 16-bit |

*Additional message 0x200+1 for steering current and battery current*

#### **0x202: Wheel Speeds** (Motor ECU → ESP32, 100 Hz)

| Byte | Field | Range | Units | Notes |
|------|-------|-------|-------|-------|
| 0-1 | Wheel FL Speed | 0-65535 | mm/s | Unsigned 16-bit |
| 2-3 | Wheel FR Speed | 0-65535 | mm/s | Unsigned 16-bit |
| 4-5 | Wheel RL Speed | 0-65535 | mm/s | Unsigned 16-bit |
| 6-7 | Wheel RR Speed | 0-65535 | mm/s | Unsigned 16-bit |

#### **0x001: Emergency Stop** (Bidirectional, Event-driven)

| Byte | Field | Range | Units | Notes |
|------|-------|-------|-------|-------|
| 0 | Command | 0-1 | bool | 0=Normal, 1=Emergency stop |
| 1 | Source | 0-255 | Node ID | Who triggered emergency |
| 2-7 | Reserved | - | - | Future expansion |

---

## 3. FAIL-SAFE MECHANISMS

### 3.1 CAN TIMEOUT DETECTION

**ESP32 Gateway ECU:**
- **Monitors:** Motor ECU heartbeat (0x301) @ 10 Hz
- **Timeout:** 500ms (5 missed messages)
- **Action:** Display critical error, sound alarm, log fault

**STM32 Motor ECU:**
- **Monitors:** Motor commands (0x100) @ 100 Hz
- **Timeout:** 100ms (10 missed messages)
- **Action:** Enter fail-safe state (motors off, relays open)

### 3.2 FAIL-SAFE STATE MACHINE (Motor ECU)

```
┌─────────────┐
│   STARTUP   │ Initial state after power-on
└──────┬──────┘
       │ CAN init OK + ESP32 handshake
       ▼
┌─────────────┐
│   STANDBY   │ Motors off, waiting for enable command
└──────┬──────┘
       │ Enable command from ESP32
       ▼
┌─────────────┐
│   ACTIVE    │ Normal operation, motors controlled by CAN
└──────┬──────┘
       │ CAN timeout OR over-current OR emergency stop
       ▼
┌─────────────┐
│  FAIL-SAFE  │ Motors off, relays open, CAN fault broadcast
└──────┬──────┘
       │ CAN recovery + fault clear
       ▼
     (back to STANDBY)
```

**State Transitions:**
- **STARTUP → STANDBY:** CAN initialized, ESP32 detected
- **STANDBY → ACTIVE:** Enable command received from ESP32
- **ACTIVE → FAIL-SAFE:** CAN timeout (100ms) OR over-current OR emergency stop
- **FAIL-SAFE → STANDBY:** CAN recovered AND fault cleared AND ESP32 acknowledges

**Fail-Safe State Actions:**
- ✅ All motor PWM → 0%
- ✅ Traction relay → OPEN (24V disconnected)
- ✅ Steering relay → OPEN (12V disconnected)
- ✅ Broadcast fault code on CAN (ID 0x002)
- ✅ Flash status LED (rapid blink)

### 3.3 OVER-CURRENT PROTECTION (Motor ECU Local)

**Thresholds:**
- **Traction motors (FL/FR/RL/RR):** 50A per motor
- **Steering motor:** 50A
- **Battery:** 100A total

**Response:**
1. **Warning Level (80% threshold):** Log warning, reduce PWM by 10%
2. **Critical Level (100% threshold):** Enter fail-safe, broadcast fault
3. **Recovery:** Manual reset via CAN command from ESP32

### 3.4 THERMAL PROTECTION (Motor ECU Local)

**Thresholds:**
- **Warning:** 80°C → Reduce power to 80%
- **Critical:** 90°C → Reduce power to 50%
- **Emergency:** 100°C → Enter fail-safe (motors off)

**Response:**
- Gradual power reduction (10%/s) to avoid abrupt torque loss
- Broadcast temperature warnings on CAN
- Recovery: Automatic when temperature drops below thresholds

---

## 4. POWER DISTRIBUTION AND SEQUENCING

### 4.1 POWER DOMAINS

```
24V Battery
 ├─> Main Relay (ESP32 controlled) → 24V_SYSTEM
 │    ├─> 12V Buck Converter → ESP32 (5V regulator)
 │    ├─> 12V Buck Converter → STM32 Motor ECU
 │    └─> Spare Relay (ESP32 controlled) → Lights/Media
 │
 ├─> Traction Relay (Motor ECU controlled) → 24V_MOTORS
 │    └─> 4x BTS7960 Traction Drivers
 │
 └─> Steering Relay (Motor ECU controlled) → 12V_STEERING
      └─> 1x BTS7960 Steering Driver
```

### 4.2 STARTUP SEQUENCE

```
1. KEY ON (GPIO 40 LOW on ESP32)
   └─> ESP32 boots (hardware power-on reset)

2. ESP32 Setup Phase
   ├─> Initialize core systems (watchdog, logger, storage)
   ├─> Assert Main Relay (GPIO 35 HIGH) → Power hold
   ├─> Initialize CAN bus (500 kbps)
   └─> Wait for Motor ECU heartbeat (timeout 5s)

3. Motor ECU Setup Phase (parallel with ESP32)
   ├─> STM32 boots (hardware power-on reset)
   ├─> Initialize I2C devices (PCA9685, MCP23017, INA226)
   ├─> Initialize sensors (wheels, encoder, temperature)
   ├─> Initialize CAN bus (500 kbps)
   └─> Send heartbeat to ESP32 (ID 0x301)

4. Handshake Phase
   ├─> ESP32 receives Motor ECU heartbeat → Send config (ID 0x102)
   ├─> Motor ECU ACKs config → Enter STANDBY state
   └─> ESP32 displays "Ready" on HUD

5. Enable Phase (user action)
   ├─> User selects gear (P/R/N/D1/D2) via shifter
   ├─> ESP32 reads pedal, sends enable command (ID 0x100)
   └─> Motor ECU transitions STANDBY → ACTIVE
```

**Total startup time:** ~3-5 seconds

### 4.3 SHUTDOWN SEQUENCE

```
1. KEY OFF (GPIO 41 LOW on ESP32) OR Emergency Stop
   └─> ESP32 detects shutdown request

2. ESP32 Shutdown Phase
   ├─> Send emergency stop command (ID 0x001)
   ├─> Wait for Motor ECU ACK (timeout 100ms)
   ├─> Stop audio playback
   ├─> Save configuration to NVS
   └─> Display "Shutting down..." on HUD

3. Motor ECU Shutdown Phase
   ├─> Receive emergency stop (ID 0x001)
   ├─> Enter FAIL-SAFE state (motors off, relays open)
   ├─> Send final status (ID 0x002)
   └─> Remain powered (waiting for power-off)

4. Power-Off Phase
   ├─> ESP32 releases Main Relay (GPIO 35 LOW, 100ms delay)
   └─> 24V_SYSTEM power lost → ESP32 and Motor ECU shut down
```

**Total shutdown time:** <1 second

---

## 5. DIAGNOSTICS AND MONITORING

### 5.1 BUILT-IN DIAGNOSTICS

**ESP32 Gateway ECU:**
- CAN bus health (TX/RX errors, message counts)
- Node liveness (heartbeat monitoring)
- Display frame rate (FPS counter)
- Heap usage (free memory)
- Boot counter (bootloop detection)

**STM32 Motor ECU:**
- CAN bus health (TX/RX errors, timeout count)
- I2C device status (PCA9685, MCP23017, INA226)
- Sensor health (wheels, encoder, temperature)
- Fault counters (over-current, over-temp, CAN timeout)
- Watchdog reset count

### 5.2 FAULT CODES

| Code | Source | Meaning | Severity | Action |
|------|--------|---------|----------|--------|
| **0x01** | Motor ECU | CAN timeout (no ESP32 commands) | 🔴 Critical | Enter fail-safe |
| **0x02** | Motor ECU | Over-current detected | 🔴 Critical | Enter fail-safe |
| **0x03** | Motor ECU | Over-temperature detected | 🟠 High | Reduce power |
| **0x04** | Motor ECU | I2C bus failure | 🟠 High | Retry, log error |
| **0x05** | Motor ECU | Sensor failure (wheels, encoder) | 🟡 Medium | Continue degraded |
| **0x10** | ESP32 | Motor ECU not responding | 🔴 Critical | Display error |
| **0x11** | ESP32 | CAN bus overrun | 🟡 Medium | Log warning |
| **0x12** | ESP32 | Pedal sensor failure | 🟠 High | Enter limp mode |

### 5.3 REMOTE DIAGNOSTICS (Future)

**CAN Logger Interface:**
- USB-CAN adapter (e.g., PCAN-USB, CANable)
- Real-time CAN bus monitoring
- Message decoding and visualization
- Fault code readout

**Diagnostic Commands (ID 0x7DF - UDS standard):**
- Read fault codes (DTC)
- Clear fault codes
- Read sensor data (live data)
- Actuator tests (motor PWM, relay toggle)

---

## 6. SAFETY REDUNDANCY

### 6.1 INDEPENDENT WATCHDOGS

**ESP32 Gateway ECU:**
- **Hardware Watchdog:** 30s timeout (ESP-IDF built-in)
- **Feed Location:** Main loop (100 Hz)
- **Reset Action:** ESP32 resets, Main Relay holds (motor ECU continues)

**STM32 Motor ECU:**
- **Hardware Watchdog:** 5s timeout (IWDG)
- **Feed Location:** Main loop (100 Hz)
- **Reset Action:** Motors disabled, relays open, STM32 resets

**Benefit:** Independent watchdogs prevent both ECUs from hanging simultaneously.

### 6.2 DUAL-CHANNEL CURRENT MONITORING

**Primary:** INA226 sensors on Motor ECU (I2C, 10 Hz)  
**Secondary:** BTS7960 current sense pins (ADC, 100 Hz)

**Cross-Check:** Compare INA226 vs. BTS7960 current readings  
**Discrepancy Action:** If difference > 20%, log fault, prefer INA226 value

### 6.3 EMERGENCY STOP PATHS

**Path 1: ESP32 Software Command**
```
ESP32 detects fault (e.g., obstacle critical)
 └─> Send emergency stop (ID 0x001)
 └─> Motor ECU receives, enters fail-safe (<10ms)
```

**Path 2: Motor ECU Local Detection**
```
Motor ECU detects fault (e.g., over-current)
 └─> Enter fail-safe immediately (<1ms)
 └─> Broadcast fault (ID 0x002) to ESP32
```

**Path 3: CAN Timeout (Automatic)**
```
Motor ECU loses ESP32 commands (100ms timeout)
 └─> Enter fail-safe automatically
 └─> No external command required
```

**Benefit:** Triple-redundant emergency stop (software, local, timeout).

---

## 7. EXPANSION AND FUTURE-PROOFING

### 7.1 PHASE 2: SENSOR ECU (Optional)

**Purpose:** Offload obstacle detection and future advanced sensors

| Subsystem | Hardware | Bus |
|-----------|----------|-----|
| **Obstacle Detection** | TOFSense-M S LiDAR | UART |
| **Future: Radar** | 77 GHz FMCW radar | SPI |
| **Future: Camera** | OV2640 or similar | DCMI |
| **CAN Interface** | CAN transceiver | CAN bus (ID 0x400) |

**Benefits:**
- Frees ESP32 UART0
- Enables sensor fusion (LiDAR + radar + camera)
- Isolated failure domain (sensor crash doesn't affect motors)

### 7.2 PHASE 2: LIGHTING ECU (Optional)

**Purpose:** Advanced lighting features (RGB underglow, dynamic turn signals)

| Subsystem | Hardware | Bus |
|-----------|----------|-----|
| **LED Strips** | WS2812B 28+16 LEDs | Data |
| **Future: Matrix LEDs** | APA102 or SK9822 | SPI |
| **CAN Interface** | CAN transceiver | CAN bus (ID 0x500) |

**Benefits:**
- Frees ESP32 GPIOs (19, 48)
- Offloads FastLED processing
- Enables complex lighting animations

### 7.3 CAN BUS SCALABILITY

**Current Design:** 500 kbps, 12% utilization (Phase 1)  
**Headroom:** 88% available for future nodes

**Max Nodes:** 110 (CAN 2.0B addressing limit, practical ~20 nodes)

**Future Nodes:**
- Battery Management System (BMS) ECU
- Tire Pressure Monitoring System (TPMS) ECU
- Infotainment ECU (Bluetooth, GPS, Wi-Fi)
- Instrument Cluster ECU (separate from main HUD)

---

## 8. COMPARISON: CURRENT vs. TARGET

| Aspect | Current (Monolithic ESP32) | Target (Multi-ECU) |
|--------|----------------------------|-------------------|
| **Architecture** | Single ESP32 controls everything | ESP32 Gateway + STM32 Motor ECU |
| **I2C Devices (ESP32)** | 10 devices (3 PCA9685 + MCP23017 + TCA9548A + 6 INA226) | 0 devices ✅ |
| **GPIO Usage (ESP32)** | 34/36 (94% full) | 10/36 (28% free) ✅ |
| **Real-Time Guarantees** | ❌ No (cooperative multitasking) | ✅ Yes (STM32 FreeRTOS) |
| **Fault Isolation** | ❌ ESP32 crash = total failure | ✅ ESP32 crash = motors fail-safe |
| **Motor Control Latency** | 10ms (I2C in main loop) | 11.2ms (CAN + STM32) ⚠️ +12% |
| **Safety Redundancy** | ❌ Single watchdog | ✅ Dual watchdogs + local fail-safes |
| **Bus Contention** | ❌ I2C shared by motors + sensors + display | ✅ Isolated buses per ECU |
| **Expansion** | ❌ 2 GPIOs left, I2C full | ✅ CAN bus, 88% bandwidth free |
| **Diagnostics** | 🟡 Serial logs only | ✅ CAN bus, fault codes, remote access |
| **Complexity** | 🟢 Low (single MCU) | 🟠 Medium (2 MCUs, CAN protocol) |
| **Cost** | 🟢 Low (~$30 BOM) | 🟡 Medium (~$50 BOM, +STM32 + CAN) |

**Verdict:** Target architecture trades slight latency increase (+1.2ms) and complexity for **massive gains** in fault isolation, determinism, and scalability.

---

## CONCLUSION

The target multi-ECU architecture transforms the vehicle from a fragile monolithic system into a robust, fault-tolerant distributed system. The design separates high-level logic (ESP32) from real-time control (STM32), enabling:

✅ **Fault Isolation:** ESP32 crash doesn't freeze motors  
✅ **Determinism:** Hard real-time motor control on STM32  
✅ **Scalability:** CAN bus supports future expansion (sensors, lighting, BMS)  
✅ **Safety:** Triple-redundant emergency stop, dual watchdogs  
✅ **Maintainability:** Clear separation of concerns, modular design  

**Next Steps:** See MIGRATION_PHASES.md for step-by-step implementation plan.

---

**Document Authority:** Automotive Systems Architect  
**Review Status:** ✅ Design proposal complete  
**Confidentiality:** Internal Use Only
