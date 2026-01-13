# CAN MIGRATION ANALYSIS

**Document Version:** 1.0  
**Firmware Version:** v2.17.1 (PHASE 14)  
**Analysis Date:** 2026-01-13  
**Classification:** Technical Architecture

---

## EXECUTIVE SUMMARY

This document analyzes the feasibility, risks, and methodology for migrating the current monolithic ESP32-S3 electric vehicle firmware to a distributed multi-ECU architecture using CAN bus communication. The analysis evaluates each subsystem for migration potential, identifies hard dependencies, and proposes a safe, phased migration path that never compromises vehicle operability.

**Key Findings:**
- ✅ **High-value offload candidates:** Traction motors, steering motor, sensor acquisition
- ⚠️  **Must remain on ESP32:** Display, HUD compositor, touch interface, power management
- ❌ **Dangerous to offload:** I2C bus control, system initialization, watchdog
- 🎯 **Recommended architecture:** 3-4 STM32 ECUs + ESP32-S3 gateway

---

## 1. REAL-TIME CRITICALITY CLASSIFICATION

### 1.1 HARD REAL-TIME Systems

**Definition:** Must respond within deterministic deadlines. Missing deadline = safety hazard.

#### 1.1.1 Traction Motor Control

**Classification:** **HARD REAL-TIME**

**Timing Requirements:**
- PWM update latency: <1ms
- Emergency stop response: <50ms
- TCS intervention: <50ms

**Consequences of Delay:**
- Loss of traction control
- Wheel slip/skid
- Vehicle instability
- Potential collision

**Current Implementation:**
- 100 Hz control loop (10ms)
- I2C PWM updates via PCA9685
- Direction control via MCP23017
- ISR-based wheel speed feedback

**Justification:**
Direct control of vehicle propulsion. Any delay in throttle response or emergency stop could result in accident. TCS must intervene within 50ms to prevent dangerous wheel slip.

---

#### 1.1.2 Steering Motor Control

**Classification:** **HARD REAL-TIME**

**Timing Requirements:**
- Position control loop: 100 Hz (10ms)
- Encoder ISR response: <1µs
- Steering command latency: <10ms

**Consequences of Delay:**
- Loss of steering precision
- Oversteer/understeer
- Driver disorientation
- Loss of vehicle control

**Current Implementation:**
- 100 Hz position control
- 1200 PPR encoder @ IRAM ISR
- PWM via PCA9685
- Real-time deadband compensation

**Justification:**
Steering is the most critical safety system. Human drivers expect immediate steering response (<100ms perceptible delay). Loss of steering control = immediate danger.

---

#### 1.1.3 Wheel Speed Sensors (ABS/TCS Input)

**Classification:** **HARD REAL-TIME**

**Timing Requirements:**
- ISR response: <1µs (IRAM)
- Pulse counting: continuous
- Speed calculation: 100 Hz

**Consequences of Delay:**
- ABS/TCS failure
- Wheel lock during braking
- Loss of anti-slip protection

**Current Implementation:**
- 4x IRAM_ATTR ISRs
- GPIO interrupt-driven
- Minimal processing in ISR

**Justification:**
These sensors feed the ABS and TCS systems. Missing pulses = incorrect speed calculation = ABS/TCS intervention errors = loss of vehicle control during emergency maneuvers.

---

### 1.2 SOFT REAL-TIME Systems

**Definition:** Important to meet deadlines, but occasional misses tolerable without safety impact.

#### 1.2.1 Current Sensors (INA226)

**Classification:** **SOFT REAL-TIME**

**Timing Requirements:**
- Update rate: 20 Hz (50ms)
- I2C transaction: <5ms
- Overcurrent detection: <100ms

**Consequences of Delay:**
- Delayed overcurrent protection
- Battery monitoring lag
- Power calculation errors

**Tolerance:**
- Can miss 1-2 readings without issue
- Overcurrent damage takes seconds, not milliseconds
- Thermal time constant >> sensing delay

**Current Implementation:**
- 20 Hz polling
- Protected by mutex
- Timeout: 100ms

**Justification:**
Electrical systems have slow time constants. A 100ms delay in detecting overcurrent won't cause immediate damage. Critical for long-term health, not instant safety.

---

#### 1.2.2 Temperature Sensors (DS18B20)

