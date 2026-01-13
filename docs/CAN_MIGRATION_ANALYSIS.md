# CAN MIGRATION ANALYSIS
# ESP32-S3 → Multi-ECU Architecture Assessment

**Document Version:** 1.0  
**Date:** 2026-01-13  
**Status:** 🔬 TECHNICAL AUDIT - NO CODE CHANGES

---

## EXECUTIVE SUMMARY

This document analyzes each subsystem in the current ESP32-S3 monolithic architecture to determine suitability for migration to dedicated STM32 ECUs communicating via CAN bus. The goal is to improve reliability, determinism, and fault isolation while maintaining safety.

**Key Finding:** 60-70% of current ESP32 workload can be safely offloaded to STM32 nodes, freeing the ESP32 to focus on high-level logic, gateway functions, and user interface.

---

## 1. REAL-TIME CRITICALITY CLASSIFICATION

### 1.1 HARD REAL-TIME (Must Never Block)

| Subsystem | Current Latency | Deadline | Jitter Tolerance | Consequence of Miss |
|-----------|----------------|----------|------------------|---------------------|
| **Wheel Speed ISR** | <1 μs | 10 μs | 0 μs | Incorrect speed → ABS/TCS failure |
| **Steering Encoder ISR** | <1 μs | 10 μs | 0 μs | Steering angle error → loss of control |
| **Emergency Stop** | <10 ms | 20 ms | 5 ms | Delayed shutdown → crash |

**Why Hard Real-Time:**
- **ISRs:** Hardware interrupt handlers must execute in microseconds to avoid missing pulses
- **Emergency Stop:** Safety requirement - must cut power within 20ms of trigger
- **Zero Tolerance:** Any delay is unacceptable in safety-critical scenarios

**Current ESP32 Capability:** ✅ ADEQUATE  
**Reason:** ESP-IDF ISR handlers with `IRAM_ATTR` provide sub-microsecond response. However, cooperative multitasking in main loop can cause jitter in non-ISR paths.

---

### 1.2 SOFT REAL-TIME (Bounded Latency Required)

| Subsystem | Current Rate | Target Latency | Jitter Tolerance | Degradation Mode |
|-----------|-------------|----------------|------------------|------------------|
| **Motor PWM Update** | 100 Hz (10ms) | 20 ms | 10 ms | Jerky acceleration |
| **Current Monitoring** | 10 Hz (100ms) | 200 ms | 50 ms | Delayed over-current protection |
| **ABS/TCS Logic** | 100 Hz (10ms) | 20 ms | 10 ms | Reduced effectiveness |
| **Obstacle Detection** | 15 Hz (66ms) | 100 ms | 33 ms | Late collision warning |
| **Pedal Reading** | 100 Hz (10ms) | 20 ms | 10 ms | Throttle lag |

**Why Soft Real-Time:**
- **Bounded Deadlines:** Must complete within a predictable time window
- **Tolerance:** Can tolerate some jitter without catastrophic failure
- **Degradation:** Performance degrades gracefully if deadlines are occasionally missed

**Current ESP32 Capability:** ⚠️ MARGINAL  
**Reason:** Single-threaded cooperative loop with I2C bus contention. If display update blocks (e.g., PSRAM DMA), control loop can miss its 10ms deadline. No real-time OS scheduler to enforce priorities.

---

### 1.3 NON-CRITICAL (Best Effort)

| Subsystem | Current Rate | Acceptable Latency | Impact of Delay |
|-----------|-------------|-------------------|-----------------|
| **HUD Update** | 30 Hz (33ms) | 100-500 ms | Stale display |
| **Audio Playback** | Async | 1-2 s | Delayed alerts |
| **LED Lighting** | Variable | 100-500 ms | Imperceptible |
| **Temperature Monitoring** | 1-5 Hz | 5-10 s | Delayed thermal warning |
| **Telemetry Logging** | 10 Hz | 1-10 s | Missing data points |
| **Touch Input** | Variable | 50-100 ms | Delayed UI response |

**Why Non-Critical:**
- **User Perception:** Human reaction time ~200ms, so 100ms latency is imperceptible
- **Safety Impact:** Delayed update does not create immediate danger
- **Recovery:** System can catch up on next cycle

**Current ESP32 Capability:** ✅ ADEQUATE  
**Reason:** Even with occasional blocking, these subsystems remain functional.

---

## 2. OFFLOAD CANDIDATE ANALYSIS

### 2.1 MOTORS (Traction 4x4 + Steering)

**Current Implementation:**
- Control: ESP32 main loop (100 Hz)
- Communication: I2C to PCA9685 + MCP23017
- PWM Generation: External PCA9685 chips
- Direction Control: MCP23017 GPIO expander

**CAN Offload Assessment:** ✅ **YES - HIGH PRIORITY**

