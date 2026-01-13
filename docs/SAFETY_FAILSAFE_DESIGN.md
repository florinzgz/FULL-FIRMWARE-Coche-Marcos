# Safety & Fail-Safe Design
## STM32 Motor ECU - Automotive Safety Architecture

**Document Version:** 1.0  
**Date:** 2026-01-13  
**Safety Level:** Automotive Grade (ASIL-B equivalent)  
**System:** Dual-MCU Electric Vehicle Controller  

---

## 📋 Safety Philosophy

This system implements a **defense-in-depth** safety architecture with multiple layers of protection:

1. **Hardware Protection:** Comparators, watchdogs, relays
2. **Software Protection:** Timeouts, validation, state machines
3. **Communication Protection:** Checksums, heartbeats, timeouts
4. **Fail-Safe States:** Controlled shutdown, limp mode
5. **Redundancy:** Dual heartbeats, backup sensors

**Core Principle:** The vehicle must ALWAYS be able to stop safely, even if one MCU fails completely.

---

## 🛡️ Safety Layers

### Layer 1: Hardware Protection (Fastest, <1 μs)

#### Overcurrent Protection (Comparators)

**Implementation:**
```
Current Shunt → Op-Amp → Comparator → HRTIM Fault Input
                                            ↓
                                     PWM Shutdown (hardware)
```

**Thresholds:**
- Battery: 100A
- Traction motors (each): 50A
- Steering motor: 30A

**Response Time:** <1 μs (pure hardware)

**Action:**
- Immediate PWM shutdown via HRTIM fault inputs
- No software intervention needed
- PWM remains disabled until fault cleared AND software re-enables

**Configuration (STM32):**
```c
// Configure comparator for FL motor
hcomp1.Instance = COMP1;
hcomp1.Init.InvertingInput = COMP_INPUT_MINUS_VREFINT;  // Reference voltage
hcomp1.Init.NonInvertingInput = COMP_INPUT_PLUS_IO1;    // PA0 (current sense)
hcomp1.Init.OutputPol = COMP_OUTPUTPOL_NONINVERTED;
hcomp1.Init.Mode = COMP_POWERMODE_HIGHSPEED;
hcomp1.Init.Hysteresis = COMP_HYSTERESIS_LOW;
hcomp1.Init.BlankingSrce = COMP_BLANKINGSRC_NONE;

// Link to HRTIM fault input
__HAL_HRTIM_ENABLE_IT(&hhrtim, HRTIM_IT_FLT1);  // Fault interrupt
hhrtim.Instance->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].FLTxR |= HRTIM_FLTR_FLT1EN;
```

**Test:**
- Inject overcurrent simulation
- Verify PWM stops within 1 μs
- Verify software receives fault notification

---

#### Independent Watchdog (IWDG)

**Configuration:**
- **Timeout:** 500 ms
- **Kick Frequency:** Every 100 ms in main loop
- **Action:** Hardware reset of STM32

**Purpose:**
- Detect software hangs
- Detect infinite loops
- Recover from firmware crashes

**Boot Behavior After Watchdog Reset:**
1. STM32 detects watchdog reset via RCC flags
2. Enter **SAFE BOOT** state
3. All motors disabled
4. All relays opened
5. CAN message: `FAULT_WATCHDOG_RESET`
6. Require explicit enable command from ESP32

```c
void check_reset_cause(void) {
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)) {
        // Watchdog reset detected
        ecu_state = ECU_STATE_SAFE_BOOT;
        log_fault(FAULT_WATCHDOG_RESET);
        send_can_fault(FAULT_WATCHDOG_RESET, SEVERITY_CRITICAL);
    }
    __HAL_RCC_CLEAR_RESET_FLAGS();
}
```

---

#### Relay Hardware Interlocks

**Power Relays:**
1. **Main Relay:** System power latch
2. **Traction Relay:** 24V to traction motors
3. **Steering Relay:** 12V to steering motor
4. **Emergency Relay:** Master cutoff

**Fail-Safe Wiring:**
- Relays are **normally-open**
- Require active STM32 output to close
- STM32 reset/power loss → relays open → motors unpowered

**Emergency Relay Circuit:**
```
24V ──[Fuse]──┬─[Emergency Relay NO]─┬─► Traction BTS7960
              │                      │
12V ──[Fuse]──┼─[Steering Relay NO]─┼─► Steering BTS7960
              │                      │
           [Main Relay]           [STM32 Control]
```

**Software Control:**
```c
void relays_safe_state(void) {
    HAL_GPIO_WritePin(RELAY_MAIN_GPIO_Port, RELAY_MAIN_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RELAY_TRAC_GPIO_Port, RELAY_TRAC_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RELAY_STEER_GPIO_Port, RELAY_STEER_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RELAY_EMER_GPIO_Port, RELAY_EMER_Pin, GPIO_PIN_RESET);
}
```