**Classification:** **SOFT REAL-TIME**

**Timing Requirements:**
- Update rate: 1 Hz (1000ms)
- Conversion time: ~750ms per sensor
- Overheating detection: <5 seconds

**Consequences of Delay:**
- Delayed thermal protection
- Motor overheating possible
- Performance degradation

**Tolerance:**
- Thermal time constant ~10-30 seconds
- Can skip multiple readings
- Conservative limits provide margin

**Current Implementation:**
- 1 Hz update (slow)
- Blocking OneWire read
- Not time-critical

**Justification:**
Motors heat slowly (thermal inertia). Even a 5-second delay in detecting overheating is acceptable with conservative temperature limits.

---

#### 1.2.3 ABS/TCS Safety Systems

**Classification:** **SOFT REAL-TIME**

**Timing Requirements:**
- Decision cycle: 100 Hz (10ms)
- Intervention latency: <100ms
- Recovery time: configurable

**Consequences of Delay:**
- Reduced effectiveness
- Longer stopping distance
- More wheel slip before intervention

**Tolerance:**
- ABS cycle time can extend to 200ms
- Still provides significant safety benefit
- Not instant like steering/throttle

**Current Implementation:**
- 100 Hz update
- Software-based slip detection
- PWM modulation output

**Justification:**
While critical for safety, ABS/TCS are "intervention" systems that act over hundreds of milliseconds. A delay from 100ms to 200ms reduces effectiveness but doesn't eliminate benefit.

---

#### 1.2.4 Obstacle Detection (LiDAR)

**Classification:** **SOFT REAL-TIME**

**Timing Requirements:**
- Sensor update: 15 Hz (sensor-limited)
- Processing: <100ms
- Reaction time: <200ms

**Consequences of Delay:**
- Reduced warning time
- Later collision avoidance
- Less effective at high speed

**Tolerance:**
- Speed-dependent (slower = more tolerance)
- Sensor already 15 Hz limited
- Can afford 100-200ms processing delay

**Current Implementation:**
- 15 Hz UART data
- Async processing
- Non-blocking

**Justification:**
At 20 km/h, vehicle moves 5.5m/s. A 200ms delay = 1.1m additional distance. Acceptable with 4m sensor range and speed reduction logic.

---

### 1.3 NON-CRITICAL Systems

**Definition:** No real-time requirements. Delays >1 second acceptable.

#### 1.3.1 Display/HUD

**Classification:** **NON-CRITICAL**

**Timing Requirements:**
- Update rate: 30 Hz (human perception)
- Frame latency: <100ms acceptable
- Can drop frames without issue

**Consequences of Delay:**
- Choppy display
- User annoyance
- No safety impact

**Current Implementation:**
- 30 Hz update
- Dirty rectangle optimization
- Falls back gracefully

**Justification:**
Display is purely informational. Vehicle operates without display (verified in standalone mode). Human perception threshold ~20-30 Hz.

---

#### 1.3.2 Audio (DFPlayer)

**Classification:** **NON-CRITICAL**

**Timing Requirements:**
- Command latency: ~100ms
- Playback start: <500ms
- Missing sounds: tolerable

**Consequences of Delay:**
- Delayed feedback
- User annoyance
- No functional impact

**Justification:**
Audio is user feedback only. Vehicle operates normally without audio. Delays up to 1 second are perceptually acceptable for non-critical sounds.

---

#### 1.3.3 LED Lighting Effects

**Classification:** **NON-CRITICAL** (except brake/turn signals)

**Timing Requirements:**
- Update rate: 20 Hz
- Effect timing: perceptual
- Turn signal: 1-2 Hz flash rate

**Consequences of Delay:**
- Delayed effects
- Desync between LEDs
- Turn signal compliance

**Tolerance:**
- Decorative effects: very tolerant
- Turn signals: must meet legal requirements (0.5-2 Hz)
- Brake lights: <200ms acceptable

**Justification:**
Most LED functions are aesthetic. Turn signals and brake lights have legal timing requirements but with wide tolerance (hundreds of milliseconds).

---

#### 1.3.4 Configuration Storage

**Classification:** **NON-CRITICAL**

**Timing Requirements:**
- Read: on boot only
- Write: user-triggered
- No real-time requirement

**Consequences of Delay:**
- Delayed config save
- User wait time
- No operational impact

