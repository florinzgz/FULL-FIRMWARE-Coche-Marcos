# Migration Strategy
## ESP32 Monolithic → Dual-MCU CAN Architecture

**Document Version:** 1.0  
**Date:** 2026-01-13  
**Project:** STM32 Motor ECU Integration  
**Strategy:** Phased, Reversible, Testable  

---

## 🎯 Migration Principles

### Core Rules

1. **Reversible:** Each phase can be rolled back
2. **Testable:** Vehicle remains operational at each phase
3. **Incremental:** Small, verifiable changes
4. **Safe-First:** Safety systems active from day one
5. **No Rewrite:** Modify existing code, don't replace

### Success Criteria

At every phase:
✅ ESP32 boots successfully  
✅ Vehicle can move (even if degraded mode)  
✅ Emergency stop works  
✅ No motor runaway possible  
✅ Telemetry visible on display  

---

## 📅 Phase Plan

### Phase 0: Preparation (Week 1)

**Goal:** Set up development environment and hardware

**Tasks:**
1. **Hardware:**
   - Procure STM32G474RE development board
   - Procure CAN transceivers (MCP2551 × 2)
   - Build CAN test harness (breadboard)
   - Wire CAN bus between ESP32 and STM32

2. **Software:**
   - Set up STM32CubeIDE project
   - Install STM32 HAL library
   - Set up PlatformIO for dual-MCU build
   - Create separate build configs for ESP32 and STM32

3. **Testing:**
   - Verify CAN communication (loopback test)
   - Test heartbeat messages
   - Test basic telemetry

**Deliverables:**
- ✅ CAN bus operational
- ✅ Both MCUs communicating
- ✅ Development tools ready

**Rollback:** N/A (nothing changed yet)

---

### Phase 1: CAN Communication Only (Week 2)

**Goal:** Establish CAN link, keep all hardware on ESP32

**Changes:**

**ESP32:**
```c
// Add CAN driver (use ESP32 TWAI)
#include "driver/twai.h"

// Initialize CAN
void can_init(void) {
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        GPIO_NUM_XX, GPIO_NUM_YY, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    
    twai_driver_install(&g_config, &t_config, &f_config);
    twai_start();
}

// Add CAN send/receive tasks
void can_rx_task(void *params) {
    twai_message_t msg;
    while (1) {
        if (twai_receive(&msg, pdMS_TO_TICKS(100)) == ESP_OK) {
            process_can_message(&msg);
        }
    }
}
```

**STM32:**
```c
// Minimal firmware: echo telemetry only
void main(void) {
    HAL_Init();
    SystemClock_Config();
    CAN_Init();
    
    while (1) {
        // Just send heartbeat
        send_stm32_heartbeat();
        HAL_Delay(100);
    }
}
```

**Testing:**
- Send heartbeat from both MCUs
- Verify CAN messages received
- Display STM32 heartbeat on ESP32 HUD
- Check bus load with logic analyzer

**Rollback:** Remove CAN code, #ifdef guards

---

### Phase 2: Motor Control via STM32 (Week 3-4)

**Goal:** Move motor PWM to STM32, ESP32 sends throttle commands

**Hardware Changes:**
1. **Disconnect ESP32 from PCA9685 PWM drivers**
2. **Connect STM32 HRTIM outputs to BTS7960 drivers**
3. **Keep MCP23017 on ESP32 for now** (shifter inputs)

**ESP32 Changes:**
```c
// Replace motor control with CAN commands
void set_motor_throttle(float throttle_pct) {
    // OLD: write to PCA9685 via I2C
    // pca9685_set_pwm(channel, pwm_value);
    
    // NEW: send CAN command
    ThrottleCommand cmd;
    cmd.throttle_pct = throttle_pct;
    cmd.flags = get_throttle_flags();  // reverse, 4x4, etc.
    cmd.sequence = throttle_sequence++;
    cmd.checksum = calc_checksum(&cmd);
    
    can_send_message(CAN_ID_THROTTLE_CMD, &cmd, sizeof(cmd));
}
```

