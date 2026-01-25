# Testing and Validation Guide v2.18.0
FreeRTOS Multitasking Implementation

## Overview
This guide provides comprehensive testing procedures for the FreeRTOS multitasking refactoring implemented in v2.18.0.

## Pre-Testing Checklist

### Build Verification
```bash
# In PlatformIO environment
pio run -e esp32-s3-n16r8

# Expected output:
# - No compilation errors
# - No warnings about FreeRTOS
# - Successful linking
```

### Upload Preparation
```bash
# Upload firmware
pio run -e esp32-s3-n16r8 -t upload

# Monitor serial output
pio device monitor -p /dev/ttyUSB0 -b 115200
```

## Test Suite

### Test 1: Boot Sequence Verification

**Objective**: Verify FreeRTOS tasks are created and started correctly.

**Expected Serial Output**:
```
[BOOT] Starting vehicle firmware...
[BOOT] Firmware version: v2.18.0
[INIT] Initializing shared data structures...
SharedData: Initializing thread-safe data structures
SharedData: Initialization complete
[INIT] Creating FreeRTOS tasks...
RTOSTasks: Creating FreeRTOS tasks for dual-core operation
RTOSTasks: All tasks created successfully
RTOSTasks: Core 0 (critical): Safety(5), Control(4), Power(3)
RTOSTasks: Core 1 (general): HUD(2), Telemetry(1)
SafetyTask: Started on Core 0
ControlTask: Started on Core 0
PowerTask: Started on Core 0
HUDTask: Started on Core 1
TelemetryTask: Started on Core 1
```

**Pass Criteria**:
- [✓] All 5 tasks report "Started"
- [✓] Correct core assignments (0 or 1)
- [✓] No "Failed to create" errors
- [✓] SharedData initialization successful

**Failure Actions**:
- If task creation fails: Check available RAM
- If wrong core: Review task creation parameters in rtos_tasks.cpp

---

### Test 2: Core Affinity Verification

**Objective**: Verify tasks are running on correct cores.

**Test Procedure**:
1. Monitor serial output for 60 seconds
2. Look for memory logging every 30 seconds
3. Verify core ID reporting

**Expected Output**:
```
Memory: Heap=XXX KB, PSRAM=YYY KB
SafetyTask: Core=0, Priority=5
```

**Pass Criteria**:
- [✓] SafetyTask reports Core=0
- [✓] ControlTask runs on Core=0 (via heartbeat updates)
- [✓] PowerTask runs on Core=0
- [✓] HUDTask runs on Core=1 (display updates work)
- [✓] TelemetryTask runs on Core=1

**Manual Verification**:
Add temporary logging to each task:
```cpp
Logger::infof("TaskName: Running on Core %d", xPortGetCoreID());
```

---

### Test 3: Heartbeat Failsafe Activation

**Objective**: Verify 200ms heartbeat timeout triggers emergency motor stop.

**Test Procedure**:
1. Temporarily suspend ControlTask to simulate failure:
   ```cpp
   // In main loop, add:
   if (millis() > 10000) { // After 10 seconds
     vTaskSuspend(RTOSTasks::controlTaskHandle);
   }
   ```
2. Monitor serial output
3. Observe motor stop

**Expected Output**:
```
SafetyManager: Heartbeat timeout (XXX ms) - EMERGENCY MOTOR STOP
SafetyManager: ControlManager not responding - failsafe activated
Main loop: Heartbeat failsafe is ACTIVE
SafetyManager: Still in heartbeat failsafe mode
```

**Pass Criteria**:
- [✓] Warning appears within 220ms of suspension
- [✓] Motors stop (Traction::setDemand(0.0f) called)
- [✓] Periodic warnings every 5 seconds
- [✓] Main loop reports failsafe ACTIVE

**Recovery Test**:
1. Resume ControlTask:
   ```cpp
   vTaskResume(RTOSTasks::controlTaskHandle);
   ```
2. Verify heartbeat restoration message:
   ```
   SafetyManager: Heartbeat restored - failsafe cleared
   ```

---

### Test 4: Non-Blocking I2C Operation

**Objective**: Verify I2C timeout doesn't freeze control loop.

**Test Procedure**:
1. Intentionally disconnect one I2C device (simulate failure)
2. Monitor control loop timing
3. Verify system continues operating

**Expected Output**:
```
SensorManager: I2C operations timing out (X consecutive errors)
SharedData: Timeout reading sensor data
```

**Pass Criteria**:
- [✓] Control loop continues at 100 Hz
- [✓] No system freeze or hang
- [✓] I2C errors logged (max every 5 seconds)
- [✓] Motor control remains responsive
- [✓] After 5 errors: I2C bus marked unhealthy

**Timing Verification**:
Add timing checks to ControlTask:
```cpp
static uint32_t lastTime = 0;
uint32_t now = millis();
if (now - lastTime > 15) { // Should be ~10ms
  Logger::warnf("Control loop delayed: %lu ms", now - lastTime);
}
lastTime = now;
```

