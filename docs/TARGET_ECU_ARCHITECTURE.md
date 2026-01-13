# TARGET ECU ARCHITECTURE

**Document Version:** 1.0  
**Firmware Version:** v2.17.1 (PHASE 14)  
**Architecture Type:** Distributed Multi-ECU with CAN Gateway  
**Date:** 2026-01-13

---

## EXECUTIVE SUMMARY

This document defines the target architecture for migrating from a monolithic ESP32-S3 system to a distributed multi-ECU architecture. The design prioritizes safety, maintainability, and graceful degradation while leveraging the strengths of both ESP32-S3 (display, connectivity, coordination) and STM32 microcontrollers (real-time motor control, sensor acquisition).

**Target Configuration:**
- **1x ESP32-S3:** Gateway + UI + System Coordinator
- **1x STM32F4:** Motor Control ECU (Traction + Steering + ABS/TCS)
- **1x STM32F1:** Sensor Acquisition ECU (Current + Temperature + Obstacle)
- **Optional 1x STM32G0:** Lighting/Auxiliary ECU

**CAN Bus:** 500 kbps, dual-redundant (optional)

---

## 1. SYSTEM ARCHITECTURE OVERVIEW

### 1.1 Multi-ECU Topology

```
                           ┌─────────────────────────┐
                           │    ESP32-S3 GATEWAY     │
                           │   + UI + COORDINATOR    │
                           │                         │
                           │  • Power Management     │
                           │  • TFT Display (480×320)│
                           │  • Touch Interface      │
                           │  • Audio (DFPlayer)     │
                           │  • Configuration        │
                           │  • Telemetry Logging    │
                           │  • CAN Gateway          │
                           │  • System Orchestration │
                           └────────┬───────┬────────┘
                                    │       │
                          ┌─────────┴───┐   │
                          │  CAN BUS    │───┘
                          │  500 kbps   │
                          └──┬─────┬────┴─────┬───────┐
                             │     │          │       │
              ┌──────────────┘     │          │       └────────────────┐
              │                    │          │                        │
    ┌─────────▼──────────┐  ┌──────▼──────┐  │          ┌─────────────▼────────┐
    │  STM32F4 (Motor)   │  │ STM32F1     │  │          │  STM32G0 (Optional)  │
    │   CONTROL ECU      │  │ (Sensor)    │  │          │   LIGHTING ECU       │
    │                    │  │  ECU        │  │          │                      │
    │ • Traction (4WD)   │  │             │  │          │ • WS2812B Front LEDs │
    │ • Steering         │  │ • 6x INA226 │  │          │ • WS2812B Rear LEDs  │
    │ • Wheel ISRs (4x)  │  │ • 4x DS18B20│  │          │ • Turn Signals       │
    │ • ABS System       │  │ • TOFSense  │  │          │ • Brake Lights       │
    │ • TCS System       │  │             │  │          │                      │
    │ • PCA9685 (3x)     │  │             │  │          │                      │
    │ • MCP23017 (1x)    │  │             │  │          │                      │
    └────────────────────┘  └─────────────┘  │          └──────────────────────┘
            │                      │          │                     │
            │                      │          │                     │
    ┌───────▼──────┐       ┌───────▼─────┐   │            ┌────────▼────────┐
    │ BTS7960 (5x) │       │ I2C Devices │   │            │  LED Strips     │
    │ Motor        │       │ TCA9548A    │   │            │  44 LEDs total  │
    │ Drivers      │       │ Multiplexer │   │            │                 │
    └──────┬───────┘       └─────────────┘   │            └─────────────────┘
           │                                  │
    ┌──────▼──────────────────────────┐      │
    │ • 4x RS775 Traction Motors      │      │
    │ • 1x RS390 Steering Motor       │      │
    │ • 1x E6B2 Encoder (1200 PPR)    │      │
    └─────────────────────────────────┘      │
                                             │
                                    ┌────────▼────────┐
                                    │  12V/24V Power  │
                                    │  Distribution   │
                                    │  (ESP32 Relays) │
                                    └─────────────────┘
```

---

## 2. ECU DETAILED SPECIFICATIONS

### 2.1 ESP32-S3 Gateway ECU

**Hardware:**
- **MCU:** ESP32-S3-WROOM-2 N16R8
- **Clock:** Dual-core Xtensa LX7 @ 240 MHz
- **RAM:** 512 KB internal + 8 MB PSRAM
- **Flash:** 16 MB
- **CAN:** External MCP2515 or SN65HVD230 transceiver

**Responsibilities:**

1. **Power Management (CRITICAL)**
   - Relay control (4x SRD-05VDC)
   - Ignition detection (GPIO 40/41)
   - Graceful shutdown sequence
   - Power hold logic

2. **User Interface**
   - TFT Display (ST7796S 480×320 @ SPI 40 MHz)
   - Touch interface (XPT2046 @ SPI 2.5 MHz)
   - HUD rendering (layered compositor)
   - Menu systems

3. **Audio**
   - DFPlayer Mini (UART1 @ 9600 baud)
   - Alert sounds
   - User feedback

4. **System Coordination**
   - CAN gateway/router
   - Mode management (P/R/N/D1/D2)
   - Telemetry aggregation
   - Data logging
   - Configuration storage (EEPROM)

5. **Safety Monitoring**
   - Watchdog (hardware + software)
   - Boot counter (bootloop detection)
   - Error aggregation
   - Limp mode coordination

