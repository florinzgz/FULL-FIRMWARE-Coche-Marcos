# MIGRATION PHASES

**Document Version:** 1.0  
**Firmware Version:** v2.17.1 (PHASE 14) - Starting Point  
**Target Architecture:** Distributed Multi-ECU with CAN Bus  
**Migration Strategy:** Phased, Non-Breaking, Reversible  
**Date:** 2026-01-13

---

## EXECUTIVE SUMMARY

This document outlines a safe, phased migration plan from the current monolithic ESP32-S3 architecture to a distributed multi-ECU system. The migration follows the principle: **"Never brick the vehicle"**. Each phase is independently testable, reversible, and maintains vehicle operability.

**Key Principles:**
1. ✅ **Shadow Mode First:** New systems operate in parallel, validate before switching
2. ✅ **Gradual Authority Transfer:** ESP32 remains in control until STM32 proven
3. ✅ **Always Drivable:** Vehicle can drive at every phase (even if degraded)
4. ✅ **Rollback Capability:** Can revert to previous phase at any time
5. ✅ **Validation Gates:** Strict criteria before proceeding to next phase

**Timeline Estimate:** 6-9 months for complete migration

---

## PHASE 0: PREPARATION & INFRASTRUCTURE

**Duration:** 2-3 weeks  
**Risk:** LOW  
**Rollback:** N/A (no changes to vehicle)

### 0.1 Objectives

- Set up CAN development environment
- Acquire hardware for testing
- Create CAN message protocol library
- Establish testing infrastructure

### 0.2 Hardware Procurement

**Purchase List:**

1. **ESP32 CAN Adapter:**
   - 1x MCP2515 CAN module (SPI)
   - 1x SN65HVD230 CAN transceiver breakout
   - Wiring: Dupont cables, breadboard

2. **STM32 Development Boards:**
   - 1x STM32F407VGT6 dev board (Motor ECU prototype)
   - 1x STM32F103RCT6 dev board (Sensor ECU prototype)
   - 1x STM32G071KBU6 dev board (Lighting ECU, optional)

3. **CAN Bus Hardware:**
   - 10m CAN bus cable (twisted pair, shielded)
   - 2x 120Ω termination resistors
   - CAN bus analyzer (e.g., PCAN-USB or CANable)

4. **Test Equipment:**
   - Logic analyzer (for debugging, e.g., Saleae)
   - Oscilloscope (CAN signal validation)
   - Bench power supply (24V, 10A)

**Total Cost Estimate:** ~$150-200 USD

---

### 0.3 Software Development Environment

**ESP32-S3:**
- ✅ Existing: PlatformIO + ESP-IDF
- ➕ Add: MCP2515 CAN library (e.g., `autowp/arduino-mcp2515`)
- ➕ Add: CAN message codec library (custom)

**STM32:**
- ➕ Install: STM32CubeIDE or PlatformIO + STM32 framework
- ➕ Install: STM32 HAL libraries (for CAN, I2C, GPIO, timers)
- ➕ Add: CAN message codec library (shared with ESP32)

**CAN Tools:**
- ➕ Install: PCAN-View or Savvycan (CAN bus monitor)
- ➕ Install: can-utils (Linux CLI tools for testing)

---

### 0.4 CAN Message Protocol Library

**Create shared library `canbus_protocol`:**

**Files:**
```
canbus_protocol/
├── can_ids.h             // CAN ID definitions
├── can_messages.h        // Message structures
├── can_encode.c/h        // Pack data into CAN frames
├── can_decode.c/h        // Unpack CAN frames
└── can_checksum.c/h      // Optional: CRC for data integrity
```

**Example Message Definition:**

```c
// can_messages.h
#pragma once
#include <stdint.h>

// Traction command message (ESP32 → Motor ECU)
typedef struct {
  uint8_t throttle_fl;  // 0-200 (percent × 2)
  uint8_t throttle_fr;
  uint8_t throttle_rl;
  uint8_t throttle_rr;
  uint8_t brake_enable; // 0=off, 1=on
  uint8_t direction;    // 0=FWD, 1=REV, 2=NEUTRAL
} TractionCommand_t;

#define CAN_ID_TRACTION_CMD  0x010
#define CAN_DLC_TRACTION_CMD 6

// Encode function
void CAN_EncodeTractionCommand(const TractionCommand_t *cmd, uint8_t *data);

// Decode function
void CAN_DecodeTractionCommand(const uint8_t *data, TractionCommand_t *cmd);
```

**Example Implementation:**

```c
// can_encode.c
void CAN_EncodeTractionCommand(const TractionCommand_t *cmd, uint8_t *data) {
  data[0] = cmd->throttle_fl;
  data[1] = cmd->throttle_fr;
  data[2] = cmd->throttle_rl;
  data[3] = cmd->throttle_rr;
  data[4] = cmd->brake_enable;
  data[5] = cmd->direction;
}

void CAN_DecodeTractionCommand(const uint8_t *data, TractionCommand_t *cmd) {
  cmd->throttle_fl = data[0];
  cmd->throttle_fr = data[1];
  cmd->throttle_rl = data[2];
  cmd->throttle_rr = data[3];
  cmd->brake_enable = data[4];
  cmd->direction = data[5];
}
```

**Build for both platforms:**
- Compile into ESP32 firmware (PlatformIO library)
- Compile into STM32 firmware (HAL integration)

---

