# CURRENT ARCHITECTURE MAP
# ESP32-S3 N16R8 Electric Vehicle Firmware

**Document Version:** 1.0  
**Firmware Version:** v2.17.1  
**Date:** 2026-01-13  
**Status:** 🔒 PRODUCTION BASELINE - DO NOT MODIFY

---

## EXECUTIVE SUMMARY

This document provides a complete technical inventory of the current single-ECU architecture running on ESP32-S3 N16R8 (16MB Flash + 8MB PSRAM). This is the baseline system **BEFORE** any CAN or multi-ECU migration.

**Current State:** All vehicle functions run on a single microcontroller with no redundancy.

---

## 1. SYSTEM INVENTORY

### 1.1 HARDWARE PLATFORM

| Component | Specification | Notes |
|-----------|--------------|-------|
| **Microcontroller** | ESP32-S3-WROOM-2 N16R8 | Dual-core Xtensa LX7 @ 240 MHz |
| **Flash Memory** | 16MB QIO @ 3.3V | 80 MHz, safe mode |
| **PSRAM** | 8MB QSPI @ 3.3V | 80 MHz, safe mode |
| **Available GPIOs** | 36 total, 34 used (94%) | 2 free GPIOs remain |
| **Development Board** | ESP32-S3-DevKitC-1 | 44-pin variant |

### 1.2 MAJOR SUBSYSTEMS

#### **MOTORS** (Safety-Critical)

| Subsystem | Hardware | Bus | GPIO/Channel | Timing |
|-----------|----------|-----|--------------|---------|
| **Traction Motors (4x4)** | 4x BTS7960 43A drivers | I2C + GPIO | PCA9685 (0x40, 0x41) + MCP23017 | 10ms loop |
| - Front Left (FL) | BTS7960 + RS550 12V | PWM via I2C | PCA 0x40 CH0-1, MCP A0-A1 | 50-100Hz |
| - Front Right (FR) | BTS7960 + RS550 12V | PWM via I2C | PCA 0x40 CH2-3, MCP A2-A3 | 50-100Hz |
| - Rear Left (RL) | BTS7960 + RS550 12V | PWM via I2C | PCA 0x41 CH0-1, MCP A4-A5 | 50-100Hz |
| - Rear Right (RR) | BTS7960 + RS550 12V | PWM via I2C | PCA 0x41 CH2-3, MCP A6-A7 | 50-100Hz |
| **Steering Motor** | BTS7960 + RS390 12V 6000RPM | PWM via I2C | PCA 0x42 CH0-1, MCP B5-B6 | 10ms loop |
| - Reduction Ratio | 1:50 gearbox | - | - | - |

#### **SENSORS** (Safety-Critical)

| Sensor | Hardware | Bus | GPIO/Channel | Frequency | Safety Role |
|--------|----------|-----|--------------|-----------|-------------|
| **Wheel Speed (4x)** | LJ12A3-4-Z/BX inductive | GPIO ISR | 7, 36, 15, 1 | 6 pulses/rev | ABS, TCS, speed |
| **Steering Encoder** | E6B2-CWZ6C 1200PR | GPIO ISR | 37 (A), 38 (B), 39 (Z) | 1200 PPR | Steering angle |
| **Pedal (Accelerator)** | A1324LUA-T Hall sensor | ADC | GPIO 4 (ADC1_CH3) | 50-100Hz | Throttle input |
| **Current Sensors (6x)** | INA226 + 50A/100A shunts | I2C + MUX | TCA9548A 0x70 CH0-5 | 10-50Hz | Over-current protection |
| - Battery 24V | INA226 + 100A shunt | I2C | TCA CH4 → INA 0x40 | 10Hz | Battery monitor |
| - Motor FL | INA226 + 50A shunt | I2C | TCA CH0 → INA 0x40 | 10Hz | Motor protection |
| - Motor FR | INA226 + 50A shunt | I2C | TCA CH1 → INA 0x40 | 10Hz | Motor protection |
| - Motor RL | INA226 + 50A shunt | I2C | TCA CH2 → INA 0x40 | 10Hz | Motor protection |
| - Motor RR | INA226 + 50A shunt | I2C | TCA CH3 → INA 0x40 | 10Hz | Motor protection |
| - Steering Motor | INA226 + 50A shunt | I2C | TCA CH5 → INA 0x40 | 10Hz | Steering protection |
| **Temperature (4x)** | DS18B20 OneWire | OneWire | GPIO 20 (shared bus) | 1-5Hz | Thermal protection |
| **Obstacle Detection** | TOFSense-M S LiDAR UART | UART0 | GPIO 44 (RX), 43 (TX) | 15Hz | Collision avoidance |

