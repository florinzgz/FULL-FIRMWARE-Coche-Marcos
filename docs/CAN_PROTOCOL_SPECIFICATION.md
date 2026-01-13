# CAN Protocol Specification
## ESP32-S3 ↔ STM32G474RE Motor ECU Communication

**Document Version:** 1.0  
**Date:** 2026-01-13  
**System:** Automotive Dual-MCU Electric Vehicle Controller  
**Bus Standard:** CAN 2.0B (29-bit Extended IDs)  
**Baud Rate:** 500 kbps  

---

## 📋 Overview

This document defines the strict CAN communication protocol between:
- **ESP32-S3** (UI + Logic + Gateway)
- **STM32G474RE** (Motor ECU + Real-Time Safety Controller)

### Protocol Principles

1. **Deterministic Timing:** Fixed message rates, no variable delays
2. **Fail-Safe:** Timeouts trigger safe states
3. **Heartbeat Monitoring:** Bidirectional liveness detection
4. **No Stale Commands:** All commands have timestamps/sequence numbers
5. **Priority-Based:** Emergency messages have highest priority

---

## 🔌 CAN Bus Configuration

### Physical Layer

| Parameter | Value | Notes |
|-----------|-------|-------|
| **Baud Rate** | 500 kbps | Automotive standard |
| **Protocol** | CAN 2.0B | 29-bit extended IDs |
| **Termination** | 120Ω at each end | ESP32 + STM32 |
| **Transceiver** | MCP2551 or TJA1050 | 3.3V compatible |
| **Isolation** | Optional galvanic | Recommended for safety |
| **Wire** | Twisted pair | CAN-H, CAN-L |

### CAN ID Structure (29-bit Extended)

```
Bits [28:24] - Priority (0 = highest)
Bits [23:16] - Source ID (0x01=ESP32, 0x02=STM32)
Bits [15:8]  - Destination ID (0x01=ESP32, 0x02=STM32, 0xFF=Broadcast)
Bits [7:0]   - Message Type
```

### Priority Levels

| Priority | Value | Message Types |
|----------|-------|---------------|
| **Critical** | 0x00 | Emergency stop, fault alerts |
| **High** | 0x01 | Control commands, heartbeat |
| **Medium** | 0x02 | Sensor telemetry |
| **Low** | 0x03 | Status, diagnostics |

---

## 📤 ESP32 → STM32 Messages (Commands)

### 0x100: Throttle Command

**Priority:** High  
**Rate:** 100 Hz (10 ms)  
**DLC:** 4 bytes  
**Timeout:** 100 ms → STM32 enters safe stop

```c
struct ThrottleCommand {
    uint8_t throttle_pct;     // 0-100%
    uint8_t flags;            // Bit flags
    uint8_t sequence;         // Rolling counter 0-255
    uint8_t checksum;         // XOR of bytes 0-2
};

// Flags bitmap:
// bit 0: reverse (0=forward, 1=reverse)
// bit 1: 4x4_enable (0=2WD, 1=4WD)
// bit 2: axis_rotation (tank turn mode)
// bit 3-7: reserved
```

**CAN ID:** `0x01012100` (Priority 1, ESP32→STM32, Type 0x00)

### 0x101: Steering Command

**Priority:** High  
**Rate:** 100 Hz (10 ms)  
**DLC:** 4 bytes  
**Timeout:** 100 ms → STM32 centers steering

```c
struct SteeringCommand {
    int16_t angle_deg;        // -45° to +45° (×10 for precision)
    uint8_t sequence;         // Rolling counter
    uint8_t checksum;         // XOR checksum
};
```

**CAN ID:** `0x01012101`

### 0x102: Gear Selection

**Priority:** High  
**Rate:** Event-driven (on change)  
**DLC:** 2 bytes  
**Timeout:** N/A (latched)

```c
struct GearCommand {
    uint8_t gear;             // 0=P, 1=R, 2=N, 3=D1, 4=D2
    uint8_t drive_mode;       // 0=Eco, 1=Normal, 2=Sport
};
```

**CAN ID:** `0x01012102`

### 0x103: Safety System Enable

**Priority:** High  
**Rate:** Event-driven  
**DLC:** 1 byte  

```c
struct SafetyEnable {
    uint8_t flags;            // Bitmap
    // bit 0: ABS enable
    // bit 1: TCS enable
    // bit 2: Regen enable
    // bit 3: Stability control
    // bit 4-7: reserved
};
```