**STM32 Implementation:**
```c
// Initialize HRTIM for motor PWM
void motor_control_init(void) {
    // Configure HRTIM at 20 kHz
    hhrtim.Instance = HRTIM1;
    hhrtim.Init.HRTIMInterruptResquests = HRTIM_IT_NONE;
    hhrtim.Init.SyncOptions = HRTIM_SYNCOPTION_NONE;
    HAL_HRTIM_Init(&hhrtim);
    
    // Configure TimerA for Motor FL
    HRTIM_TimeBaseInitTypeDef timebase = {0};
    timebase.Period = 8500;  // 20 kHz @ 170 MHz
    timebase.PrescalerRatio = HRTIM_PRESCALERRATIO_MUL32;
    timebase.Mode = HRTIM_MODE_CONTINUOUS;
    HAL_HRTIM_TimeBaseConfig(&hhrtim, HRTIM_TIMERINDEX_TIMER_A, &timebase);
    
    // Configure PWM output
    HRTIM_OutputInitTypeDef output = {0};
    output.Polarity = HRTIM_OUTPUTPOLARITY_HIGH;
    output.SetSource = HRTIM_OUTPUTSET_TIMPER;
    output.ResetSource = HRTIM_OUTPUTRESET_TIMCMP1;
    HAL_HRTIM_WaveformOutputConfig(&hhrtim, HRTIM_TIMERINDEX_TIMER_A,
                                    HRTIM_OUTPUT_TA1, &output);
    
    // Start PWM
    HAL_HRTIM_WaveformCountStart(&hhrtim, HRTIM_TIMERID_TIMER_A);
    HAL_HRTIM_WaveformOutputStart(&hhrtim, HRTIM_OUTPUT_TA1);
}

// Apply throttle from CAN command
void apply_motor_throttle(uint8_t throttle_pct) {
    uint16_t pwm_value = (throttle_pct * 8500) / 100;  // 0-100% → 0-8500 counts
    
    // Set compare value for PWM duty cycle
    __HAL_HRTIM_SETCOMPARE(&hhrtim, HRTIM_TIMERINDEX_TIMER_A,
                           HRTIM_COMPAREUNIT_1, pwm_value);
}
```

**Testing:**
1. Bench test: STM32 PWM with function generator
2. Connect to one motor only (FL)
3. Send throttle via CAN, verify motor responds
4. Gradually add other motors
5. Test emergency stop via CAN
6. Test timeout behavior (disconnect ESP32)

**Rollback:** Reconnect ESP32 to PCA9685, disable STM32 PWM

---

### Phase 3: Sensors Move to STM32 (Week 5-6)

**Goal:** Move encoders, wheel sensors, current sensors to STM32

**Hardware Changes:**
1. **Disconnect wheel sensors from ESP32 GPIO**
2. **Connect to STM32 GPIO + EXTI**
3. **Disconnect steering encoder from ESP32**
4. **Connect to STM32 TIM4 (hardware quadrature decoder)**
5. **Keep INA226 on ESP32 I2C for now** (or migrate to ADC shunts)

**ESP32 Changes:**
```c
// Remove encoder ISR
// OLD:
// void IRAM_ATTR encoder_isr_a(void) { ... }

// Replace with CAN telemetry consumer
void update_steering_angle_from_can(void) {
    // Receive steering status from STM32
    if (can_receive_nonblocking(&msg)) {
        if (msg.id == CAN_ID_STEERING_STATUS) {
            SteeringStatus *status = (SteeringStatus*)msg.data;
            steering_angle_deg = status->angle_actual_deg / 10.0;
        }
    }
}
```

**STM32 Implementation:**
```c
// Configure TIM4 as encoder interface
void encoder_init(void) {
    htim4.Instance = TIM4;
    htim4.Init.Period = 4800 - 1;  // 1200 PPR × 4 edges
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Prescaler = 0;
    
    TIM_Encoder_InitTypeDef encoder = {0};
    encoder.EncoderMode = TIM_ENCODERMODE_TI12;
    encoder.IC1Polarity = TIM_ICPOLARITY_RISING;
    encoder.IC2Polarity = TIM_ICPOLARITY_RISING;
    
    HAL_TIM_Encoder_Init(&htim4, &encoder);
    HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_ALL);
}

// Read encoder position
int16_t get_encoder_position(void) {
    return __HAL_TIM_GET_COUNTER(&htim4);
}

// Z-index interrupt for center detection
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == ENCODER_Z_Pin) {
        // Reset counter on Z pulse
        __HAL_TIM_SET_COUNTER(&htim4, 2400);  // Center position
        encoder_centered = true;
    }
}
```

