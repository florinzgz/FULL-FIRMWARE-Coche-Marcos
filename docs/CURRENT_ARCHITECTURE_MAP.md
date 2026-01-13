# CURRENT ARCHITECTURE MAP

**Document Version:** 1.0  
**Firmware Version:** v2.17.1 (PHASE 14)  
**Hardware Platform:** ESP32-S3 N16R8 (16MB Flash QIO + 8MB PSRAM QSPI @ 3.3V)  
**Architecture:** Monolithic Single-ECU  
**Date:** 2026-01-13

---

## EXECUTIVE SUMMARY

This document provides a comprehensive technical map of the current production-grade electric vehicle firmware running on a single ESP32-S3 microcontroller. The system manages all vehicle functions including propulsion, steering, safety systems, telemetry, and user interface in a monolithic architecture.

**Key Metrics:**
- **Total Lines of Code:** ~30,115 lines
- **Source Files:** 75 .cpp files, 85 .h files
- **GPIO Utilization:** 34/36 GPIOs (94%)
- **I2C Devices:** 10 devices (3x PCA9685, 1x MCP23017, 6x INA226 via TCA9548A)
- **Update Frequency:** 10ms main loop (100 Hz)
- **Real-time Interrupts:** 7 ISRs (4 wheel sensors, 2 encoder, 1 touch)

---

## 1. SYSTEM OVERVIEW

### 1.1 Hardware Platform

```
┌─────────────────────────────────────────────────────────────────┐
│                    ESP32-S3-WROOM-2 N16R8                       │
│  • Dual-core Xtensa LX7 @ 240 MHz                              │
│  • 16MB Flash (QIO mode, 4-bit, 3.3V)                          │
│  • 8MB PSRAM (QSPI mode, 4-bit, 3.3V)                          │
│  • 36 GPIOs available (34 used)                                │
│  • Hardware FreeRTOS support                                    │
└─────────────────────────────────────────────────────────────────┘
```

### 1.2 Power Architecture

```
24V Battery ──┬──> Buck 5V ──> ESP32-S3 + Peripherals
              ├──> 12V Rail ──> Steering Motor + Auxiliaries
              └──> 24V Rail ──> Traction Motors (4x)
```

**Power Management:**
- 4x Relay Control (SRD-05VDC-SL-C)
  - RELAY_MAIN (GPIO 35): Power Hold
  - RELAY_TRAC (GPIO 5): 24V Traction
  - RELAY_DIR (GPIO 6): 12V Steering
  - RELAY_SPARE (GPIO 46): Auxiliary/Lights
- Dual-pin ignition detection (GPIO 40/41)
- 5-second graceful shutdown sequence

### 1.3 System Timing

```
Main Loop: 10ms (SYSTEM_TICK_MS)
├─ PowerManager::update()      ~1ms
├─ SensorManager::update()     ~2-3ms (I2C operations)
├─ SafetyManager::update()     ~1ms
├─ ModeManager::update()       ~0.5ms
├─ ControlManager::update()    ~2ms
├─ TelemetryManager::update()  ~0.5ms
└─ HUDManager::update()        ~3-4ms (SPI display)

Total: ~10-12ms per cycle (allowing headroom)
```

---

## 2. MAJOR SUBSYSTEMS INVENTORY

### 2.1 PROPULSION SYSTEM

#### 2.1.1 Traction Motors (4WD Independent Control)

**Hardware:**
- 4x RS775 motors (15000 RPM nominal, 1:75 gear reduction)
- 4x BTS7960 H-Bridge drivers (43A continuous)
- 2x PCA9685 PWM controllers (I2C 0x40, 0x41)
- 1x MCP23017 GPIO expander (I2C 0x20) for direction control

**Interfaces:**
- **Bus:** I2C @ 400kHz
- **Timing:** PWM @ 1kHz, 12-bit resolution (0-4095)
- **Control Pins (MCP23017):** 8 pins (IN1/IN2 for each motor)