**CAN ID:** `0x01012103`

### 0x104: Emergency Stop

**Priority:** Critical  
**Rate:** Event-driven  
**DLC:** 1 byte  
**Action:** Immediate motor shutdown

```c
struct EmergencyStop {
    uint8_t reason;           // 0=user, 1=obstacle, 2=fault
};
```

**CAN ID:** `0x00012104` (Priority 0 - Critical!)

### 0x105: ESP32 Heartbeat

**Priority:** High  
**Rate:** 10 Hz (100 ms)  
**DLC:** 2 bytes  
**Timeout:** STM32 waits 200 ms, then safe stop

```c
struct ESP32Heartbeat {
    uint8_t sequence;         // Incrementing counter
    uint8_t status;           // 0=OK, 1=degraded, 2=fault
};
```

**CAN ID:** `0x01012105`

---

## 📥 STM32 → ESP32 Messages (Telemetry)

### 0x200: Wheel Speeds

**Priority:** Medium  
**Rate:** 100 Hz  
**DLC:** 8 bytes  

```c
struct WheelSpeeds {
    uint16_t fl_rpm;          // Front-left wheel RPM
    uint16_t fr_rpm;          // Front-right wheel RPM
    uint16_t rl_rpm;          // Rear-left wheel RPM
    uint16_t rr_rpm;          // Rear-right wheel RPM
};
```

**CAN ID:** `0x02011200`

### 0x201: Motor Currents

**Priority:** Medium  
**Rate:** 20 Hz (50 ms)  
**DLC:** 8 bytes  

```c
struct MotorCurrents {
    uint16_t fl_current_ma;   // FL motor current (mA)
    uint16_t fr_current_ma;   
    uint16_t rl_current_ma;   
    uint16_t rr_current_ma;   
};
```

**CAN ID:** `0x02011201`

### 0x202: Motor Temperatures

**Priority:** Low  
**Rate:** 2 Hz (500 ms)  
**DLC:** 4 bytes  

```c
struct MotorTemperatures {
    uint8_t fl_temp_c;        // FL motor temp (°C)
    uint8_t fr_temp_c;        
    uint8_t rl_temp_c;        
    uint8_t rr_temp_c;        
};
```

**CAN ID:** `0x03011202`

### 0x203: Steering Status

**Priority:** Medium  
**Rate:** 100 Hz  
**DLC:** 4 bytes  

```c
struct SteeringStatus {
    int16_t angle_actual_deg; // Actual steering angle (×10)
    uint16_t motor_current_ma;// Steering motor current
};
```

**CAN ID:** `0x02011203`

### 0x204: Battery Status

**Priority:** Medium  
**Rate:** 10 Hz  
**DLC:** 4 bytes  

```c
struct BatteryStatus {
    uint16_t voltage_mv;      // Battery voltage (mV)
    int16_t current_ma;       // Battery current (mA, negative=regen)
};
```

**CAN ID:** `0x02011204`

### 0x205: ABS Status

**Priority:** High  
**Rate:** 20 Hz (when active), 2 Hz (when idle)  
**DLC:** 2 bytes  

```c
struct ABSStatus {
    uint8_t active_wheels;    // Bitmap: bits 0-3 = FL/FR/RL/RR
    uint8_t avg_slip_pct;     // Average slip percentage
};
```

**CAN ID:** `0x01011205`

### 0x206: TCS Status

**Priority:** High  
**Rate:** 20 Hz (when active), 2 Hz (when idle)  
**DLC:** 2 bytes  

```c
struct TCSStatus {
    uint8_t active_wheels;    // Bitmap: bits 0-3 = FL/FR/RL/RR
    uint8_t power_reduction_pct; // Power reduction applied
};
```

**CAN ID:** `0x01011206`

### 0x207: Fault Codes

**Priority:** Critical  
**Rate:** Event-driven  
**DLC:** 4 bytes  

```c
struct FaultCodes {
    uint16_t fault_bitmap;    // Fault flags (see below)
    uint8_t severity;         // 0=info, 1=warn, 2=error, 3=critical
    uint8_t subsystem;        // Motor index or 0xFF=global
};

// Fault bitmap:
// bit 0: Overcurrent FL
// bit 1: Overcurrent FR
// bit 2: Overcurrent RL
// bit 3: Overcurrent RR
// bit 4: Overcurrent steering
// bit 5: Overcurrent battery
// bit 6: Overheat FL
// bit 7: Overheat FR
// bit 8: Overheat RL
// bit 9: Overheat RR
// bit 10: Encoder error
// bit 11: Wheel sensor error
// bit 12: CAN timeout
// bit 13: Watchdog reset
// bit 14: MCP23017 I2C fault
// bit 15: Reserved
```

