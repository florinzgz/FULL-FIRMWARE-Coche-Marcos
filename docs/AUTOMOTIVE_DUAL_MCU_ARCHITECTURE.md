# Automotive Dual-MCU Architecture: ESP32-S3 + STM32G474RE
## Safety-Based Partitioning for Electric Vehicle Control System

**Document Version:** 1.0  
**Date:** 2026-01-13  
**Architecture Type:** 2-Node CAN-Based Automotive System  
**Safety Philosophy:** Real-Time + Safety on STM32, Perception + UI on ESP32

---

## 📋 Executive Summary

This document defines a **2-node automotive architecture** (NOT multi-ECU) for migrating the current ESP32-S3 based electric vehicle control system to a dual-MCU topology:

- **Node 1: ESP32-S3** — Brain, UI, perception, logic, gateway
- **Node 2: ONE STM32G474RE** — Powertrain + real-time safety ECU

**Critical Principle:** There is NO need for multiple STM32s. One STM32G474RE ECU is sufficient and optimal for all real-time safety-critical control.

---

## 🎯 Design Philosophy

### Safety-First Partitioning

The partition is based on **safety criticality**, not convenience:

```
IF failure can cause:
    - Physical harm
    - Loss of vehicle control
    - Motor runaway
    - Fire risk
THEN → STM32G474 ECU (real-time safety domain)

IF failure causes:
    - UI freeze
    - Display glitch
    - Audio problem
    - LED malfunction
THEN → ESP32-S3 (user experience domain)
```

### Why NOT Multi-ECU?

**One STM32G474RE is sufficient because:**
1. **17 timers** (including HRTIM) handle all motors + encoders simultaneously
2. **5 ADCs** with DMA can sample all currents and temperatures in parallel
3. **3 CAN FD** interfaces (only need 1 for ESP32 communication)
4. **16-channel DMA** handles sensor data without CPU intervention
5. **107 I/O pins** (64 in LQFP64 package) sufficient for all real-time signals

Adding more STM32s would:
- ❌ Increase cost unnecessarily
- ❌ Add CAN bus complexity
- ❌ Create synchronization problems
- ❌ Reduce reliability (more components = more failure modes)

---

## 🔬 STM32G474RE Hardware Capabilities Analysis

### From Official Datasheet Review

#### Processor & Performance
- **Core:** ARM Cortex-M4 @ 170 MHz, 213 DMIPS
- **FPU:** Single-precision floating point
- **DSP:** Hardware DSP instructions
- **MPU:** Memory protection unit
- **Execution:** 0-wait-state from Flash at 170 MHz

#### Memory
- **Flash:** 512 KB (dual-bank, ECC)
- **SRAM:** 128 KB (with CCM, parity check)
- **OTP:** 1 KB

#### Real-Time Control Hardware

**Timers (17 Total):**
- **HRTIM:** 6×16-bit counters, **184 ps resolution**, 12 PWM outputs
  - Dead-time insertion (configurable)
  - Fault inputs (immediate PWM shutdown)
  - ADC trigger synchronization
  - **Perfect for motor PWM**
  
- **Motor Control Timers:** Advanced PWM with complementary outputs
- **General Purpose:** 32-bit, encoder mode, input capture
- **Basic Timers:** Time base generation
- **Watchdog:** Independent and window watchdog

**ADCs (5× 12-bit, up to 4 Msps):**
- **Simultaneous sampling:** Read multiple channels at exact same time
- **Hardware oversampling:** Up to 16-bit resolution
- **DMA support:** Zero CPU overhead
- **Triggering:** From HRTIM, synchronized with PWM
- **42 channels total**
- **Application:** Sample all motor currents simultaneously

**DACs (7× 12-bit):**
- 3 external buffered
- 4 internal unbuffered
- **Not needed for this application**

**Comparators (7× ultra-fast, rail-to-rail):**
- **Latency:** Nanosecond-scale
- **Output:** Can trigger HRTIM fault input
- **Application:** Overcurrent protection WITHOUT software

**Op-Amps (6× with PGA mode):**
- Programmable gain amplifier
- **Application:** Current sensing amplification

**Math Accelerators:**
- **CORDIC:** sin, cos, atan2, sqrt acceleration (for future FOC if needed)
- **FMAC:** Filter math accelerator

#### Communication Interfaces

**CAN (3× CAN FD):**
- **Flexible Data-Rate:** Up to 5 Mbps data phase
- **Filters:** Hardware message filtering
- **FIFOs:** Reduce interrupt load
- **Application:** Communication with ESP32-S3

**Others (available but not primary):**
- 4× I2C (backup if CAN fails)
- 4× SPI
- 6× UART
- USB Device

#### DMA (16 channels)
- **Peripheral-to-Memory:** ADC data streaming
- **Memory-to-Peripheral:** PWM update
- **Memory-to-Memory:** Data shuffling
- **Zero CPU load** for repetitive transfers

#### GPIO
- **Available:** 54 I/O in UFQFPN48 package
- **Speed:** Up to 80 MHz toggle rate
- **Interrupt:** All pins can trigger EXTI
- **5V tolerant:** Many pins