#### **DISPLAY AND HUD** (User Interface)

| Component | Hardware | Bus | GPIO | Frame Rate | Safety Role |
|-----------|----------|-----|------|------------|-------------|
| **TFT Display** | ST7796S 480x320 SPI | SPI | SCK=10, MOSI=11, MISO=12, CS=16, DC=13, RST=14 | 30 FPS | Visual feedback |
| **Touchscreen** | XPT2046 SPI | SPI (shared) | CS=21, IRQ=47 | Variable | User input |
| **Backlight** | PWM controlled | LEDC | GPIO 42 | PWM | Display visibility |
| **Compositor** | Software rendering | PSRAM | Frame buffers in PSRAM | 30 FPS | HUD overlay |

#### **AUDIO SYSTEM**

| Component | Hardware | Bus | GPIO | Timing | Safety Role |
|-----------|----------|-----|------|--------|-------------|
| **Audio Player** | DFPlayer Mini | UART1 | TX=18, RX=17 | Async | Safety alerts, turn signals |

#### **LIGHTING**

| Component | Hardware | Bus | GPIO | Control | Safety Role |
|-----------|----------|-----|------|---------|-------------|
| **Front LEDs** | WS2812B 28 LEDs | Data | GPIO 19 | FastLED | Headlights, turn signals |
| **Rear LEDs** | WS2812B 16 LEDs | Data | GPIO 48 | FastLED | Brake lights, turn signals |

#### **POWER MANAGEMENT** (Safety-Critical)

| Component | Function | Control | GPIO | Safety Role |
|-----------|----------|---------|------|-------------|
| **Main Relay** | Power hold circuit | Output | GPIO 35 | Prevents shutdown during operation |
| **Traction Relay** | 24V traction power | Output | GPIO 5 | Emergency cutoff |
| **Steering Relay** | 12V steering power | Output | GPIO 6 | Emergency cutoff |
| **Spare Relay** | Lights/media power | Output | GPIO 46 (strapping pin) | Auxiliary functions |
| **Key ON Detection** | Ignition sense | Input | GPIO 40 (INPUT_PULLUP) | Boot trigger |
| **Key OFF Request** | Shutdown request | Input | GPIO 41 (INPUT_PULLUP) | Shutdown trigger |

#### **INPUT DEVICES**

| Device | Hardware | Interface | GPIO/MCP | Timing | Safety Role |
|--------|----------|-----------|----------|--------|-------------|
| **Shifter** | 5-position rotary | MCP23017 GPIO | MCP B0-B4 (P,R,N,D1,D2) | Polling 10ms | Gear selection |
| **Lights Button** | Physical button | GPIO | GPIO 2 | Polling 10ms | Lights control |

#### **I2C DEVICES** (Control Bus)