### 0.5 Bench Testing Setup

**Create test bench:**

```
┌──────────────┐    CAN Bus     ┌──────────────┐
│   ESP32-S3   │◄──────────────►│  STM32F407   │
│  (MCP2515)   │   500 kbps     │  (bxCAN)     │
└──────┬───────┘                └──────┬───────┘
       │                               │
       │ USB Serial                    │ ST-Link
       │ (monitor)                     │ (debug)
       ▼                               ▼
  ┌─────────┐                    ┌─────────┐
  │   PC    │◄───────────────────┤   PC    │
  │ Monitor │    (optional)      │  IDE    │
  └─────────┘                    └─────────┘
       ▲
       │
   ┌───┴────┐
   │ PCAN   │ (CAN analyzer)
   │  USB   │
   └────────┘
```

**Test Scenarios:**
1. ESP32 transmit → STM32 receive
2. STM32 transmit → ESP32 receive
3. Heartbeat exchange (10 Hz)
4. Message flood test (100 Hz sustained)
5. Bus-off recovery test

**Acceptance Criteria:**
- ✅ CAN bus operational at 500 kbps
- ✅ <1% message loss at 100 Hz
- ✅ Latency <3ms worst-case
- ✅ Clean CAN signals on oscilloscope
- ✅ Automatic bus recovery after fault

---

### 0.6 Documentation & Planning

**Create:**
1. ✅ CAN message specification (complete, in TARGET_ECU_ARCHITECTURE.md)
2. ✅ Wiring diagram (ESP32 + STM32 + CAN bus)
3. ✅ Test procedures (bench tests)
4. ✅ Risk assessment (per phase)
5. ✅ Rollback procedures (per phase)

**Review:**
- Architecture documents (this document + TARGET_ECU_ARCHITECTURE.md)
- Current system documentation (CURRENT_ARCHITECTURE_MAP.md)

---

### 0.7 Phase 0 Completion Criteria

**Checklist:**
- [ ] All hardware procured and tested
- [ ] CAN protocol library implemented and compiled
- [ ] Bench test setup functional
- [ ] CAN bus operational at 500 kbps
- [ ] Message exchange validated (ESP32 ↔ STM32)
- [ ] Documentation complete
- [ ] Team trained on CAN tools
- [ ] Risk assessment reviewed and approved

**Exit Gate:** Sign-off from lead engineer before proceeding to Phase 1.

---

## PHASE 1: SHADOW CAN - ESP32 RETAINS FULL AUTHORITY

**Duration:** 3-4 weeks  
**Risk:** LOW  
**Rollback:** Simple (disable CAN, revert to v2.17.1)

### 1.1 Objectives

- Install CAN hardware on vehicle (ESP32 + STM32 Motor ECU)
- ESP32 continues to control all motors (existing I2C path)
- ESP32 transmits shadow CAN commands (parallel to I2C)
- STM32 Motor ECU receives CAN, validates, but does NOT control motors
- STM32 echoes back what it WOULD do (shadow feedback)
- Validate CAN communication in real vehicle environment

**Key Point:** Vehicle operates normally. CAN is passive observer.

---

### 1.2 Hardware Installation

**Add to Vehicle:**

1. **STM32F407 Motor ECU:**
   - Mount: Near existing motor controllers (minimize wiring)
   - Power: Tap into 5V rail (or dedicated 12V → 5V buck)
   - Wiring: See table below

2. **CAN Bus Wiring:**
   - Run twisted pair from ESP32 to STM32 (~30cm)
   - Add 120Ω termination at both ends
   - Shield grounded at one point

3. **Connections:**

| Device | CAN_H | CAN_L | GND | VCC |
|--------|-------|-------|-----|-----|
| ESP32 (MCP2515) | → Bus | → Bus | Common GND | 3.3V |
| STM32F407 (bxCAN) | → Bus | → Bus | Common GND | 5V |

**Note:** Do NOT connect STM32 to I2C bus yet. Motors remain ESP32-controlled.

---

### 1.3 ESP32 Firmware Changes

**File:** `src/control/traction.cpp`

**Add:**
```cpp
// At top of file
#include "canbus.h"  // New CAN interface
extern CANBus canBus;

// In Traction::update() function, AFTER applying PWM via I2C:
void Traction::update() {
  // ... existing code (pedal read, calculation, etc.) ...
  
  // Existing I2C motor control (UNCHANGED)
  for (int i = 0; i < 4; i++) {
    applyHardwareControl(i, s.pwm[i], s.reverse[i]);
  }
  
  // ✅ NEW: Shadow CAN transmission
  TractionCommand_t cmd;
  cmd.throttle_fl = s.pwm[FL] / 2;  // Convert 0-200% to 0-100%
  cmd.throttle_fr = s.pwm[FR] / 2;
  cmd.throttle_rl = s.pwm[RL] / 2;
  cmd.throttle_rr = s.pwm[RR] / 2;
  cmd.brake_enable = s.brakeActive ? 1 : 0;
  cmd.direction = s.reverse[0] ? 1 : 0;  // Assume all same direction
  
  canBus.sendTractionCommand(&cmd);  // Non-blocking, fire-and-forget
  
  // Note: No waiting for response in Phase 1
}
```

**File:** `src/control/steering_motor.cpp`