**CAN Messages Transmitted:**
- Traction command (100 Hz)
- Steering command (100 Hz)
- LED command (20 Hz)
- System mode (10 Hz)
- Heartbeat (10 Hz)

**CAN Messages Received:**
- Motor feedback (100 Hz)
- Sensor telemetry (20 Hz)
- Safety status (100 Hz)
- ECU heartbeats (10 Hz)

**Failsafe Behavior:**
- If CAN fails: Enter safe mode, alert driver, attempt recovery
- If display fails: Continue operation, use audio/LED feedback
- If any ECU heartbeat lost: Disable affected subsystem, limp mode

**Interfaces:**
| Peripheral | Interface | Usage |
|------------|-----------|-------|
| TFT Display | SPI (HSPI) | 480×320 ST7796S |
| Touch | SPI (shared) | XPT2046 resistive |
| CAN Bus | SPI + GPIO | MCP2515 transceiver |
| Audio | UART1 | DFPlayer Mini |
| Power Relays | GPIO (4x) | SRD-05VDC control |
| Ignition | GPIO (2x) | ON/OFF detection |
| Status LEDs | GPIO (optional) | Debugging |

---

### 2.2 STM32F4 Motor Control ECU

**Hardware:**
- **MCU:** STM32F407VGT6 (recommended)
- **Clock:** ARM Cortex-M4 @ 168 MHz
- **RAM:** 192 KB SRAM
- **Flash:** 1 MB
- **FPU:** Yes (critical for ABS/TCS math)
- **CAN:** Built-in CAN peripheral (bxCAN)
- **Timers:** Advanced timers for PWM

**Responsibilities:**

1. **Traction Motor Control (4WD)**
   - 2x PCA9685 PWM controllers (I2C 0x40, 0x41)
   - 8 channels PWM @ 1 kHz, 12-bit
   - MCP23017 direction control (I2C 0x20, 8 pins)
   - 4x BTS7960 H-bridge drivers
   - Current limiting per wheel
   - Differential throttle (for TCS)

2. **Steering Motor Control**
   - 1x PCA9685 PWM controller (I2C 0x42)
   - 2 channels PWM @ 1 kHz
   - MCP23017 direction control (2 pins)
   - 1x BTS7960 H-bridge driver
   - Position PID control loop (100 Hz)
   - Current limiting (30A max)

3. **Encoder Processing**
   - E6B2-CWZ6C quadrature encoder (1200 PPR)
   - Hardware encoder interface (TIM2 encoder mode)
   - A/B/Z signal processing
   - Real-time position tracking
   - Center detection

4. **Wheel Speed Sensing**
   - 4x inductive sensor inputs
   - GPIO interrupts → hardware timer capture
   - Pulse counting per wheel
   - Speed calculation (100 Hz)

5. **ABS System**
   - Wheel slip detection
   - Individual wheel intervention
   - Pressure modulation via PWM reduction
   - 10 Hz cycle rate (100ms pulses)

6. **TCS System**
   - Wheel spin detection
   - Differential throttle reduction
   - Per-wheel intervention
   - Integration with traction control

**CAN Messages Transmitted:**
- Motor feedback (100 Hz)
  - 4x wheel currents
  - 1x steering current
  - 4x wheel speeds
  - Steering position
  - Status flags
- Safety status (100 Hz)
  - ABS active (per wheel)
  - TCS active (per wheel)
  - Slip ratios
  - Fault codes
- Heartbeat (10 Hz)

**CAN Messages Received:**
- Traction command (100 Hz)
  - 4x throttle values
  - Brake enable
  - Direction
- Steering command (100 Hz)
  - Target position
  - Max speed
- System mode (10 Hz)
- ESP32 heartbeat (10 Hz)

**Failsafe Behavior:**
```
CAN Heartbeat Lost (100ms timeout):
1. Gradual deceleration (5-second ramp to zero)
2. Motors off (PWM = 0, all channels)
3. Steering center (PID target = 0)
4. ABS/TCS disabled
5. Status LED: rapid flash
6. Wait for CAN recovery or power cycle
```

**I2C Bus (Local):**
| Device | Address | Purpose |
|--------|---------|---------|
| PCA9685 #1 | 0x40 | Front traction motors |
| PCA9685 #2 | 0x41 | Rear traction motors |
| PCA9685 #3 | 0x42 | Steering motor |
| MCP23017 | 0x20 | Direction control (all motors) |

**GPIO Allocation:**
- 4x Wheel sensor inputs (TIM3/4 channels)
- 3x Encoder inputs (TIM2 encoder mode)
- 2x I2C (SDA/SCL)
- 2x CAN (CAN1 TX/RX)
- 1x Status LED
- Reserve: 2-3 GPIOs for expansion

**Critical Timing:**
- ABS/TCS loop: 10ms (100 Hz)
- Position control: 10ms (100 Hz)
- Encoder update: ISR-driven (<1µs)
- CAN TX: 10ms (100 Hz)

---

### 2.3 STM32F1 Sensor Acquisition ECU

**Hardware:**
- **MCU:** STM32F103C8T6 (Blue Pill) or STM32F103RCT6
- **Clock:** ARM Cortex-M3 @ 72 MHz
- **RAM:** 20 KB SRAM (C8) or 48 KB (RC)
- **Flash:** 64 KB (C8) or 256 KB (RC)
- **CAN:** Built-in CAN peripheral

**Responsibilities:**