| Device | Address | Function | Channels | Critical |
|--------|---------|----------|----------|----------|
| **TCA9548A** | 0x70 | I2C Multiplexer (INA226) | 8 channels, 6 used | ✅ Yes |
| **PCA9685 Front** | 0x40 | PWM Driver (FL+FR motors) | 16 channels, 4 used | ✅ Yes |
| **PCA9685 Rear** | 0x41 | PWM Driver (RL+RR motors) | 16 channels, 4 used | ✅ Yes |
| **PCA9685 Steering** | 0x42 | PWM Driver (steering motor) | 16 channels, 2 used | ✅ Yes |
| **MCP23017** | 0x20 | GPIO Expander (IN1/IN2 + Shifter) | 16 GPIOs, 13 used | ✅ Yes |
| **INA226 (6x)** | 0x40 (via TCA) | Current/Voltage Sensors | 6 sensors | ✅ Yes |

---

## 2. COMMUNICATION BUSES

### 2.1 I2C BUS (Primary Control)

**Configuration:**
- **SDA:** GPIO 8
- **SCL:** GPIO 9
- **Frequency:** 400 kHz (Fast Mode)
- **Timeout:** Read 100ms, Write 50ms
- **Pull-ups:** External 4.7kΩ required

**Bus Loading:**
```
TCA9548A (0x70)
├── CH0: INA226 (0x40) - Motor FL current
├── CH1: INA226 (0x40) - Motor FR current
├── CH2: INA226 (0x40) - Motor RL current
├── CH3: INA226 (0x40) - Motor RR current
├── CH4: INA226 (0x40) - Battery 24V
└── CH5: INA226 (0x40) - Steering motor current

PCA9685 (0x40) - Front motors PWM
PCA9685 (0x41) - Rear motors PWM
PCA9685 (0x42) - Steering motor PWM
MCP23017 (0x20) - GPIO expander
```

**Critical Dependencies:**
- ✅ **ALL motor control depends on I2C bus health**
- ✅ **Shifter reading depends on MCP23017**
- ✅ **Current monitoring depends on TCA9548A + INA226**

**Failure Mode:** If I2C bus hangs, **vehicle loses all motor control** (100ms timeout prevents total lockup).

### 2.2 SPI BUS (Display)

**Configuration:**
- **SCK:** GPIO 10
- **MOSI:** GPIO 11
- **MISO:** GPIO 12
- **TFT CS:** GPIO 16
- **Touch CS:** GPIO 21
- **Frequency:** 40 MHz (display), 2.5 MHz (touch)

**Critical Dependencies:**
- ❌ NOT safety-critical (display-only)
- ⚠️ High bandwidth usage (480x320x16bpp @ 30 FPS = ~7 MB/s theoretical)

### 2.3 UART BUSES

| UART | Device | TX | RX | Baudrate | Protocol | Critical |
|------|--------|----|----|----------|----------|----------|
| **UART0** | TOFSense-M S LiDAR | 43 | 44 | 115200 | 9-byte packets, header 0x57 | ⚠️ Safety (collision) |
| **UART1** | DFPlayer Mini | 18 | 17 | 9600 | DFPlayer protocol | ❌ No |

### 2.4 GPIO INTERRUPT SOURCES

| Interrupt | Pin | Function | Frequency | Handler | Critical |
|-----------|-----|----------|-----------|---------|----------|
| **IRAM_ATTR wheelISR0** | GPIO 7 | Wheel FL pulses | Variable | Pulse counter | ✅ Yes (speed) |
| **IRAM_ATTR wheelISR1** | GPIO 36 | Wheel FR pulses | Variable | Pulse counter | ✅ Yes (speed) |
| **IRAM_ATTR wheelISR2** | GPIO 15 | Wheel RL pulses | Variable | Pulse counter | ✅ Yes (speed) |
| **IRAM_ATTR wheelISR3** | GPIO 1 | Wheel RR pulses | Variable | Pulse counter | ✅ Yes (speed) |
| **IRAM_ATTR isrEncA** | GPIO 37 | Encoder A (quadrature) | High freq | Encoder decode | ✅ Yes (steering) |
| **IRAM_ATTR isrEncZ** | GPIO 39 | Encoder Z (zero index) | Once/rev | Centering | ✅ Yes (steering) |