**Add:**
```cpp
// Similar shadow CAN for steering
void SteeringMotor::update() {
  // ... existing PID control, I2C PWM output ...
  
  // ✅ NEW: Shadow CAN
  SteeringCommand_t cmd;
  cmd.target_position = targetPosition;
  cmd.max_speed = maxSpeed;
  canBus.sendSteeringCommand(&cmd);
}
```

**File:** `src/core/canbus.cpp` (NEW)

**Implement:**
```cpp
#include "canbus.h"
#include <MCP2515.h>  // Library for MCP2515
#include <SPI.h>

MCP2515 mcp2515(PIN_CAN_CS);  // CS pin for MCP2515

void CANBus::init() {
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);  // 500 kbps, 8 MHz crystal
  mcp2515.setNormalMode();
  Logger::info("CAN bus initialized @ 500 kbps");
}

void CANBus::sendTractionCommand(const TractionCommand_t *cmd) {
  can_frame frame;
  frame.can_id = CAN_ID_TRACTION_CMD;
  frame.can_dlc = CAN_DLC_TRACTION_CMD;
  CAN_EncodeTractionCommand(cmd, frame.data);
  
  if (mcp2515.sendMessage(&frame) != MCP2515::ERROR_OK) {
    Logger::warn("CAN TX fail: Traction command");
  }
}

// Similar for steering, heartbeat, etc.
```

---

### 1.4 STM32 Motor ECU Firmware (Phase 1)

**Responsibilities:**
- Receive CAN messages from ESP32
- Validate data
- Log what it WOULD do
- Send shadow feedback (what motors WOULD receive)
- Do NOT control real hardware yet

**Main Loop:**
```c
void main(void) {
  HAL_Init();
  SystemClock_Config();
  CAN_Init();  // Initialize bxCAN peripheral
  UART_Init(); // For debugging
  
  while(1) {
    // 1. Check CAN RX
    if (CAN_MessageAvailable()) {
      ProcessCANMessage();
    }
    
    // 2. Send shadow feedback (what we WOULD output)
    SendShadowFeedback();
    
    // 3. Monitor heartbeat
    CheckHeartbeat();
    
    // 4. Delay
    HAL_Delay(10);  // 100 Hz loop
  }
}

void ProcessCANMessage() {
  CAN_RxHeaderTypeDef rxHeader;
  uint8_t rxData[8];
  
  if (HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &rxHeader, rxData) == HAL_OK) {
    switch(rxHeader.StdId) {
      case CAN_ID_TRACTION_CMD:
        TractionCommand_t cmd;
        CAN_DecodeTractionCommand(rxData, &cmd);
        
        // Validate
        if (cmd.throttle_fl > 200) {
          UART_Print("ERROR: Invalid throttle FL\n");
          return;
        }
        
        // Log what we WOULD do
        UART_Printf("Shadow: FL=%d FR=%d RL=%d RR=%d\n",
                    cmd.throttle_fl, cmd.throttle_fr,
                    cmd.throttle_rl, cmd.throttle_rr);
        
        // Store for feedback
        g_shadowTraction = cmd;
        break;
        
      case CAN_ID_STEERING_CMD:
        // Similar
        break;
        
      case CAN_ID_HEARTBEAT_ESP32:
        g_lastHeartbeat = HAL_GetTick();
        break;
    }
  }
}

void SendShadowFeedback() {
  static uint32_t lastSend = 0;
  if (HAL_GetTick() - lastSend < 10) return;  // 100 Hz rate limit
  lastSend = HAL_GetTick();
  
  // Send what motors WOULD see (calculated from shadow command)
  MotorFeedback_t feedback;
  feedback.current_fl = 0;  // Placeholder (no real hardware)
  feedback.current_fr = 0;
  feedback.current_rl = 0;
  feedback.current_rr = 0;
  feedback.steering_current = 0;
  feedback.steering_position = 0;
  feedback.status = 0x01;  // Motors enabled (shadow)
  
  CAN_TxHeaderTypeDef txHeader;
  uint8_t txData[8];
  txHeader.StdId = CAN_ID_MOTOR_FEEDBACK;
  txHeader.DLC = 8;
  txHeader.RTR = CAN_RTR_DATA;
  txHeader.IDE = CAN_ID_STD;
  
  CAN_EncodeMotorFeedback(&feedback, txData);
  
  uint32_t txMailbox;
  HAL_CAN_AddTxMessage(&hcan1, &txHeader, txData, &txMailbox);
}
```

---

### 1.5 Testing & Validation

**Bench Test (Before Vehicle):**

1. **CAN Communication:**
   - ESP32 sends 100 Hz traction commands
   - STM32 receives and logs all messages
   - STM32 sends 100 Hz shadow feedback
   - ESP32 receives and logs feedback
   - **Acceptance:** <1% message loss, <3ms latency

2. **Heartbeat:**
   - ESP32 sends 10 Hz heartbeat
   - STM32 monitors, timeout detection (100ms)
   - **Acceptance:** STM32 detects timeout within 150ms

3. **Endurance:**
   - Run for 1 hour continuous
   - **Acceptance:** No CAN errors, no resets

**Vehicle Test:**

1. **Install Hardware:**
   - Mount STM32 Motor ECU
   - Wire CAN bus
   - Power up system

2. **Drive Test:**
   - Drive normally (ESP32 controls via I2C, as before)
   - Monitor CAN bus (PCAN USB logger)
   - Observe STM32 UART output (shadow logs)
   - **Acceptance:**
     - Vehicle drives normally (no change in behavior)
     - CAN messages transmitted consistently
     - STM32 receives and logs all commands
     - No CAN bus errors