1. **Current Sensing (6x INA226)**
   - TCA9548A I2C multiplexer (0x70)
   - 6 channels: 4x motors + 1x battery + 1x steering
   - 20 Hz update rate (50ms per scan cycle)
   - Overcurrent detection
   - Power calculation

2. **Temperature Sensing (4x DS18B20)**
   - OneWire protocol
   - 4 sensors: one per traction motor
   - 1 Hz update rate
   - Thermal protection thresholds
   - Blocking read (750ms per sensor, sequential)

3. **Obstacle Detection (TOFSense-M S)**
   - UART @ 115200 baud
   - 8×8 matrix LiDAR (64 points)
   - 15 Hz update rate (sensor-limited)
   - Zone extraction (front/left/right/rear)
   - Distance filtering

**CAN Messages Transmitted:**
- Sensor telemetry (20 Hz)
  - 6x current (A)
  - 6x voltage (V)
  - 6x power (W)
  - 4x temperature (°C)
  - Battery SOC estimate
- Obstacle data (15 Hz)
  - 4x zone minimum distance (mm)
  - 4x zone status (clear/warning/danger)
  - Matrix summary
- Heartbeat (10 Hz)

**CAN Messages Received:**
- ESP32 heartbeat (10 Hz)
- Configuration updates (rare)

**Failsafe Behavior:**
```
CAN Heartbeat Lost (100ms timeout):
1. Continue sensor acquisition (autonomous)
2. Log error internally
3. Buffer data (circular buffer, 10 samples)
4. Resume CAN transmission on recovery
5. Status LED: slow flash (degraded mode)
```

**I2C Bus (Local):**
| Device | Address | Purpose |
|--------|---------|---------|
| TCA9548A | 0x70 | INA226 multiplexer |
| INA226 (×6) | 0x40 | Current sensors (via TCA channels 0-5) |

**UART:**
- UART1: TOFSense-M S (RX only, 115200 baud)

**OneWire:**
- GPIO: DS18B20 temperature bus

**GPIO Allocation:**
- 2x I2C (SDA/SCL)
- 2x CAN (CAN1 TX/RX)
- 2x UART (RX/TX, though TX unused for LiDAR)
- 1x OneWire (temperature)
- 1x Status LED

**Critical Timing:**
- Current scan: 50ms (20 Hz)
- Temperature read: 1 Hz (non-critical, blocking OK)
- Obstacle processing: 15 Hz (sensor-driven)
- CAN TX: 20 Hz (telemetry)

---

### 2.4 STM32G0 Lighting ECU (Optional)

**Hardware:**
- **MCU:** STM32G071KBU6
- **Clock:** ARM Cortex-M0+ @ 64 MHz
- **RAM:** 36 KB SRAM
- **Flash:** 128 KB
- **CAN:** Built-in FDCAN

**Responsibilities:**

1. **LED Strip Control (WS2812B)**
   - Front LEDs: 28 pixels (GPIO + DMA + Timer)
   - Rear LEDs: 16 pixels (GPIO + DMA + Timer)
   - Effects: KITT scanner, rainbow, throttle gradient
   - Update rate: 20 Hz

2. **Safety Lighting**
   - Turn signals (legal compliance: 0.5-2 Hz)
   - Brake lights (<200ms activation)
   - Reverse lights
   - Running lights
   - Hazard lights

**CAN Messages Received:**
- LED command (20 Hz)
  - Front mode
  - Rear mode
  - Turn signal
  - Brightness
  - Throttle percentage (for gradient effect)

**CAN Messages Transmitted:**
- Heartbeat (10 Hz)
- Status (10 Hz)

**Failsafe Behavior:**
```
CAN Heartbeat Lost (100ms timeout):
1. Freeze in last commanded state
2. OR: Safe lighting mode (running lights only)
3. Turn signals continue flashing if active
4. Status LED: rapid flash
```

**GPIO Allocation:**
- 2x WS2812B data (DMA + Timer)
- 2x CAN (FDCAN TX/RX)
- 1x Status LED

**Why Optional:**
- LEDs can remain on ESP32 (low priority offload)
- Adds cost and complexity
- Consider only if ESP32 CPU overloaded

---

## 3. CAN BUS PROTOCOL

### 3.1 Bus Configuration

**Physical Layer:**
- **Standard:** CAN 2.0A (11-bit identifiers)
- **Speed:** 500 kbps
- **Transceiver:** SN65HVD230 or TJA1050 (3.3V)
- **Termination:** 120Ω at each end
- **Topology:** Linear bus, stub length <30cm

**Optional: Dual-Redundant CAN:**
- CAN1: Primary (500 kbps)
- CAN2: Backup (500 kbps)
- Automatic failover on CAN1 error
- Increases reliability, doubles cost

### 3.2 Message ID Allocation

**CAN ID Structure (11-bit):**
```
Bits [10:8] - Priority (0=highest, 7=lowest)
Bits [7:4]  - Source ECU
Bits [3:0]  - Message Type
```

**ECU IDs:**
- 0x0: ESP32 Gateway
- 0x1: STM32F4 Motor Control
- 0x2: STM32F1 Sensor
- 0x3: STM32G0 Lighting (if present)

**Priority Levels:**
- 0: Emergency stop, safety-critical commands
- 1: High-frequency control (100 Hz)
- 2: Sensor telemetry (20 Hz)
- 3: Status/heartbeat (10 Hz)
- 4-7: Low priority (configuration, diagnostics)

### 3.3 Message Definitions

#### 3.3.1 Traction Command (ESP32 → Motor ECU)