**Justification:**
Configuration operations happen at boot or during user menu interaction. Seconds of delay are acceptable.

---

#### 1.3.5 Telemetry Logging

**Classification:** **NON-CRITICAL**

**Timing Requirements:**
- Update rate: 10 Hz
- Logging latency: <1s
- Data loss: tolerable

**Consequences of Delay:**
- Delayed logs
- Missing data points
- Diagnostic impact only

**Justification:**
Logging is for post-hoc analysis. Real-time logging not required. Missing data points reduce diagnostic value but don't affect operation.

---

## 2. COUPLING & RISK ANALYSIS

### 2.1 Tight Couplings

#### 2.1.1 TFT Display ↔ HUD Compositor ↔ PSRAM

**Coupling Strength:** VERY HIGH

**Dependencies:**
```
TFT_eSPI library ──> ESP32-S3 PSRAM
      │
      ├──> 300KB framebuffer
      ├──> Sprite rendering (200KB)
      └──> SPI peripheral (HSPI)
           │
           └──> HUD Compositor
                ├──> Multiple layers
                ├──> Dirty rectangle tracking
                └──> Render pipeline
```

**Risk:**
- Cannot split across ECUs
- PSRAM dependency (ESP32-S3 specific)
- SPI bus tied to ESP32 pins
- TFT_eSPI library not portable to STM32

**Recommendation:** **KEEP ON ESP32-S3**

---

#### 2.1.2 I2C Bus ↔ Motor Controllers ↔ GPIO Expander

**Coupling Strength:** HIGH

**Dependencies:**
```
PCA9685 (3x PWM) + MCP23017 (GPIO) ──> I2C Bus @ GPIO 8/9
                │
                ├──> Traction motors (4x)
                ├──> Steering motor (1x)
                └──> Direction control (8 pins)
                     │
                     └──> BTS7960 drivers (5x)
```

**Risk:**
- I2C bus hang = total motor loss
- Mutex protection required
- Single bus = single point of failure
- Recovery mechanism critical

**Recommendation:** **CAN OFFLOAD WITH CAUTION**
- STM32 becomes I2C master for motors
- ESP32 sends high-level commands via CAN
- STM32 handles low-level PWM/timing
- Bus isolation improves reliability

---

#### 2.1.3 Wheel Sensors ↔ ABS ↔ TCS ↔ Traction Control

**Coupling Strength:** HIGH

**Dependencies:**
```
Wheel ISRs (4x) ──> Speed calculation
                         │
                         ├──> ABS system
                         ├──> TCS system
                         └──> Speedometer
                              │
                              └──> Traction::update()
                                   └──> PWM modulation
```

**Risk:**
- ABS/TCS need real-time wheel data
- ISRs must be on same MCU as decision logic
- Cross-ECU latency breaks control loop

**Recommendation:** **COLOCATE ON SAME ECU**
- Option A: All on ESP32 (current)
- Option B: All on STM32 Motor ECU
- Do NOT split across ECUs

---

### 2.2 Crash Propagation Paths

#### 2.2.1 I2C Bus Hang

**Trigger:**
- PCA9685 locks up
- MCP23017 NAK storm
- Bus contention
- Electrical noise

**Propagation:**
```
I2C Hang ──> Mutex timeout (100ms)
    │
    ├──> Motor control freeze
    ├──> Traction update blocked
    ├──> Steering update blocked
    └──> Current sensor reads blocked
         │
         └──> Main loop delay (>10ms)
              │
              └──> Watchdog timeout (30s)
                   │
                   └──> SYSTEM RESET
```

**Mitigation:**
- I2C recovery mechanism (active)
- Mutex timeouts
- Watchdog protection

**CAN Migration Benefit:**
- Isolates I2C bus per ECU
- ESP32 I2C failure ≠ motor ECU failure
- Motor control continues if ESP32 I2C hangs

---

#### 2.2.2 Display/SPI Crash

**Trigger:**
- TFT_eSPI exception
- Touch controller hang
- SPI bus error
- PSRAM allocation failure

**Propagation:**
```
Display Exception ──> Try/catch in HUDManager::init()
    │
    ├──> initialized = false
    ├──> System continues without display
    └──> NO CRASH (isolated)
```

**Mitigation:**
- Exception handling in place
- Graceful degradation
- Vehicle operates without display