**Wheel Speed Sensors:**
```c
// EXTI interrupts for wheel sensors
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    uint32_t now = HAL_GetTick();
    
    if (GPIO_Pin == WHEEL_FL_Pin) {
        wheel_pulse_time_ms[WHEEL_FL] = now - wheel_last_pulse_ms[WHEEL_FL];
        wheel_last_pulse_ms[WHEEL_FL] = now;
        wheel_pulse_count[WHEEL_FL]++;
    }
    // Repeat for FR, RL, RR
}

// Calculate RPM from pulse timing
uint16_t calculate_wheel_rpm(uint8_t wheel) {
    if (wheel_pulse_time_ms[wheel] == 0) return 0;
    
    // 6 pulses per revolution
    // RPM = (60000 ms/min) / (pulse_time_ms × 6 pulses/rev)
    return 60000 / (wheel_pulse_time_ms[wheel] * 6);
}
```

**Testing:**
1. Verify encoder reading via CAN
2. Compare with old ESP32 readings (both active)
3. Verify wheel speeds via CAN
4. Check ABS logic still works with CAN data
5. Test 100 Hz sensor update rate

**Rollback:** Reconnect sensors to ESP32, disable STM32 sensor code

---

### Phase 4: Current Sensing Migration (Week 7)

**Goal:** Move INA226 current sensors from ESP32 I2C to STM32 (ADC or I2C)

**Option A: Keep INA226 on I2C**
- Move I2C bus from ESP32 to STM32
- Simpler migration
- Slower (I2C latency)

**Option B: Replace with ADC Shunts**
- Add current shunt resistors
- Add op-amp amplifier circuits
- Connect to STM32 ADC inputs
- Faster, synchronized sampling
- Requires hardware changes

**Recommended: Option A first, then Option B**

**ESP32 Changes:**
```c
// Remove INA226 I2C reads
// OLD:
// float current = ina226_read_current(MOTOR_FL);

// NEW: consume from CAN
void update_motor_currents_from_can(void) {
    if (can_receive_nonblocking(&msg)) {
        if (msg.id == CAN_ID_MOTOR_CURRENTS) {
            MotorCurrents *currents = (MotorCurrents*)msg.data;
            motor_current_fl = currents->fl_current_ma / 1000.0;
            motor_current_fr = currents->fr_current_ma / 1000.0;
            motor_current_rl = currents->rl_current_ma / 1000.0;
            motor_current_rr = currents->rr_current_ma / 1000.0;
        }
    }
}
```

**STM32 Implementation (I2C):**
```c
// Read INA226 on STM32
void update_motor_currents(void) {
    motor_currents.fl_current_ma = ina226_read_current_ma(INA_FL_ADDR);
    motor_currents.fr_current_ma = ina226_read_current_ma(INA_FR_ADDR);
    motor_currents.rl_current_ma = ina226_read_current_ma(INA_RL_ADDR);
    motor_currents.rr_current_ma = ina226_read_current_ma(INA_RR_ADDR);
    
    // Send via CAN
    can_send_message(CAN_ID_MOTOR_CURRENTS, &motor_currents, sizeof(motor_currents));
}
```

**Testing:**
1. Compare INA226 readings (ESP32 vs STM32)
2. Verify current telemetry on HUD
3. Test overcurrent protection logic
4. Verify 20 Hz update rate

**Rollback:** Reconnect INA226 I2C to ESP32

---

### Phase 5: Temperature & MCP23017 (Week 8)

**Goal:** Move remaining I2C devices and OneWire to STM32

**Hardware Changes:**
1. **Move DS18B20 OneWire from ESP32 GPIO to STM32 GPIO**
2. **Move MCP23017 from ESP32 I2C to STM32 I2C**
3. **ESP32 now has NO motor/sensor hardware**

**ESP32 Changes:**
```c
// Remove all sensor reads
// All data now comes from CAN

// Shifter state from CAN
void update_shifter_from_can(void) {
    // STM32 reads MCP23017, sends gear state
    if (can_receive_nonblocking(&msg)) {
        if (msg.id == CAN_ID_GEAR_COMMAND) {
            GearCommand *gear = (GearCommand*)msg.data;
            current_gear = gear->gear;
        }
    }
}
```