**CAN ID:** 0x010 (Priority 0, ESP32 source, Type 0)  
**DLC:** 6 bytes  
**Rate:** 100 Hz  
**Timeout:** 100ms

| Byte | Field | Range | Unit |
|------|-------|-------|------|
| 0 | Throttle FL | 0-200 | % (×2 for resolution) |
| 1 | Throttle FR | 0-200 | % |
| 2 | Throttle RL | 0-200 | % |
| 3 | Throttle RR | 0-200 | % |
| 4 | Brake Enable | 0/1 | Boolean |
| 5 | Direction | 0/1/2 | 0=FWD, 1=REV, 2=NEUTRAL |

**Failsafe:** If not received for 100ms, Motor ECU enters safe stop.

---

#### 3.3.2 Steering Command (ESP32 → Motor ECU)

**CAN ID:** 0x011 (Priority 0, ESP32 source, Type 1)  
**DLC:** 3 bytes  
**Rate:** 100 Hz  
**Timeout:** 100ms

| Byte | Field | Range | Unit |
|------|-------|-------|------|
| 0-1 | Target Position | -32768 to +32767 | Encoder counts |
| 2 | Max Speed | 0-100 | % of max RPM |

**Failsafe:** If not received for 100ms, center steering (position = 0).

---

#### 3.3.3 Motor Feedback (Motor ECU → ESP32)

**CAN ID:** 0x110 (Priority 1, Motor source, Type 0)  
**DLC:** 8 bytes  
**Rate:** 100 Hz

| Byte | Field | Range | Unit |
|------|-------|-------|------|
| 0 | Current FL | 0-250 | A ÷ 2 |
| 1 | Current FR | 0-250 | A ÷ 2 |
| 2 | Current RL | 0-250 | A ÷ 2 |
| 3 | Current RR | 0-250 | A ÷ 2 |
| 4 | Steering Current | 0-250 | A ÷ 2 (max 125A = 62.5A) |
| 5-6 | Steering Position | -32768 to +32767 | Encoder counts |
| 7 | Status | Bitfield | See below |

**Status Byte (Byte 7):**
```
Bit 0: Motors enabled
Bit 1: Steering enabled
Bit 2: Emergency stop active
Bit 3: Overcurrent fault
Bit 4-7: Reserved
```

---

#### 3.3.4 Wheel Speed Data (Motor ECU → ESP32)

**CAN ID:** 0x111 (Priority 1, Motor source, Type 1)  
**DLC:** 8 bytes  
**Rate:** 100 Hz

| Byte | Field | Range | Unit |
|------|-------|-------|------|
| 0-1 | Speed FL | 0-65535 | mm/s |
| 2-3 | Speed FR | 0-65535 | mm/s |
| 4-5 | Speed RL | 0-65535 | mm/s |
| 6-7 | Speed RR | 0-65535 | mm/s |

---

#### 3.3.5 Safety Status (Motor ECU → ESP32)

**CAN ID:** 0x112 (Priority 1, Motor source, Type 2)  
**DLC:** 5 bytes  
**Rate:** 100 Hz

| Byte | Field | Range | Unit |
|------|-------|-------|------|
| 0 | ABS Active Flags | Bitfield | Bit 0-3: FL/FR/RL/RR |
| 1 | TCS Active Flags | Bitfield | Bit 0-3: FL/FR/RL/RR |
| 2 | Slip Ratio FL | 0-200 | % slip ÷ 2 |
| 3 | Slip Ratio FR | 0-200 | % slip ÷ 2 |
| 4 | Avg Slip Ratio | 0-200 | % slip ÷ 2 |

---

#### 3.3.6 Sensor Telemetry (Sensor ECU → ESP32)

**CAN ID:** 0x220 (Priority 2, Sensor source, Type 0)  
**DLC:** 8 bytes  
**Rate:** 20 Hz  
**Sequence:** Multi-frame message (5 frames total)

**Frame 1: Traction Current**

| Byte | Field | Range | Unit |
|------|-------|-------|------|
| 0 | Current FL | 0-250 | A ÷ 2 |
| 1 | Current FR | 0-250 | A ÷ 2 |
| 2 | Current RL | 0-250 | A ÷ 2 |
| 3 | Current RR | 0-250 | A ÷ 2 |
| 4 | Voltage FL | 0-250 | V ÷ 10 |
| 5 | Voltage FR | 0-250 | V ÷ 10 |
| 6 | Voltage RL | 0-250 | V ÷ 10 |
| 7 | Voltage RR | 0-250 | V ÷ 10 |

**Frame 2: Battery + Steering**

| Byte | Field | Range | Unit |
|------|-------|-------|------|
| 0-1 | Battery Current | 0-65535 | A × 100 (0.01A res) |
| 2-3 | Battery Voltage | 0-65535 | V × 100 (0.01V res) |
| 4-5 | Battery Power | 0-65535 | W |
| 6 | Steering Current | 0-250 | A ÷ 2 |
| 7 | Battery SOC | 0-100 | % |

**Frame 3: Temperatures**

| Byte | Field | Range | Unit |
|------|-------|-------|------|
| 0 | Temp Motor FL | 0-255 | °C (offset -40) |
| 1 | Temp Motor FR | 0-255 | °C (offset -40) |
| 2 | Temp Motor RL | 0-255 | °C (offset -40) |
| 3 | Temp Motor RR | 0-255 | °C (offset -40) |
| 4-7 | Reserved | - | - |