**Total Interrupt Load:** 6 ISRs, all marked IRAM_ATTR for flash cache safety.

---

## 3. SOFTWARE ARCHITECTURE

### 3.1 MANAGER HIERARCHY

```
Level 0 (Core System)
├── System::init()        - System state
├── Storage::init()       - EEPROM config
├── Watchdog::init()      - 30s hardware watchdog
└── Logger::init()        - Logging system

Level 1 (No Dependencies)
└── PowerManager          - Relay control

Level 2 (Depend on Level 1)
├── SensorManager         → PowerManager
└── HUDManager           → PowerManager

Level 3 (Depend on Level 2)
├── SafetyManager        → SensorManager
├── ControlManager       → SensorManager, SafetyManager
└── ModeManager          → SensorManager, SafetyManager

Level 4 (Depend on All)
└── TelemetryManager     → All managers
```

### 3.2 MAIN LOOP EXECUTION ORDER

```cpp
void loop() {
    Watchdog::feed();               // 1. ALWAYS FIRST
    
    PowerManager::update();         // 2. Power monitoring
    SensorManager::update();        // 3. Read all sensors
    SafetyManager::update();        // 4. Safety validation
    
    VehicleMode mode = ModeManager::getCurrentMode();
    ModeManager::update();          // 5. Mode management
    
    ControlManager::update();       // 6. Motor control
    
    TelemetryManager::update();     // 7. Data logging
    HUDManager::update();           // 8. Display update
    
    delay(SYSTEM_TICK_MS);          // 9. Tick delay (10ms)
}
```

**Loop Frequency:** 100 Hz (10ms cycle)  
**Watchdog Timeout:** 30 seconds (300x safety margin)

### 3.3 TASKS AND THREADS

**Operating Model:** Single-threaded cooperative multitasking  
**FreeRTOS Usage:** Minimal (no custom tasks)  
**Loop Task Stack:** 32 KB (increased from default 8KB for bootloop prevention)

**No FreeRTOS tasks found** - All execution happens in main loop.

### 3.4 MEMORY LAYOUT

| Region | Size | Usage | Critical |
|--------|------|-------|----------|
| **Flash (Code)** | ~2-3 MB | Firmware binary | ✅ Yes |
| **PSRAM (Heap)** | 8 MB | Frame buffers, sprites | ❌ No (display only) |
| **SRAM (Stack)** | 512 KB total | Runtime variables | ✅ Yes |
| **EEPROM (Persistent)** | 24 KB (NVS) | Configuration, calibration | ⚠️ Yes (config) |

**Heap Usage:**
- Boot: ~312 KB free
- Post-init: ~280 KB free  
- Runtime: ~250-280 KB free  
- **Minimum safe:** 50 KB

---

## 4. TIMING CHARACTERISTICS

### 4.1 CRITICAL TIMING PATHS

| Subsystem | Update Rate | Latency Budget | Timeout | Failure Mode |
|-----------|-------------|----------------|---------|--------------|
| **Wheel Speed ISR** | Event-driven | <1 μs | N/A | Missed pulse → incorrect speed |
| **Encoder ISR** | Event-driven | <1 μs | N/A | Missed edge → steering error |
| **Motor PWM Update** | 100 Hz | 10 ms | 100 ms (I2C) | Motor freeze → emergency stop |
| **Current Monitoring** | 10 Hz | 100 ms | 100 ms (I2C) | No protection → potential damage |
| **Obstacle Detection** | 15 Hz | 66 ms | 100 ms (UART) | No warning → collision risk |
| **Pedal Reading** | 100 Hz | 10 ms | N/A (ADC) | Stale data → incorrect throttle |
| **HUD Update** | 30 Hz | 33 ms | N/A | Stale display → driver confusion |
| **Watchdog Feed** | 100 Hz | 10 ms | 30 s | System reset |