---

## 🏗️ System Architecture

### Current State (Single ESP32-S3)

```
┌───────────────────────────────────────────────────────────────┐
│                      ESP32-S3 N16R8                           │
│                    (Does Everything)                          │
│                                                               │
│  ┌─────────────┐  ┌─────────────┐  ┌──────────────┐         │
│  │ TFT Display │  │   Motors    │  │   Sensors    │         │
│  │   + Touch   │  │  + Steering │  │  + Encoders  │         │
│  │   + HUD     │  │  + PWM (I2C)│  │  + Current   │         │
│  └─────────────┘  └─────────────┘  └──────────────┘         │
│                                                               │
│  ┌─────────────┐  ┌─────────────┐  ┌──────────────┐         │
│  │  WS2812B    │  │    Audio    │  │    LiDAR     │         │
│  │    LEDs     │  │  DFPlayer   │  │  TOFSense    │         │
│  └─────────────┘  └─────────────┘  └──────────────┘         │
│                                                               │
│  Problem: Real-time control mixed with UI rendering          │
│  Risk: UI crash could affect motor safety                    │
└───────────────────────────────────────────────────────────────┘
```

### Proposed State (Dual-MCU with CAN)

```
┌──────────────────────────────────┐       CAN FD        ┌─────────────────────────────────┐
│         ESP32-S3 N16R8           │◄═══════════════════►│      STM32G474RE ECU            │
│     (Brain + UI + Perception)    │     500 kbps        │  (Powertrain + Real-Time Safety)│
│                                  │                     │                                 │
│  ┌────────────────────────────┐  │                     │  ┌────────────────────────────┐ │
│  │ TFT 480×320 + Touch        │  │                     │  │ 4× Motor PWM (HRTIM)       │ │
│  │ HUD Rendering              │  │                     │  │ - FL, FR, RL, RR           │ │
│  │ Menu System                │  │                     │  │ - 20 kHz, 184ps resolution │ │
│  └────────────────────────────┘  │                     │  └────────────────────────────┘ │
│                                  │                     │                                 │
│  ┌────────────────────────────┐  │                     │  ┌────────────────────────────┐ │
│  │ TOFSense-M S LiDAR         │  │                     │  │ 1× Steering PWM (HRTIM)    │ │
│  │ - Obstacle Detection       │  │                     │  │ - RS390 motor              │ │
│  │ - 8×8 matrix, 64 points    │  │                     │  │ - Dead-time protection     │ │
│  │ - Decision logic           │  │                     │  └────────────────────────────┘ │
│  └────────────────────────────┘  │                     │                                 │
│                                  │  CAN Messages:      │  ┌────────────────────────────┐ │
│  ┌────────────────────────────┐  │  ESP32→STM32:       │  │ Current Sensing (ADC+DMA)  │ │
│  │ WS2812B LEDs (44 total)    │  │  - Throttle demand  │  │ - 6× INA226 replacement    │ │
│  │ - 28 front, 16 rear        │  │  - Steering demand  │  │ - 5× ADC simultaneous      │ │
│  │ - RMT peripheral           │  │  - Mode (P/R/N/D)   │  │ - Battery, 4 motors, steer │ │
│  │ - Animations               │  │  - ABS/TCS enable   │  │ - Overcurrent comparators  │ │
│  └────────────────────────────┘  │                     │  └────────────────────────────┘ │
│                                  │  STM32→ESP32:       │                                 │
│  ┌────────────────────────────┐  │  - Wheel speeds     │  ┌────────────────────────────┐ │
│  │ Audio DFPlayer Mini        │  │  - Motor currents   │  │ Temperature (DS18B20)      │ │
│  │ - Track selection          │  │  - Temperatures     │  │ - 4× motors                │ │
│  │ - Volume control           │  │  - Encoder position │  │ - OneWire on GPIO          │ │
│  │ - UART control             │  │  - Fault codes      │  │ - Overheat detection       │ │
│  └────────────────────────────┘  │  - ABS/TCS status   │  └────────────────────────────┘ │
│                                  │                     │                                 │
│  ┌────────────────────────────┐  │                     │  ┌────────────────────────────┐ │
│  │ Vehicle Logic              │  │                     │  │ Wheel Speed Sensors        │ │
│  │ - Mode selection           │  │                     │  │ - 4× inductive (GPIO+INT)  │ │
│  │ - Adaptive cruise          │  │                     │  │ - 6 pulses/rev             │ │
│  │ - Obstacle avoidance       │  │                     │  │ - Speed calculation        │ │
│  │ - Telemetry logging        │  │                     │  └────────────────────────────┘ │
│  └────────────────────────────┘  │                     │                                 │
│                                  │                     │  ┌────────────────────────────┐ │
│  ┌────────────────────────────┐  │                     │  │ Steering Encoder E6B2      │ │
│  │ USB Logging                │  │                     │  │ - 1200 PPR quadrature      │ │
│  │ - Debug output             │  │                     │  │ - A/B/Z channels           │ │
│  │ - Data recording           │  │                     │  │ - Position tracking        │ │
│  └────────────────────────────┘  │                     │  └────────────────────────────┘ │
│                                  │                     │                                 │
│  Crash Impact: UI freeze,        │                     │  ┌────────────────────────────┐ │
│  no motor danger                 │                     │  │ Safety Systems             │ │
│                                  │                     │  │ - ABS (wheel slip detect)  │ │
└──────────────────────────────────┘                     │  │ - TCS (traction control)   │ │
                                                         │  │ - Emergency stop           │ │
                                                         │  │ - Watchdog (IWDG)          │ │
                                                         │  └────────────────────────────┘ │
                                                         │                                 │
                                                         │  ┌────────────────────────────┐ │
                                                         │  │ Relays (GPIO outputs)      │ │
                                                         │  │ - Main power relay         │ │
                                                         │  │ - Traction 24V relay       │ │
                                                         │  │ - Steering 12V relay       │ │
                                                         │  │ - Emergency cutoff         │ │
                                                         │  └────────────────────────────┘ │
                                                         │                                 │
                                                         │  Crash Impact: Vehicle stops   │
                                                         │  safely (controlled shutdown)  │
                                                         │                                 │
                                                         └─────────────────────────────────┘
```