**Files:**
- `src/control/traction.cpp` (26,899 bytes)
- `include/traction.h`
- `src/control/tcs_system.cpp` (8,549 bytes) - Traction Control

**Safety Role:** CRITICAL
- Direct vehicle propulsion control
- Emergency stop capability
- Current monitoring per wheel (INA226)
- TCS intervention for slip control

**Timing Requirements:**
- Update rate: 100 Hz (every 10ms)
- PWM update latency: <1ms
- Emergency stop response: <50ms

---

#### 2.1.2 Steering Motor

**Hardware:**
- 1x RS390 motor (6000 RPM, 1:50 gear reduction)
- 1x BTS7960 H-Bridge driver
- 1x PCA9685 PWM controller (I2C 0x42)
- 1x E6B2-CWZ6C quadrature encoder (1200 PPR)

**Interfaces:**
- **Bus:** I2C @ 400kHz + GPIO interrupts
- **Encoder Pins:** GPIO 37 (A), GPIO 38 (B), GPIO 39 (Z)
- **PWM:** 1kHz, 12-bit resolution

**Files:**
- `src/control/steering_motor.cpp` (8,177 bytes)
- `src/input/steering.cpp` (9,931 bytes)
- `src/control/steering_model.cpp` (2,043 bytes)

**Safety Role:** CRITICAL
- Direct steering control
- Encoder feedback for position control
- Current limiting (30A max)
- Deadband compensation

**Timing Requirements:**
- Position control loop: 100 Hz
- Encoder ISR: IRAM_ATTR (hard real-time)
- Response latency: <10ms

---

### 2.2 SENSOR SYSTEMS

#### 2.2.1 Current Sensors (6x INA226)

**Hardware:**
- 6x INA226 current/voltage/power monitors
- 1x TCA9548A I2C multiplexer (address 0x70)
- External shunts: 4x 50A (motors), 1x 100A (battery), 1x 50A (steering)

**Distribution:**
- Channel 0: Front-Left Motor (50A shunt)
- Channel 1: Front-Right Motor (50A shunt)
- Channel 2: Rear-Left Motor (50A shunt)
- Channel 3: Rear-Right Motor (50A shunt)
- Channel 4: 24V Battery (100A shunt)
- Channel 5: Steering Motor (50A shunt)

**Interfaces:**
- **Bus:** I2C @ 400kHz via TCA9548A
- **Update Rate:** 20 Hz (50ms interval)
- **Protected by:** FreeRTOS mutex

**Files:**
- `src/sensors/current.cpp` (13,731 bytes)
- `include/current.h`

**Safety Role:** HIGH
- Overcurrent detection
- Power monitoring
- Battery state tracking
- Motor health diagnostics

**Timing Requirements:**
- Update rate: 20 Hz (non-blocking)
- I2C transaction: <5ms per sensor
- Mutex timeout: 100ms

---

#### 2.2.2 Wheel Speed Sensors (4x Inductive)

**Hardware:**
- 4x LJ12A3-4-Z/BX inductive proximity sensors
- 2x HY-M158 optocouplers (PC817, 12V→3.3V isolation)
- 6 pulses per wheel revolution

**Interfaces:**
- **Pins:** GPIO 7 (FL), GPIO 36 (FR), GPIO 15 (RL), GPIO 1 (RR)
- **Interrupts:** 4x IRAM_ATTR ISRs on RISING edge
- **Processing:** Pulse counting + speed calculation

**Files:**
- `src/sensors/wheels.cpp` (4,451 bytes)
- `include/wheels.h`

**Safety Role:** CRITICAL
- ABS system input
- TCS system input
- Speedometer
- Slip detection

**Timing Requirements:**
- ISR response: <1µs (IRAM)
- Speed calculation: 100 Hz
- Sensor timeout: 1000ms (stopped detection)

---

#### 2.2.3 Temperature Sensors (4x DS18B20)

**Hardware:**
- 4x DS18B20 OneWire digital temperature sensors
- Parallel bus configuration
- One sensor per traction motor