### 4.2 BUS ARBITRATION TIMING

**I2C Bus (400 kHz):**
- Single byte transfer: ~25 μs
- INA226 read (2 bytes): ~50 μs
- PCA9685 PWM write (3 bytes): ~75 μs
- **Worst case (6 INA226 + 3 PCA9685):** ~600 μs per cycle

**SPI Bus (40 MHz):**
- Display pixel (16-bit): 0.4 μs
- Full frame (480x320): ~77 ms theoretical (30 FPS = 33 ms actual with partial updates)

### 4.3 INTERRUPT PRIORITY

All ISRs use default ESP-IDF priority (level 1). No real-time OS task priorities configured.

---

## 5. SAFETY SYSTEMS

### 5.1 SAFETY LAYERS

| Layer | Function | Response Time | Failure Impact |
|-------|----------|---------------|----------------|
| **Watchdog** | Detects system hang | 30 s timeout | System reset |
| **Emergency Stop** | Immediate relay cutoff | <10 ms | Safe shutdown |
| **ABS System** | Prevents wheel lockup | 10 ms cycle | Loss of traction control |
| **TCS System** | Prevents wheel slip | 10 ms cycle | Loss of power control |
| **Obstacle Safety** | Collision avoidance | 66 ms (sensor lag) | Potential collision |
| **Over-current Protection** | INA226 monitoring | 100 ms | Motor damage risk |
| **Thermal Protection** | DS18B20 monitoring | 1-5 s | Motor damage risk |

### 5.2 FAILSAFE BEHAVIORS

| Failure Condition | Detection | Response | Recovery |
|-------------------|-----------|----------|----------|
| **I2C bus hang** | 100ms timeout | Skip operation, log error | Auto-retry next cycle |
| **Sensor invalid** | Data validation | Use last valid value | Auto-recover on valid data |
| **Over-current** | INA226 threshold | Emergency stop | Manual reset required |
| **Over-temperature** | DS18B20 threshold | Power reduction → emergency stop | Cool down required |
| **Obstacle critical** | LiDAR <20cm | Emergency brake | Manual override |
| **Watchdog timeout** | 30s no feed | Hardware reset | Full system reboot |

### 5.3 DEGRADED MODES

| Mode | Trigger | Functionality | Safety Level |
|------|---------|---------------|--------------|
| **FULL** | All sensors OK | Full operation | ✅ Optimal |
| **MINIMAL** | Battery + 2 wheels OK | Basic movement, no telemetry | ⚠️ Reduced |
| **EMERGENCY** | Battery OK only | Safety systems only | ⚠️ Limited |
| **SAFE** | Critical failure | System stopped, monitoring only | 🔒 Maximum |

---

## 6. POWER DISTRIBUTION

### 6.1 POWER DOMAINS

| Domain | Voltage | Relay | Current | Devices Powered |
|--------|---------|-------|---------|-----------------|
| **Main** | 24V | GPIO 35 | Variable | ESP32 power hold circuit |
| **Traction** | 24V | GPIO 5 | 200A max | 4x traction motors (BTS7960) |
| **Steering** | 12V | GPIO 6 | 50A max | Steering motor (BTS7960) |
| **Auxiliary** | 12V | GPIO 46 | 10A max | Lights, media |

### 6.2 POWER SEQUENCING

**Startup:**
1. Key ON (GPIO 40 LOW) → ESP32 boots
2. Main relay (GPIO 35 HIGH) → Power hold
3. Initialize sensors and I2C
4. Traction relay (GPIO 5 HIGH) → Enable motors
5. Steering relay (GPIO 6 HIGH) → Enable steering

**Shutdown:**
1. Key OFF (GPIO 41 LOW) or emergency
2. Steering relay (GPIO 6 LOW) → Disable steering
3. Traction relay (GPIO 5 LOW) → Disable motors
4. Save config to EEPROM
5. Main relay (GPIO 35 LOW) → Power off (100ms delay)