**CAN Migration Impact:**
- Display already isolated
- No improvement needed

---

#### 2.2.3 Sensor Failure

**Trigger:**
- Temperature sensor disconnect
- Wheel sensor failure
- Obstacle LiDAR timeout
- Current sensor I2C error

**Propagation:**
```
Sensor Failure ──> Error logging
    │
    ├──> Placeholder data returned
    ├──> Safety system degradation (if critical)
    └──> Continue operation with warning
```

**Mitigation:**
- Timeout detection
- Fallback values
- Error codes logged

**CAN Migration Benefit:**
- Distributed sensing improves reliability
- STM32 sensors independent of ESP32 issues

---

### 2.3 USB Stability Impact

**Current System:**
- USB serial @ 115200 baud
- Logging output
- Diagnostic interface

**Modules Affecting USB:**
- Logger::* functions (all subsystems)
- Serial.print* calls
- Exception handlers

**CAN Migration Impact:**
- ESP32 remains USB gateway
- STM32 ECUs send logs via CAN
- USB traffic may increase
- Risk: CAN logging flood could overwhelm USB

**Recommendation:**
- Rate-limit CAN diagnostic messages
- Prioritize critical messages
- Buffer non-critical logs

---

### 2.4 Vehicle Safety Impact Matrix

| Subsystem | Current State | If Failed | Migration Risk |
|-----------|---------------|-----------|----------------|
| **Traction Motors** | ESP32 I2C | Loss of propulsion | ✅ REDUCES risk (isolates I2C) |
| **Steering Motor** | ESP32 I2C | Loss of steering | ✅ REDUCES risk (isolates I2C) |
| **Wheel Sensors** | ESP32 GPIO ISR | ABS/TCS fail | ⚠️  NEUTRAL (must colocate with ABS) |
| **ABS/TCS** | ESP32 software | Wheel lock | ⚠️  NEUTRAL (logic can move) |
| **Current Sensors** | ESP32 I2C | Overcurrent protection delayed | ⚠️  NEUTRAL (slow time constant) |
| **Display** | ESP32 SPI | No visual feedback | ❌ CANNOT MIGRATE (PSRAM tied) |
| **Power Mgmt** | ESP32 GPIO | Loss of power control | ❌ DANGEROUS (must control relays) |
| **Watchdog** | ESP32 HW/SW | System hang | ❌ MUST REMAIN (ESP32 responsibility) |

---

## 3. CAN OFFLOAD CANDIDATE ANALYSIS

### 3.1 Traction Motor Control

**Can it be moved to STM32?** ✅ **YES**

**Latency Sensitivity:**
- **Current:** <1ms PWM update
- **CAN Latency:** 1-2ms @ 500 kbps
- **Verdict:** Acceptable with local control loop

**Failure Impact:**
- **ESP32→STM32 CAN failure:** STM32 maintains last command (safe stop)
- **STM32 failure:** ESP32 detects via heartbeat timeout
- **Severity:** HIGH (loss of propulsion)

**Required Bandwidth:**
```
CAN Message: Traction Command (100 Hz)
├─ 4x wheel throttle values (4 bytes)
├─ 1x brake enable (1 byte)
├─ 1x direction (1 byte)
└─ Total: 6 bytes × 100 Hz = 600 bytes/sec = 4.8 kbps
```

**Required Determinism:**
- 10ms command interval
- CAN @ 500 kbps: <1ms message time
- **Verdict:** CAN determinism sufficient

**Architecture:**
```
ESP32 (Gateway) ──CAN──> STM32 Motor ECU
                             │
     High-level commands     ├──> PCA9685 PWM (local I2C)
     (throttle, direction)   ├──> MCP23017 direction (local I2C)
                             ├──> BTS7960 drivers (4x)
                             └──> Wheel motors (4x)
                                  │
                                  └──> Current feedback (CAN)
```

**Benefits:**
- ✅ Isolates motor I2C bus from ESP32
- ✅ Reduces ESP32 I2C traffic
- ✅ Dedicated MCU for motor control
- ✅ Better PWM timing (STM32 hardware timers)

**Risks:**
- ⚠️  CAN bus failure = loss of propulsion
- ⚠️  Requires heartbeat/watchdog between ECUs
- ⚠️  Increased system complexity