**Interfaces:**
- **Bus:** OneWire @ GPIO 20
- **Protocol:** Dallas 1-Wire
- **Resolution:** 12-bit (0.0625°C)

**Files:**
- `src/sensors/temperature.cpp` (8,018 bytes)
- `include/temperature.h`

**Safety Role:** MEDIUM
- Thermal protection
- Motor overheating detection
- Preventive throttle reduction

**Timing Requirements:**
- Update rate: 1 Hz (slow, blocking)
- Conversion time: ~750ms per sensor
- Non-critical path

---

#### 2.2.4 Obstacle Detection (TOFSense-M S LiDAR)

**Hardware:**
- 1x TOFSense-M S 8x8 Matrix LiDAR
- UART interface (115200 baud)
- 64 distance points, 4m range, 65° FOV

**Interfaces:**
- **Bus:** UART0 (native)
- **Pins:** GPIO 44 (RX), GPIO 43 (TX - unused)
- **Protocol:** 400-byte frame @ ~15 Hz

**Files:**
- `src/sensors/obstacle_detection.cpp` (18,804 bytes)
- `src/safety/obstacle_safety.cpp` (11,673 bytes)
- `include/obstacle_detection.h`

**Safety Role:** HIGH
- Collision avoidance
- Speed reduction in proximity
- Emergency braking trigger

**Timing Requirements:**
- Update rate: 15 Hz (sensor limited)
- UART processing: async, non-blocking
- Reaction time: <100ms

---

#### 2.2.5 Pedal Input (Analog Hall Sensor)

**Hardware:**
- 1x A1324LUA-T Hall effect sensor
- Analog output: 0-5V → voltage divider → 0-3.3V
- ADC input

**Interfaces:**
- **Pin:** GPIO 4 (ADC1_CH3)
- **Resolution:** 12-bit ADC (0-4095)
- **Filtering:** EMA filter (α=0.2)

**Files:**
- `src/input/pedal.cpp` (6,326 bytes)
- `include/pedal.h`

**Safety Role:** CRITICAL
- Throttle control input
- Deadzone validation
- Safety threshold: 5% rest tolerance

**Timing Requirements:**
- Update rate: 100 Hz
- ADC read: <100µs
- Filter response: ~50ms (5 time constants)

---

### 2.3 USER INTERFACE

#### 2.3.1 TFT Display (ST7796S 480x320)

**Hardware:**
- ST7796S controller
- SPI interface @ 40 MHz
- 16-bit color (RGB565)
- Backlight PWM control

**Interfaces:**
- **Bus:** HSPI
- **Pins:** 
  - SCLK=10, MOSI=11, MISO=12
  - CS=16, DC=13, RST=14
  - Backlight=42 (PWM)
- **Framebuffer:** 480x320x2 = 300KB (in PSRAM)

**Files:**
- `src/hud/hud.cpp` (66,995 bytes)
- `src/hud/hud_manager.cpp` (42,803 bytes)
- `src/hud/hud_compositor.cpp` (22,857 bytes)
- `src/hud/gauges.cpp` (15,475 bytes)
- `src/hud/render_engine.cpp` (6,234 bytes)

**Safety Role:** LOW
- Non-critical display
- Error indication
- User feedback
- System continues operation if display fails

**Timing Requirements:**
- Update rate: ~30 Hz (33ms)
- SPI transaction: ~10ms per full frame
- Dirty rectangle optimization active

---

#### 2.3.2 Touch Screen (XPT2046)

**Hardware:**
- XPT2046 resistive touch controller
- SPI interface @ 2.5 MHz (shared with TFT)
- Interrupt-driven detection

**Interfaces:**
- **CS Pin:** GPIO 21
- **IRQ Pin:** GPIO 47
- **Calibration:** Stored in EEPROM

**Files:**
- `src/hud/touch_calibration.cpp` (15,892 bytes)
- `src/hud/touch_map.cpp` (1,460 bytes)

