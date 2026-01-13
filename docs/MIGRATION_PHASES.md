# MIGRATION PHASES
# Safe Transition to Multi-ECU CAN Architecture

**Document Version:** 1.0  
**Date:** 2026-01-13  
**Status:** 🗺️ IMPLEMENTATION ROADMAP - NOT STARTED

---

## EXECUTIVE SUMMARY

This document defines a **phased, non-destructive migration strategy** to transition from the current monolithic ESP32-S3 architecture to a multi-ECU CAN-based system. The approach ensures **the vehicle remains operational** throughout the migration and can **roll back at any phase** if issues arise.

**Critical Principle:** **NEVER brick the vehicle.** Each phase is independently testable and reversible.

---

## MIGRATION PHILOSOPHY

### Core Principles

1. **Shadow Mode First:** New CAN infrastructure runs in parallel with existing I2C control, no authority transfer
2. **Progressive Validation:** Extensive testing at each phase before proceeding
3. **Rollback Capability:** Each phase can revert to previous state without data loss
4. **No Downtime:** Vehicle remains drivable during development (test in "standalone" mode)
5. **Safety-First:** No phase compromises safety systems

### Risk Mitigation

- ✅ **Dual Operation:** Old and new systems run simultaneously until validated
- ✅ **Compile-Time Flags:** Use `#ifdef` to switch between old/new code
- ✅ **Shadow Monitoring:** Compare CAN vs. I2C values, log discrepancies
- ✅ **Manual Override:** Physical switch to disable CAN and revert to I2C
- ✅ **Watchdog Protection:** All phases maintain watchdog feeding

---

## PHASE 0: PRE-MIGRATION BASELINE

**Duration:** 1-2 days  
**Goal:** Establish stable baseline and testing infrastructure  
**Reversibility:** N/A (no changes)

### Tasks

#### 0.1 Baseline Documentation
- [ ] Capture current firmware build (git tag `v2.17.1-pre-can`)
- [ ] Document all sensor calibrations (pedal, encoder, touch)
- [ ] Backup EEPROM configuration to file
- [ ] Record baseline performance metrics:
  - Loop frequency (Hz)
  - Heap free (KB)
  - I2C transaction time (μs)
  - Motor response latency (ms)

#### 0.2 Test Infrastructure Setup
- [ ] Create `tests/can_migration/` directory
- [ ] Implement automated test suite:
  - I2C device enumeration test
  - Motor PWM generation test
  - Sensor reading validation test
- [ ] Set up CAN bus monitoring hardware:
  - USB-CAN adapter (e.g., CANable, PCAN-USB)
  - CAN bus analyzer software (e.g., BUSMASTER, SavvyCAN)

#### 0.3 Hardware Preparation
- [ ] Order STM32F405 development board (e.g., Black Pill)
- [ ] Order CAN transceivers (2x TJA1050 or MCP2551)
- [ ] Order CAN bus cable (twisted pair, shielded)
- [ ] Order 120Ω termination resistors (2x)
- [ ] Breadboard prototype setup for initial CAN testing

### Deliverables
- ✅ Git tag `v2.17.1-pre-can` (baseline)
- ✅ Sensor calibration backup file
- ✅ Performance metrics report (baseline.csv)
- ✅ CAN hardware kit ready

### Success Criteria
- All existing tests pass (100%)
- Vehicle operates normally (no degraded modes)
- Full backup created and verified

---

## PHASE 1: SHADOW CAN INFRASTRUCTURE

**Duration:** 1-2 weeks  
**Goal:** Add CAN bus hardware and software, ESP32 still controls everything  
**Reversibility:** Remove CAN transceiver, revert to `v2.17.1-pre-can`

### Architecture

```
ESP32 (Master - Full Control)
 ├─> I2C → Motors (ACTIVE CONTROL)
 ├─> GPIO → Relays (ACTIVE CONTROL)
 └─> CAN → Motor ECU (SHADOW ONLY, NO CONTROL)

STM32 Motor ECU (Shadow - Monitoring Only)
 ├─> CAN ← ESP32 (receives commands, doesn't act)
 ├─> I2C → Sensors (reads locally)
 └─> CAN → ESP32 (sends sensor data for comparison)
```