| Criteria | Rating | Justification |
|----------|--------|---------------|
| **Latency Sensitivity** | 🟡 Medium | 10ms control loop, 20ms deadline, CAN adds 1-2ms |
| **Determinism Required** | ✅ High | Motor control needs predictable timing |
| **Bandwidth** | 🟢 Low | 4 bytes PWM + 1 byte direction = 5 bytes @ 100Hz = 500 B/s |
| **Failure Impact** | 🔴 Critical | Loss of propulsion (but fail-safe to STOP is acceptable) |
| **Decoupling Benefit** | ✅ High | Removes I2C contention from ESP32 main loop |

**Recommended Architecture:**
- **STM32 Motor ECU** controls all 5 motors (4 traction + 1 steering)
- **Inputs from ESP32 via CAN:** Throttle demand (0-100%), steering angle (-45° to +45°), gear (P/R/N/D1/D2)
- **Outputs to ESP32 via CAN:** Motor currents, temperatures, status flags
- **Local Control:** STM32 handles PWM generation, over-current protection, thermal limiting
- **Failsafe:** CAN timeout → STM32 enters safe state (motors off)

**Benefits:**
- ✅ Deterministic motor control (no ESP32 jitter)
- ✅ Removes 9 I2C devices from ESP32 bus (3x PCA9685 + 6x INA226)
- ✅ Offloads ~30% of ESP32 main loop CPU time
- ✅ Isolated failure domain (motor ECU crash doesn't kill display)

**Risks:**
- ⚠️ CAN bus becomes single point of failure for propulsion
- ⚠️ Adds 1-2ms latency to control loop (acceptable)
- ⚠️ Requires robust CAN failsafe (timeout detection, safe state)

---

### 2.2 WHEEL SPEED SENSORS (4x Inductive)

**Current Implementation:**
- Sensing: GPIO ISRs on ESP32 (IRAM_ATTR)
- Frequency: Event-driven (6 pulses/revolution)
- Processing: Pulse counting + speed calculation in main loop

**CAN Offload Assessment:** ✅ **YES - MEDIUM PRIORITY**

| Criteria | Rating | Justification |
|----------|--------|---------------|
| **Latency Sensitivity** | 🟡 Medium | ABS/TCS needs 10ms updates, CAN adds 1-2ms |
| **Determinism Required** | ✅ High | Speed data must be consistent for safety systems |
| **Bandwidth** | 🟢 Low | 4 wheels × 4 bytes = 16 bytes @ 100Hz = 1.6 kB/s |
| **Failure Impact** | 🟠 High | Loss of ABS/TCS (vehicle still operable) |
| **Decoupling Benefit** | 🟢 Medium | Frees 4 ESP32 GPIOs and 4 ISRs |

**Recommended Architecture:**
- **STM32 Motor ECU** reads wheel sensors locally (co-located with motor control)
- **Outputs to ESP32 via CAN:** Wheel speeds (km/h), pulse counts, validity flags
- **Local Processing:** STM32 calculates speeds from pulse timing
- **Failsafe:** Invalid speed → ABS/TCS disabled, notify ESP32

**Benefits:**
- ✅ Frees 4 ESP32 GPIOs (7, 36, 15, 1)
- ✅ Removes ISR load from ESP32
- ✅ Speed data always synchronized with motor control

**Risks:**
- ⚠️ CAN latency may affect ABS/TCS reaction time (1-2ms added)
- ⚠️ Must ensure CAN message freshness (timestamp required)

---

### 2.3 STEERING ENCODER (E6B2-CWZ6C 1200PR)

**Current Implementation:**
- Sensing: GPIO ISRs on ESP32 (quadrature decoding)
- Frequency: High (1200 pulses/rev × RPM)
- Processing: Encoder state machine in ISR

**CAN Offload Assessment:** ⚠️ **DANGEROUS - KEEP ON ESP32**

| Criteria | Rating | Justification |
|----------|--------|---------------|
| **Latency Sensitivity** | 🔴 Very High | High-frequency quadrature, CAN too slow |
| **Determinism Required** | ✅ Critical | Missing pulses = incorrect angle |
| **Bandwidth** | 🔴 High | Encoder state changes at kHz rates |
| **Failure Impact** | 🔴 Critical | Loss of steering angle = loss of control |
| **Decoupling Benefit** | ❌ Negative | Moving to STM32 adds latency, no benefit |

**Recommended Architecture:**
- **KEEP on ESP32** - Quadrature decoding is too fast for CAN
- **Alternative:** STM32 Motor ECU reads encoder locally, sends **angle** (not pulses) to ESP32 via CAN
  - STM32 handles high-speed quadrature decoding
  - ESP32 receives angle updates @ 100 Hz via CAN (4 bytes)
  - Bandwidth: 4 bytes @ 100Hz = 400 B/s ✅ Acceptable

**Revised Assessment:** ✅ **YES - via Motor ECU (Angle Only)**

**Benefits:**
- ✅ Frees 3 ESP32 GPIOs (37, 38, 39)
- ✅ Removes high-frequency ISR from ESP32
- ✅ Co-located with steering motor control

**Risks:**
- ⚠️ CAN latency affects steering feedback (1-2ms delay acceptable for angle display)
- ⚠️ Must ensure encoder initialization (Z-index centering) happens on STM32

---

### 2.4 PEDAL (Hall Sensor ADC)

**Current Implementation:**
- Sensing: ADC on ESP32 GPIO 4 (ADC1_CH3)
- Frequency: 100 Hz
- Processing: ADC read + filtering in main loop

**CAN Offload Assessment:** ❌ **NO - KEEP ON ESP32**

| Criteria | Rating | Justification |
|----------|--------|---------------|
| **Latency Sensitivity** | 🔴 High | Driver input must be immediate (<20ms) |
| **Determinism Required** | 🟡 Medium | Can tolerate some jitter |
| **Bandwidth** | 🟢 Low | 2 bytes @ 100Hz = 200 B/s |
| **Failure Impact** | 🔴 Critical | Loss of throttle control |
| **Decoupling Benefit** | ❌ Negative | ADC is native to ESP32, no benefit to offload |

**Recommended Architecture:**
- **KEEP on ESP32** - Direct ADC reading is fastest
- ESP32 reads pedal, sends throttle demand to Motor ECU via CAN

**Rationale:**
- Pedal is **human interface** - belongs with UI/logic layer (ESP32)
- CAN would add unnecessary latency for primary driver input
- ESP32 ADC is simpler than STM32 ADC + CAN round-trip

---

### 2.5 CURRENT SENSORS (6x INA226)

**Current Implementation:**
- Sensing: I2C via TCA9548A multiplexer
- Frequency: 10 Hz (100ms cycle)
- Communication: 6 I2C reads per cycle

**CAN Offload Assessment:** ✅ **YES - HIGH PRIORITY**

| Criteria | Rating | Justification |
|----------|--------|---------------|
| **Latency Sensitivity** | 🟡 Medium | Over-current protection needs <200ms |
| **Determinism Required** | 🟡 Medium | Can tolerate some jitter |
| **Bandwidth** | 🟢 Low | 6 sensors × 4 bytes = 24 bytes @ 10Hz = 240 B/s |
| **Failure Impact** | 🟠 High | Loss of protection (but motors can have local limits) |
| **Decoupling Benefit** | ✅ Very High | Removes 7 I2C devices (TCA + 6 INA226) from ESP32 |

**Recommended Architecture:**
- **STM32 Motor ECU** reads all 6 INA226 sensors locally
- **Local Protection:** STM32 enforces current limits directly on motors
- **Outputs to ESP32 via CAN:** Current values, over-current flags, battery voltage
- **Failsafe:** CAN timeout → ESP32 uses last known values, STM32 continues local protection

**Benefits:**
- ✅ Removes I2C contention from ESP32 (7 devices gone)
- ✅ Faster over-current protection (STM32 local, no CAN latency)
- ✅ Simplified ESP32 I2C bus (only MCP23017 remains... wait, no - see Shifter section)

**Risks:**
- ⚠️ ESP32 loses direct current visibility (depends on CAN updates)
- ⚠️ Battery monitoring becomes indirect (acceptable, 10Hz is sufficient)

---

### 2.6 TEMPERATURE SENSORS (4x DS18B20)

**Current Implementation:**
- Sensing: OneWire bus on GPIO 20
- Frequency: 1-5 Hz (slow, 750ms conversion time)
- Processing: OneWire protocol + temperature calculation

**CAN Offload Assessment:** ✅ **YES - LOW PRIORITY**

| Criteria | Rating | Justification |
|----------|--------|---------------|
| **Latency Sensitivity** | 🟢 Low | Thermal response is slow (seconds to minutes) |
| **Determinism Required** | ❌ No | Temperature changes slowly |
| **Bandwidth** | 🟢 Very Low | 4 sensors × 2 bytes = 8 bytes @ 1Hz = 8 B/s |
| **Failure Impact** | 🟡 Medium | Delayed thermal warning (motors can survive short overheats) |
| **Decoupling Benefit** | 🟢 Low | Frees 1 GPIO, removes OneWire protocol overhead |

**Recommended Architecture:**
- **STM32 Motor ECU** reads DS18B20 sensors locally
- **Outputs to ESP32 via CAN:** Temperature values @ 1Hz, over-temp flags
- **Local Protection:** STM32 reduces motor power on over-temp
- **Failsafe:** CAN timeout → ESP32 assumes last known temps, STM32 continues local protection

**Benefits:**
- ✅ Frees GPIO 20
- ✅ Removes slow OneWire protocol from ESP32 loop
- ✅ Temperature monitoring co-located with motor control

**Risks:**
- ⚠️ Very low risk (temperature changes slowly, 1Hz CAN updates are sufficient)

---

### 2.7 OBSTACLE DETECTION (TOFSense-M S LiDAR)

**Current Implementation:**
- Sensing: UART0 on ESP32 (GPIO 44 RX, 43 TX)
- Frequency: 15 Hz (66ms updates)
- Protocol: 9-byte packets, 115200 baud

**CAN Offload Assessment:** ⚠️ **MAYBE - LOW PRIORITY**

| Criteria | Rating | Justification |
|----------|--------|---------------|
| **Latency Sensitivity** | 🟡 Medium | Collision avoidance needs <100ms warning |
| **Determinism Required** | 🟡 Medium | Predictable updates preferred |
| **Bandwidth** | 🟡 Medium | 9 bytes @ 15Hz = 135 B/s (UART), 4 bytes @ 15Hz = 60 B/s (CAN) |
| **Failure Impact** | 🟡 Medium | Loss of collision warning (driver must be vigilant) |
| **Decoupling Benefit** | 🟢 Low | Frees UART0, but ESP32 has multiple UARTs |

**Recommended Architecture - Option 1 (Keep on ESP32):**
- **KEEP on ESP32** - Direct UART connection
- Obstacle logic remains on ESP32 (high-level decision making)

**Recommended Architecture - Option 2 (Dedicated Sensor ECU):**
- **STM32 Sensor ECU** reads LiDAR via UART
- **Outputs to ESP32 via CAN:** Distance, obstacle level (SAFE/CAUTION/WARNING/CRITICAL)
- **Local Processing:** STM32 handles UART protocol parsing
- **Benefits:** Frees ESP32 UART0, potential for multi-sensor fusion in future

**Verdict:** ⚠️ **DEFER to Phase 2+**  
**Rationale:** Low immediate benefit. Obstacle detection is a high-level safety function that naturally belongs with ESP32 decision logic. Offloading to STM32 adds complexity without significant gain unless adding more sensors.

---

### 2.8 SHIFTER (5-Position Rotary)

**Current Implementation:**
- Sensing: MCP23017 GPIO expander (I2C, 5 pins)
- Frequency: 100 Hz polling
- Processing: Read 5 GPIO states, decode gear position

**CAN Offload Assessment:** ⚠️ **MAYBE - KEEP ON ESP32**

| Criteria | Rating | Justification |
|----------|--------|---------------|
| **Latency Sensitivity** | 🟡 Medium | Gear changes are human-paced (~1s transitions) |
| **Determinism Required** | ❌ No | Human input is inherently non-deterministic |
| **Bandwidth** | 🟢 Very Low | 1 byte @ 100Hz = 100 B/s (overkill, could be 10Hz) |
| **Failure Impact** | 🔴 Critical | Incorrect gear = dangerous (e.g., reverse instead of drive) |
| **Decoupling Benefit** | ❌ Negative | MCP23017 is shared with motor control |

**Recommended Architecture:**
- **Option 1 (Keep on ESP32):** ESP32 reads MCP23017, sends gear to Motor ECU via CAN
- **Option 2 (Move to Motor ECU):** STM32 Motor ECU reads MCP23017 (it already controls motor direction via MCP23017), sends gear status to ESP32

**Verdict:** ✅ **YES - via Motor ECU (Shared MCP23017)**  
**Rationale:** MCP23017 is already used for motor direction control (IN1/IN2 signals). The Shifter shares the same I2C device. Moving the entire MCP23017 to the Motor ECU makes sense - STM32 reads shifter position, controls motor direction accordingly, and reports gear status to ESP32 for display.

**Benefits:**
- ✅ Removes MCP23017 from ESP32 I2C bus (last I2C device gone!)
- ✅ Shifter reading co-located with motor direction control (logical grouping)

**Risks:**
- ⚠️ ESP32 loses direct shifter visibility (depends on CAN updates)
- ⚠️ Gear display on HUD has 10ms latency (acceptable, human-paced)

---

### 2.9 DISPLAY AND HUD (ST7796S TFT + Touch)

**Current Implementation:**
- Display: SPI bus, 30 FPS rendering
- Touchscreen: SPI bus (shared), IRQ-driven
- Compositor: Software rendering with PSRAM frame buffers
- CPU Load: ~20-30% of ESP32 main loop

**CAN Offload Assessment:** ❌ **NO - KEEP ON ESP32**

| Criteria | Rating | Justification |
|----------|--------|---------------|
| **Latency Sensitivity** | 🟢 Low | Human perception tolerates 33ms (30 FPS) |
| **Determinism Required** | ❌ No | Display updates are best-effort |
| **Bandwidth** | 🔴 Very High | 480×320×16bpp = 307 kB/frame, CAN cannot handle this |
| **Failure Impact** | 🟡 Medium | Loss of visual feedback (vehicle still operable) |
| **Decoupling Benefit** | ❌ Negative | Display is ESP32's primary role in target architecture |

**Recommended Architecture:**
- **KEEP on ESP32** - This is ESP32's core responsibility in multi-ECU design
- ESP32 becomes **Gateway + UI + Logic** controller
- Receives sensor data via CAN, renders HUD, sends commands to actuators

**Rationale:**
- Display is high-bandwidth, low-latency, and requires powerful graphics capabilities (PSRAM, DMA, SPI)
- ESP32-S3 is ideal for UI work (more powerful than STM32)
- In target architecture, ESP32 focuses on **UI, gateway, and decision logic**

---

### 2.10 AUDIO SYSTEM (DFPlayer Mini)

**Current Implementation:**
- Communication: UART1 (GPIO 18 TX, 17 RX)
- Frequency: Async (command-driven)
- Protocol: DFPlayer serial protocol

**CAN Offload Assessment:** ❌ **NO - KEEP ON ESP32**

| Criteria | Rating | Justification |
|----------|--------|---------------|
| **Latency Sensitivity** | 🟢 Low | Audio alerts can tolerate 1-2s delay |
| **Determinism Required** | ❌ No | Async playback, best-effort |
| **Bandwidth** | 🟢 Very Low | Commands only, ~10 bytes @ 1Hz = 10 B/s |
| **Failure Impact** | 🟢 Low | Loss of audio (visual alerts remain) |
| **Decoupling Benefit** | ❌ Negative | Audio is part of user feedback (belongs with UI) |

**Recommended Architecture:**
- **KEEP on ESP32** - Audio alerts are part of UI/feedback
- ESP32 triggers audio based on CAN data (e.g., over-temp alert from Motor ECU)

**Rationale:**
- Audio is **user interface** function - belongs with ESP32
- DFPlayer is simple UART device, no benefit to offload

---

### 2.11 LIGHTING (WS2812B LEDs)

**Current Implementation:**
- Front LEDs: GPIO 19, 28 LEDs
- Rear LEDs: GPIO 48, 16 LEDs
- Control: FastLED library
- Frequency: Variable (turn signals ~1Hz blink, headlights static)

**CAN Offload Assessment:** ⚠️ **MAYBE - LOW PRIORITY**

| Criteria | Rating | Justification |
|----------|--------|---------------|
| **Latency Sensitivity** | 🟢 Low | Lighting changes are slow (human perception ~100ms) |
| **Determinism Required** | ❌ No | Cosmetic function |
| **Bandwidth** | 🟡 Medium | 44 LEDs × 3 bytes/LED = 132 bytes @ 30Hz = 4 kB/s (if animating) |
| **Failure Impact** | 🟡 Medium | Loss of lights (safety issue at night) |
| **Decoupling Benefit** | 🟢 Low | Frees 2 GPIOs, removes FastLED overhead |

**Recommended Architecture - Option 1 (Keep on ESP32):**
- **KEEP on ESP32** - Lighting logic co-located with UI
- ESP32 controls lights based on vehicle state (brake, turn signal, headlights)

**Recommended Architecture - Option 2 (Dedicated Lighting ECU):**
- **STM32 Lighting ECU** controls WS2812B strips
- **Inputs from ESP32 via CAN:** Headlights on/off, turn signal left/right, brake active
- **Local Control:** STM32 handles LED animations, patterns, brightness
- **Benefits:** Frees ESP32 GPIOs, offloads FastLED processing

**Verdict:** ⚠️ **DEFER to Phase 2+**  
**Rationale:** Low immediate benefit. Lighting is simple and low-frequency. In Phase 1, keep on ESP32. In Phase 2+, consider a dedicated Lighting ECU if adding more complex lighting features (e.g., RGB underglow, dynamic turn signals).

---

### 2.12 POWER MANAGEMENT (Relays)

**Current Implementation:**
- Main Relay: GPIO 35 (power hold)
- Traction Relay: GPIO 5 (24V motors)
- Steering Relay: GPIO 6 (12V steering)
- Spare Relay: GPIO 46 (lights/media)

**CAN Offload Assessment:** ⚠️ **PARTIAL**

| Subsystem | Offload? | Rationale |
|-----------|----------|-----------|
| **Main Relay (GPIO 35)** | ❌ **NO** | Must remain on ESP32 (power hold circuit) |
| **Traction Relay (GPIO 5)** | ✅ **YES** | Move to Motor ECU (co-located with traction motors) |
| **Steering Relay (GPIO 6)** | ✅ **YES** | Move to Motor ECU (co-located with steering motor) |
| **Spare Relay (GPIO 46)** | ❌ **NO** | Keep on ESP32 (lighting/media control) |

**Recommended Architecture:**
- **Main Relay:** Stays on ESP32 (critical for power-on self-hold)
- **Traction + Steering Relays:** Move to STM32 Motor ECU
  - STM32 controls power to motors locally
  - ESP32 sends "enable/disable" commands via CAN
  - **Failsafe:** CAN timeout → STM32 disables relays (safe state)
- **Spare Relay:** Stays on ESP32 (lighting/media control)

**Benefits:**
- ✅ Motor ECU has local relay control (faster emergency stop)
- ✅ Frees 2 ESP32 GPIOs (5, 6)

**Risks:**
- ⚠️ ESP32 loses direct relay control (must trust STM32 failsafe)
- ⚠️ CAN timeout = motors disabled (acceptable failsafe)

---

## 3. COUPLING AND RISK DETECTION

### 3.1 TIGHT COUPLINGS (Existing System)

| Coupling | Type | Risk Level | Impact | Decoupling Strategy |
|----------|------|------------|--------|---------------------|
| **TFT ↔ HUD ↔ PSRAM** | Memory allocation | 🔴 Critical | Display crash → PSRAM corruption → system crash | ❌ Cannot decouple (inherent to ESP32 graphics) |
| **All Motors ↔ I2C Bus** | Shared bus | 🔴 Critical | I2C hang → all motors freeze | ✅ **Decouple via Motor ECU** |
| **Shifter ↔ Motor Direction ↔ MCP23017** | Shared I2C device | 🔴 Critical | MCP23017 failure → no gear read + no motor direction | ✅ **Move entire MCP23017 to Motor ECU** |
| **Current Sensors ↔ TCA9548A ↔ I2C** | Shared bus + MUX | 🟠 High | TCA failure → no current monitoring | ✅ **Move to Motor ECU** |
| **USB Serial ↔ Logger** | Blocking I/O | 🟡 Medium | USB buffer full → logger blocks → loop stalls | ⚠️ Add timeout to logger |

### 3.2 CRASH PROPAGATION PATHS (Existing System)

**Path 1: Display → PSRAM → System**
```
TFT rendering uses DMA + PSRAM
 ├─> PSRAM corruption (faulty pointer, buffer overflow)
 ├─> Memory fault exception
 ├─> Watchdog timeout (if in infinite loop)
 └─> System reset (30s delay)
```
**Mitigation via CAN Migration:**
- ✅ Offload motors to STM32 → Display crash doesn't freeze motors
- ⚠️ Display crash still resets ESP32 (but motors continue under STM32 local control)

**Path 2: I2C Hang → Motor Freeze**
```
I2C device stops responding (PCA9685, MCP23017, TCA9548A)
 ├─> ESP32 I2C read blocks (100ms timeout)
 ├─> Control loop misses deadline
 ├─> Motors receive stale PWM values
 └─> Vehicle continues at last commanded speed (DANGEROUS)
```
**Mitigation via CAN Migration:**
- ✅ **Motor ECU handles I2C locally** → ESP32 I2C hang doesn't affect motors
- ✅ CAN timeout → Motor ECU enters safe state (motors off)

**Path 3: Watchdog Timeout → Hard Reset**
```
ESP32 loop blocks (infinite loop, I2C hang, PSRAM DMA stall)
 ├─> Watchdog not fed for 30s
 ├─> Hardware watchdog triggers reset
 ├─> All GPIO outputs reset to default state
 └─> Motors lose direction signals (BTS7960 enters undefined state)
```
**Mitigation via CAN Migration:**
- ✅ **Motor ECU independent** → ESP32 reset doesn't affect motors
- ✅ Motor ECU continues operating in fail-safe mode

---

### 3.3 MODULES AFFECTING USB/BOOT

| Module | USB Risk | Boot Risk | Mitigation |
|--------|----------|-----------|------------|
| **Logger (Serial)** | 🟡 Medium (blocking writes) | ❌ No | Add timeout to Serial.print() |
| **TFT Display** | ⚠️ Low (PSRAM DMA can conflict) | 🔴 High (PSRAM init can fail) | Boot guard implemented ✅ |
| **I2C Devices** | ❌ No | 🟠 Medium (init failures can cascade) | Individual device timeouts ✅ |
| **PSRAM** | ❌ No | 🔴 Critical (wrong config = bootloop) | Verified config (QIO/QSPI) ✅ |

**Impact of CAN Migration:**
- ✅ Removing I2C devices from ESP32 reduces boot failure points
- ✅ Motor ECU can boot independently of ESP32

---

### 3.4 MODULES AFFECTING VEHICLE SAFETY

| Module | Safety Role | Current Failure Mode | Post-Migration Failure Mode |
|--------|-------------|----------------------|------------------------------|
| **Motor Control** | Propulsion | I2C hang → freeze | CAN timeout → safe stop ✅ |
| **Steering Encoder** | Angle feedback | ISR miss → incorrect angle | CAN timeout → open-loop ⚠️ |
| **Wheel Sensors** | ABS/TCS | ISR miss → incorrect speed | CAN timeout → disable ABS/TCS ✅ |
| **Current Sensors** | Over-current protection | I2C timeout → no protection | Local STM32 protection ✅ |
| **Emergency Stop** | Immediate shutdown | Relay GPIO controlled by ESP32 | Relay controlled by STM32 ✅ |

**Safety Improvement via CAN Migration:**
- ✅ Motor ECU provides **local fail-safes** (independent of ESP32 health)
- ✅ CAN timeout is **predictable** (e.g., 100ms) vs. I2C hang (indeterminate)
- ✅ **Fault isolation:** ESP32 crash doesn't propagate to motors

---

## 4. BANDWIDTH AND LATENCY ANALYSIS

### 4.1 CAN BUS BANDWIDTH CALCULATION

**Proposed CAN Configuration:**
- **Bitrate:** 500 kbps (standard automotive CAN)
- **Theoretical Max Throughput:** ~40 kB/s (accounting for overhead)

**Message Traffic (100 Hz Control Loop):**

| Message | Direction | Size | Rate | Bandwidth |
|---------|-----------|------|------|-----------|
| **Motor Commands** | ESP32 → Motor ECU | 8 bytes | 100 Hz | 800 B/s |
| **Motor Status** | Motor ECU → ESP32 | 16 bytes | 100 Hz | 1.6 kB/s |
| **Wheel Speeds** | Motor ECU → ESP32 | 16 bytes | 100 Hz | 1.6 kB/s |
| **Current Sensors** | Motor ECU → ESP32 | 24 bytes | 10 Hz | 240 B/s |
| **Temperature Sensors** | Motor ECU → ESP32 | 8 bytes | 1 Hz | 8 B/s |
| **Shifter Status** | Motor ECU → ESP32 | 2 bytes | 100 Hz | 200 B/s |
| **Steering Angle** | Motor ECU → ESP32 | 4 bytes | 100 Hz | 400 B/s |
| **Emergency Stop** | Bidirectional | 1 byte | Event | Negligible |
| **Heartbeat** | Bidirectional | 1 byte | 10 Hz | 20 B/s |

**Total Bandwidth:** ~4.9 kB/s  
**Bus Utilization:** 4.9 kB/s / 40 kB/s = **12.3%** ✅ Excellent headroom

### 4.2 LATENCY BUDGET

**Current System (I2C):**
- ESP32 loop cycle: 10ms
- I2C read (PCA9685): ~75 μs
- Total loop time: ~10ms (motor command to PWM update)

**Post-Migration (CAN):**
- ESP32 loop cycle: 10ms (unchanged)
- CAN message transmission: ~0.2ms (8-byte message @ 500kbps)
- STM32 processing: ~1ms (PWM update)
- **Total latency:** 10ms (ESP32) + 0.2ms (CAN TX) + 1ms (STM32) = **11.2ms**

**Latency Increase:** 1.2ms (+12%) ✅ Acceptable  
**Deadline Compliance:** 11.2ms < 20ms deadline ✅ OK

---

## 5. FAILURE MODES AND EFFECTS ANALYSIS (FMEA)

### 5.1 CAN BUS FAILURE

**Failure:** CAN bus physical layer fault (wire break, short circuit)

| Effect | Severity | Detection | Mitigation |
|--------|----------|-----------|------------|
| ESP32 ↔ Motor ECU communication lost | 🔴 Critical | CAN timeout (100ms) | Motor ECU enters safe state (motors off) |
| ESP32 cannot command motors | 🔴 Critical | No ACK on CAN TX | Display error, sound alarm |
| ESP32 loses sensor visibility | 🟠 High | Missing CAN messages | Use last known values, enter limp mode |

**Fail-Safe Strategy:**
- ✅ Motor ECU detects CAN timeout (no messages for 100ms) → Disables motors
- ✅ ESP32 detects CAN timeout → Displays error, sounds alarm
- ✅ Vehicle coasts to safe stop

**Recovery:**
- CAN bus repair required
- Manual reset after repair

---

### 5.2 MOTOR ECU FAILURE

**Failure:** STM32 Motor ECU software crash, hardware fault, or power loss

| Effect | Severity | Detection | Mitigation |
|--------|----------|-----------|------------|
| Motors stop responding | 🔴 Critical | No CAN messages from Motor ECU (100ms) | ESP32 detects, displays error |
| No current/temperature monitoring | 🟠 High | Missing CAN data | ESP32 enters limp mode |
| No wheel speed data | 🟠 High | Missing CAN data | Disable ABS/TCS |

**Fail-Safe Strategy:**
- ✅ ESP32 detects Motor ECU failure (no heartbeat for 500ms)
- ✅ ESP32 displays critical error, sounds alarm
- ✅ Vehicle is inoperable until Motor ECU repaired

**Recovery:**
- Motor ECU reboot (if software crash)
- Hardware repair (if hardware fault)

---

### 5.3 ESP32 FAILURE

**Failure:** ESP32 software crash, PSRAM failure, or power loss

| Effect | Severity | Detection | Mitigation |
|--------|----------|-----------|------------|
| No display/UI | 🟡 Medium | User observation | Vehicle still operable (Motor ECU continues) |
| No new motor commands | 🔴 Critical | Motor ECU detects CAN timeout (100ms) | Motor ECU enters safe state (motors off) |
| No audio alerts | 🟢 Low | User observation | Visual alerts remain (Motor ECU status LEDs) |

**Fail-Safe Strategy:**
- ✅ **Motor ECU operates independently** - detects ESP32 failure via CAN timeout
- ✅ Motor ECU enters safe state (motors off)
- ✅ Vehicle coasts to safe stop

**Recovery:**
- ESP32 reboot (automatic via boot guard if bootloop)
- Display returns, user can continue driving

**Key Improvement over Current System:**
- 🎯 **ESP32 crash no longer freezes motors** - Motor ECU has local fail-safe

---

## 6. SUMMARY: OFFLOAD DECISION MATRIX

| Subsystem | Offload to STM32? | Priority | Complexity | Risk | Benefit |
|-----------|-------------------|----------|------------|------|---------|
| **Motors (Traction 4x4)** | ✅ YES | 🔴 High | 🟡 Medium | 🟡 Medium | 🟢 Very High |
| **Motor (Steering)** | ✅ YES | 🔴 High | 🟡 Medium | 🟡 Medium | 🟢 Very High |
| **Wheel Speed Sensors** | ✅ YES | 🟠 Medium | 🟢 Low | 🟡 Medium | 🟢 High |
| **Steering Encoder** | ✅ YES (Angle only) | 🟠 Medium | 🟡 Medium | 🟡 Medium | 🟢 High |
| **Current Sensors (INA226)** | ✅ YES | 🔴 High | 🟢 Low | 🟢 Low | 🟢 Very High |
| **Temperature Sensors** | ✅ YES | 🟢 Low | 🟢 Low | 🟢 Low | 🟡 Medium |
| **Shifter** | ✅ YES (via MCP23017) | 🟡 Medium | 🟢 Low | 🟡 Medium | 🟢 High |
| **Relays (Traction/Steering)** | ✅ YES | 🟠 Medium | 🟢 Low | 🟡 Medium | 🟢 High |
| **Pedal** | ❌ NO | - | - | - | - |
| **Obstacle Detection** | ⚠️ DEFER | 🟢 Low | 🟡 Medium | 🟡 Medium | 🟡 Medium |
| **Display/HUD** | ❌ NO | - | - | - | - |
| **Audio** | ❌ NO | - | - | - | - |
| **Lighting** | ⚠️ DEFER | 🟢 Low | 🟢 Low | 🟢 Low | 🟡 Medium |

**Phase 1 Offload (High Priority):**
- ✅ Motors (Traction 4x4 + Steering)
- ✅ Wheel Speed Sensors
- ✅ Steering Encoder (Angle)
- ✅ Current Sensors (INA226)
- ✅ Temperature Sensors
- ✅ Shifter (via MCP23017)
- ✅ Relays (Traction/Steering)

**Deferred to Phase 2+:**
- ⚠️ Obstacle Detection
- ⚠️ Lighting

**Remains on ESP32:**
- ❌ Pedal (ADC)
- ❌ Display/HUD
- ❌ Audio
- ❌ Main Relay (power hold)

---

## CONCLUSION

**Recommended Migration:**
- **60-70% of ESP32 workload** can be safely offloaded to a single **STM32 Motor ECU**
- **ESP32 Role:** Gateway, UI, logic, high-level decision making
- **STM32 Role:** Real-time motor control, sensor acquisition, local fail-safes

**Key Benefits:**
- ✅ **Fault Isolation:** ESP32 crash doesn't freeze motors
- ✅ **Determinism:** STM32 provides hard real-time motor control
- ✅ **Bus Decoupling:** Removes 10+ I2C devices from ESP32 bus
- ✅ **Improved Safety:** Local fail-safes on Motor ECU

**Key Risks:**
- ⚠️ CAN bus becomes critical path (mitigated by fail-safe design)
- ⚠️ Increased latency (1-2ms, acceptable for control loop)
- ⚠️ System complexity increases (more ECUs to maintain)

**Next Steps:** See TARGET_ECU_ARCHITECTURE.md for detailed multi-ECU design and MIGRATION_PHASES.md for implementation roadmap.

---

**Document Authority:** Automotive Systems Architect  
**Review Status:** ✅ Technical assessment complete  
**Confidentiality:** Internal Use Only