**Recommendation:** **YES - High Value Candidate**

---

### 3.2 Steering Motor Control

**Can it be moved to STM32?** ✅ **YES**

**Latency Sensitivity:**
- **Current:** 10ms position control loop
- **CAN Latency:** 1-2ms
- **Verdict:** Acceptable for position commands

**Failure Impact:**
- **CAN failure:** STM32 enters safe mode (center steering)
- **STM32 failure:** ESP32 detects timeout, alerts driver
- **Severity:** CRITICAL (loss of steering)

**Required Bandwidth:**
```
CAN Message: Steering Command (100 Hz)
├─ Target position (2 bytes, encoder counts)
├─ Max speed (1 byte)
└─ Total: 3 bytes × 100 Hz = 300 bytes/sec = 2.4 kbps

CAN Message: Steering Feedback (100 Hz)
├─ Current position (2 bytes)
├─ Current (2 bytes)
├─ Status (1 byte)
└─ Total: 5 bytes × 100 Hz = 500 bytes/sec = 4 kbps
```

**Required Determinism:**
- Position loop: 10ms (100 Hz)
- CAN message time: <1ms
- **Verdict:** Determinism maintained

**Architecture:**
```
ESP32 (Gateway) ──CAN──> STM32 Steering ECU
                             │
     Position commands       ├──> Encoder reader (local GPIO ISR)
     (target angle)          ├──> PCA9685 PWM (local I2C)
                             ├──> MCP23017 direction (local I2C)
                             └──> BTS7960 driver
                                  │
                                  └──> RS390 motor + encoder
                                       │
                                       └──> Position feedback (CAN)
```

**Benefits:**
- ✅ Encoder ISRs on STM32 (better real-time)
- ✅ Isolated I2C bus
- ✅ Hardware encoder interface on STM32
- ✅ Reduced ESP32 ISR load

**Risks:**
- ⚠️  CAN failure = loss of steering control
- ⚠️  Most safety-critical offload
- ⚠️  Requires robust failsafe

**Recommendation:** **YES - But Requires Most Care**

---

### 3.3 Sensor Acquisition (Current, Temperature, Wheels)

**Can it be moved to STM32?** ✅ **YES**

**Latency Sensitivity:**
- **Current Sensors:** 50ms update → CAN adds 1-2ms (tolerable)
- **Temperature:** 1s update → CAN insignificant
- **Wheel Sensors:** ISR-driven → MUST colocate with ABS/TCS

**Failure Impact:**
- **CAN failure:** ESP32 uses last known values, safe limits
- **STM32 failure:** ESP32 detects timeout, enters limp mode
- **Severity:** MEDIUM

**Required Bandwidth:**
```
CAN Message: Sensor Telemetry (20 Hz)
├─ 6x current values (12 bytes)
├─ 6x voltage values (12 bytes)
├─ 4x temperatures (4 bytes)
├─ 4x wheel speeds (8 bytes)
└─ Total: 36 bytes × 20 Hz = 720 bytes/sec = 5.76 kbps
```

**Architecture:**
```
STM32 Sensor ECU
├──> TCA9548A + 6x INA226 (local I2C)
├──> DS18B20 temperature (local OneWire)
└──> Sensor data (CAN @ 20 Hz)
     │
     └──> ESP32 (Gateway)
          └──> HUD display
```

**Benefits:**
- ✅ Reduces ESP32 I2C traffic significantly
- ✅ Isolates sensor I2C failures
- ✅ Dedicated sensor processing
- ✅ ESP32 HUD remains responsive

**Risks:**
- ⚠️  CAN bandwidth (5.76 kbps continuous)
- ⚠️  Sensor data latency (not critical)

**Recommendation:** **YES - Good Candidate**

---

### 3.4 ABS/TCS Logic

**Can it be moved to STM32?** ✅ **YES - With Wheel Sensors**

**Latency Sensitivity:**
- **Current:** 100 Hz decision loop
- **CAN Latency:** Adds 1-2ms to intervention
- **Verdict:** Acceptable if colocated with wheel sensors

**Failure Impact:**
- **CAN failure:** ABS/TCS disabled, normal braking still works
- **STM32 failure:** ESP32 detects, disables ABS/TCS
- **Severity:** MEDIUM (vehicle drivable without ABS/TCS)