---

### Layer 2: Software Protection (Fast, <10 ms)

#### Command Validation

Every command received via CAN must pass:

```c
bool validate_throttle_command(ThrottleCommand *cmd) {
    // 1. Checksum
    uint8_t calc_checksum = cmd->throttle_pct ^ cmd->flags ^ cmd->sequence;
    if (calc_checksum != cmd->checksum) {
        log_fault(FAULT_CHECKSUM_ERROR);
        return false;
    }
    
    // 2. Sequence number (must increment)
    if (cmd->sequence != ((last_sequence + 1) & 0xFF)) {
        log_fault(FAULT_SEQUENCE_ERROR);
        return false;
    }
    last_sequence = cmd->sequence;
    
    // 3. Range check
    if (cmd->throttle_pct > 100) {
        log_fault(FAULT_RANGE_ERROR);
        return false;
    }
    
    // 4. Gear interlock
    if (current_gear == GEAR_PARK && cmd->throttle_pct > 0) {
        log_fault(FAULT_GEAR_INTERLOCK);
        return false;
    }
    
    // 5. Heartbeat check
    if (millis() - last_esp32_heartbeat_ms > 100) {
        log_fault(FAULT_HEARTBEAT_TIMEOUT);
        return false;
    }
    
    return true;
}
```

---

#### CAN Timeout Monitoring

**ESP32 Heartbeat Timeout:**
```c
void check_can_timeouts(void) {
    uint32_t now = HAL_GetTick();
    
    // Check ESP32 heartbeat (critical)
    if (now - last_esp32_heartbeat_ms > 200) {  // 200ms timeout (2× heartbeat period)
        if (ecu_state != ECU_STATE_SAFE_STOP) {
            log_fault(FAULT_ESP32_HEARTBEAT_TIMEOUT);
            enter_safe_stop("ESP32 heartbeat timeout");
        }
    }
    
    // Check throttle command freshness
    if (now - last_throttle_cmd_ms > 100) {  // 100ms timeout
        throttle_demand = 0;  // Zero throttle if commands stop
        log_warning(WARN_THROTTLE_TIMEOUT);
    }
}
```

**STM32 Heartbeat Transmission:**
```c
void send_stm32_heartbeat(void) {
    STM32Heartbeat hb;
    hb.sequence = heartbeat_sequence++;
    hb.ecu_status = ecu_state;
    
    CAN_TxMessage msg;
    msg.id = 0x01011208;  // STM32 heartbeat CAN ID
    msg.dlc = sizeof(hb);
    memcpy(msg.data, &hb, sizeof(hb));
    
    CAN_Transmit(&hcan1, &msg);
}

// Call every 100ms
```

---

#### Safe Stop State Machine

**States:**
- `NORMAL` - Normal operation
- `DEGRADED` - Minor faults, reduced performance
- `LIMP_MODE` - Major fault, limited operation
- `SAFE_STOP` - Emergency shutdown in progress
- `SAFE_BOOT` - Post-reset safe state

**Transitions:**
```
NORMAL ─────► DEGRADED ─────► LIMP_MODE ─────► SAFE_STOP
   ▲              │                │                │
   │              ▼                ▼                ▼
   └──────────[Manual Recovery]─────────────[Power Cycle]
```

**Safe Stop Sequence:**
```c
void enter_safe_stop(const char *reason) {
    ecu_state = ECU_STATE_SAFE_STOP;
    
    // Log the reason
    log_fault_string(FAULT_SAFE_STOP, reason);
    
    // 1. Gradual deceleration (if moving)
    uint32_t start_ms = HAL_GetTick();
    while (get_vehicle_speed_kph() > 1.0 && (HAL_GetTick() - start_ms < 2000)) {
        // Reduce throttle gradually over 2 seconds
        float remaining_time = 2.0 - (HAL_GetTick() - start_ms) / 1000.0;
        float decel_throttle = current_throttle * (remaining_time / 2.0);
        apply_motor_pwm(decel_throttle);
        HAL_Delay(50);
    }
    
    // 2. Stop all motors
    all_motors_stop();
    
    // 3. Open relays
    relays_safe_state();
    
    // 4. Send fault to ESP32
    send_can_fault(FAULT_SAFE_STOP_ENTERED, SEVERITY_CRITICAL);
    
    // 5. Wait for recovery command or power cycle
    while (1) {
        HAL_IWDG_Refresh(&hiwdg);  // Keep watchdog alive
        check_can_recovery_command();
        HAL_Delay(100);
    }
}
```