**Frame 4: Obstacle Zones**

| Byte | Field | Range | Unit |
|------|-------|-------|------|
| 0-1 | Front Min Distance | 0-4000 | mm |
| 2-3 | Left Min Distance | 0-4000 | mm |
| 4-5 | Right Min Distance | 0-4000 | mm |
| 6-7 | Rear Min Distance | 0-4000 | mm |

**Frame 5: Obstacle Status**

| Byte | Field | Range | Unit |
|------|-------|-------|------|
| 0 | Front Status | 0-2 | 0=Clear, 1=Warn, 2=Danger |
| 1 | Left Status | 0-2 | 0=Clear, 1=Warn, 2=Danger |
| 2 | Right Status | 0-2 | 0=Clear, 1=Warn, 2=Danger |
| 3 | Rear Status | 0-2 | 0=Clear, 1=Warn, 2=Danger |
| 4-7 | Reserved | - | - |

---

#### 3.3.7 LED Command (ESP32 → Lighting ECU)

**CAN ID:** 0x030 (Priority 0, ESP32 source, Type 0 for Lighting)  
**DLC:** 4 bytes  
**Rate:** 20 Hz

| Byte | Field | Range | Unit |
|------|-------|-------|------|
| 0 | Front Mode | 0-10 | See enum below |
| 1 | Rear Mode | 0-10 | See enum below |
| 2 | Turn Signal | 0-2 | 0=Off, 1=Left, 2=Right |
| 3 | Brightness | 0-255 | PWM value |

**Front/Rear Mode Enum:**
- 0: OFF
- 1: Running lights
- 2: Brake lights
- 3: Reverse lights
- 4: KITT scanner
- 5: Rainbow
- 6: Throttle gradient
- 7-10: Reserved

---

#### 3.3.8 Heartbeat Messages

**All ECUs transmit heartbeat @ 10 Hz**

**CAN IDs:**
- 0x300: ESP32 heartbeat
- 0x310: Motor ECU heartbeat
- 0x320: Sensor ECU heartbeat
- 0x330: Lighting ECU heartbeat (if present)

**DLC:** 2 bytes  
**Rate:** 10 Hz

| Byte | Field | Range | Unit |
|------|-------|-------|------|
| 0 | Sequence Counter | 0-255 | Increments each message |
| 1 | Health Status | 0-255 | 0=OK, 1=Degraded, 2=Error |

**Timeout Detection:**
- If heartbeat not received for 100ms → ECU assumed offline
- Trigger failsafe on receiving ECU

---

### 3.4 CAN Bus Load Calculation

**Total Message Load:**

| Message | Rate | DLC | Overhead | Total Bits | Bandwidth |
|---------|------|-----|----------|------------|-----------|
| Traction Cmd | 100 Hz | 6 | 47 | 95 | 9.5 kbps |
| Steering Cmd | 100 Hz | 3 | 47 | 71 | 7.1 kbps |
| Motor Feedback | 100 Hz | 8 | 47 | 111 | 11.1 kbps |
| Wheel Speeds | 100 Hz | 8 | 47 | 111 | 11.1 kbps |
| Safety Status | 100 Hz | 5 | 47 | 87 | 8.7 kbps |
| Sensor Frame 1-5 | 20 Hz | 8 | 47 | 111 | 11.1 kbps |
| LED Command | 20 Hz | 4 | 47 | 79 | 1.58 kbps |
| Heartbeats (4x) | 10 Hz | 2 | 47 | 63 | 2.52 kbps |

**Total: ~62.6 kbps @ 500 kbps = 12.5% utilization**

**Margin:** 87.5% available for diagnostics, configuration, future expansion

**Worst-case latency:** ~10 messages queued × 0.26 ms = 2.6 ms

---

## 4. FAILSAFE MECHANISMS

### 4.1 CAN Bus Failure Detection

**Method:**
- Heartbeat monitoring (100ms timeout)
- CAN error counters (hardware)
- Bus-off state detection

**ESP32 Response:**
```
CAN Bus Failure Detected:
1. Set CAN_FAULT flag
2. Attempt bus recovery (reset transceiver, reinit CAN peripheral)
3. Retry 3 times (500ms interval)
4. If recovery fails:
   a. Enter SAFE MODE
   b. Alert driver (HUD + audio + LED flash)
   c. Disable remote motor control
   d. Log error code
   e. Allow manual power cycle
```

**STM32 ECU Response:**
```
CAN Heartbeat Timeout (100ms):
1. Set CAN_TIMEOUT flag
2. Activate failsafe outputs:
   - Motor ECU: Gradual deceleration (5s ramp), motors off, steering center
   - Sensor ECU: Continue acquisition, buffer data
   - Lighting ECU: Freeze in safe state (running lights)
3. Flash status LED (rapid = fault)
4. Monitor for CAN recovery
5. Resume on heartbeat restore
```

---

### 4.2 ECU Failure Detection

**Heartbeat Protocol:**
- Every ECU sends heartbeat @ 10 Hz
- Sequence counter increments each message
- Health status byte (0=OK, 1=Degraded, 2=Fault)

**ESP32 Monitoring Logic:**
```
For each ECU:
  If heartbeat not received for 100ms:
    1. Mark ECU as OFFLINE
    2. Trigger ECU-specific failsafe
    3. Alert driver (HUD)
    4. Log error
    5. Enter LIMP MODE
    
  If sequence counter skips:
    1. Warning: possible CAN message loss
    2. Log diagnostic
    
  If health status != OK:
    1. Request diagnostic data (CAN message)
    2. Display warning
```