---

## 🎯 Component Allocation: Safety-Based Decision Matrix

### Decision Criteria

For each component, ask:

1. **Can failure cause physical harm?** → STM32
2. **Must it work even if ESP32 crashes?** → STM32
3. **Is it hard real-time (<1ms jitter)?** → STM32
4. **Does it directly control motors/power?** → STM32
5. **Is it perception/UI/non-critical?** → ESP32

### Complete Component Classification

| Component | Current MCU | New MCU | Reason |
|-----------|-------------|---------|--------|
| **MOTORS & ACTUATION** ||||
| 4× Traction motors (BTS7960) | ESP32 (PCA9685 I2C) | ✅ **STM32 HRTIM** | Real-time PWM, overcurrent protection |
| 1× Steering motor (BTS7960) | ESP32 (PCA9685 I2C) | ✅ **STM32 HRTIM** | Safety-critical, must not lose control |
| Motor direction (MCP23017 I2C) | ESP32 | ✅ **STM32 GPIO** | Direct control, remove I2C latency |
| **SENSORS - SAFETY CRITICAL** ||||
| 4× Wheel speed (inductive) | ESP32 GPIO | ✅ **STM32 GPIO+EXTI** | ABS/TCS requires deterministic timing |
| 1× Steering encoder E6B2-CWZ6C | ESP32 GPIO | ✅ **STM32 Timer Encoder** | Hardware quadrature decode, 1200 PPR |
| 6× Current sensors (INA226 I2C) | ESP32 via TCA9548A | ✅ **STM32 ADC+shunts** | Replace with analog shunts + ADC |
| 4× Motor temp (DS18B20) | ESP32 OneWire | ✅ **STM32 GPIO OneWire** | Overheat protection must be local |
| **SENSORS - PERCEPTION** ||||
| TOFSense-M S LiDAR (8×8) | ESP32 UART | ✅ **ESP32 UART** | Obstacle DETECTION, not avoidance |
| Pedal analog (Hall A1324) | ESP32 ADC | ➡️ **Both** | ESP32 reads, sends via CAN to STM32 |
| **POWER CONTROL** ||||
| 4× Relays (main, traction, steering, aux) | ESP32 GPIO | ✅ **STM32 GPIO** | Emergency shutdown must be local |
| Power key detection | ESP32 GPIO | ➡️ **Both** | ESP32 boots first, STM32 confirms |
| **USER INTERFACE** ||||
| TFT Display ST7796S 480×320 | ESP32 SPI | ✅ **ESP32 SPI** | 16MB Flash needed for framebuffer |
| Touch XPT2046 | ESP32 SPI | ✅ **ESP32 SPI** | UI input, not safety-critical |
| **LIGHTING** ||||
| 2× WS2812B LED strips (44 LEDs) | ESP32 RMT | ✅ **ESP32 RMT** | User feedback, ESP32 has RMT peripheral |
| **AUDIO** ||||
| DFPlayer Mini | ESP32 UART | ✅ **ESP32 UART** | User feedback, not safety |
| **COMMUNICATION** ||||
| USB logging | ESP32 USB | ✅ **ESP32 USB** | Debug/telemetry only |
| **PALANCA (Shifter)** ||||
| Shifter P/R/N/D1/D2 inputs | ESP32 via MCP23017 | ✅ **ESP32 GPIO** | Mode selection, ESP32 sends via CAN |

---

## 🔌 STM32G474RE Pin Assignment

### Hardware Interface Map