3. **Stress Test:**
   - Hard acceleration, braking, steering
   - Emergency stop test
   - **Acceptance:**
     - CAN keeps up with rapid changes
     - No message loss during transients

---

### 1.6 Validation Metrics

**Collect Data:**
- CAN message count (per message type)
- CAN error count (TX fail, RX overflow)
- Latency measurements (timestamp comparison)
- Bus utilization (%)
- Shadow vs. actual comparison (pedal → throttle → CAN)

**Analysis:**
- Generate plots: CAN traffic over time
- Compare shadow commands to actual I2C outputs
- Identify any discrepancies

**Acceptance Criteria:**
- [ ] CAN operational for 10+ hours continuous driving
- [ ] Message loss <0.1%
- [ ] Latency <5ms (90th percentile)
- [ ] Bus utilization <20%
- [ ] Shadow commands match actual motor control ±5%
- [ ] No CAN-induced system crashes
- [ ] Vehicle behavior unchanged (driver perception)

---

### 1.7 Phase 1 Completion Criteria

**Checklist:**
- [ ] STM32 Motor ECU installed on vehicle
- [ ] CAN bus wired and operational
- [ ] ESP32 transmits shadow CAN commands
- [ ] STM32 receives and validates commands
- [ ] STM32 sends shadow feedback
- [ ] Validation metrics meet acceptance criteria
- [ ] 10+ hours of shadow operation logged
- [ ] No CAN-related issues observed
- [ ] Rollback procedure tested and documented

**Exit Gate:** Management approval to proceed to Phase 2.

---

## PHASE 2: MIRROR CONTROL - DUAL OPERATION

**Duration:** 3-4 weeks  
**Risk:** MEDIUM  
**Rollback:** Medium (disable STM32 outputs, revert to ESP32 I2C)

### 2.1 Objectives

- STM32 Motor ECU now controls real hardware (I2C → PCA9685, MCP23017)
- ESP32 ALSO continues to control via I2C (parallel/redundant)
- Both ESP32 and STM32 drive the same motors simultaneously
- Compare outputs: ESP32 I2C vs. STM32 I2C
- Validate STM32 control accuracy before giving it authority

**Key Point:** Motors receive commands from BOTH controllers. Must agree.

---

### 2.2 Hardware Changes

**Connect STM32 to Motor I2C Bus:**

**Wiring:**
```
          I2C Bus (SDA=GPIO8, SCL=GPIO9)
                    │
        ┌───────────┴───────────┐
        │                       │
   ┌────▼────┐            ┌─────▼─────┐
   │  ESP32  │            │  STM32F4  │
   │  (SDA8) │            │  (PB7)    │
   │  (SCL9) │            │  (PB6)    │
   └─────────┘            └───────────┘
        │                       │
        └───────────┬───────────┘
                    │
         ┌──────────┴──────────┐
         │  I2C Devices:       │
         │  • PCA9685 @ 0x40   │
         │  • PCA9685 @ 0x41   │
         │  • PCA9685 @ 0x42   │
         │  • MCP23017 @ 0x20  │
         └─────────────────────┘
```

**⚠️ CRITICAL: I2C Multi-Master Arbitration**

I2C supports multi-master, BUT:
- ESP32 and STM32 must NOT transmit simultaneously
- Use time-slicing: ESP32 transmits even 10ms cycles, STM32 odd cycles
- OR: Use I2C mutex (shared via CAN message)

**Recommended: Time-Slicing (Simpler)**

**ESP32 Timing:**
```
t=0ms:   ESP32 updates I2C (PCA9685, MCP23017)
t=5ms:   ESP32 sends CAN command
t=10ms:  ESP32 updates I2C again
t=15ms:  ESP32 sends CAN command
...
```

**STM32 Timing:**
```
t=0ms:   Wait (ESP32 turn)
t=5ms:   STM32 updates I2C (PCA9685, MCP23017)
t=10ms:  Wait
t=15ms:  STM32 updates I2C
...
```

**Result:** Each controller updates I2C at 50 Hz (interleaved)

---

### 2.3 ESP32 Firmware Changes

**File:** `src/control/traction.cpp`

**Modify:**
```cpp
void Traction::update() {
  static uint32_t lastUpdate = 0;
  uint32_t now = millis();
  
  // ... pedal read, calculation ...
  
  // ✅ PHASE 2: Time-sliced I2C update
  // Only update I2C on even 10ms cycles
  if ((now / 10) % 2 == 0) {  // Even cycles
    // Apply I2C control (existing code)
    for (int i = 0; i < 4; i++) {
      applyHardwareControl(i, s.pwm[i], s.reverse[i]);
    }
  }
  
  // Always send CAN (shadow command for STM32)
  canBus.sendTractionCommand(&cmd);
  
  // Optional: Receive STM32 feedback, compare
  if (canBus.hasMotorFeedback()) {
    MotorFeedback_t feedback;
    canBus.getMotorFeedback(&feedback);
    
    // Compare STM32 output to our intent
    compareFeedback(&feedback);
  }
}

void compareFeedback(const MotorFeedback_t *feedback) {
  // Log discrepancies between ESP32 intent and STM32 execution
  int diff_fl = abs(s.pwm[FL] - feedback->actual_pwm_fl);
  if (diff_fl > 10) {  // >5% difference
    Logger::warnf("Mirror: FL mismatch ESP32=%d STM32=%d", 
                  s.pwm[FL], feedback->actual_pwm_fl);
  }
  // Similar for FR, RL, RR, steering
}
```