**Required Bandwidth:**
```
CAN Message: Safety Status (100 Hz)
├─ ABS active (1 bit per wheel)
├─ TCS active (1 bit per wheel)
├─ Slip ratios (4 bytes)
└─ Total: 5 bytes × 100 Hz = 500 bytes/sec = 4 kbps
```

**Architecture:**
```
STM32 Motor ECU
├──> Wheel sensor ISRs (local GPIO)
├──> ABS/TCS logic (local computation)
└──> Traction modulation (local PWM)
     │
     └──> Safety status (CAN to ESP32 for HUD)
```

**Benefits:**
- ✅ Colocates wheel sensors + ABS/TCS logic
- ✅ Reduces cross-ECU latency
- ✅ Safety system independent of ESP32

**Risks:**
- ⚠️  Safety logic on separate ECU
- ⚠️  Requires certification/validation

**Recommendation:** **YES - If Colocated with Motors**

---

### 3.5 Display/HUD

**Can it be moved to STM32?** ❌ **NO - DANGEROUS**

**Why NOT:**
1. **PSRAM Dependency:**
   - TFT_eSPI requires 300KB+ framebuffer
   - ESP32-S3 has 8MB PSRAM
   - STM32 typically <512KB RAM
   - Would require external RAM chip

2. **Library Compatibility:**
   - TFT_eSPI is ESP32-specific
   - Deep integration with ESP32 SPI peripheral
   - Would require complete rewrite

3. **Complexity:**
   - HUD compositor: 22KB of code
   - Dirty rectangle tracking
   - Multi-layer rendering
   - Touch calibration system

4. **CAN Bandwidth:**
   - Full frame: 480×320×2 = 300KB
   - Even at 1 Mbps: 2.4 seconds per frame
   - Compressed: still impractical

**Recommendation:** **KEEP ON ESP32 - NON-NEGOTIABLE**

---

### 3.6 Power Management

**Can it be moved to STM32?** ❌ **DANGEROUS**

**Why NOT:**
1. **Relay Control:**
   - RELAY_MAIN (GPIO 35) controls ESP32 power hold
   - If STM32 controls power, can't turn off STM32
   - Circular dependency

2. **Ignition Detection:**
   - GPIO 40/41 on ESP32
   - Must read on same MCU as relay control

3. **Graceful Shutdown:**
   - 5-second sequence coordinates all systems
   - ESP32 must orchestrate shutdown
   - Audio playback during shutdown

**Recommendation:** **KEEP ON ESP32**

---

### 3.7 Obstacle Detection (LiDAR)

**Can it be moved to STM32?** ⚠️  **POSSIBLE - Low Priority**

**Latency Sensitivity:**
- **Current:** 15 Hz sensor update (sensor-limited)
- **CAN Latency:** Irrelevant (slow sensor)
- **Verdict:** CAN latency not a factor

**Failure Impact:**
- **CAN failure:** No obstacle detection
- **STM32 failure:** Same
- **Severity:** MEDIUM (nice to have, not critical)

**Required Bandwidth:**
```
CAN Message: Obstacle Data (15 Hz)
Option A: Full matrix (64 points)
├─ 64 distances × 2 bytes = 128 bytes
└─ 128 bytes × 15 Hz = 1920 bytes/sec = 15.36 kbps

Option B: Processed zones (4 sectors)
├─ 4 minimum distances × 2 bytes = 8 bytes
├─ 4 status flags = 4 bytes
└─ 12 bytes × 15 Hz = 180 bytes/sec = 1.44 kbps
```

**Architecture:**
```
STM32 Sensor ECU
├──> TOFSense UART (local)
├──> Process 8x8 matrix
├──> Extract zones
└──> Send compressed data (CAN)
     │
     └──> ESP32 (safety decisions + HUD)
```

**Benefits:**
- ✅ Reduces ESP32 UART processing
- ✅ Can preprocess sensor data

**Risks:**
- ⚠️  Low value (sensor already slow)
- ⚠️  Adds complexity without much gain

**Recommendation:** **MAYBE - Low Priority**

---

### 3.8 Audio (DFPlayer)

**Can it be moved to STM32?** ⚠️  **POSSIBLE - Low Value**

**Latency Sensitivity:**
- **Current:** ~100ms command latency
- **CAN Latency:** Negligible
- **Verdict:** No latency issue