```
STM32G474RE LQFP64 Package (54 I/O available)
═══════════════════════════════════════════════════════════

MOTORS (HRTIM - 10 pins)
├─ PA8  (HRTIM_CHA1) → Motor FL Forward PWM
├─ PA9  (HRTIM_CHA2) → Motor FL Reverse PWM  
├─ PA10 (HRTIM_CHB1) → Motor FR Forward PWM
├─ PA11 (HRTIM_CHB2) → Motor FR Reverse PWM
├─ PA12 (HRTIM_CHC1) → Motor RL Forward PWM
├─ PA13 (HRTIM_CHC2) → Motor RL Reverse PWM
├─ PA14 (HRTIM_CHD1) → Motor RR Forward PWM
├─ PA15 (HRTIM_CHD2) → Motor RR Reverse PWM
├─ PB0  (HRTIM_CHE1) → Steering Forward PWM
└─ PB1  (HRTIM_CHE2) → Steering Reverse PWM

MOTOR DIRECTION (GPIO - 10 pins)
├─ PC0 → FL IN1 (direction)
├─ PC1 → FL IN2
├─ PC2 → FR IN1
├─ PC3 → FR IN2
├─ PA0 → RL IN1
├─ PA1 → RL IN2
├─ PA2 → RR IN1
├─ PA3 → RR IN2
├─ PB6 → Steering R_EN
└─ PB7 → Steering L_EN

CURRENT SENSING (ADC - 6 pins + comparators)
├─ PA4  (ADC2_IN17) → Battery current shunt
├─ PA5  (ADC2_IN13) → Motor FL current shunt
├─ PA6  (ADC2_IN3)  → Motor FR current shunt
├─ PA7  (ADC2_IN4)  → Motor RL current shunt
├─ PB11 (ADC1_IN14) → Motor RR current shunt
└─ PB12 (ADC4_IN3)  → Steering current shunt

├─ PA0 (COMP1_INP) → Overcurrent comparator FL
├─ PA1 (COMP2_INP) → Overcurrent comparator FR
└─ ... (comparators can trigger HRTIM fault)

WHEEL SPEED SENSORS (GPIO+EXTI - 4 pins)
├─ PB2 (EXTI2)  → Wheel FL speed sensor
├─ PB3 (EXTI3)  → Wheel FR speed sensor
├─ PB4 (EXTI4)  → Wheel RL speed sensor
└─ PB5 (EXTI5)  → Wheel RR speed sensor

STEERING ENCODER (Timer in Encoder Mode - 3 pins)
├─ PB8 (TIM4_CH3) → Encoder A (1200 PPR)
├─ PB9 (TIM4_CH4) → Encoder B
└─ PC6 (GPIO)     → Encoder Z (center detect)

TEMPERATURE SENSORS (OneWire - 1 pin)
└─ PC7 (GPIO) → DS18B20 bus (4 sensors)

RELAYS (GPIO - 4 pins)
├─ PC8  → Main power relay
├─ PC9  → Traction 24V relay
├─ PC10 → Steering 12V relay
└─ PC11 → Emergency cutoff relay

CAN COMMUNICATION (CAN1 - 2 pins)
├─ PB8 (CAN1_RX) → CAN receive from ESP32
└─ PB9 (CAN1_TX) → CAN transmit to ESP32

DEBUG (Optional - 2 pins)
├─ PA13 (SWDIO) → SWD debug
└─ PA14 (SWCLK) → SWD clock

═══════════════════════════════════════════════════════════
Total Pins Used: 42/54 (78% utilization, with 12 pins spare for expansion)
═══════════════════════════════════════════════════════════
```

**Note:** Pin assignment optimized for:
- HRTIM channels grouped by motor
- ADC channels on different ADCs for simultaneous sampling
- Encoder on Timer4 (hardware quadrature decoder)
- CAN on dedicated pins
- EX TI lines for fast wheel speed interrupts

---

## 📡 CAN Bus Communication Protocol

### CAN Configuration

- **Baud Rate:** 500 kbps (automotive standard)
- **Protocol:** CAN 2.0B (29-bit extended IDs)
- **Bus:** Single-wire CAN with 120Ω terminators at each end
- **Isolation:** Optocouplers between MCUs (safety)

### Message Definitions

#### ESP32 → STM32 (Commands)

| CAN ID | Name | DLC | Data | Rate | Priority |
|--------|------|-----|------|------|----------|
| `0x100` | Throttle Command | 2 | `[demand_pct, flags]` | 20 Hz | High |
| `0x101` | Steering Command | 2 | `[angle_deg, speed_pct]` | 20 Hz | High |
| `0x102` | Mode Command | 2 | `[mode, submode]` | Event | Medium |
| `0x103` | Safety Enable | 1 | `[ABS|TCS|REGEN bits]` | Event | High |
| `0x104` | Emergency Stop | 0 | `[]` | Event | **Critical** |
| `0x105` | Heartbeat | 1 | `[sequence]` | 10 Hz | Low |

**Throttle Command Detail (0x100):**
```
Byte 0: demand_pct (0-100, percentage)
Byte 1: flags
  - bit 0: reverse
  - bit 1: 4×4 mode
  - bit 2: axis rotation (tank turn)
  - bit 3-7: reserved
```