**ECU-Specific Failsafes:**

**Motor ECU Offline:**
```
1. Assume motors frozen in last state (dangerous)
2. Alert driver: "MOTOR CONTROL LOST - STOP VEHICLE"
3. Disable throttle input (pedal ignored)
4. Flash all LEDs (emergency indication)
5. Sound continuous alarm
6. Log critical error
7. Await manual power cycle
```

**Sensor ECU Offline:**
```
1. Use last known values (with timeout)
2. Alert driver: "SENSOR FAULT - REDUCED PERFORMANCE"
3. Apply conservative limits:
   - Max speed: 50%
   - Max current: 70%
   - No temperature protection (assume worst-case)
4. Continue operation in degraded mode
5. Log warning
```

**Lighting ECU Offline:**
```
1. LEDs remain in last state (acceptable)
2. Alert driver: "LIGHTING FAULT"
3. Continue operation normally
4. Log warning
```

---

### 4.3 Watchdog Architecture

**Per-ECU Watchdog:**

**ESP32:**
- Hardware watchdog: 30 seconds
- Software feed: every 10ms (main loop)
- Boot counter: 3 boots in 60s → safe mode

**STM32 Motor ECU:**
- Independent watchdog (IWDG): 1 second
- Fed by main control loop (every 10ms)
- Resets ECU on hang → ESP32 detects offline → failsafe

**STM32 Sensor ECU:**
- Independent watchdog (IWDG): 2 seconds
- Fed by main sensor loop (every 50ms)
- Resets ECU on hang → ESP32 detects offline → degraded mode

**STM32 Lighting ECU:**
- Independent watchdog (IWDG): 5 seconds
- Fed by LED update loop (every 50ms)
- Resets ECU on hang → LEDs freeze (acceptable)

**Cross-ECU Watchdog (CAN Heartbeats):**
- ESP32 monitors all ECUs via heartbeat
- Each ECU monitors ESP32 heartbeat
- 100ms timeout → failsafe activation

---

### 4.4 Power Loss Handling

**Graceful Shutdown Sequence (ESP32 Coordinated):**

```
Ignition OFF detected (GPIO 41):
1. ESP32: Send SHUTDOWN_WARNING to all ECUs (CAN)
2. Wait 100ms for acknowledgments
3. ESP32: Play shutdown audio (DFPlayer, 2 seconds)
4. ESP32: Send SHUTDOWN_NOW command (CAN)
5. Motor ECU:
   - Disable all PWM outputs
   - Motors off immediately
   - Steering centered
   - Send ACK
6. Sensor ECU:
   - Save config to EEPROM (if dirty)
   - Send ACK
7. Lighting ECU:
   - LEDs off or safe state
   - Send ACK
8. ESP32:
   - Save configuration
   - Clear boot counter
   - Log shutdown
   - Delay 5 seconds (power hold)
   - Release RELAY_MAIN → power off
```

**Emergency Power Loss (no graceful shutdown):**
- All ECUs have local voltage monitors
- If voltage drops below threshold → immediate safe state
- Motors off, steering centered, LEDs off
- ESP32 power hold cannot prevent (battery disconnect)

---

## 5. BOOT SEQUENCE (Multi-ECU)

### 5.1 Power-Up Sequence

```
1. Power Applied (24V battery)
   ├──> Buck 5V regulator powers ESP32
   └──> ESP32 GPIO initializes

2. ESP32 Boot (First to start, ~500ms)
   ├──> Serial UART init (diagnostics)
   ├──> Check boot counter (bootloop detection)
   ├──> Power relay init (RELAY_MAIN = HIGH, power hold)
   ├──> CAN peripheral init (MCP2515 SPI)
   ├──> Display init (TFT + touch)
   └──> Boot screen displayed

3. ESP32 Power Distribution
   ├──> Enable RELAY_TRAC (12V auxiliaries) → STM32 ECUs power up
   ├──> Enable RELAY_DIR (24V motors) → BTS7960 drivers powered
   └──> Wait 100ms for STM32 boot

4. STM32 ECUs Boot (~200ms each, parallel)
   ├──> Motor ECU:
   │    ├──> I2C init (PCA9685 + MCP23017)
   │    ├──> Encoder timer init (TIM2 encoder mode)
   │    ├──> Wheel sensor GPIO init
   │    ├──> CAN init (bxCAN peripheral)
   │    └──> Send READY heartbeat
   │
   ├──> Sensor ECU:
   │    ├──> I2C init (TCA9548A + INA226)
   │    ├──> OneWire init (DS18B20)
   │    ├──> UART init (TOFSense)
   │    ├──> CAN init
   │    └──> Send READY heartbeat
   │
   └──> Lighting ECU (if present):
        ├──> WS2812B GPIO + DMA init
        ├──> CAN init
        └──> Send READY heartbeat

5. ESP32 ECU Discovery (100ms timeout per ECU)
   ├──> Wait for Motor ECU heartbeat
   │    └──> If timeout: CRITICAL ERROR (cannot drive without motors)
   ├──> Wait for Sensor ECU heartbeat
   │    └──> If timeout: WARNING (can drive with degraded telemetry)
   └──> Wait for Lighting ECU heartbeat
        └──> If timeout: WARNING (can drive without LEDs)

6. ESP32 System Initialization
   ├──> Send initial commands to ECUs:
   │    ├──> Motor ECU: Zero throttle, center steering
   │    ├──> Sensor ECU: Start acquisition
   │    └──> Lighting ECU: Running lights
   ├──> Load configuration from EEPROM
   ├──> Initialize managers (Safety, Mode, Telemetry)
   └──> Clear boot counter (successful boot)

7. Ready State (~2-3 seconds total)
   ├──> ESP32: Display main HUD
   ├──> Motor ECU: Motors idle, ready for commands
   ├──> Sensor ECU: Streaming telemetry
   └──> System ready for operation
```