**Failure Impact:**
- **CAN failure:** No audio (non-critical)
- **STM32 failure:** Same
- **Severity:** LOW

**Required Bandwidth:**
```
CAN Message: Audio Commands (event-driven)
├─ Command (1 byte)
├─ Track number (1 byte)
└─ Avg: ~10 messages/minute = negligible
```

**Recommendation:** **MAYBE - But Why?**
- No performance benefit
- Adds complexity
- Audio already non-critical
- Keep on ESP32 for simplicity

---

### 3.9 LED Lighting

**Can it be moved to STM32?** ⚠️  **POSSIBLE**

**Latency Sensitivity:**
- **Current:** 20 Hz update
- **CAN Latency:** Not critical
- **Verdict:** Acceptable

**Failure Impact:**
- **CAN failure:** LEDs freeze in last state
- **STM32 failure:** No lighting
- **Severity:** MEDIUM (safety lighting important)

**Required Bandwidth:**
```
CAN Message: LED Commands (20 Hz)
├─ Front mode (1 byte)
├─ Rear mode (1 byte)
├─ Turn signal (1 byte)
├─ Brightness (1 byte)
└─ Total: 4 bytes × 20 Hz = 80 bytes/sec = 640 bps
```

**Benefits:**
- ✅ Reduces ESP32 WS2812B bitstream processing
- ✅ Offloads FastLED library overhead

**Risks:**
- ⚠️  Turn signal/brake light safety critical
- ⚠️  Must maintain legal compliance

**Recommendation:** **MAYBE - Medium Priority**

---

## 4. CAN BANDWIDTH ANALYSIS

### 4.1 Message Inventory

| Message | Rate | Size | Bandwidth | Priority |
|---------|------|------|-----------|----------|
| Traction Command | 100 Hz | 6 B | 4.8 kbps | HIGH |
| Traction Feedback | 100 Hz | 8 B | 6.4 kbps | HIGH |
| Steering Command | 100 Hz | 3 B | 2.4 kbps | HIGH |
| Steering Feedback | 100 Hz | 5 B | 4.0 kbps | HIGH |
| Sensor Telemetry | 20 Hz | 36 B | 5.76 kbps | MEDIUM |
| Safety Status | 100 Hz | 5 B | 4.0 kbps | HIGH |
| Obstacle Data | 15 Hz | 12 B | 1.44 kbps | LOW |
| LED Commands | 20 Hz | 4 B | 0.64 kbps | LOW |
| Heartbeats | 10 Hz | 2 B | 0.16 kbps | HIGH |

**Total Bandwidth:** ~29.6 kbps

### 4.2 CAN Bus Utilization

```
CAN 2.0A @ 500 kbps:
- Theoretical: 500 kbps
- Practical: ~70% utilization = 350 kbps
- Required: ~30 kbps
- Utilization: 30/350 = 8.6%
```

**Verdict:** ✅ **Plenty of headroom**

### 4.3 Message Latency

```
Single CAN frame (11-bit ID + 8 bytes data):
- Bit time @ 500 kbps: 2 µs
- Frame time: ~130 bits × 2 µs = 260 µs
- Queue delay (worst case, 8 frames): 2.08 ms
- Total latency: <3 ms
```

**Verdict:** ✅ **Acceptable for all systems**

---

## 5. FAILURE MODE ANALYSIS

### 5.1 CAN Bus Failure

**Scenario:** Physical CAN bus short/open circuit

**Detection:**
- Heartbeat timeout (100ms)
- Message TX fail counters
- Bus-off state

**ESP32 Response:**
```
CAN Failure Detected
├──> Enter Safe Mode
├──> Disable remote motor control
├──> Alert driver (HUD + audio + LEDs)
├──> Attempt bus recovery (3 retries)
└──> If recovery fails:
     ├──> Maintain last safe state
     ├──> Log error
     └──> Allow manual restart
```

**STM32 Motor ECU Response:**
```
CAN Heartbeat Timeout (100ms)
├──> Enter Failsafe Mode
├──> Gradual deceleration (5 seconds)
├──> Stop motors (PWM = 0)
├──> Center steering (if steering ECU)
└──> Wait for CAN recovery
```

**Impact:** Vehicle stops safely, requires manual restart

---