**Mode Command Detail (0x102):**
```
Byte 0: mode
  - 0 = Park
  - 1 = Reverse
  - 2 = Neutral
  - 3 = Drive 1
  - 4 = Drive 2
Byte 1: submode (Eco/Normal/Sport)
```

#### STM32 → ESP32 (Telemetry)

| CAN ID | Name | DLC | Data | Rate | Priority |
|--------|------|-----|------|------|----------|
| `0x200` | Wheel Speeds | 8 | `[FL_rpm, FR_rpm, RL_rpm, RR_rpm]` | 50 Hz | Medium |
| `0x201` | Motor Currents | 8 | `[FL_A, FR_A, RL_A, RR_A]` | 20 Hz | Medium |
| `0x202` | Temperatures | 8 | `[FL_C, FR_C, RL_C, RR_C]` | 2 Hz | Low |
| `0x203` | Steering State | 4 | `[angle_deg, speed_rpm]` | 50 Hz | Medium |
| `0x204` | Battery Status | 4 | `[voltage_V, current_A]` | 10 Hz | Medium |
| `0x205` | ABS Status | 2 | `[active_wheels, slip_pct]` | 20 Hz | High |
| `0x206` | TCS Status | 2 | `[active_wheels, reduction_pct]` | 20 Hz | High |
| `0x207` | Fault Codes | 4 | `[fault_bitmap, severity]` | Event | **Critical** |
| `0x208` | Heartbeat | 1 | `[sequence]` | 10 Hz | Low |

**Wheel Speeds Detail (0x200):**
```
Bytes 0-1: FL wheel RPM (uint16, big-endian)
Bytes 2-3: FR wheel RPM
Bytes 4-5: RL wheel RPM
Bytes 6-7: RR wheel RPM
```

**Fault Codes Detail (0x207):**
```
Byte 0-1: fault_bitmap
  - bit 0: Overcurrent FL
  - bit 1: Overcurrent FR
  - bit 2: Overcurrent RL
  - bit 3: Overcurrent RR
  - bit 4: Overcurrent steering
  - bit 5: Overcurrent battery
  - bit 6: Overheat FL
  - bit 7: Overheat FR
  - bit 8: Overheat RL
  - bit 9: Overheat RR
  - bit 10: Encoder error
  - bit 11: Wheel sensor error
  - bit 12: CAN timeout
  - bit 13-15: reserved
Byte 2: severity (0=info, 1=warning, 2=error, 3=critical)
Byte 3: subsystem (motor index or 0xFF for global)
```

---

## ⚡ Real-Time Performance Budget

### STM32G474 Execution Loop

```
Main Loop @ 20 kHz (50 μs period)
═════════════════════════════════════════════════════════════

├─ ADC Sampling (DMA, parallel)           ┤ 5 μs
│  └─ 6 channels, triggered by HRTIM      │
│
├─ Temperature Read (OneWire, cached)     ┤ 0 μs (background)
│
├─ Wheel Speed Update (interrupt-driven)  ┤ 2 μs
│  └─ 4 EXTI handlers, increment counters │
│
├─ Encoder Position Read (hardware)       ┤ 0.5 μs
│  └─ TIM4->CNT register read             │
│
├─ ABS Calculation                        ┤ 8 μs
│  └─ Wheel slip detection, per wheel     │
│
├─ TCS Calculation                        ┤ 8 μs
│  └─ Power modulation, per wheel         │
│
├─ PWM Update (HRTIM, hardware)           ┤ 1 μs
│  └─ Write to compare registers          │
│
├─ CAN TX (telemetry, buffered)           ┤ 3 μs
│  └─ Queue messages, hardware sends      │
│
├─ CAN RX (commands, interrupt)           ┤ 0 μs (background)
│
├─ Watchdog Kick                          ┤ 0.2 μs
│
├─ Fault Monitoring                       ┤ 2 μs
│  └─ Comparator status, relay control    │
│
└─ Idle / Margin                          ┤ 20.3 μs
                                          │
═════════════════════════════════════════════════════════════
Total CPU Load: ~30 μs / 50 μs = 60% @ 170 MHz
Margin: 40% for future features
═════════════════════════════════════════════════════════════
```

### ESP32-S3 Execution (No Hard Real-Time)

```
Main Loop @ variable (FreeRTOS)
═════════════════════════════════════════════════════════════

Core 0 (UI Task)
├─ Display Rendering                      ┤ ~20 ms
├─ Touch Processing                       ┤ ~5 ms
├─ Menu Updates                           ┤ ~2 ms
└─ LED Animations                         ┤ ~3 ms

Core 1 (Logic Task)
├─ CAN RX (telemetry from STM32)          ┤ ~1 ms
├─ CAN TX (commands to STM32)             ┤ ~1 ms
├─ LiDAR Processing                       ┤ ~10 ms
├─ Obstacle Logic                         ┤ ~5 ms
├─ Audio Control                          ┤ ~2 ms
└─ USB Logging                            ┤ ~3 ms

═════════════════════════════════════════════════════════════
No hard deadlines - soft real-time OK
If ESP32 freezes: STM32 continues motor control safely
═════════════════════════════════════════════════════════════
```