---

### 2.4 STM32 Motor ECU Firmware (Phase 2)

**Add:**
- I2C peripheral init (HAL_I2C_Init)
- PCA9685 driver library
- MCP23017 driver library
- Real PWM output to motors

**Main Loop:**
```c
void main(void) {
  HAL_Init();
  SystemClock_Config();
  CAN_Init();
  I2C_Init();          // ✅ NEW
  PCA9685_Init();      // ✅ NEW
  MCP23017_Init();     // ✅ NEW
  
  while(1) {
    uint32_t now = HAL_GetTick();
    
    // 1. Check CAN RX (ESP32 commands)
    if (CAN_MessageAvailable()) {
      ProcessCANMessage();
    }
    
    // 2. ✅ PHASE 2: Apply motor control (on odd 10ms cycles)
    if ((now / 10) % 2 == 1) {  // Odd cycles
      ApplyMotorControl();
    }
    
    // 3. Send feedback (actual outputs)
    SendMotorFeedback();
    
    // 4. Monitor heartbeat
    CheckHeartbeat();
    
    HAL_Delay(10);
  }
}

void ApplyMotorControl() {
  // Use latest CAN command from ESP32
  TractionCommand_t *cmd = &g_lastTractionCmd;
  
  // Convert to PWM ticks (0-200% → 0-4095)
  uint16_t pwm_fl = (cmd->throttle_fl * 4095) / 200;
  uint16_t pwm_fr = (cmd->throttle_fr * 4095) / 200;
  uint16_t pwm_rl = (cmd->throttle_rl * 4095) / 200;
  uint16_t pwm_rr = (cmd->throttle_rr * 4095) / 200;
  
  // Apply via PCA9685
  PCA9685_SetPWM(PCA_FRONT, PCA_FRONT_CH_FL_FWD, pwm_fl);
  PCA9685_SetPWM(PCA_FRONT, PCA_FRONT_CH_FR_FWD, pwm_fr);
  PCA9685_SetPWM(PCA_REAR, PCA_REAR_CH_RL_FWD, pwm_rl);
  PCA9685_SetPWM(PCA_REAR, PCA_REAR_CH_RR_FWD, pwm_rr);
  
  // Direction via MCP23017
  MCP23017_WritePin(MCP_PIN_FL_IN1, cmd->direction == 0 ? 1 : 0);
  MCP23017_WritePin(MCP_PIN_FL_IN2, cmd->direction == 1 ? 1 : 0);
  // ... similar for FR, RL, RR
  
  // Log actual outputs for comparison
  g_actualPWM_FL = pwm_fl;
  g_actualPWM_FR = pwm_fr;
  g_actualPWM_RL = pwm_rl;
  g_actualPWM_RR = pwm_rr;
}

void SendMotorFeedback() {
  MotorFeedback_t feedback;
  feedback.actual_pwm_fl = g_actualPWM_FL;
  feedback.actual_pwm_fr = g_actualPWM_FR;
  feedback.actual_pwm_rl = g_actualPWM_RL;
  feedback.actual_pwm_rr = g_actualPWM_RR;
  // ... pack and send via CAN
}
```

---

### 2.5 Testing & Validation

**Bench Test:**

1. **I2C Arbitration:**
   - Run ESP32 and STM32 simultaneously
   - Monitor I2C bus with logic analyzer
   - **Acceptance:** No bus collisions, clean waveforms

2. **Output Comparison:**
   - Drive motors via both controllers
   - Measure PWM output with oscilloscope
   - **Acceptance:** ESP32 and STM32 PWM agree within ±2%

**Vehicle Test:**

1. **Static Test (Vehicle on Jacks):**
   - Raise vehicle (wheels off ground)
   - Apply throttle (slowly)
   - Observe motor response
   - Monitor CAN logs + ESP32 comparison logs
   - **Acceptance:**
     - Motors respond to throttle
     - No jerking or oscillation
     - ESP32 logs show agreement with STM32 outputs

2. **Dynamic Test (Driving):**
   - Drive on flat surface (low speed, <10 km/h)
   - Gradual acceleration, braking, turning
   - Monitor for anomalies
   - **Acceptance:**
     - Vehicle drives smoothly
     - No unexpected motor behavior
     - ESP32 comparison logs show <5% discrepancy

3. **Stress Test:**
   - Hard acceleration
   - Emergency stop
   - Rapid steering
   - **Acceptance:**
     - Vehicle responds correctly
     - No I2C bus errors
     - No motor glitches

---

### 2.6 Validation Metrics

**Collect:**
- PWM output measurements (ESP32 vs STM32)
- I2C bus utilization
- Motor current (via INA226, if available)
- Driver subjective feedback (does it feel the same?)

**Analysis:**
- Plot ESP32 vs STM32 outputs over time
- Calculate correlation (should be >0.99)
- Identify any divergence

**Acceptance Criteria:**
- [ ] I2C arbitration successful (no collisions)
- [ ] PWM outputs agree within ±2% (ESP32 vs STM32)
- [ ] Vehicle drives identically to Phase 1
- [ ] No subjective difference (driver blind test)
- [ ] No I2C errors logged
- [ ] 5+ hours of mirror operation without issues