---

### 5.2 Boot Failure Scenarios

**Scenario 1: Motor ECU fails to boot**
```
ESP32 detects Motor ECU heartbeat timeout (100ms):
1. Display CRITICAL ERROR: "MOTOR ECU OFFLINE - DO NOT DRIVE"
2. Flash all LEDs red (if Lighting ECU OK)
3. Sound alarm (continuous beep)
4. Do NOT enter operation mode
5. Wait for manual power cycle or ECU recovery
```

**Scenario 2: Sensor ECU fails to boot**
```
ESP32 detects Sensor ECU heartbeat timeout:
1. Display WARNING: "SENSOR ECU OFFLINE - DEGRADED MODE"
2. Use placeholder sensor values:
   - Current: 0A (assume safe)
   - Temperature: 80°C (assume warm, apply thermal limits)
   - Obstacle: 4000mm (no obstacles, disable safety)
3. Allow operation with reduced performance
4. Log warning
```

**Scenario 3: Lighting ECU fails to boot (optional ECU)**
```
ESP32 detects Lighting ECU timeout:
1. Display info: "LIGHTING ECU OFFLINE"
2. No LEDs (acceptable)
3. Continue normal operation
4. Log info
```

---

## 6. HARDWARE RECOMMENDATIONS

### 6.1 Development Kits

**ESP32-S3:**
- ✅ **Current:** ESP32-S3-DevKitC-1 (44-pin, N16R8 module)
- Continue using existing hardware
- Add MCP2515 CAN transceiver module (SPI)

**STM32F4 Motor ECU:**
- ✅ **Recommended:** STM32F407VGT6 development board
  - 100 pins (plenty of GPIO for expansion)
  - FPU for ABS/TCS math
  - Hardware encoder support (TIM2-5)
  - Built-in CAN peripheral
- Alternative: STM32F405RGT6 (64 pins, cheaper)

**STM32F1 Sensor ECU:**
- ✅ **Recommended:** STM32F103RCT6 (256KB flash, 48KB RAM)
  - More RAM for sensor buffering
  - Built-in CAN
- Budget option: STM32F103C8T6 "Blue Pill" (may be tight on RAM)

**STM32G0 Lighting ECU (Optional):**
- ✅ **Recommended:** STM32G071KBU6
  - Low cost
  - FDCAN support
  - Sufficient for LED control

---

### 6.2 CAN Transceivers

**For ESP32 (SPI CAN controller):**
- ✅ MCP2515 + SN65HVD230 module
- Low cost, proven design
- SPI interface (ESP32 has plenty)

**For STM32 (built-in CAN peripheral):**
- ✅ SN65HVD230 standalone transceiver
- 3.3V compatible
- Low power, ~5mA quiescent

---

### 6.3 PCB Integration (Future Production)

**Single PCB Design:**
```
┌────────────────────────────────────────────────┐
│              Main Control PCB                  │
│                                                │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐    │
│  │  ESP32   │  │ STM32F4  │  │ STM32F1  │    │
│  │  S3      │  │  Motor   │  │  Sensor  │    │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘    │
│       │             │             │           │
│  ┌────┴─────────────┴─────────────┴─────┐    │
│  │         CAN Bus (trace)              │    │
│  │  Dual 120Ω termination resistors    │    │
│  └──────────────────────────────────────┘    │
│                                                │
│  Power Section:                               │
│  • 24V → 12V Buck (steering, sensors)        │
│  • 12V → 5V Buck (logic, ESP32, STM32s)      │
│  • Relay drivers (4x)                         │
│                                                │
│  Connectors:                                  │
│  • Motor drivers (5x, BTS7960 via header)    │
│  • Sensor inputs (wheel, encoder, temp)      │
│  • Display (SPI + Touch)                      │
│  • Power (24V battery)                        │
│  • Expansion header                           │
└────────────────────────────────────────────────┘
```

**Benefits:**
- Single board = lower cost, easier assembly
- Shared power supply (3x buck regulators)
- Short CAN bus traces = high reliability
- Integrated testing

---

## 7. SOFTWARE ARCHITECTURE

### 7.1 ESP32 Gateway Software

**RTOS Tasks:**
```
Task 1: Main Control Loop (Core 1, Priority Normal)
├──> Power management (10 Hz)
├──> Mode management (10 Hz)
├──> CAN gateway (send commands, 100 Hz)
└──> Watchdog feed (10 Hz)

Task 2: HUD Update (Core 1, Priority Low)
├──> Display rendering (30 Hz)
├──> Touch processing (event-driven)
└──> Menu handling

Task 3: CAN RX Handler (Core 1, Priority High)
├──> Receive CAN messages (IRQ-driven)
├──> Parse and update data structures
└──> Trigger updates (callbacks)

Task 4: Audio & Logging (Core 0, Priority Low)
├──> DFPlayer commands (event-driven)
├──> Serial logging (rate-limited)
└──> Telemetry storage
```