---

## 🛡️ Failure Modes and Mitigation

### Scenario 1: ESP32 Crashes

**Impact:**
- ❌ Display freezes
- ❌ Touch unresponsive
- ❌ LEDs停止动画
- ❌ Audio stops
- ❌ LiDAR data lost
- ✅ **Motors continue running** (STM32 independent)

**STM32 Response:**
1. **CAN Heartbeat Timeout** detected (100 ms)
2. **Enter Safe Mode:**
   - Gradually reduce throttle to 0% over 2 seconds
   - Maintain steering control
   - Enable ABS/TCS automatically
   - Activate hazard relays
3. **Wait for operator:**
   - Pedal still works (direct ADC to STM32 option)
   - Steering still works
   - Vehicle remains controllable
4. **ESP32 Recovery:**
   - When heartbeat resumes, exit safe mode
   - Resume normal operation

**Mitigation:**
- Dual heartbeat (ESP32→STM32 and STM32→ESP32)
- Watchdog on ESP32
- Brownout detector

### Scenario 2: STM32 Crashes

**Impact:**
- ❌ **Motors stop** (critical failure)
- ❌ Steering unresponsive
- ❌ All telemetry lost
- ✅ Display still works
- ✅ Operator aware of failure

**ESP32 Response:**
1. **CAN Heartbeat Timeout** detected (100 ms)
2. **Display CRITICAL ALERT:**
   - Red screen
   - "MOTOR CONTROLLER FAULT"
   - "PULL OVER SAFELY"
   - Audio alarm
3. **Log fault event** to USB
4. **Wait for power cycle**

**STM32 Internal Protection:**
- **Independent Watchdog (IWDG):** Resets MCU if software hangs
- **Window Watchdog (WWDG):** Detects timing violations
- **Hardware Comparators:** Cut PWM on overcurrent (no software)
- **Relays open automatically:** Power disconnected

**Mitigation:**
- STM32 is MORE reliable than ESP32 (simpler, deterministic)
- Hardware-level protections (comparators, watchdog)
- Dual-redundancy could add second STM32 (future, if needed)

### Scenario 3: CAN Bus Failure

**Impact:**
- Communication lost between MCUs

**STM32 Response:**
1. Enter safe mode (as in ESP32 crash)
2. Use last known valid commands (with timeout)
3. Monitor pedal directly if wired to STM32 ADC (optional)

**ESP32 Response:**
1. Display "COMMUNICATION ERROR"
2. Log event
3. Attempt CAN recovery (reset transceiver)

**Mitigation:**
- CAN has built-in error detection (CRC, ACK)
- Fallback to I2C (slower, backup channel)
- Pedal wired to both MCUs (redundancy)

### Scenario 4: Sensor Failure

**Single Wheel Speed Sensor Failed:**
- STM32 detects (no pulses for >500ms at speed)
- Disable ABS on that wheel only
- Notify ESP32 → Display warning
- Continue operation (3-wheel ABS)

**Encoder Failed:**
- STM32 detects (no Z pulse, erratic counts)
- Switch to open-loop steering
- Limit steering speed
- Notify ESP32 → Display warning

**Current Sensor Failed:**
- ADC reads out of range
- Use estimated current from PWM duty
- Disable TCS on that motor
- Notify ESP32 → Display warning

**Temperature Sensor Failed:**
- OneWire timeout
- Use conservative thermal model
- Reduce power limit on that motor
- Notify ESP32 → Display warning

---

## 🔧 Why One STM32 is Sufficient

### Hardware Resource Analysis

**Timers:**
- Need: 4 motors + 1 steering = 5 PWM pairs = 10 channels
- Available: HRTIM has 12 outputs → ✅ **Sufficient with margin**

**ADCs:**
- Need: 6 current sensors sampled simultaneously
- Available: 5 ADCs × 42 channels → ✅ **Plenty**

**GPIO:**
- Need:
  - 10 PWM outputs (HRTIM)
  - 10 direction pins
  - 4 wheel sensors
  - 3 encoder pins
  - 4 relay pins
  - 2 CAN pins
  - 1 OneWire pin
  - = **34 pins**
- Available: 54 I/O → ✅ **Sufficient**

**DMA:**
- Need: ADC streaming, PWM updates
- Available: 16 channels → ✅ **More than enough**

**Processing Power:**
- Current ESP32 does everything @ 240 MHz
- STM32 only does real-time control @ 170 MHz
- Load: ~60% → ✅ **40% margin for future**

**Why NOT Multiple STM32s:**
1. **No benefit:** All motors/sensors fit on one MCU
2. **Increased cost:** ~$6 per STM32
3. **CAN complexity:** More nodes = more messages
4. **Synchronization:** ABS/TCS needs all wheels instantly
5. **Reliability:** More components = more failure modes