### 5.2 STM32 ECU Failure

**Scenario:** STM32 crashes, hangs, or reboots

**Detection:**
- Heartbeat timeout on ESP32 (100ms)
- CAN message sequence error
- Watchdog timeout on STM32

**ESP32 Response:**
```
STM32 Heartbeat Lost
├──> Log critical error
├──> Disable affected subsystem
├──> Alert driver (HUD)
├──> Attempt to re-initialize STM32 (CAN reset command)
└──> Enter Limp Mode
     ├──> Fixed speed (if traction ECU failed)
     ├──> Manual steering (if steering ECU failed)
     └──> Continue with degraded capability
```

**Impact:** Degraded operation, limp home mode

---

### 5.3 ESP32 Failure

**Scenario:** ESP32 crashes or hangs

**Detection:**
- STM32 detects heartbeat timeout
- Power manager detects watchdog timeout

**STM32 Response:**
```
ESP32 Heartbeat Lost
├──> Maintain last command for 5 seconds
├──> Gradual deceleration
├──> Safe stop
└──> Enter autonomous safe mode
     ├──> Motors off
     ├──> Steering centered
     └──> Wait for ESP32 reboot or manual intervention
```

**Impact:** Vehicle stops, awaits ESP32 recovery

---

## 6. DETERMINISM REQUIREMENTS

### 6.1 Hard Real-Time Constraints

**Traction Motor PWM:**
- **Requirement:** <1ms update latency
- **CAN Message Time:** 260 µs
- **Processing Time:** <500 µs on STM32
- **Total:** <1ms ✅

**Steering Position Control:**
- **Requirement:** 10ms loop time
- **CAN Latency:** <3ms (worst case)
- **Processing:** <2ms
- **Total:** <5ms (within 10ms budget) ✅

**Wheel Sensor ISRs:**
- **Requirement:** <1µs ISR latency
- **Implementation:** Local to STM32 (no CAN involved) ✅

### 6.2 Soft Real-Time Constraints

**Current Sensor Updates:**
- **Requirement:** 50ms update interval
- **CAN Latency:** <3ms
- **I2C Read:** ~5ms
- **Total:** <10ms (within 50ms budget) ✅

**Safety Status (ABS/TCS):**
- **Requirement:** 10ms decision loop
- **Local Processing:** (no CAN if colocated)
- **Status Update to ESP32:** 10ms (for HUD only) ✅

**Verdict:** ✅ **All timing requirements met with CAN architecture**

---

## SUMMARY TABLE: OFFLOAD CANDIDATES

| Subsystem | Offload? | Priority | Bandwidth | Risk | Benefit |
|-----------|----------|----------|-----------|------|---------|
| Traction Motors | ✅ YES | HIGH | 11.2 kbps | MEDIUM | ⭐⭐⭐⭐⭐ |
| Steering Motor | ✅ YES | HIGH | 6.4 kbps | HIGH | ⭐⭐⭐⭐⭐ |
| Current Sensors | ✅ YES | MEDIUM | 5.76 kbps | LOW | ⭐⭐⭐⭐ |
| Wheel Sensors | ✅ YES* | HIGH | (local) | MEDIUM | ⭐⭐⭐⭐ |
| ABS/TCS Logic | ✅ YES* | MEDIUM | 4 kbps | MEDIUM | ⭐⭐⭐ |
| Temperature | ✅ YES | LOW | (rare) | LOW | ⭐⭐ |
| Obstacle LiDAR | ⚠️  MAYBE | LOW | 1.44 kbps | LOW | ⭐⭐ |
| LED Lighting | ⚠️  MAYBE | LOW | 0.64 kbps | LOW | ⭐⭐ |
| Audio | ⚠️  MAYBE | LOW | ~0 kbps | LOW | ⭐ |
| Display/HUD | ❌ NO | - | - | - | - |
| Power Mgmt | ❌ NO | - | - | - | - |
| Touch | ❌ NO | - | - | - | - |
| Storage | ❌ NO | - | - | - | - |

\* Must colocate with motor control ECU

**Total Recommended Bandwidth:** ~30 kbps @ 500 kbps CAN = **6% utilization**

---

## NEXT DOCUMENT

See `TARGET_ECU_ARCHITECTURE.md` for the proposed multi-ECU system design.

---

**END OF DOCUMENT**