**CAN ID:** `0x00011207` (Critical priority!)

### 0x208: STM32 Heartbeat

**Priority:** High  
**Rate:** 10 Hz (100 ms)  
**DLC:** 2 bytes  
**Timeout:** ESP32 waits 200 ms, then displays critical fault

```c
struct STM32Heartbeat {
    uint8_t sequence;         // Incrementing counter
    uint8_t ecu_status;       // 0=OK, 1=degraded, 2=limp_mode
};
```

**CAN ID:** `0x01011208`

---

## ⏱️ Timing Requirements

### Message Rates Summary

| Message | Rate | Period | Timeout | Action on Timeout |
|---------|------|--------|---------|-------------------|
| Throttle Command | 100 Hz | 10 ms | 100 ms | STM32: Safe stop |
| Steering Command | 100 Hz | 10 ms | 100 ms | STM32: Center steering |
| ESP32 Heartbeat | 10 Hz | 100 ms | 200 ms | STM32: Safe stop |
| STM32 Heartbeat | 10 Hz | 100 ms | 200 ms | ESP32: Display fault |
| Wheel Speeds | 100 Hz | 10 ms | 200 ms | ESP32: Warning |
| Motor Currents | 20 Hz | 50 ms | 500 ms | ESP32: Warning |
| Temperatures | 2 Hz | 500 ms | 2000 ms | ESP32: Warning |

### Latency Budget

**Critical Path: Pedal → Motor PWM**

```
ESP32: Pedal ADC read           10 μs
ESP32: CAN message prepare      20 μs
CAN: Bus transmission (8 bytes) 160 μs @ 500kbps
STM32: CAN RX interrupt         10 μs
STM32: Command processing       20 μs
STM32: HRTIM PWM update         1 μs
────────────────────────────────────
Total Latency:                  221 μs

vs. Current I2C PCA9685:        850 μs
Improvement:                    3.8× faster
```

---

## 🛡️ Fail-Safe Behavior

### STM32 Fail-Safe States

**If ESP32 CAN messages stop for >100ms:**

1. **Immediate Actions (within 1 ms):**
   - Set all motor PWM to 0%
   - Open traction relay
   - Open steering relay
   - Activate emergency brake (if equipped)

2. **Safe Stop Sequence:**
   - Gradual deceleration if vehicle moving
   - Maintain steering control for 2 seconds
   - Log fault code `FAULT_CAN_TIMEOUT_ESP32`
   - Set `ecu_status = LIMP_MODE`

3. **Recovery:**
   - When ESP32 heartbeat resumes, wait for 3 consecutive valid messages
   - Exit limp mode only if commanded by ESP32
   - Log recovery event

**If ESP32 sends invalid/corrupted messages:**
- Ignore message
- Increment error counter
- If >10 errors in 1 second → enter safe stop
- Log fault `FAULT_CAN_CHECKSUM_ERROR`

### ESP32 Fail-Safe States

**If STM32 heartbeat stops for >200ms:**

1. **Display Critical Alert:**
   - Red full-screen warning
   - "MOTOR CONTROLLER FAULT"
   - "VEHICLE DISABLED - CONTACT SERVICE"
   - Audio alarm (continuous beep)

2. **UI Actions:**
   - Disable all drive controls
   - Show last known telemetry (greyed out)
   - Log fault event to SD card
   - Send fault report via USB (if connected)

3. **Do NOT Attempt:**
   - Motor control via I2C (removed)
   - Direct relay control
   - Sensor reading

**Recovery:**
- Requires power cycle or STM32 reset
- Display recovery confirmation when STM32 heartbeat resumes

---

## 🔒 Safety Interlocks

### Command Validation (STM32)

Before accepting any command, STM32 MUST verify:

1. **Sequence Number:** Incrementing (wraps at 255)
2. **Checksum:** XOR of payload bytes
3. **Range Check:** Values within valid bounds
4. **Heartbeat:** ESP32 heartbeat received within last 100 ms
5. **Gear Interlock:** No throttle in Park, no reverse >5 km/h