**Data Flow:**
```
CAN RX ISR ──> Queue ──> CAN RX Task ──> Update globals ──> HUD Task
                                       └──> Main Loop
                                       
Main Loop ──> Decision Logic ──> CAN TX ──> STM32 ECUs
          └──> Mode Manager ───────────┘
```

---

### 7.2 STM32F4 Motor ECU Software

**Main Loop (10ms super-loop):**
```
while(1) {
  t_start = HAL_GetTick();
  
  // 1. Process CAN RX (commands from ESP32)
  ProcessCANCommands();  // ~0.5ms
  
  // 2. Read wheel sensors (from ISR counters)
  UpdateWheelSpeeds();   // ~0.2ms
  
  // 3. Read encoder position (from TIM2)
  UpdateSteeringPosition();  // ~0.1ms
  
  // 4. Safety systems (ABS/TCS)
  ABS_Update();  // ~1ms
  TCS_Update();  // ~1ms
  
  // 5. Position control (steering)
  SteeringPID_Update();  // ~0.5ms
  
  // 6. Traction control (4WD)
  TractionControl_Update();  // ~1ms
  
  // 7. Apply hardware outputs (I2C PWM)
  ApplyMotorPWM();  // ~2ms (I2C transactions)
  
  // 8. Send CAN feedback
  SendCANFeedback();  // ~0.5ms
  
  // 9. Watchdog feed
  HAL_IWDG_Refresh();
  
  // 10. Sleep until next cycle
  t_elapsed = HAL_GetTick() - t_start;
  if(t_elapsed < 10) {
    HAL_Delay(10 - t_elapsed);
  }
}
```

**ISRs:**
- Wheel sensor ISRs (4x, increment counters)
- CAN RX ISR (copy to buffer, set flag)
- Encoder update (handled by hardware TIM2)

---

### 7.3 STM32F1 Sensor ECU Software

**Main Loop (50ms super-loop):**
```
while(1) {
  t_start = HAL_GetTick();
  
  // 1. Process CAN RX (config from ESP32)
  ProcessCANCommands();  // ~0.5ms
  
  // 2. Read current sensors (6x INA226 via TCA9548A)
  UpdateCurrentSensors();  // ~30ms (I2C, sequential)
  
  // 3. Read temperature sensors (1 Hz, state machine)
  static uint8_t temp_state = 0;
  if(temp_state == 0) {
    UpdateTemperatureSensors();  // ~750ms (blocking, OneWire)
    temp_state = 20;  // Skip next 20 cycles (1s @ 50ms)
  } else {
    temp_state--;
  }
  
  // 4. Process obstacle LiDAR (UART RX, async)
  ProcessObstacleData();  // ~5ms
  
  // 5. Send CAN telemetry
  SendCANTelemetry();  // ~2ms (5 frames)
  
  // 6. Watchdog feed
  HAL_IWDG_Refresh();
  
  // 7. Sleep
  t_elapsed = HAL_GetTick() - t_start;
  if(t_elapsed < 50) {
    HAL_Delay(50 - t_elapsed);
  }
}
```

**Note:** Temperature read blocks, but only once per second, acceptable.

---

## 8. MIGRATION BENEFITS SUMMARY

### 8.1 Reliability Improvements

✅ **I2C Bus Isolation:**
- Current: Single I2C bus hang = total system failure
- Target: Motor I2C isolated from sensor I2C
- Benefit: Motor control continues if sensor I2C fails

✅ **Load Distribution:**
- Current: ESP32 at 85% CPU utilization
- Target: ESP32 at 40%, STM32s at 50%
- Benefit: More headroom, better responsiveness

✅ **Redundancy Potential:**
- Current: No redundancy
- Target: Dual CAN bus option
- Benefit: Safety-critical communication backup

✅ **Dedicated Real-Time Processing:**
- Current: ESP32 handles everything
- Target: STM32F4 dedicated to motor control
- Benefit: Better PWM timing, encoder processing

---

### 8.2 Performance Improvements

✅ **Better ISR Handling:**
- Current: 7 ISRs on ESP32 (shared with WiFi core, disabled)
- Target: Encoder ISRs on STM32 (dedicated, hardware timers)
- Benefit: Lower latency, deterministic timing

✅ **HUD Responsiveness:**
- Current: Display update competes with I2C motor control
- Target: ESP32 dedicated to UI
- Benefit: Smoother display, no frame drops

✅ **Sensor Acquisition:**
- Current: I2C sensors block motor control loop
- Target: Sensors on separate ECU
- Benefit: Sensors don't delay motor updates

---

### 8.3 Scalability

✅ **Easy to Add Features:**
- Current: ESP32 nearly out of GPIO and CPU
- Target: Add ECUs as needed (e.g., battery BMS, HVAC)
- Benefit: Future-proof architecture

✅ **Modular Development:**
- Current: Monolithic firmware, hard to test subsystems
- Target: Each ECU developed/tested independently
- Benefit: Parallel development, easier debugging

---

### 8.4 Maintainability

✅ **Subsystem Isolation:**
- Current: Bug in sensor code can crash motor control
- Target: ECUs independent, faults isolated
- Benefit: Easier to localize bugs

✅ **Firmware Updates:**
- Current: Update entire ESP32 firmware (risky)
- Target: Update individual ECUs (less risk)
- Benefit: Can update sensors without touching motor control

---

## NEXT DOCUMENT

See `MIGRATION_PHASES.md` for the step-by-step migration plan.

---

**END OF DOCUMENT**