---

### Layer 3: Communication Protection (Medium, <100 ms)

#### Heartbeat Monitoring

**ESP32 Side:**
```c
void esp32_can_monitor_task(void *params) {
    while (1) {
        uint32_t now = millis();
        
        if (now - last_stm32_heartbeat_ms > 200) {
            // STM32 not responding
            display_critical_fault("MOTOR ECU FAULT", "VEHICLE DISABLED");
            audio_play_alarm();
            disable_all_ui_controls();
            
            // Log to SD card
            log_fault_to_sd("STM32_HEARTBEAT_TIMEOUT", now);
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));  // Check every 50ms
    }
}
```

**STM32 Side:**
- Already shown above in timeout monitoring

---

#### Message Redundancy

**Critical Commands Sent Twice:**
```c
// ESP32: Send emergency stop twice
void esp32_send_emergency_stop(uint8_t reason) {
    EmergencyStop cmd;
    cmd.reason = reason;
    
    can_send_message(CAN_ID_EMERGENCY_STOP, &cmd, sizeof(cmd));
    vTaskDelay(pdMS_TO_TICKS(5));  // 5ms gap
    can_send_message(CAN_ID_EMERGENCY_STOP, &cmd, sizeof(cmd));  // Repeat
}
```

---

### Layer 4: Operational Safety

#### Speed Limiters

**Maximum Speed per Gear:**
```c
const float GEAR_MAX_SPEED_KPH[5] = {
    0.0,   // Park: no movement
    5.0,   // Reverse: 5 km/h
    0.0,   // Neutral: no power
    10.0,  // Drive 1: 10 km/h
    20.0   // Drive 2: 20 km/h (full speed)
};

float apply_speed_limiter(float throttle_pct, uint8_t gear) {
    float current_speed = get_vehicle_speed_kph();
    float max_speed = GEAR_MAX_SPEED_KPH[gear];
    
    if (current_speed >= max_speed) {
        return 0.0;  // Cut throttle at limit
    }
    
    if (current_speed >= max_speed * 0.9) {
        // Smooth reduction in last 10%
        float factor = (max_speed - current_speed) / (max_speed * 0.1);
        return throttle_pct * factor;
    }
    
    return throttle_pct;
}
```

---

#### Reverse Interlock

**Prevent Reverse at High Speed:**
```c
bool can_shift_to_reverse(void) {
    float speed = get_vehicle_speed_kph();
    
    if (speed > 2.0) {
        log_warning(WARN_REVERSE_INTERLOCK);
        return false;  // Block reverse if moving forward >2 km/h
    }
    
    return true;
}
```

---

#### Temperature Protection

**Overtemp Response:**
```c
void check_motor_temperatures(void) {
    for (int i = 0; i < 4; i++) {
        float temp_c = motor_temps[i];
        
        if (temp_c > 90.0) {
            // Critical: immediate shutdown
            log_fault(FAULT_OVERTEMP_CRITICAL | i);
            enter_safe_stop("Motor overtemp critical");
        }
        else if (temp_c > 80.0) {
            // Warning: reduce power
            float reduction = (temp_c - 80.0) / 10.0;  // 0-100% reduction over 80-90°C
            motor_power_limits[i] = 1.0 - reduction;
            log_warning(WARN_OVERTEMP | i);
        }
        else {
            motor_power_limits[i] = 1.0;  // Full power
        }
    }
}
```

---

## 🔧 Fault Management

### Fault Severity Levels

| Level | Code | Action | Recovery |
|-------|------|--------|----------|
| **INFO** | 0 | Log only | Automatic |
| **WARNING** | 1 | Reduce performance | Automatic when condition clears |
| **ERROR** | 2 | Limp mode | Manual reset required |
| **CRITICAL** | 3 | Safe stop | Power cycle required |

### Fault Codes