---

## 7. CRITICAL DEPENDENCIES

### 7.1 SINGLE POINT OF FAILURE ANALYSIS

| Component | Function | If Lost | Workaround | Impact |
|-----------|----------|---------|------------|--------|
| **ESP32-S3** | Main controller | Total system failure | NONE | 🔴 CRITICAL |
| **I2C Bus** | Motor control | No propulsion | NONE | 🔴 CRITICAL |
| **TCA9548A** | INA226 access | No current monitoring | Continue without protection | 🟠 HIGH |
| **All 3x PCA9685** | Motor PWM | No propulsion | NONE | 🔴 CRITICAL |
| **MCP23017** | Shifter + motor direction | No gear change / motor direction | NONE | 🔴 CRITICAL |
| **All 4x Wheel Sensors** | Speed + ABS/TCS | Degraded safety | Continue without ABS/TCS | 🟠 HIGH |
| **Steering Encoder** | Steering angle | Manual steering estimation | Continue with open-loop | 🟡 MEDIUM |
| **TOFSense-M S** | Obstacle detection | No collision avoidance | Continue without assist | 🟡 MEDIUM |
| **TFT Display** | Visual feedback | No driver display | Continue blind | 🟡 MEDIUM |
| **Main Relay** | Power hold | System shutdown | External power hold | 🔴 CRITICAL |

### 7.2 CASCADE FAILURE SCENARIOS

**Scenario 1: I2C Bus Failure**
```
I2C bus hangs
 ├─> PCA9685 unavailable → No motor PWM → EMERGENCY STOP
 ├─> MCP23017 unavailable → No shifter reading → GEAR LOCKED
 └─> TCA9548A unavailable → No INA226 → NO OVERCURRENT PROTECTION
```

**Scenario 2: Power Supply Instability**
```
24V brownout
 ├─> ESP32 brownout detector → System reset
 ├─> Motors lose power → Uncontrolled coast
 └─> Loss of telemetry → Unknown state
```

**Scenario 3: Watchdog Timeout**
```
Software hang (blocked I2C, infinite loop)
 ├─> Watchdog not fed for 30s
 ├─> Hardware watchdog triggers reset
 ├─> System reboots
 └─> Boot counter increments → Safe mode on repeated failures
```

---

## 8. COUPLING ANALYSIS

### 8.1 TIGHT COUPLINGS

| Module A | Module B | Coupling Type | Risk | Decoupling Difficulty |
|----------|----------|---------------|------|----------------------|
| **TFT Display** | **PSRAM** | Memory allocation | 🔴 High | Hard (requires full refactor) |
| **HUDManager** | **TFT_eSPI** | Direct library calls | 🔴 High | Hard (rendering engine) |
| **ControlManager** | **I2C Bus** | Hardware dependency | 🔴 Critical | Hard (motor control) |
| **SensorManager** | **I2C Bus** | Hardware dependency | 🟠 Medium | Medium (some sensors GPIO) |
| **Traction** | **PCA9685** | PWM generation | 🔴 Critical | Hard (motor PWM) |
| **Shifter** | **MCP23017** | GPIO reading | 🔴 Critical | Medium (could use direct GPIO) |

### 8.2 CRASH PROPAGATION PATHS

**USB Crash Propagation:**
- ⚠️ **HUDManager (TFT rendering)** can cause USB issues if PSRAM fails during DMA operations
- ⚠️ **Logger (Serial output)** blocks if USB buffer full (no timeout implemented)

**Boot Failure Propagation:**
- ✅ **Boot guard** implemented (v2.17.1) - detects bootloops and enters safe mode
- ✅ **Stack overflow protection** - increased loop stack to 32KB
- ⚠️ **I2C device init failures** can cause cascading failures in dependent managers

### 8.3 ISOLATION ANALYSIS