**Safety Role:** LOW
- Menu navigation only
- Physical button backup available
- Not required for vehicle operation

**Timing Requirements:**
- Scan rate: ~60 Hz (when touched)
- Debounce: 100ms
- Non-blocking

---

#### 2.3.3 LED Lighting (WS2812B)

**Hardware:**
- 28x WS2812B front LEDs (GPIO 19)
- 16x WS2812B rear LEDs (GPIO 48)
- Intelligent RGB control

**Interfaces:**
- **Protocol:** WS2812B (800 kHz bitstream)
- **Library:** FastLED
- **Effects:** KITT scanner, rainbow, throttle gradient

**Files:**
- `src/lighting/led_controller.cpp` (16,215 bytes)
- `include/led_controller.h`

**Safety Role:** MEDIUM
- Turn signal indication
- Brake lights
- Reverse lights
- Running lights

**Timing Requirements:**
- Update rate: 20 Hz (50ms)
- Bit timing: critical (±150ns tolerance)
- Protected by watchdog feeds

---

#### 2.3.4 Audio (DFPlayer Mini)

**Hardware:**
- DFPlayer Mini MP3 module
- UART interface @ 9600 baud
- SD card storage (68 audio tracks)

**Interfaces:**
- **Bus:** UART1
- **Pins:** GPIO 18 (TX), GPIO 17 (RX)
- **Control:** Command-based protocol

**Files:**
- `src/audio/dfplayer.cpp` (6,819 bytes)
- `include/dfplayer.h`

**Safety Role:** LOW
- User feedback only
- Warning sounds
- Non-critical operation

**Timing Requirements:**
- Command latency: ~100ms
- Non-blocking queue
- Failure tolerant

---

### 2.4 SAFETY SYSTEMS

#### 2.4.1 ABS (Anti-lock Braking System)

**Implementation:**
- Software-based wheel slip detection
- Pressure modulation via PWM reduction
- Multi-wheel coordination

**Interfaces:**
- **Inputs:** 4x wheel speeds, vehicle reference speed
- **Outputs:** PWM modulation to traction system
- **Cycle Rate:** 10 Hz (100ms cycles)

**Files:**
- `src/safety/abs_system.cpp` (5,823 bytes)
- `include/abs_system.h`

**Safety Role:** CRITICAL
- Prevents wheel lock
- Maintains steering control during braking
- Slip threshold: 15%
- Min activation speed: 10 km/h

**Timing Requirements:**
- Update rate: 100 Hz
- Intervention latency: <100ms
- Recovery time: configurable

---

#### 2.4.2 TCS (Traction Control System)

**Implementation:**
- Wheel slip monitoring
- Differential throttle reduction
- Per-wheel intervention

**Files:**
- `src/control/tcs_system.cpp` (8,549 bytes)
- `include/tcs_system.h`

**Safety Role:** CRITICAL
- Prevents wheel spin
- Maintains traction
- Works with ABS

**Timing Requirements:**
- Update rate: 100 Hz
- Response: <50ms

---

#### 2.4.3 Regenerative Braking (AI-Enhanced)

**Implementation:**
- Intelligent regen based on vehicle state
- Gradual engagement
- Current limiting

**Files:**
- `src/safety/regen_ai.cpp` (7,523 bytes)
- `include/regen_ai.h`

**Safety Role:** MEDIUM
- Energy recovery
- Brake assist
- Smooth deceleration

**Timing Requirements:**
- Update rate: 100 Hz
- Engagement: gradual (200ms ramp)

---

#### 2.4.4 Watchdog System

**Implementation:**
- Hardware watchdog @ 30 seconds
- Software feed every loop iteration
- Boot counter for bootloop detection

**Files:**
- `src/core/watchdog.cpp` (2,683 bytes)
- `src/core/boot_guard.cpp` (4,945 bytes)

**Safety Role:** CRITICAL
- System reset on hang
- Bootloop prevention
- Safe mode activation