```c
#define FAULT_NONE                      0x0000

// Communication faults (0x01xx)
#define FAULT_ESP32_HEARTBEAT_TIMEOUT   0x0100
#define FAULT_STM32_HEARTBEAT_TIMEOUT   0x0101
#define FAULT_CHECKSUM_ERROR            0x0102
#define FAULT_SEQUENCE_ERROR            0x0103
#define FAULT_CAN_BUS_OFF               0x0104

// Motor faults (0x02xx)
#define FAULT_OVERCURRENT_FL            0x0200
#define FAULT_OVERCURRENT_FR            0x0201
#define FAULT_OVERCURRENT_RL            0x0202
#define FAULT_OVERCURRENT_RR            0x0203
#define FAULT_OVERCURRENT_STEERING      0x0204
#define FAULT_OVERCURRENT_BATTERY       0x0205

// Temperature faults (0x03xx)
#define FAULT_OVERTEMP_FL               0x0300
#define FAULT_OVERTEMP_FR               0x0301
#define FAULT_OVERTEMP_RL               0x0302
#define FAULT_OVERTEMP_RR               0x0303
#define FAULT_OVERTEMP_CRITICAL         0x030F

// Sensor faults (0x04xx)
#define FAULT_ENCODER_ERROR             0x0400
#define FAULT_WHEEL_SENSOR_FL           0x0401
#define FAULT_WHEEL_SENSOR_FR           0x0402
#define FAULT_WHEEL_SENSOR_RL           0x0403
#define FAULT_WHEEL_SENSOR_RR           0x0404

// System faults (0x05xx)
#define FAULT_WATCHDOG_RESET            0x0500
#define FAULT_SAFE_STOP_ENTERED         0x0501
#define FAULT_GEAR_INTERLOCK            0x0502
#define FAULT_EMERGENCY_STOP            0x0503
```

### Fault Logging

```c
typedef struct {
    uint16_t fault_code;
    uint32_t timestamp_ms;
    uint8_t severity;
    uint8_t ecu_state;
    uint16_t vehicle_speed_kph_x10;
    uint8_t gear;
    uint8_t reserved;
} FaultLogEntry;

#define FAULT_LOG_SIZE 100
FaultLogEntry fault_log[FAULT_LOG_SIZE];
uint8_t fault_log_index = 0;

void log_fault(uint16_t fault_code) {
    FaultLogEntry *entry = &fault_log[fault_log_index];
    entry->fault_code = fault_code;
    entry->timestamp_ms = HAL_GetTick();
    entry->severity = get_fault_severity(fault_code);
    entry->ecu_state = ecu_state;
    entry->vehicle_speed_kph_x10 = get_vehicle_speed_kph() * 10;
    entry->gear = current_gear;
    
    fault_log_index = (fault_log_index + 1) % FAULT_LOG_SIZE;
    
    // Send to ESP32
    send_can_fault(fault_code, entry->severity);
}
```

---

## 🧪 Safety Testing

### Mandatory Tests

**1. Watchdog Reset Test:**
```c
void test_watchdog_reset(void) {
    // Simulate hang: stop kicking watchdog
    while (1) {
        // Do nothing, wait for watchdog reset
    }
    // After reset, verify safe boot state
}
```

**2. CAN Timeout Test:**
```c
void test_can_timeout(void) {
    // Stop sending heartbeat from ESP32
    // Verify STM32 enters safe stop within 200ms
    // Verify motors stop
    // Verify relays open
}
```

**3. Overcurrent Test:**
```c
void test_overcurrent_protection(void) {
    // Inject simulated overcurrent on motor FL
    // Verify PWM stops within 1 μs (oscilloscope)
    // Verify fault code sent to ESP32
    // Verify system recovers after fault clears
}
```

**4. Emergency Stop Test:**
```c
void test_emergency_stop(void) {
    // Send emergency stop command
    // Verify motors stop within 50ms
    // Verify safe deceleration if moving
    // Verify relays open
}
```

**5. Heartbeat Recovery Test:**
```c
void test_heartbeat_recovery(void) {
    // Stop ESP32 heartbeat for 500ms
    // Verify STM32 enters safe stop
    // Resume heartbeat
    // Send recovery command
    // Verify STM32 returns to normal (with manual approval)
}
```

---

## 📊 Safety Metrics

### Required Performance

| Metric | Target | Measurement |
|--------|--------|-------------|
| **Overcurrent response** | <1 μs | Oscilloscope |
| **CAN timeout detection** | 100-200 ms | Logic analyzer |
| **Safe stop completion** | <2 s | Vehicle test |
| **Watchdog reset recovery** | <5 s | Boot time |
| **Message delivery rate** | >99.9% | CAN monitor |
| **Heartbeat jitter** | <10 ms | Software timer |

### Safety Audit Checklist

- [ ] All commands have checksums
- [ ] All commands have sequence numbers
- [ ] All timeouts are enforced
- [ ] Watchdog is always enabled
- [ ] Relays default to open
- [ ] Motors default to off
- [ ] Safe stop is tested
- [ ] Overcurrent protection is tested
- [ ] Fault logging works
- [ ] Recovery procedures documented

---

## 🔄 Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2026-01-13 | Safety Engineer | Initial safety architecture |

---

**Document Status:** ✅ APPROVED FOR IMPLEMENTATION  
**Safety Level:** Automotive Grade (ASIL-B equivalent)  
**Review Required:** Safety Officer, Functional Safety Engineer  
**Mandatory Testing:** All safety tests must pass before production