**Well Isolated:**
- ✅ Audio system (DFPlayer) - fails gracefully
- ✅ Obstacle detection - fail-safe to disabled state
- ✅ LED lighting - cosmetic failure only

**Poorly Isolated:**
- ❌ Motor control (I2C) - single bus for all motors
- ❌ Shifter reading (MCP23017) - no fallback if I2C fails
- ❌ Current sensing (TCA9548A) - single multiplexer for all sensors

---

## 9. RESOURCE UTILIZATION

### 9.1 GPIO USAGE

**Total GPIOs:** 36 available  
**Used:** 34 (94% utilization)  
**Free:** GPIO 3, GPIO 40, GPIO 41 (after v2.15.0 power pin migration)

**Strapping Pins in Use:**
- ⚠️ GPIO 46 (Boot mode / ROM log) - **RELAY_SPARE** - potential boot issues

### 9.2 I2C ADDRESS SPACE

**Used Addresses:**
- 0x20: MCP23017 (GPIO expander)
- 0x40: PCA9685 Front, INA226 (via TCA9548A)
- 0x41: PCA9685 Rear
- 0x42: PCA9685 Steering
- 0x70: TCA9548A (I2C multiplexer)

**Available Addresses:** Many (7-bit space = 128 addresses)

### 9.3 PWM CHANNELS

**PCA9685 Front (0x40):** 4/16 channels used  
**PCA9685 Rear (0x41):** 4/16 channels used  
**PCA9685 Steering (0x42):** 2/16 channels used  
**LEDC (Backlight):** 1 channel used

**Total available:** 48 PWM channels, 11 used (23%)

---

## 10. MAINTENANCE AND DIAGNOSTICS

### 10.1 BUILT-IN DIAGNOSTICS

- ✅ Sensor health monitoring (SystemStatus API)
- ✅ Input device diagnostics (InputDeviceStatus API)
- ✅ I2C timeout detection and logging
- ✅ Boot counter and bootloop detection
- ✅ Watchdog feed counter
- ✅ Memory usage tracking (heap free)

### 10.2 LOGGING INFRASTRUCTURE

**Logger Levels:**
- DEBUG (0)
- INFO (1) ← Current level
- WARNING (2)
- ERROR (3)

**Log Destinations:**
- Serial UART (115200 baud)
- No file logging implemented

### 10.3 CONFIGURATION PERSISTENCE

**Storage Backend:** NVS (Non-Volatile Storage) - 24KB  
**Stored Data:**
- Sensor calibration (pedal, encoder)
- Display calibration (touch)
- User preferences (brightness, mode)
- Obstacle detection thresholds

---

## CONCLUSION

The current architecture is a **monolithic single-ECU design** where the ESP32-S3 is a single point of failure for all vehicle functions. The system has evolved organically with strong safety features (watchdog, boot guard, degraded modes) but lacks redundancy and real-time determinism.

**Key Strengths:**
- ✅ Comprehensive safety layers (watchdog, emergency stop, degraded modes)
- ✅ Well-structured manager hierarchy with clear dependencies
- ✅ Good diagnostic and logging infrastructure
- ✅ Boot protection (bootloop detection, safe mode)

**Key Weaknesses:**
- ❌ Single point of failure (ESP32-S3 controls everything)
- ❌ I2C bus is critical path for all motor control
- ❌ No redundancy for safety-critical sensors
- ❌ Tight coupling between display and PSRAM can affect system stability
- ❌ No CAN bus for multi-ECU architecture
- ❌ No real-time guarantees (cooperative multitasking only)

**Next Steps:** See CAN_MIGRATION_ANALYSIS.md for offload candidates and TARGET_ECU_ARCHITECTURE.md for proposed multi-ECU design.

---

**Document Authority:** Automotive Systems Architect  
**Review Status:** ✅ Technical baseline established  
**Confidentiality:** Internal Use Only