**When Would You Need Multiple STM32s?**
- ❌ NOT for this application
- ✅ Large vehicles (>8 motors)
- ✅ Redundant systems (aircraft, safety-critical automotive)
- ✅ Distributed architecture (separate zones physically distant)

---

## 📊 Component Migration Plan

### Phase 1: STM32 Bring-Up (Hardware)

**1.1 Current Sensing Migration**
- **Remove:** 6× INA226 (I2C) + TCA9548A multiplexer
- **Add:** 6× Current shunt resistors (e.g., 0.001Ω, 75mV @ 75A)
- **Add:** 6× Op-amp circuits (STM32 internal or external)
- **Connect:** Shunt outputs to STM32 ADC pins
- **Benefit:** Real-time sampling, no I2C latency

**1.2 Motor Control Migration**
- **Remove:** 3× PCA9685 PWM drivers (I2C)
- **Add:** Direct connections BTS7960 to STM32 HRTIM pins
- **Remove:** MCP23017 GPIO expander (motor directions)
- **Add:** Direct connections BTS7960 IN1/IN2 to STM32 GPIO
- **Benefit:** Deterministic PWM, no I2C jitter

**1.3 Sensor Connections**
- **Move:** 4× Wheel sensors from ESP32 GPIO to STM32 EXTI
- **Move:** Encoder A/B/Z from ESP32 GPIO to STM32 Timer4
- **Move:** DS18B20 OneWire from ESP32 to STM32 GPIO
- **Keep:** Pedal on ESP32 ADC (send via CAN)

**1.4 Relay Control**
- **Move:** All 4 relays from ESP32 GPIO to STM32 GPIO
- **Reason:** Emergency shutdown must be local to motor controller

### Phase 2: CAN Bus Implementation

**2.1 Hardware**
- CAN transceiver: MCP2551 or TJA1050
- 120Ω terminators at each end
- Twisted pair cable
- Optional: Galvanic isolation (ISO1050)

**2.2 Software**
- STM32: HAL_CAN driver, 500 kbps
- ESP32: TWAI driver (CAN controller), 500 kbps
- Message definitions (see protocol section)
- Error handling and timeout detection

### Phase 3: Safety Features (STM32)

**3.1 ABS Implementation**
- Algorithm: Compare wheel speeds, detect slip
- Action: Modulate PWM to prevent lockup
- Rate: 20 kHz loop

**3.2 TCS Implementation**
- Algorithm: Detect wheel spin, reduce power
- Action: Modulate PWM to prevent slip
- Rate: 20 kHz loop

**3.3 Emergency Stop**
- Hardware: Comparators for overcurrent
- Software: Watchdog for hang detection
- Action: Open all relays, PWM to 0

### Phase 4: ESP32 Integration

**4.1 CAN Communication**
- Send throttle/steering commands
- Receive telemetry
- Heartbeat monitoring

**4.2 UI Updates**
- Display telemetry from STM32
- Show ABS/TCS indicators
- Fault code display

**4.3 LiDAR Integration**
- Obstacle detection stays on ESP32
- Decision logic stays on ESP32
- Send throttle reduction via CAN (not直接 motor control)

---

## 💡 Why This Design is Correct

### Principle 1: Safety Through Separation

✅ **Real-time control isolated from UI**
- UI crash cannot affect motor control
- Display glitch cannot cause runaway
- Touch freeze cannot disable brakes

✅ **Hardware protection layers**
- Comparators (nanosecond response)
- Watchdogs (millisecond response)
- Software (microsecond response)

### Principle 2: Right Tool for Right Job

✅ **ESP32-S3 for perception:**
- 16 MB Flash for UI assets
- Dual-core for parallel processing
- WiFi/BLE (disabled now, future OTA)
- RMT for WS2812B LEDs

✅ **STM32G474 for control:**
- HRTIM for ultra-precise PWM
- 5 ADCs for simultaneous sampling
- Comparators for instant protection
- Deterministic real-time execution

### Principle 3: Proven Automotive Architecture

✅ **This is how real cars work:**
```
ECU (Engine Control Unit) = STM32 role
  - Manages powertrain
  - Real-time control
  - Safety-critical

HMI (Human-Machine Interface) = ESP32 role
  - Displays information
  - User input
  - Non-critical
```

✅ **Examples:**
- Tesla: MCU (Infotainment) separate from Motor Controller
- BMW: iDrive (UI) separate from DME (Engine)
- Toyota: Navigation (UI) separate from ECM (Engine)

### Principle 4: Why LEDs Stay on ESP32

❌ **WRONG:** "LEDs are safety-critical because they're lights"

✅ **CORRECT:** "LEDs are user feedback, NOT safety"

**Analysis:**
- LEDs show status (turn signals, brake lights aesthetics)
- **Actual braking:** Controlled by STM32 (motor reversal, ABS)
- **Actual turns:** Controlled by STM32 (steering motor)
- LEDs failing → Visual cue lost, but vehicle still safe
- LEDs on STM32 → Wastes precious real-time I/O pins

**ESP32 has RMT peripheral:** Perfect for WS2812B timing
**STM32 has no RMT:** Would need SPI bit-banging (waste)