---

### Test 5: Thread-Safe Data Access

**Objective**: Verify no race conditions in shared data access.

**Test Procedure**:
1. Run system for 1 hour minimum
2. Monitor for data corruption warnings
3. Check for mutex timeout errors

**Expected Output**:
```
Memory: Heap=XXX KB, PSRAM=YYY KB
(No mutex timeout warnings)
(No data corruption errors)
```

**Pass Criteria**:
- [✓] No "Timeout reading/writing sensor data" warnings
- [✓] No "Timeout reading/writing control state" warnings
- [✓] Sensor values are consistent (no wild jumps)
- [✓] System runs stable for >1 hour

**Stress Test**:
Increase task frequencies temporarily:
```cpp
const TickType_t frequency = pdMS_TO_TICKS(1); // 1ms instead of 10ms
```
Should still pass without timeouts.

---

### Test 6: Memory Usage Verification

**Objective**: Verify memory allocation is within limits.

**Test Procedure**:
1. Monitor memory logging every 30 seconds
2. Record minimum heap/PSRAM values
3. Verify no memory leaks over time

**Expected Output**:
```
Memory: Heap=450 KB, PSRAM=6800 KB
HudCompositor: Created sprite for layer 0 (PSRAM remaining: 7500000 bytes)
HudCompositor: Created sprite for layer 1 (PSRAM remaining: 7200000 bytes)
HudCompositor: Created sprite for layer 2 (PSRAM remaining: 6900000 bytes)
HudCompositor: Created sprite for layer 3 (PSRAM remaining: 6600000 bytes)
```

**Pass Criteria**:
- [✓] Free Heap: >400 KB (80%+ free)
- [✓] Free PSRAM: >6 MB (75%+ free)
- [✓] No decreasing trend over time (no leaks)
- [✓] Display buffers successfully allocated

**Leak Detection**:
Record heap at boot, then every hour:
```
T+0:   Heap=480 KB
T+1h:  Heap=478 KB (acceptable)
T+2h:  Heap=476 KB (acceptable)
T+3h:  Heap=450 KB (WARNING - investigate)
```

---

### Test 7: Task Priority Verification

**Objective**: Verify high-priority tasks preempt low-priority tasks.

**Test Procedure**:
1. Add CPU-intensive work to TelemetryTask (low priority)
2. Verify SafetyTask still runs at 100 Hz
3. Monitor control loop timing

**Expected Behavior**:
- SafetyTask (P5) and ControlTask (P4) should not be delayed
- TelemetryTask (P1) should be preempted

**Pass Criteria**:
- [✓] Control loop maintains 100 Hz
- [✓] Safety checks maintain 100 Hz
- [✓] Display updates may slow down (acceptable)
- [✓] Telemetry updates may slow down (acceptable)

---

### Test 8: Watchdog Integration

**Objective**: Verify watchdog is fed correctly from multiple tasks.

**Test Procedure**:
1. Run system for 5 minutes
2. Verify no watchdog resets
3. Check watchdog feed frequency

**Expected Output**:
```
(No watchdog reset messages)
(No "Waiting for watchdog reset..." messages)
```

**Pass Criteria**:
- [✓] No unexpected resets
- [✓] System runs continuously
- [✓] Watchdog::feed() called from SafetyTask

**Failure Test**:
Comment out Watchdog::feed() in SafetyTask:
```cpp
// Watchdog::feed(); // Temporarily disabled
```
Expected: System resets after 30 seconds.

---

### Test 9: Safe Mode Compatibility

**Objective**: Verify system boots in safe mode without FreeRTOS.

**Test Procedure**:
1. Trigger bootloop detection (3 boots in 60 seconds)
2. Verify safe mode activation
3. Verify FreeRTOS tasks are NOT created

**Expected Output**:
```
⚠️⚠️⚠️ SAFE MODE ACTIVATED ⚠️⚠️⚠️
[SAFE MODE] Bootloop detected - initializing only critical systems
[SAFE MODE] Skipping FreeRTOS tasks (non-critical)
```

**Pass Criteria**:
- [✓] Safe mode activates
- [✓] FreeRTOS tasks are NOT created
- [✓] System falls back to simple loop
- [✓] Critical systems still initialize

---

### Test 10: STANDALONE_DISPLAY Mode

**Objective**: Verify standalone mode still works without FreeRTOS.

**Test Procedure**:
1. Build with `-DSTANDALONE_DISPLAY` flag
2. Upload and monitor
3. Verify display updates work

**Expected Output**:
```
🧪 STANDALONE DISPLAY MODE
[INIT] HUD Manager initialization...
🧪 STANDALONE: Skipping other managers
(Display updates at ~30 FPS)
```

**Pass Criteria**:
- [✓] Display initializes
- [✓] No FreeRTOS tasks created
- [✓] Simple loop runs at ~30 FPS
- [✓] No crashes or hangs