---

### 2.7 Phase 2 Completion Criteria

**Checklist:**
- [ ] STM32 connected to motor I2C bus
- [ ] Time-sliced I2C updates operational
- [ ] ESP32 and STM32 both control motors
- [ ] Output comparison validates agreement
- [ ] Vehicle drives smoothly in mirror mode
- [ ] Validation metrics meet acceptance criteria
- [ ] Rollback procedure tested

**Exit Gate:** Approval to proceed to Phase 3 (authority transfer).

---

## PHASE 3: AUTHORITY TRANSFER - STM32 TAKES CONTROL

**Duration:** 2-3 weeks  
**Risk:** HIGH  
**Rollback:** High (requires ESP32 to retake I2C control)

### 3.1 Objectives

- Disable ESP32 I2C motor control
- STM32 Motor ECU becomes sole authority
- ESP32 sends high-level commands via CAN only
- STM32 handles all low-level motor control (I2C, PWM, timing)
- Validate stability and safety

**Key Point:** ESP32 gives up direct motor access. STM32 is in charge.

---

### 3.2 ESP32 Firmware Changes

**File:** `src/control/traction.cpp`

**Modify:**
```cpp
// PHASE 3: Disable ESP32 I2C motor control
#define MOTOR_CONTROL_VIA_CAN  1  // Feature flag

void Traction::update() {
  // ... pedal read, calculation ...
  
#if MOTOR_CONTROL_VIA_CAN
  // PHASE 3: CAN-only control
  TractionCommand_t cmd;
  cmd.throttle_fl = s.pwm[FL] / 2;
  cmd.throttle_fr = s.pwm[FR] / 2;
  cmd.throttle_rl = s.pwm[RL] / 2;
  cmd.throttle_rr = s.pwm[RR] / 2;
  cmd.brake_enable = s.brakeActive ? 1 : 0;
  cmd.direction = s.reverse[0] ? 1 : 0;
  
  if (!canBus.sendTractionCommand(&cmd)) {
    Logger::error("CAN TX fail - CRITICAL");
    // ⚠️ FAILSAFE: Revert to I2C?
    // OR: Enter safe mode, stop vehicle
    enterSafeMode();
  }
  
  // Receive feedback from STM32 (for telemetry/HUD)
  if (canBus.hasMotorFeedback()) {
    MotorFeedback_t feedback;
    canBus.getMotorFeedback(&feedback);
    updateTelemetry(&feedback);
  }
  
#else
  // PHASE 1/2: Direct I2C control (fallback)
  for (int i = 0; i < 4; i++) {
    applyHardwareControl(i, s.pwm[i], s.reverse[i]);
  }
#endif
}
```

**File:** `src/core/canbus.cpp`

**Add Error Handling:**
```cpp
bool CANBus::sendTractionCommand(const TractionCommand_t *cmd) {
  can_frame frame;
  frame.can_id = CAN_ID_TRACTION_CMD;
  frame.can_dlc = CAN_DLC_TRACTION_CMD;
  CAN_EncodeTractionCommand(cmd, frame.data);
  
  MCP2515::ERROR result = mcp2515.sendMessage(&frame);
  
  if (result != MCP2515::ERROR_OK) {
    Logger::errorf("CAN TX fail: code %d", result);
    
    // Increment error counter
    g_canTxErrors++;
    
    // If too many errors, trigger recovery
    if (g_canTxErrors > 10) {
      Logger::error("CAN bus failure - attempting recovery");
      recoverCANBus();
    }
    
    return false;
  }
  
  // Success
  g_canTxErrors = 0;  // Reset on success
  return true;
}

void recoverCANBus() {
  Logger::warn("CAN bus recovery initiated");
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();
  delay(100);  // Allow bus to stabilize
  Logger::info("CAN bus recovery complete");
}
```

---

### 3.3 STM32 Motor ECU Firmware (Phase 3)

**No hardware changes needed.**

**Modify timing:**
- STM32 now updates I2C every 10ms (was 20ms in Phase 2)
- Full authority, no time-slicing

**Main Loop:**
```c
void main(void) {
  // ... init ...
  
  while(1) {
    uint32_t now = HAL_GetTick();
    
    // 1. Check CAN RX (ESP32 commands)
    if (CAN_MessageAvailable()) {
      ProcessCANMessage();
      g_lastCommandTime = now;  // Heartbeat tracking
    }
    
    // 2. Heartbeat timeout check
    if (now - g_lastCommandTime > 100) {
      // ESP32 heartbeat lost - FAILSAFE
      ActivateFailsafe();
    } else {
      // Normal operation
      // 3. Apply motor control (every 10ms)
      ApplyMotorControl();
    }
    
    // 4. Send feedback
    SendMotorFeedback();
    
    HAL_Delay(10);
  }
}

void ActivateFailsafe() {
  static bool failsafeActive = false;
  
  if (!failsafeActive) {
    Logger_UART("FAILSAFE: ESP32 heartbeat lost - stopping motors\n");
    failsafeActive = true;
    g_failsafeStartTime = HAL_GetTick();
  }
  
  // Gradual deceleration (5 seconds)
  uint32_t elapsed = HAL_GetTick() - g_failsafeStartTime;
  if (elapsed < 5000) {
    // Ramp down throttle over 5 seconds
    float rampDown = 1.0f - (float)elapsed / 5000.0f;
    g_lastTractionCmd.throttle_fl *= rampDown;
    g_lastTractionCmd.throttle_fr *= rampDown;
    g_lastTractionCmd.throttle_rl *= rampDown;
    g_lastTractionCmd.throttle_rr *= rampDown;
    ApplyMotorControl();
  } else {
    // After 5 seconds, motors off
    StopAllMotors();
    CenterSteering();
  }
  
  // Flash status LED rapidly
  HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
}

void StopAllMotors() {
  // Set all PWM to 0
  PCA9685_SetPWM(PCA_FRONT, PCA_FRONT_CH_FL_FWD, 0);
  PCA9685_SetPWM(PCA_FRONT, PCA_FRONT_CH_FL_REV, 0);
  // ... all channels to 0
}

void CenterSteering() {
  // PID target = 0 (center position)
  g_steeringTarget = 0;
  // Apply centering PWM (gentle)
  SteeringPID_Update();
}
```