**If validation fails:**
- Reject command
- Keep previous valid command for up to 100 ms
- Then enter safe stop

### Redundancy Checks

**Watchdog:**
- Independent watchdog (IWDG) on STM32
- Timeout: 500 ms
- Kicked every 100 ms in main loop
- Failure → STM32 hardware reset → Safe boot state

**Overcurrent Protection:**
- Hardware comparators on STM32
- Threshold: 100A battery, 50A motors, 30A steering
- Response time: <1 μs (hardware)
- Action: Immediately cut PWM via HRTIM fault input

**Overtemperature Protection:**
- Software check every 500 ms
- Threshold: 80°C warning, 90°C shutdown
- Action: Gradual power reduction → safe stop

---

## 📊 Bus Load Analysis

### Message Bandwidth

| Message | Rate | Bytes | Bitrate |
|---------|------|-------|---------|
| Throttle | 100 Hz | 13* | 10.4 kbps |
| Steering | 100 Hz | 13* | 10.4 kbps |
| ESP32 Heartbeat | 10 Hz | 13* | 1.0 kbps |
| Wheel Speeds | 100 Hz | 17* | 13.6 kbps |
| Motor Currents | 20 Hz | 17* | 2.7 kbps |
| Temperatures | 2 Hz | 13* | 0.2 kbps |
| Steering Status | 100 Hz | 13* | 10.4 kbps |
| Battery Status | 10 Hz | 13* | 1.0 kbps |
| STM32 Heartbeat | 10 Hz | 13* | 1.0 kbps |
| **Total** | | | **51 kbps** |

\* CAN frame overhead: 1 bit start, 29 bit ID, 4 bit DLC, N×8 data, 15 bit CRC, 7 bit EOF = ~13 bytes for 4-byte payload

**Bus utilization:** 51 kbps / 500 kbps = **10.2%**  
**Margin:** 89.8% for expansion

---

## 🧪 Testing & Validation

### Functional Tests

1. **Normal Operation:**
   - All messages transmitted at specified rates
   - Latency <300 μs end-to-end
   - No dropped messages for 1 hour continuous operation

2. **Timeout Handling:**
   - Disconnect ESP32 CAN → STM32 safe stop within 100 ms
   - Disconnect STM32 CAN → ESP32 displays fault within 200 ms
   - Reconnect → recovery within 500 ms

3. **Error Injection:**
   - Send invalid checksums → STM32 rejects
   - Send out-of-range values → STM32 clamps/rejects
   - Send stale sequence numbers → STM32 rejects

4. **Bus Saturation:**
   - Add 400 kbps background traffic
   - Verify critical messages still delivered
   - Verify priority arbitration works

### Acceptance Criteria

✅ **Pass if:**
- All messages delivered 99.9% of time (normal operation)
- Timeout detection within specified limits
- Safe stop completes within 200 ms
- No stale commands executed (>100 ms old)
- Bus utilization <50% under normal load
- Watchdog triggers on hung MCU

❌ **Fail if:**
- Any safety timeout missed
- CAN errors >1% of messages
- Latency >500 μs consistently
- Recovery takes >1 second

---

## 📝 Implementation Notes

### ESP32 CAN Driver

Use ESP32 TWAI (CAN) controller:
```c
twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
    GPIO_NUM_XX,  // TX
    GPIO_NUM_XX,  // RX
    TWAI_MODE_NORMAL
);
twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
```

### STM32 CAN Driver

Use STM32 CAN peripheral (bxCAN):
```c
CAN_HandleTypeDef hcan1;
hcan1.Instance = CAN1;
hcan1.Init.Prescaler = 6;  // 500 kbps @ 48 MHz APB1
hcan1.Init.Mode = CAN_MODE_NORMAL;
hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
hcan1.Init.TimeSeg1 = CAN_BS1_13TQ;
hcan1.Init.TimeSeg2 = CAN_BS2_2TQ;
```

### Message Priority

Configure CAN filters to prioritize critical messages:
- Emergency stop: Filter bank 0, highest priority
- Heartbeats: Filter bank 1
- Commands: Filter bank 2
- Telemetry: Filter bank 3+

---

## 🔄 Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2026-01-13 | System Architect | Initial protocol definition |

---

**Document Status:** ✅ APPROVED FOR IMPLEMENTATION  
**Review Required:** Safety Engineer, Embedded Systems Lead  
**Next Review:** After Phase 1 testing