**Timing Requirements:**
- Feed interval: <30s
- Boot detection: 3 boots in 60s window

---

### 2.5 CORE INFRASTRUCTURE

#### 2.5.1 I2C Bus Management

**Configuration:**
- **Frequency:** 400 kHz (fast mode)
- **Pins:** SDA=GPIO 8, SCL=GPIO 9
- **Recovery:** Automated bus recovery system
- **Protection:** FreeRTOS mutex

**Devices (10 total):**
1. PCA9685 @ 0x40 (Front motors)
2. PCA9685 @ 0x41 (Rear motors)
3. PCA9685 @ 0x42 (Steering motor)
4. MCP23017 @ 0x20 (GPIO expander)
5. TCA9548A @ 0x70 (INA226 multiplexer)
6-11. INA226 @ 0x40 (6x, via TCA channels 0-5)

**Files:**
- `src/i2c.cpp` (4,685 bytes)
- `src/core/i2c_recovery.cpp` (11,352 bytes)

**Safety Role:** CRITICAL
- All motor control depends on I2C
- Bus hang = loss of control
- Recovery system essential

**Timing Requirements:**
- Transaction timeout: 100ms
- Recovery time: <500ms
- Mutex protected

---

#### 2.5.2 Storage & Configuration

**Implementation:**
- EEPROM persistence (Preferences library)
- Touch calibration data
- User settings
- Error log ring buffer

**Files:**
- `src/core/storage.cpp` (11,495 bytes)
- `src/core/eeprom_persistence.cpp` (10,948 bytes)
- `src/core/config_storage.cpp` (5,766 bytes)

**Safety Role:** LOW
- Configuration persistence
- Non-volatile storage
- Graceful degradation on failure

---

#### 2.5.3 Logging & Telemetry

**Implementation:**
- Multi-level logging (DEBUG, INFO, WARN, ERROR)
- Serial output @ 115200 baud
- Telemetry data collection

**Files:**
- `src/core/logger.cpp` (3,062 bytes)
- `src/core/telemetry.cpp` (8,110 bytes)

**Safety Role:** LOW
- Diagnostics only
- Not required for operation

---

## 3. INTERRUPT SERVICE ROUTINES (ISRs)

All ISRs are marked `IRAM_ATTR` for execution from RAM (critical timing):

| ISR Name | Pin | Trigger | Purpose | Frequency |
|----------|-----|---------|---------|-----------|
| `wheelISR0` | GPIO 7 | RISING | FL wheel speed | ~6 Hz @ 1 km/h |
| `wheelISR1` | GPIO 36 | RISING | FR wheel speed | ~6 Hz @ 1 km/h |
| `wheelISR2` | GPIO 15 | RISING | RL wheel speed | ~6 Hz @ 1 km/h |
| `wheelISR3` | GPIO 1 | RISING | RR wheel speed | ~6 Hz @ 1 km/h |
| `isrEncA` | GPIO 37 | CHANGE | Encoder A channel | ~1200 Hz max |
| `isrEncZ` | GPIO 39 | RISING | Encoder Z (center) | ~1 Hz |
| *(Touch IRQ)* | GPIO 47 | FALLING | Touch detection | Event-driven |

**Total ISR Load:** Variable, low overhead (<1% CPU)

---

## 4. MEMORY UTILIZATION

### 4.1 Static Allocation

```
ESP32-S3 Internal RAM (~400KB available):
├─ .text (code): ~250KB
├─ .data (initialized): ~30KB
├─ .bss (uninitialized): ~50KB
└─ Heap (dynamic): ~70KB free

PSRAM (8MB external):
├─ TFT framebuffer: 300KB
├─ Sprite buffers: 200KB
├─ Audio buffers: 100KB
└─ Dynamic allocations: ~7.4MB available
```

### 4.2 Critical Heap Requirements

- Minimum heap for init: 50KB
- Minimum heap after init: 25KB
- Watchdog monitoring enabled