---

### 3.4 Testing & Validation

**Bench Test:**

1. **Failsafe Test:**
   - Disconnect CAN bus (simulate failure)
   - **Acceptance:** STM32 enters failsafe within 100ms, motors ramp down

2. **Recovery Test:**
   - Reconnect CAN bus after failsafe
   - **Acceptance:** STM32 resumes normal operation

**Vehicle Test:**

1. **Normal Operation:**
   - Drive normally (all speeds, maneuvers)
   - **Acceptance:** Identical behavior to Phase 2

2. **Failsafe Test (CRITICAL):**
   - While driving slowly (<5 km/h), disconnect ESP32 power
   - **Expected:**
     - STM32 detects heartbeat loss
     - Vehicle decelerates smoothly (5 seconds)
     - Motors stop
     - Steering centers
   - **Acceptance:** Vehicle stops safely, no jerking

3. **Stress Test:**
   - Hard acceleration, braking, steering
   - Emergency stop
   - **Acceptance:** Vehicle responds correctly, no issues

4. **Endurance:**
   - Drive for 2+ hours
   - **Acceptance:** No CAN errors, no failsafes triggered

---

### 3.5 Validation Metrics

**Collect:**
- CAN message statistics (TX success rate, latency)
- Failsafe activation count (should be 0 in normal operation)
- Driver feedback (subjective comparison to original)

**Acceptance Criteria:**
- [ ] CAN TX success rate >99.9%
- [ ] Latency <3ms (95th percentile)
- [ ] Failsafe test passes (smooth stop)
- [ ] Vehicle drives identically to Phase 2
- [ ] 10+ hours of CAN-controlled operation
- [ ] No unintended failsafe activations

---

### 3.6 Phase 3 Completion Criteria

**Checklist:**
- [ ] ESP32 I2C motor control disabled
- [ ] STM32 sole authority for motor control
- [ ] CAN communication robust (>99.9% success)
- [ ] Failsafe mechanism validated
- [ ] Vehicle drives normally under CAN control
- [ ] Endurance test passed
- [ ] Rollback procedure ready (revert to Phase 2)

**Exit Gate:** Final approval before Phase 4 (safety enforcement).

---

## PHASE 4: SAFETY ENFORCEMENT - PRODUCTION READY

**Duration:** 2-3 weeks  
**Risk:** LOW (refinement only)  
**Rollback:** Low (but why would you?)

### 4.1 Objectives

- Harden failsafe mechanisms
- Add redundancy (optional: dual CAN)
- Implement watchdog cross-checks
- Add comprehensive diagnostics
- Certify for production use

---

### 4.2 Safety Enhancements

**ESP32:**

1. **CAN Health Monitoring:**
   - Track message loss rate
   - Alert if >1% loss
   - Auto-recovery on bus errors

2. **STM32 Health Check:**
   - Monitor heartbeat sequence counter
   - Detect STM32 resets (counter discontinuity)
   - Alert driver if STM32 reboots

3. **User Interface:**
   - Display CAN status on HUD
   - Warning if degraded mode
   - Error codes for diagnostics

**STM32:**

1. **Enhanced Failsafe:**
   - Multiple timeout thresholds
   - Graceful degradation (reduce speed vs. full stop)
   - Configurable failsafe behavior

2. **Self-Diagnostics:**
   - I2C bus health check
   - PWM output validation
   - Encoder sanity checks

3. **Watchdog:**
   - Independent watchdog (IWDG) enabled
   - Fed by main loop (1 second timeout)
   - Reset on hang

---

### 4.3 Optional: Dual-Redundant CAN

**If budget allows:**

- Add second CAN bus (CAN2)
- Transmit critical messages on both buses
- Failover if CAN1 fails

**Implementation:**
- ESP32: 2x MCP2515 modules (different SPI CS pins)
- STM32: Use CAN2 peripheral (if available)
- Message routing: Critical messages (traction, steering) on both buses

**Cost:** +$20-30 hardware

---

### 4.4 Testing & Certification

**Final Testing:**

1. **Fault Injection:**
   - Disconnect CAN randomly during driving
   - Pull power from STM32
   - Inject bad messages
   - **Acceptance:** System handles all faults gracefully

2. **Long-Term Reliability:**
   - 50+ hours of operation
   - Various conditions (speed, terrain, weather)
   - **Acceptance:** No unexpected failures

3. **Performance Validation:**
   - Measure acceleration, braking, steering response
   - Compare to Phase 0 baseline
   - **Acceptance:** ≤5% performance change