### Tasks

#### 1.1 ESP32 CAN Driver Integration
- [ ] Add TWAI (CAN) driver to `platformio.ini`
- [ ] Implement CAN initialization in `src/core/can_bus.cpp`:
  ```cpp
  namespace CANBus {
    void init();  // 500 kbps, no filters yet
    bool send(uint32_t id, uint8_t* data, uint8_t len);
    bool receive(uint32_t& id, uint8_t* data, uint8_t& len);
  }
  ```
- [ ] Add CAN TX task (100 Hz, sends motor commands to shadow ECU)
- [ ] Add CAN RX task (100 Hz, receives sensor data from shadow ECU)
- [ ] Add compile flag `#define SHADOW_CAN_ENABLED`

#### 1.2 STM32 Motor ECU Development
- [ ] Set up STM32CubeIDE project (STM32F405RGT6)
- [ ] Implement CAN driver (bxCAN, 500 kbps)
- [ ] Implement CAN RX handler (receive motor commands, log but don't act)
- [ ] Port I2C sensor reading code from ESP32:
  - PCA9685 PWM drivers (don't write PWM, just init)
  - MCP23017 GPIO expander (read shifter only)
  - TCA9548A + INA226 current sensors
  - DS18B20 temperature sensors
- [ ] Implement CAN TX (send sensor data to ESP32 @ 100 Hz)

#### 1.3 Shadow Validation Logic
- [ ] Implement `src/test/can_shadow_validator.cpp`:
  ```cpp
  namespace ShadowValidator {
    void compareMotorCommands();  // CAN TX vs. I2C actual
    void compareSensorData();     // CAN RX vs. local I2C
    void logDiscrepancies();      // Write to Serial
  }
  ```
- [ ] Log to file (SD card or Serial) for offline analysis

#### 1.4 Hardware Integration
- [ ] Wire CAN transceivers to ESP32 and STM32:
  - ESP32 TWAI: TX=GPIO 5, RX=GPIO 4 (reassign GPIOs if needed)
  - STM32 bxCAN1: TX=PB9, RX=PB8 (STM32F405)
- [ ] Connect CAN_H and CAN_L between transceivers (twisted pair)
- [ ] Install 120Ω termination resistors at both ends
- [ ] Power STM32 from separate 12V supply (isolated from ESP32)

### Testing

#### 1.5 Shadow Mode Validation
- [ ] **Test 1:** CAN bus communication (send/receive loopback)
- [ ] **Test 2:** Motor commands transmitted on CAN (ID 0x100)
- [ ] **Test 3:** Sensor data received from STM32 (ID 0x200-0x204)
- [ ] **Test 4:** Data comparison (ESP32 I2C vs. STM32 CAN)
  - Wheel speeds: <1% error
  - Current sensors: <5% error
  - Temperatures: <2°C error
- [ ] **Test 5:** CAN bus failure (disconnect cable, verify ESP32 continues on I2C)

### Deliverables
- ✅ Git tag `v2.18.0-shadow-can` (ESP32 + STM32 shadow mode)
- ✅ CAN message definitions (`docs/CAN_PROTOCOL_v1.md`)
- ✅ Shadow validation report (`tests/shadow_validation_report.csv`)

### Success Criteria
- ESP32 controls vehicle normally (I2C path unchanged)
- CAN bus transmits/receives with <1% packet loss
- Sensor data matches within tolerance (ESP32 I2C vs. STM32 local)
- No performance degradation (loop frequency unchanged)

### Rollback Plan
- Disable `SHADOW_CAN_ENABLED` flag, recompile
- Remove CAN hardware (optional)
- Revert to `v2.17.1-pre-can` tag

---

## PHASE 2: MIRROR CONTROL

**Duration:** 2-3 weeks  
**Goal:** STM32 Motor ECU controls actuators based on CAN commands, but ESP32 I2C remains active (redundant control)  
**Reversibility:** Disable `MIRROR_CONTROL_ENABLED`, revert to Phase 1

### Architecture

```
ESP32 (Master - Full Control)
 ├─> I2C → Motors (ACTIVE CONTROL)
 ├─> GPIO → Relays (ACTIVE CONTROL)
 └─> CAN → Motor ECU (sends commands)

STM32 Motor ECU (Mirror - Parallel Control)
 ├─> CAN ← ESP32 (receives commands)
 ├─> I2C → Motors (MIRRORS ESP32 COMMANDS)
 ├─> GPIO → Relays (MIRRORS ESP32 COMMANDS)
 └─> CAN → ESP32 (sends sensor data + ACKs)
```

**Key Change:** STM32 now writes PWM to motors, but ESP32 also writes PWM (both active).

### Tasks

#### 2.1 ESP32 Modifications
- [ ] Add compile flag `#define MIRROR_CONTROL_ENABLED`
- [ ] Modify motor control to support dual-write:
  ```cpp
  #ifdef MIRROR_CONTROL_ENABLED
    // Write to I2C (old path)
    pca9685.setPWM(channel, 0, pwmValue);
    
    // Send via CAN (new path)
    CANBus::sendMotorCommand(throttle, steeringAngle);
  #else
    // I2C only (Phase 1 mode)
    pca9685.setPWM(channel, 0, pwmValue);
  #endif
  ```

#### 2.2 STM32 Motor Control Implementation
- [ ] Implement motor control state machine:
  ```cpp
  enum MotorState {
    STARTUP,
    STANDBY,
    MIRROR_ACTIVE,  // Phase 2 state
    AUTONOMOUS,     // Phase 3 state
    FAIL_SAFE
  };
  ```
- [ ] Implement PWM generation (PCA9685 I2C writes)
- [ ] Implement relay control (GPIO outputs)
- [ ] Implement CAN command handler:
  ```cpp
  void handleMotorCommand(uint8_t* data) {
    float throttle = data[0] / 2.0f;  // 0-100%
    float steeringAngle = data[1] - 90;  // -90° to +90°
    
    // Calculate PWM values
    uint16_t pwmFL = calculatePWM(throttle, steeringAngle, WHEEL_FL);
    // ... (repeat for FR, RL, RR, steering)
    
    // Write to PCA9685
    pca9685_write(PCA_FRONT, CH_FL_FWD, pwmFL);
    // ...
  }
  ```

#### 2.3 Mirror Validation
- [ ] Implement `src/test/can_mirror_validator.cpp`:
  ```cpp
  namespace MirrorValidator {
    void compareActuatorOutputs();  // ESP32 I2C PWM vs. STM32 I2C PWM
    void detectConflicts();         // Flag if ESP32 and STM32 write different values
    void logMirrorMetrics();        // Latency, accuracy, conflicts
  }
  ```
- [ ] Add oscilloscope probes to measure:
  - ESP32 I2C write timing
  - STM32 I2C write timing
  - PWM output (verify both match)

### Testing

#### 2.4 Mirror Mode Validation
- [ ] **Test 1:** Single actuator (motor FL only)
  - ESP32 sends 50% throttle
  - Verify both ESP32 and STM32 write same PWM to PCA9685
  - Oscilloscope: PWM duty cycle 50% ±1%
- [ ] **Test 2:** All motors simultaneously
  - ESP32 sends 75% throttle, 30° steering
  - Verify all 5 motors receive correct PWM (4 traction + 1 steering)
  - No I2C conflicts (both ESP32 and STM32 coexist on bus)
- [ ] **Test 3:** CAN latency measurement
  - Timestamp: ESP32 command sent
  - Timestamp: STM32 PWM applied
  - Verify latency <20ms
- [ ] **Test 4:** Conflict detection
  - Inject error: STM32 writes wrong PWM value
  - Verify validator logs discrepancy

### Deliverables
- ✅ Git tag `v2.19.0-mirror-control`
- ✅ Mirror validation report (`tests/mirror_validation_report.csv`)
- ✅ Oscilloscope waveforms (PWM comparison screenshots)

### Success Criteria
- ESP32 and STM32 both write identical PWM values (<1% difference)
- No I2C bus conflicts (both masters coexist)
- Vehicle responds normally to pedal and steering input
- CAN latency <20ms (ESP32 command → STM32 PWM)

### Rollback Plan
- Disable `MIRROR_CONTROL_ENABLED` flag, recompile
- STM32 stops writing to I2C, returns to shadow mode (Phase 1)
- ESP32 continues controlling motors via I2C

---

## PHASE 3: AUTHORITY TRANSFER

**Duration:** 2-3 weeks  
**Goal:** STM32 Motor ECU becomes primary controller, ESP32 I2C control disabled (CAN-only)  
**Reversibility:** Re-enable `MIRROR_CONTROL_ENABLED`, revert to Phase 2

### Architecture

```
ESP32 (Master - High-Level Only)
 ├─> CAN → Motor ECU (sends commands)
 └─> I2C ✖️ (DISABLED)

STM32 Motor ECU (Autonomous - Primary Control)
 ├─> CAN ← ESP32 (receives commands)
 ├─> I2C → Motors (PRIMARY CONTROL)
 ├─> GPIO → Relays (PRIMARY CONTROL)
 └─> CAN → ESP32 (sends sensor data + status)
```

**Key Change:** ESP32 no longer writes to I2C motors. STM32 is sole actuator controller.

### Tasks

#### 3.1 ESP32 Modifications
- [ ] Add compile flag `#define CAN_AUTHORITY_TRANSFER`
- [ ] Disable I2C motor control:
  ```cpp
  #ifdef CAN_AUTHORITY_TRANSFER
    // CAN only (new path)
    CANBus::sendMotorCommand(throttle, steeringAngle);
  #else
    // Dual-write (Phase 2 mode)
    pca9685.setPWM(channel, 0, pwmValue);
    CANBus::sendMotorCommand(throttle, steeringAngle);
  #endif
  ```
- [ ] Remove I2C device initialization:
  - ~~PCA9685 (0x40, 0x41, 0x42)~~ - No longer needed
  - ~~MCP23017 (0x20)~~ - No longer needed
  - ~~TCA9548A (0x70) + INA226~~ - No longer needed
- [ ] Update hardware init error handling (no longer fatal if I2C devices missing)

#### 3.2 STM32 Autonomous Operation
- [ ] Transition state machine to `AUTONOMOUS` mode
- [ ] Implement CAN timeout fail-safe:
  ```cpp
  void monitorCANTimeout() {
    if (millis() - lastCANCommandMs > 100) {
      // No ESP32 command for 100ms → Enter fail-safe
      enterFailSafe();
    }
  }
  ```
- [ ] Implement emergency stop handler:
  ```cpp
  void handleEmergencyStop() {
    // Immediate relay cutoff
    digitalWrite(PIN_TRACTION_RELAY, LOW);
    digitalWrite(PIN_STEERING_RELAY, LOW);
    
    // Zero all PWM
    for (int i = 0; i < 16; i++) {
      pca9685_write(PCA_FRONT, i, 0);
      pca9685_write(PCA_REAR, i, 0);
      pca9685_write(PCA_STEERING, i, 0);
    }
    
    // Broadcast fault
    CANBus::sendFault(FAULT_EMERGENCY_STOP);
  }
  ```

#### 3.3 Safety Enforcement
- [ ] Implement over-current protection (local on STM32):
  ```cpp
  void enforceCurrentLimits() {
    for (int i = 0; i < 6; i++) {
      if (currentSensors[i] > CURRENT_THRESHOLD[i]) {
        reducePower(i, 0.8f);  // Reduce to 80%
        
        if (currentSensors[i] > CURRENT_CRITICAL[i]) {
          enterFailSafe();  // Critical over-current
        }
      }
    }
  }
  ```
- [ ] Implement thermal protection (local on STM32):
  ```cpp
  void enforceThermalLimits() {
    for (int i = 0; i < 4; i++) {
      if (motorTemps[i] > 80) {
        reducePower(i, 0.8f);  // Warning level
      }
      if (motorTemps[i] > 100) {
        enterFailSafe();  // Critical temperature
      }
    }
  }
  ```

### Testing

#### 3.4 Authority Transfer Validation
- [ ] **Test 1:** Normal operation (ESP32 → CAN → STM32)
  - Drive vehicle normally (pedal, steering, shifter)
  - Verify smooth operation (no jerky movements)
  - Log CAN latency: <20ms
- [ ] **Test 2:** CAN timeout fail-safe
  - Disconnect ESP32 CAN (unplug cable)
  - Verify STM32 enters fail-safe within 100ms
  - Verify motors stop, relays open
  - Reconnect CAN, verify recovery
- [ ] **Test 3:** Emergency stop (ESP32 command)
  - ESP32 sends emergency stop (ID 0x001)
  - Verify STM32 responds within 10ms
  - Verify motors stop, relays open
- [ ] **Test 4:** Over-current protection (local STM32)
  - Inject fault: Short circuit motor (controlled)
  - Verify STM32 detects over-current
  - Verify STM32 enters fail-safe (no ESP32 involvement)
- [ ] **Test 5:** Thermal protection (local STM32)
  - Simulate high temperature (heat motor with external source)
  - Verify STM32 reduces power at 80°C
  - Verify STM32 enters fail-safe at 100°C

### Deliverables
- ✅ Git tag `v2.20.0-can-authority`
- ✅ Authority transfer validation report
- ✅ Fail-safe test results (timeout, over-current, thermal)

### Success Criteria
- Vehicle operates normally under CAN control (ESP32 → STM32)
- CAN timeout triggers fail-safe within 100ms
- Emergency stop responds within 10ms
- Local protections (current, thermal) work independently of ESP32
- No ESP32 I2C activity (oscilloscope verification)

### Rollback Plan
- Re-enable `MIRROR_CONTROL_ENABLED` flag, recompile
- ESP32 re-initializes I2C devices (PCA9685, MCP23017, etc.)
- Dual-write mode resumes (Phase 2)

---

## PHASE 4: SAFETY ENFORCEMENT AND OPTIMIZATION

**Duration:** 2-4 weeks  
**Goal:** Harden fail-safes, optimize CAN performance, add diagnostics, finalize production system  
**Reversibility:** Revert to Phase 3 (authority transfer without optimizations)

### Tasks

#### 4.1 Fail-Safe Hardening
- [ ] Implement heartbeat monitoring (bidirectional):
  - ESP32 sends heartbeat (ID 0x300) @ 10 Hz
  - STM32 sends heartbeat (ID 0x301) @ 10 Hz
  - Timeout: 500ms (5 missed messages) → Fault state
- [ ] Implement CAN bus health monitoring:
  - Track TX/RX error counters
  - Detect bus-off state, trigger recovery
  - Log CAN statistics (message counts, errors)
- [ ] Implement graceful degradation:
  - If 1 wheel sensor fails → Continue with 3 sensors (disable ABS/TCS)
  - If 1 current sensor fails → Use BTS7960 current sense (ADC backup)
  - If temperature sensor fails → Assume safe temp, log warning

#### 4.2 CAN Performance Optimization
- [ ] Implement CAN message filtering (hardware):
  - ESP32: Accept IDs 0x200-0x204, 0x301 (Motor ECU messages)
  - STM32: Accept IDs 0x100-0x102, 0x001, 0x300 (ESP32 messages)
- [ ] Optimize message packing (reduce CAN traffic):
  - Combine related data (e.g., all wheel speeds in one message)
  - Use appropriate data rates (e.g., 1 Hz for temperature, 100 Hz for wheel speeds)
- [ ] Implement DMA for CAN TX/RX (STM32):
  - Offload CAN I/O to DMA, free CPU cycles
  - Reduce ISR overhead

#### 4.3 Diagnostics and Logging
- [ ] Implement fault code system:
  - DTC (Diagnostic Trouble Codes) following SAE J2012 standard
  - Store fault codes in STM32 flash (persistent across resets)
  - Read/clear via CAN (UDS protocol, ID 0x7DF)
- [ ] Implement CAN logger (ESP32):
  - Save CAN messages to SD card (if available)
  - Playback for offline analysis
- [ ] Implement remote diagnostics:
  - USB-CAN adapter support
  - Real-time monitoring via BUSMASTER or SavvyCAN

#### 4.4 Production Readiness
- [ ] Code review (all CAN-related code)
- [ ] Static analysis (Cppcheck, Clang-Tidy)
- [ ] MISRA C compliance check (safety-critical paths)
- [ ] Update documentation:
  - README: Add CAN migration section
  - docs/CAN_PROTOCOL_v1.md: Finalize message specs
  - docs/TROUBLESHOOTING_CAN.md: Add common issues and fixes

### Testing

#### 4.5 End-to-End Validation
- [ ] **Test 1:** Long-duration drive test (1 hour continuous operation)
  - Monitor CAN bus health (no errors, no bus-off)
  - Monitor CPU usage (ESP32 <70%, STM32 <80%)
  - Monitor memory usage (no leaks)
- [ ] **Test 2:** Stress test (rapid inputs)
  - Rapid pedal changes (0-100% in 100ms)
  - Rapid steering changes (-45° to +45° in 200ms)
  - Gear shifts (P → D → R → D)
  - Verify no CAN overruns, no missed commands
- [ ] **Test 3:** Fault injection (deliberate failures)
  - Disconnect 1 wheel sensor → Verify graceful degradation
  - Disconnect 1 current sensor → Verify backup (BTS7960 ADC)
  - Disconnect CAN cable → Verify fail-safe (motors off)
  - STM32 watchdog timeout → Verify reset and recovery
- [ ] **Test 4:** Environmental stress
  - Cold boot (-10°C) → Verify CAN initialization
  - Hot operation (50°C ambient) → Verify no thermal throttling
  - Vibration test (simulate rough terrain) → Verify CAN cable integrity

### Deliverables
- ✅ Git tag `v3.0.0-can-production` (production release)
- ✅ End-to-end validation report
- ✅ Fault injection test results
- ✅ Production documentation package:
  - CAN protocol spec (`docs/CAN_PROTOCOL_v1.md`)
  - Troubleshooting guide (`docs/TROUBLESHOOTING_CAN.md`)
  - Wiring diagram (`docs/CAN_WIRING_DIAGRAM.pdf`)

### Success Criteria
- 1 hour continuous operation with zero CAN errors
- Stress test passes (no overruns, no missed commands)
- All fault injection tests pass (graceful degradation)
- Environmental stress test passes (-10°C to 50°C)
- Code passes static analysis (zero critical issues)

### Rollback Plan
- Revert to Phase 3 (`v2.20.0-can-authority`)
- Remove optimizations (DMA, message filtering) if they cause issues
- Fall back to basic CAN operation

---

## POST-MIGRATION: PHASE 5+ (FUTURE EXPANSION)

### Phase 5: Sensor ECU (Optional)

**Goal:** Offload obstacle detection to dedicated STM32 Sensor ECU

**Tasks:**
- [ ] Develop STM32 Sensor ECU firmware
- [ ] Migrate TOFSense-M S LiDAR from ESP32 to Sensor ECU
- [ ] Add CAN messages for obstacle data (ID 0x400+)
- [ ] Integrate obstacle safety logic on ESP32 (consume CAN data)

**Timeline:** 3-4 weeks

### Phase 6: Lighting ECU (Optional)

**Goal:** Offload LED control to dedicated STM32 Lighting ECU

**Tasks:**
- [ ] Develop STM32 Lighting ECU firmware
- [ ] Migrate WS2812B control from ESP32 to Lighting ECU
- [ ] Add CAN messages for lighting commands (ID 0x500+)
- [ ] Implement dynamic lighting features (RGB underglow, matrix LEDs)

**Timeline:** 2-3 weeks

---

## RISK MANAGEMENT

### Critical Risks and Mitigations

| Risk | Probability | Impact | Mitigation |
|------|------------|--------|------------|
| **CAN bus wiring failure** | 🟡 Medium | 🔴 Critical | Use automotive-grade twisted pair, shielded, with locking connectors. Test continuity before each phase. |
| **STM32 firmware bug** | 🟠 High | 🔴 Critical | Extensive shadow mode testing (Phase 1). Dual-write validation (Phase 2). Watchdog protection. |
| **I2C bus conflict (Phase 2)** | 🟡 Medium | 🟠 High | Monitor I2C with logic analyzer. Use arbitration if needed. Implement retry logic. |
| **CAN timeout false positives** | 🟡 Medium | 🟡 Medium | Tune timeout values (100ms initial, adjust based on testing). Add hysteresis (require N consecutive misses). |
| **Increased latency** | 🟢 Low | 🟡 Medium | Measure latency at each phase. Ensure <20ms budget. Optimize CAN message packing. |
| **Component failure (STM32)** | 🟢 Low | 🔴 Critical | Use automotive-grade components. Add spare STM32 board. Implement fail-safe to ESP32 I2C fallback (future). |

### Rollback Triggers

**Automatic Rollback (to previous phase):**
- CAN error rate >1% (bus instability)
- Watchdog reset loop (3+ consecutive resets)
- Critical sensor data mismatch (e.g., wheel speeds off by >10%)

**Manual Rollback (operator decision):**
- Vehicle behavior anomaly (jerky movements, unexpected stops)
- Failed validation test (see success criteria per phase)
- User discomfort or safety concern

---

## TESTING STRATEGY

### Test Levels

**1. Unit Tests (Per-Phase)**
- CAN driver (send/receive loopback)
- Message encoding/decoding
- Timeout detection
- Fail-safe state machine

**2. Integration Tests (Per-Phase)**
- ESP32 ↔ STM32 communication
- Shadow/mirror/authority validation
- Sensor data comparison
- Actuator output comparison

**3. System Tests (End-to-End)**
- Normal driving scenarios (pedal, steering, gear changes)
- Fail-safe scenarios (CAN timeout, over-current, emergency stop)
- Long-duration stability (1+ hour operation)
- Environmental stress (temperature, vibration)

**4. Acceptance Tests (Final Validation)**
- Real-world driving (urban, highway, off-road)
- Driver feedback (comfort, responsiveness)
- Safety validation (emergency stop, obstacle avoidance)
- Regulatory compliance (if applicable)

---

## SUCCESS METRICS

### Phase 1: Shadow CAN
- ✅ CAN bus operational (TX/RX with <1% loss)
- ✅ Sensor data matches (ESP32 I2C vs. STM32 local, <5% error)
- ✅ No performance degradation (loop frequency unchanged)

### Phase 2: Mirror Control
- ✅ Dual-write works (ESP32 and STM32 write identical PWM)
- ✅ No I2C conflicts (both masters coexist)
- ✅ CAN latency <20ms

### Phase 3: Authority Transfer
- ✅ CAN-only control works (ESP32 I2C disabled)
- ✅ Fail-safe triggers within 100ms (CAN timeout)
- ✅ Local protections work (over-current, thermal)

### Phase 4: Safety Enforcement
- ✅ 1+ hour stable operation (zero CAN errors)
- ✅ All fault injection tests pass
- ✅ Code passes static analysis (zero critical issues)

---

## TIMELINE ESTIMATE

| Phase | Duration | Dependencies | Milestones |
|-------|----------|--------------|------------|
| **Phase 0** | 1-2 days | None | Baseline established |
| **Phase 1** | 1-2 weeks | Phase 0 | Shadow CAN operational |
| **Phase 2** | 2-3 weeks | Phase 1 | Mirror control validated |
| **Phase 3** | 2-3 weeks | Phase 2 | Authority transfer complete |
| **Phase 4** | 2-4 weeks | Phase 3 | Production release |
| **Total** | **8-13 weeks** | - | **v3.0.0-can-production** |

**Aggressive Timeline:** 8 weeks (assumes no major issues)  
**Conservative Timeline:** 13 weeks (includes re-testing, debugging)  
**Realistic Timeline:** **10-12 weeks** (2.5-3 months)

---

## CONCLUSION

This phased migration plan ensures a **safe, reversible transition** from monolithic ESP32 to multi-ECU CAN architecture. Each phase is independently testable, and the vehicle **never loses operability** during development.

**Key Success Factors:**
- ✅ Shadow mode prevents authority transfer before validation
- ✅ Mirror mode allows dual-write comparison (catch bugs early)
- ✅ Compile-time flags enable instant rollback
- ✅ Fail-safe mechanisms protect against CAN bus failures
- ✅ Extensive testing at each phase ensures safety

**Next Actions:**
1. Review and approve this migration plan
2. Execute Phase 0 (baseline and hardware prep)
3. Begin Phase 1 (shadow CAN infrastructure)

**Estimated Timeline:** 10-12 weeks to production-ready multi-ECU system.

---

**Document Authority:** Automotive Systems Architect  
**Review Status:** ✅ Migration roadmap complete  
**Confidentiality:** Internal Use Only