---

## 5. COMMUNICATION BUSES

### 5.1 Bus Inventory

| Bus | Frequency | Usage | Critical |
|-----|-----------|-------|----------|
| **I2C** | 400 kHz | Motors, sensors, GPIO expander | ✅ YES |
| **SPI (HSPI)** | 40 MHz | Display + Touch | ❌ NO |
| **UART0** | 115200 | TOFSense LiDAR | ⚠️  MEDIUM |
| **UART1** | 9600 | DFPlayer audio | ❌ NO |
| **OneWire** | ~16 kHz | Temperature sensors | ❌ NO |
| **WS2812B** | 800 kHz | LED strips | ⚠️  MEDIUM |

### 5.2 Bus Failure Modes

**I2C Bus Hang:**
- **Impact:** Loss of motor control, system crash
- **Mitigation:** Automated recovery, mutex protection
- **Recovery Time:** <500ms

**SPI Display Failure:**
- **Impact:** No visual feedback
- **Mitigation:** System continues operation
- **Fallback:** LED indicators, audio alerts

**UART Timeout:**
- **Impact:** No obstacle detection or audio
- **Mitigation:** Graceful degradation
- **Behavior:** Placeholder mode, continue operation

---

## 6. TASK ARCHITECTURE

### 6.1 FreeRTOS Configuration

```
Main Loop (Arduino loop):
- Priority: Normal
- Stack: 32KB
- Core: Core 1

Background Tasks:
- WiFi task: DISABLED (v2.11.0 standalone mode)
- Watchdog feed: Every 10ms
- No custom FreeRTOS tasks created
```

### 6.2 Blocking Operations

**Identified Blocking Calls:**
- OneWire temperature read: ~750ms per sensor (3s total for 4 sensors)
- I2C transactions: <5ms per device (mutex protected)
- SPI display updates: ~10ms per frame

**Mitigation:**
- Temperature reads infrequent (1 Hz)
- I2C protected by timeouts
- Display updates optimized with dirty rectangles

---

## 7. DEPENDENCY GRAPH

```
                          ┌─────────────┐
                          │ PowerManager│
                          └──────┬──────┘
                                 │
                    ┌────────────┴────────────┐
                    ▼                         ▼
            ┌──────────────┐          ┌─────────────┐
            │SensorManager │          │SafetyManager│
            └──────┬───────┘          └──────┬──────┘
                   │                         │
          ┌────────┴────────┐         ┌──────┴──────┐
          ▼                 ▼         ▼             ▼
    ┌─────────┐      ┌──────────┐  ┌────┐      ┌────┐
    │ Current │      │  Wheels  │  │ABS │      │TCS │
    │ Sensors │      │Temperature│  └────┘      └────┘
    └─────────┘      └──────────┘      │              
          │                 │           │              
          └─────────┬───────┴───────────┘
                    ▼
            ┌───────────────┐
            │ ControlManager│
            └───────┬───────┘
                    │
          ┌─────────┴─────────┐
          ▼                   ▼
    ┌──────────┐        ┌──────────┐
    │ Traction │        │ Steering │
    │  Motor   │        │  Motor   │
    └──────────┘        └──────────┘
          │                   │
          └─────────┬─────────┘
                    ▼
               ┌─────────┐
               │   I2C   │
               │   Bus   │
               └─────────┘

    ┌────────────┐        ┌──────────┐
    │HUDManager  │◄───────┤ All Data │
    └────────────┘        └──────────┘
```

---

## 8. FAILURE PROPAGATION PATHS

### 8.1 Critical Failures (System Stops)

```
I2C Bus Hang ──> Motor Control Lost ──> WATCHDOG RESET
   │
   ├──> Recovery Attempt (500ms)
   └──> If recovered: Continue operation
```

### 8.2 Non-Critical Failures (Degraded Mode)