4. **User Acceptance:**
   - Driver blind test (Phase 0 vs. Phase 4)
   - **Acceptance:** Cannot distinguish

---

### 4.5 Documentation

**Create Production Docs:**

1. **Wiring Diagram:** Complete system schematic
2. **CAN Database (DBC File):** For diagnostic tools
3. **Troubleshooting Guide:** Common issues and solutions
4. **Maintenance Manual:** How to service CAN system

---

### 4.6 Phase 4 Completion Criteria

**Checklist:**
- [ ] All safety enhancements implemented
- [ ] Fault injection tests passed
- [ ] Long-term reliability demonstrated
- [ ] Performance validation complete
- [ ] User acceptance test passed
- [ ] Production documentation complete
- [ ] System certified for unrestricted use

**Final Gate:** Production release approval.

---

## ROLLBACK PROCEDURES

### Rollback from Phase 1 to Phase 0

**Steps:**
1. Disable CAN transmission in ESP32 firmware (comment out canBus.send* calls)
2. Recompile and flash ESP32
3. Power cycle vehicle
4. Verify normal operation (I2C-only control)

**Time:** 10 minutes  
**Risk:** NONE

---

### Rollback from Phase 2 to Phase 1

**Steps:**
1. Disconnect STM32 from I2C bus (physically or disable in firmware)
2. Change ESP32 time-slicing back to full-time I2C updates
3. Recompile and flash ESP32
4. Power cycle vehicle
5. Verify normal operation

**Time:** 20 minutes  
**Risk:** LOW

---

### Rollback from Phase 3 to Phase 2

**Steps:**
1. Set `MOTOR_CONTROL_VIA_CAN = 0` in ESP32 firmware
2. Recompile and flash ESP32
3. Modify STM32 to time-slice again (50% duty)
4. Flash STM32
5. Power cycle vehicle
6. Verify mirror operation

**Time:** 30 minutes  
**Risk:** MEDIUM (requires both firmware updates)

---

### Rollback from Phase 4 to Phase 0 (Nuclear Option)

**Steps:**
1. Flash ESP32 with Phase 0 firmware (v2.17.1 original)
2. Disconnect STM32 ECU from vehicle (power + CAN + I2C)
3. Remove CAN hardware (optional)
4. Power cycle vehicle
5. Verify vehicle drives as original

**Time:** 1 hour  
**Risk:** LOW (back to known good state)

---

## RISK MITIGATION SUMMARY

**Critical Risks:**

| Risk | Phase | Likelihood | Impact | Mitigation |
|------|-------|------------|--------|------------|
| CAN bus failure | 3, 4 | LOW | HIGH | Failsafe stop, dual CAN option |
| STM32 crash | 3, 4 | LOW | HIGH | Watchdog reset, ESP32 detects |
| I2C collision | 2 | MEDIUM | MEDIUM | Time-slicing, tested |
| Message loss | 1-4 | LOW | MEDIUM | Retries, error counters |
| Wiring error | 1-4 | MEDIUM | HIGH | Pre-flight checks, bench tests |

**Mitigation Success Criteria:**
- All critical risks have multiple layers of protection
- Failsafes tested and validated
- Rollback procedures documented and practiced

---

## TIMELINE ESTIMATE

| Phase | Duration | Dependencies | Cumulative |
|-------|----------|--------------|------------|
| **Phase 0:** Preparation | 2-3 weeks | None | 3 weeks |
| **Phase 1:** Shadow CAN | 3-4 weeks | Phase 0 complete | 7 weeks |
| **Phase 2:** Mirror Control | 3-4 weeks | Phase 1 complete | 11 weeks |
| **Phase 3:** Authority Transfer | 2-3 weeks | Phase 2 complete | 14 weeks |
| **Phase 4:** Safety Enforcement | 2-3 weeks | Phase 3 complete | 17 weeks |

**Total: 17 weeks (~4 months) to 24 weeks (~6 months)**

**Realistic with buffer:** **6-9 months**

---

## SUCCESS CRITERIA

**Phase 1 Success:**
- ✅ CAN operational, shadow commands match actual
- ✅ 10+ hours logged without issues

**Phase 2 Success:**
- ✅ Dual control operational, outputs agree
- ✅ Vehicle drives identically

**Phase 3 Success:**
- ✅ STM32 sole authority, ESP32 via CAN only
- ✅ Failsafe validated, 10+ hours logged

**Phase 4 Success:**
- ✅ Production-grade safety, long-term reliability
- ✅ User acceptance, ready for unrestricted use

**Project Success:**
- ✅ Vehicle operates on distributed multi-ECU architecture
- ✅ Improved reliability, performance, scalability
- ✅ No degradation in user experience
- ✅ Full documentation and maintainability

---

## CONCLUSION

This phased migration plan ensures a safe, controlled transition from monolithic ESP32 to distributed multi-ECU architecture. By following the principle of **"shadow first, mirror second, authority transfer last"**, the vehicle remains operational and safe at every stage.

**Key Takeaways:**
1. Never compromise vehicle operability
2. Validate extensively before each authority transfer
3. Have rollback procedures ready at every phase
4. Test failsafes as rigorously as normal operation
5. Don't rush - safety trumps speed

**The vehicle must never be bricked. If in doubt, roll back and iterate.**

---

**END OF DOCUMENT**