**Result:** LEDs on ESP32 = correct decision

### Principle 5: Why LiDAR Stays on ESP32

❌ **WRONG:** "LiDAR detects obstacles, must be on STM32"

✅ **CORRECT:** "LiDAR is perception, decision is on ESP32, execution via CAN"

**Analysis:**
- LiDAR: TOFSense-M S, 8×8 matrix, 400 bytes UART protocol
- **Perception:** ESP32 parses data, builds obstacle map
- **Decision:** ESP32 determines "obstacle too close"
- **Action:** ESP32 sends "reduce throttle 50%" via CAN to STM32
- **Execution:** STM32 applies motor control

**Why NOT on STM32:**
- 400-byte UART parsing → Wasted CPU on STM32
- Obstacle logic → Complex, not real-time-critical
- 512 KB Flash on STM32 → Limited but workable for perception algorithms

**ESP32 advantages:**
- 16 MB Flash → Room for obstacle algorithms
- Dual-core → Core 0 renders map on display, Core 1 processes
- If LiDAR fails → Display shows warning, vehicle still drives

**Result:** LiDAR on ESP32 = correct decision

---

## 📈 Latency Budget Analysis

### Critical Path: Pedal → Motor PWM

**Current System (ESP32 only):**
```
Pedal ADC read         →  100 μs  (I2C PCA9685 latency)
Traction calculation   →   50 μs
I2C write to PCA9685   →  500 μs  (I2C @ 400kHz)
PCA9685 PWM update     →  200 μs  (internal processing)
──────────────────────────────────
Total: ~850 μs (worst case)
```

**New System (ESP32 + STM32 via CAN):**
```
Pedal ADC read (ESP32)  →   10 μs  (native ADC)
CAN message TX (ESP32)  →   50 μs  (500 kbps)
CAN bus transmission    →   50 μs  (8 bytes @ 500 kbps)
CAN message RX (STM32)  →   10 μs  (interrupt)
Traction calculation    →   10 μs  (STM32 faster)
HRTIM PWM update        →    1 μs  (hardware register)
──────────────────────────────────
Total: ~131 μs (worst case)

Improvement: 6.5× faster! ✅
```

### Non-Critical Path: LiDAR → Display

**Current System:**
```
LiDAR UART RX           →   5 ms  (400 bytes @ 921600 baud)
Obstacle processing     →  10 ms
Display update          →  20 ms
──────────────────────────────────
Total: ~35 ms (acceptable for UI)
```

**New System (same, no change needed):**
```
Same as current: ~35 ms ✅
```

---

## 🎓 Summary & Recommendations

### ✅ This Architecture is Correct Because:

1. **Safety-driven partitioning:** Real-time + safety on STM32, perception + UI on ESP32
2. **One STM32 is sufficient:** All motors, sensors, relays fit comfortably
3. **Hardware-matched:** STM32G474RE designed for motor control, ESP32-S3 for UI/connectivity
4. **Automotive-grade:** Follows proven ECU + HMI pattern from real vehicles
5. **Failure isolation:** ESP32 crash doesn't affect motors, STM32 crash is detectable
6. **Latency improved:** 6.5× faster pedal-to-motor response
7. **Cost-effective:** Minimal hardware changes, uses existing peripherals optimally

### ❌ Why Multi-ECU (>2 MCUs) Would Be Wrong:

1. **No resource constraint:** One STM32 has capacity for all real-time tasks
2. **Increased cost:** Each additional STM32 = +$6, CAN transceivers = +$2
3. **CAN bus load:** More nodes = more messages = higher latency
4. **Synchronization complexity:** ABS/TCS needs all wheels instantly
5. **More failure modes:** Each MCU is a potential failure point
6. **Diminishing returns:** No performance gain for this application

### 📋 Next Steps:

1. **Prototype STM32 firmware:**
   - HRTIM PWM generation
   - ADC + DMA current sensing
   - Encoder interface
   - Wheel speed interrupts
   - ABS/TCS algorithms
   - CAN communication

2. **CAN protocol implementation:**
   - Message definitions
   - Heartbeat monitoring
   - Timeout handling
   - Error recovery

3. **ESP32 firmware modification:**
   - Remove PCA9685/MCP23017 code
   - Add CAN driver
   - Modify traction/steering to send CAN instead of I2C
   - Add telemetry display

4. **Hardware modification:**
   - Replace INA226 with shunt resistors
   - Remove I2C PWM drivers
   - Add CAN transceivers
   - Rewire sensors to STM32

5. **Testing & validation:**
   - Bench test STM32 motor control
   - Verify CAN communication
   - Test failure modes
   - Road test with monitoring

---

**Document Status:** ✅ Complete  
**Review Required:** Hardware Engineer, Safety Engineer  
**Implementation Timeline:** 8-12 weeks  
**Estimated Cost:** $50-100 in additional components  

---

**Author:** Automotive Embedded Systems Architect  
**Date:** 2026-01-13  
**Version:** 1.0 - Initial Design