```
Display Failure ──> Visual feedback lost
   │
   └──> System continues with LED/Audio feedback

Audio Failure ──> No sound
   │
   └──> System continues normally

Temperature Sensor Fail ──> Thermal protection lost
   │
   └──> Continue with conservative limits
```

### 8.3 Safety-Critical Failures

```
Wheel Sensor Failure ──> ABS/TCS disabled for that wheel
   │
   └──> Alert user, continue operation

Pedal Sensor Failure ──> Enter limp mode
   │
   └──> Fixed speed, no throttle response

Steering Encoder Fail ──> Position control degraded
   │
   └──> Open-loop control with timeout
```

---

## 9. BOOT SEQUENCE

```
1. Serial Init (115200 baud)
2. Boot Counter Check (bootloop detection)
3. System::init()
   ├─ PSRAM validation
   ├─ GPIO configuration
   └─ FreeRTOS mutex creation
4. Storage::init()
   └─ Load configuration from EEPROM
5. Watchdog::init()
   └─ 30-second hardware watchdog
6. Logger::init()
7. Manager Initialization:
   ├─ PowerManager::init()
   ├─ SensorManager::init()
   │  ├─ I2C bus initialization
   │  ├─ Current sensors (6x INA226)
   │  ├─ Wheel sensors (4x inductive)
   │  ├─ Temperature sensors (4x DS18B20)
   │  └─ Obstacle detection (TOFSense)
   ├─ SafetyManager::init()
   │  ├─ ABS system
   │  ├─ TCS system
   │  └─ Regen AI
   ├─ HUDManager::init()
   │  ├─ TFT display (ST7796S)
   │  ├─ Touch calibration (XPT2046)
   │  └─ Compositor layers
   ├─ ControlManager::init()
   │  ├─ Traction motors (4x)
   │  ├─ Steering motor
   │  └─ Relay control
   ├─ TelemetryManager::init()
   └─ ModeManager::init()
8. Boot Counter Clear (successful boot)
9. Enter main loop()
```

**Boot Time:** ~2-3 seconds (typical)

---

## 10. CONFIGURATION MANAGEMENT

### 10.1 Runtime Configurable

- LED brightness
- Touch calibration
- ABS parameters (slip threshold, min speed)
- TCS parameters
- Display brightness
- Audio volume
- Power shutdown delay

### 10.2 Compile-Time Constants

- PWM frequencies
- I2C addresses
- GPIO pin assignments
- System tick rate (10ms)
- Watchdog timeout (30s)
- Stack sizes (32KB loop, 16KB event)

---

## 11. ERROR HANDLING STRATEGY

### 11.1 Error Codes

**System maintains central error code registry:**
- 200-299: System errors
- 300-399: Sensor errors
- 400-499: Control errors
- 500-599: Communication errors
- 600-699: Display errors
- 700-799: Audio errors

**Files:**
- `include/error_codes.h`
- Logged to Serial and stored in ring buffer

### 11.2 Recovery Mechanisms

1. **Watchdog Reset:** Ultimate recovery (30s timeout)
2. **I2C Recovery:** Automated bus recovery on hang
3. **Boot Counter:** Safe mode after 3 boots in 60s
4. **Graceful Degradation:** Non-critical systems fail soft
5. **Retry Logic:** Transient failures get 3 retries

---

## 12. PRODUCTION READINESS

### 12.1 Safety Features

✅ Watchdog protection  
✅ Bootloop detection  
✅ I2C bus recovery  
✅ Current limiting  
✅ Thermal protection  
✅ ABS/TCS safety systems  
✅ Obstacle detection  
✅ Graceful shutdown sequence  
✅ Error logging  
✅ Safe mode fallback

### 12.2 Reliability Features

✅ Mutex-protected I2C  
✅ Input validation (std::isfinite checks)  
✅ PWM channel validation  
✅ Non-blocking operations  
✅ Timeout protection  
✅ Dirty rectangle optimization  
✅ Memory leak prevention  
✅ Stack overflow protection (32KB)

### 12.3 Known Limitations