**STM32 Implementation:**
```c
// Read MCP23017 for shifter
uint8_t read_shifter_position(void) {
    uint8_t portb = mcp23017_read_gpio_b();
    
    // Check which position is active (LOW = selected)
    if (!(portb & (1 << MCP_PIN_SHIFTER_P))) return GEAR_PARK;
    if (!(portb & (1 << MCP_PIN_SHIFTER_R))) return GEAR_REVERSE;
    if (!(portb & (1 << MCP_PIN_SHIFTER_N))) return GEAR_NEUTRAL;
    if (!(portb & (1 << MCP_PIN_SHIFTER_D1))) return GEAR_DRIVE1;
    if (!(portb & (1 << MCP_PIN_SHIFTER_D2))) return GEAR_DRIVE2;
    
    return GEAR_PARK;  // Default to park if nothing detected
}

// Read DS18B20 temperatures
void update_motor_temperatures(void) {
    ds18b20_start_conversion();
    HAL_Delay(750);  // Wait for conversion
    
    motor_temps.fl_temp_c = ds18b20_read_temp(DS18B20_FL_ADDR);
    motor_temps.fr_temp_c = ds18b20_read_temp(DS18B20_FR_ADDR);
    motor_temps.rl_temp_c = ds18b20_read_temp(DS18B20_RL_ADDR);
    motor_temps.rr_temp_c = ds18b20_read_temp(DS18B20_RR_ADDR);
    
    // Send via CAN
    can_send_message(CAN_ID_MOTOR_TEMPS, &motor_temps, sizeof(motor_temps));
}
```

**Testing:**
1. Verify temperature readings
2. Verify shifter detection
3. Test overtemp protection
4. Verify ESP32 has NO hardware control left

**Rollback:** Reconnect to ESP32

---

### Phase 6: Relays & Safety (Week 9)

**Goal:** Move relay control to STM32, enable full safety stack

**Hardware Changes:**
1. **Move relay control GPIOs from ESP32 to STM32**
2. **Wire emergency stop button to STM32 (optional)**

**ESP32 Changes:**
```c
// Remove relay control
// OLD:
// digitalWrite(PIN_RELAY_MAIN, HIGH);

// NEW: send command via CAN
void enable_traction_relay(bool enable) {
    // STM32 controls relays directly
    // ESP32 can request via safety enable message
    SafetyEnable cmd;
    cmd.flags = enable ? SAFETY_TRACTION_ENABLE : 0;
    can_send_message(CAN_ID_SAFETY_ENABLE, &cmd, sizeof(cmd));
}
```

**STM32 Implementation:**
```c
// Full safety stack
void safety_check_and_control_relays(void) {
    bool safe_to_enable = true;
    
    // Check all safety conditions
    if (ecu_state != ECU_STATE_NORMAL) safe_to_enable = false;
    if (millis() - last_esp32_heartbeat_ms > 200) safe_to_enable = false;
    if (any_motor_overcurrent()) safe_to_enable = false;
    if (any_motor_overtemp()) safe_to_enable = false;
    
    // Control relays
    if (safe_to_enable && traction_enable_requested) {
        HAL_GPIO_WritePin(RELAY_TRAC_GPIO_Port, RELAY_TRAC_Pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(RELAY_TRAC_GPIO_Port, RELAY_TRAC_Pin, GPIO_PIN_RESET);
    }
    
    // Repeat for other relays
}
```

**Testing:**
1. Verify relay control from STM32
2. Test timeout → relay opens
3. Test overcurrent → relay opens
4. Test emergency stop
5. **Full safety audit**

**Rollback:** Reconnect relays to ESP32

---

### Phase 7: ABS/TCS Integration (Week 10)

**Goal:** ABS/TCS logic stays on ESP32, uses CAN sensor data