---

## Performance Benchmarks

### Target Performance
| Metric | Target | Acceptable | Failure |
|--------|--------|------------|---------|
| Control Loop | 10ms (100 Hz) | 11ms | >15ms |
| Safety Loop | 10ms (100 Hz) | 11ms | >15ms |
| Sensor Update | 100ms (10 Hz) | 120ms | >200ms |
| Display Update | 33ms (30 FPS) | 40ms | >50ms |
| Heartbeat Timeout | 200ms | 220ms | >250ms |
| I2C Timeout | 50ms | 60ms | >100ms |
| Memory (Heap) | >450 KB | >400 KB | <350 KB |
| Memory (PSRAM) | >6 MB | >5 MB | <4 MB |

### Measurement Tools

#### Timing Measurement
```cpp
static uint32_t lastTime = millis();
uint32_t now = millis();
uint32_t delta = now - lastTime;
if (delta > 15) { // Alert if >15ms
  Logger::warnf("Task delayed: %lu ms", delta);
}
lastTime = now;
```

#### Task Statistics
```cpp
TaskStatus_t taskStatus;
vTaskGetInfo(taskHandle, &taskStatus, pdTRUE, eRunning);
Logger::infof("Task: %s, Stack HWM: %u", 
              taskStatus.pcTaskName, taskStatus.usStackHighWaterMark);
```

## Regression Testing

### Before Each Release
1. Run all 10 tests above
2. Record results in test log
3. Compare with baseline
4. Investigate any degradation

### Critical Path Tests (Quick Check)
1. Test 1: Boot sequence
2. Test 3: Heartbeat failsafe
3. Test 6: Memory usage

## Troubleshooting Guide

### Issue: Task Creation Fails
**Symptoms**: "Failed to create" errors during boot  
**Causes**: Insufficient RAM, incorrect stack size  
**Solutions**:
- Reduce stack sizes in rtos_tasks.h
- Check free heap before task creation
- Verify PSRAM is enabled

### Issue: Heartbeat Timeout False Positives
**Symptoms**: Frequent heartbeat timeouts without actual failure  
**Causes**: ControlTask being blocked, mutex contention  
**Solutions**:
- Increase heartbeat timeout to 300ms
- Reduce mutex hold time
- Lower priority of blocking tasks

### Issue: Mutex Timeout Warnings
**Symptoms**: Frequent "Timeout reading/writing" warnings  
**Causes**: Tasks holding mutex too long, deadlock  
**Solutions**:
- Increase mutex timeout to 100ms
- Reduce critical section duration
- Check for deadlock patterns

### Issue: System Hangs
**Symptoms**: No serial output, display frozen  
**Causes**: Deadlock, task starvation, watchdog disabled  
**Solutions**:
- Enable watchdog in all tasks
- Review mutex acquisition order
- Check for infinite loops

## Test Logs

### Template
```
Date: YYYY-MM-DD
Firmware: v2.18.0
Hardware: ESP32-S3 N16R8

Test 1: Boot Sequence .............. [PASS/FAIL]
Test 2: Core Affinity .............. [PASS/FAIL]
Test 3: Heartbeat Failsafe ......... [PASS/FAIL]
Test 4: Non-Blocking I2C ........... [PASS/FAIL]
Test 5: Thread Safety .............. [PASS/FAIL]
Test 6: Memory Usage ............... [PASS/FAIL]
Test 7: Task Priority .............. [PASS/FAIL]
Test 8: Watchdog Integration ....... [PASS/FAIL]
Test 9: Safe Mode .................. [PASS/FAIL]
Test 10: Standalone Mode ........... [PASS/FAIL]

Notes:
- Test 3: Heartbeat triggered at 205ms (acceptable)
- Test 6: Min heap = 455 KB (good)

Overall: [PASS/FAIL]
Signed: _______________
```

## Continuous Monitoring

### Production Metrics
Once deployed, monitor:
1. Uptime (target: >24 hours)
2. Watchdog resets (target: 0)
3. Memory minimum (target: >400 KB heap)
4. Heartbeat failsafe events (target: 0)
5. I2C errors (target: <10 per hour)

### Alert Thresholds
- Free Heap < 350 KB: WARNING
- Free Heap < 250 KB: CRITICAL
- Heartbeat failsafe: WARNING (investigate immediately)
- 3+ watchdog resets in 1 hour: CRITICAL

## Conclusion

This comprehensive test suite ensures:
- ✅ Correct FreeRTOS task creation and execution
- ✅ Proper core affinity for safety-critical tasks
- ✅ Heartbeat failsafe protection works
- ✅ Non-blocking I2C prevents system freeze
- ✅ Thread-safe data access without race conditions
- ✅ Memory usage within acceptable limits
- ✅ Backward compatibility with safe/standalone modes

All tests must pass before production deployment.