⚠️  **Single Point of Failure:** ESP32-S3 handles all functions  
⚠️  **I2C Bus Criticality:** Motor control depends on single I2C bus  
⚠️  **No Redundancy:** No backup for critical sensors  
⚠️  **Display Coupling:** HUD and critical data share resources  
⚠️  **Memory Constraints:** PSRAM required for display buffers  
⚠️  **CPU Utilization:** ~80-90% during peak load

---

## 13. PERFORMANCE METRICS

### 13.1 Timing Analysis

| Operation | Target | Actual | Margin |
|-----------|--------|--------|--------|
| Main loop | 10ms | 10-12ms | ⚠️  Tight |
| I2C transaction | 5ms | 2-4ms | ✅ Good |
| Display update | 33ms | 30-35ms | ✅ Good |
| ISR latency | <1µs | <500ns | ✅ Excellent |
| Watchdog feed | <30s | ~10ms | ✅ Excellent |

### 13.2 Resource Utilization

```
CPU Cores:
├─ Core 0: WiFi (disabled), background tasks (~5%)
└─ Core 1: Main loop, Arduino tasks (~85%)

Memory:
├─ Internal RAM: ~70KB free of ~400KB
├─ PSRAM: ~7.4MB free of 8MB
└─ Flash: ~14MB free of 16MB

GPIO:
├─ Used: 34 pins
├─ Available: 2 pins (GPIO 3, 40, 41 recently freed)
└─ Reserved: Strapping pins (0, 3, 45, 46)
```

---

## APPENDIX A: FILE INVENTORY

### A.1 Source Files by Category

**Control (6 files, ~61KB):**
- `adaptive_cruise.cpp` (5,529 bytes)
- `relays.cpp` (11,113 bytes)
- `steering_model.cpp` (2,043 bytes)
- `steering_motor.cpp` (8,177 bytes)
- `tcs_system.cpp` (8,549 bytes)
- `traction.cpp` (26,899 bytes)

**Sensors (6 files, ~55KB):**
- `car_sensors.cpp` (13,731 bytes)
- `current.cpp` (10,701 bytes)
- `obstacle_detection.cpp` (18,804 bytes)
- `sensors.cpp` (6,927 bytes)
- `temperature.cpp` (8,018 bytes)
- `wheels.cpp` (4,451 bytes)

**HUD/Display (18 files, ~287KB):**
- Major: `hud.cpp` (67KB), `hud_manager.cpp` (43KB), `menu_hidden.cpp` (46KB)
- Supporting: 15 additional rendering and menu files

**Safety (3 files, ~25KB):**
- `abs_system.cpp`, `obstacle_safety.cpp`, `regen_ai.cpp`

**Core (14 files, ~96KB):**
- System, storage, I2C recovery, logging, telemetry, watchdog

**Total:** 75 .cpp files, 85 .h files, ~30,115 lines

---

## APPENDIX B: CRITICAL PIN MAPPINGS

See `include/pins.h` for complete mapping. Key assignments:

**Power:**
- GPIO 35: RELAY_MAIN (power hold)
- GPIO 40: KEY_ON detection
- GPIO 41: KEY_OFF detection

**Motors:**
- I2C @ GPIO 8/9: All motor PWM controllers

**Sensors:**
- GPIO 7, 36, 15, 1: Wheel speed sensors
- GPIO 37, 38, 39: Steering encoder
- GPIO 4: Pedal (ADC)
- GPIO 20: Temperature (OneWire)
- GPIO 44: Obstacle LiDAR (UART RX)

**Display:**
- GPIO 10-14, 16, 21, 42, 47: TFT + Touch

**Lighting:**
- GPIO 19: Front LEDs (WS2812B)
- GPIO 48: Rear LEDs (WS2812B)

---

## DOCUMENT CONTROL

**Author:** AI Copilot System Architect  
**Reviewed:** Pending  
**Approved:** Pending  
**Next Review:** Before CAN migration Phase 1

---

**END OF DOCUMENT**