**ESP32 Changes:**
```c
// ABS logic already exists, just change data source
void abs_update(void) {
    // OLD: read wheel speeds from GPIO ISR
    // float wheel_speeds[4] = { wheel_fl_rpm, wheel_fr_rpm, wheel_rl_rpm, wheel_rr_rpm };
    
    // NEW: get from CAN telemetry
    WheelSpeeds speeds = get_latest_wheel_speeds_from_can();
    float wheel_speeds[4] = { speeds.fl_rpm, speeds.fr_rpm, speeds.rl_rpm, speeds.rr_rpm };
    
    // Rest of ABS logic unchanged
    float slip[4];
    for (int i = 0; i < 4; i++) {
        slip[i] = calculate_slip(wheel_speeds[i], vehicle_speed);
    }
    
    // Apply ABS modulation
    for (int i = 0; i < 4; i++) {
        if (slip[i] > ABS_THRESHOLD) {
            motor_demand[i] *= 0.5;  // Reduce power
            abs_active[i] = true;
        }
    }
    
    // Send modulated throttle to STM32
    send_throttle_command(motor_demand);
}
```

**No STM32 Changes** (ABS logic on ESP32)

**Testing:**
1. Simulate wheel slip
2. Verify ABS activates
3. Check telemetry shows ABS status
4. Road test (if safe)

**Rollback:** N/A (ABS stays on ESP32)

---

## 📊 Migration Checklist

### Pre-Migration

- [ ] Backup current ESP32 firmware
- [ ] Document all GPIO assignments
- [ ] Create wiring diagram
- [ ] Order STM32 dev boards (2× for redundancy)
- [ ] Order CAN transceivers
- [ ] Set up version control branch `feature/dual-mcu`

### Phase Completion Criteria

Each phase requires:
- [ ] Code compiles without errors
- [ ] Both MCUs boot successfully
- [ ] Vehicle can move (even in degraded mode)
- [ ] Emergency stop works
- [ ] Telemetry visible
- [ ] Rollback procedure tested
- [ ] Code committed to Git

### Final Validation

- [ ] All sensors reading correctly
- [ ] All motors controlled via STM32
- [ ] CAN bus stable (no errors)
- [ ] Safety tests pass
- [ ] 1 hour continuous operation
- [ ] Road test in controlled environment
- [ ] Production PCB designed (optional)

---

## 🔧 Troubleshooting Guide

### Common Issues

**CAN Bus Not Working:**
- Check termination (120Ω at both ends)
- Verify baud rate (500 kbps)
- Check TX/RX pins not swapped
- Use logic analyzer to see traffic

**STM32 Not Responding:**
- Check power supply (3.3V stable)
- Verify crystal oscillator (8 MHz)
- Flash LED blink test first
- Use SWD debugger

**Motor Control Not Working:**
- Check HRTIM clock configuration
- Verify PWM frequency with oscilloscope
- Check BTS7960 enable signals
- Test with single motor first

**Sensors Not Reading:**
- Verify voltage levels (3.3V logic)
- Check pull-up resistors
- Test sensors individually
- Use multimeter/oscilloscope

---

## 🔄 Rollback Procedures

### Quick Rollback (During Development)

```bash
# Revert to previous ESP32-only firmware
git checkout main
pio run -t upload

# Reconnect hardware to ESP32
# (refer to original wiring diagram)
```

### Emergency Rollback (In Vehicle)

1. **Power off vehicle**
2. **Disconnect STM32 from CAN bus**
3. **Reconnect ESP32 to PCA9685, MCP23017, sensors**
4. **Flash previous firmware**
5. **Test basic operation**
6. **Resume when safe**

---

## 📋 Documentation Updates

As each phase completes, update:

1. **Wiring Diagrams:** Reflect new connections
2. **Pin Assignments:** Update GPIO tables
3. **CAN Protocol:** Add new messages as implemented
4. **Troubleshooting:** Add solutions found
5. **README:** Update architecture description

---

## 🎓 Lessons Learned (To Be Updated)

**What Worked:**
- (To be filled during implementation)

**What Didn't:**
- (To be filled during implementation)

**Recommendations:**
- (To be filled during implementation)

---

## 🔄 Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2026-01-13 | System Architect | Initial migration plan |

---

**Document Status:** ✅ APPROVED FOR EXECUTION  
**Risk Level:** Medium (phased approach mitigates risk)  
**Estimated Duration:** 10 weeks  
**Prerequisites:** Hardware availability, development environment setup
