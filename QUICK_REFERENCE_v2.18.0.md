# Quick Reference - FreeRTOS Multitasking v2.18.0

## At a Glance

### What Changed
✅ **Single-threaded** → **FreeRTOS dual-core multitasking**  
✅ **Blocking I2C** → **50ms timeout, non-blocking**  
✅ **No motor protection** → **200ms heartbeat failsafe**  
✅ **Manual loop timing** → **vTaskDelayUntil() precision**  

### Key Files Created
1. `include/rtos_tasks.h` - Task definitions
2. `src/core/rtos_tasks.cpp` - Task implementation
3. `include/shared_data.h` - Thread-safe data
4. `src/core/shared_data.cpp` - Data implementation
5. `src/managers/SafetyManagerEnhanced.cpp` - Heartbeat failsafe
6. `src/managers/SensorManagerEnhanced.cpp` - Non-blocking I2C

### Key Files Modified
1. `src/main.cpp` - FreeRTOS integration
2. `src/managers/SafetyManager.h` - Enhanced functions
3. `src/managers/SensorManager.h` - Enhanced functions

## Task Architecture

```
┌─────────────────────────────────────────────┐
│         ESP32-S3 Dual Core System           │
├─────────────────────┬───────────────────────┤
│   Core 0 (Critical) │   Core 1 (General)    │
│   @ 240 MHz         │   @ 240 MHz           │
├─────────────────────┼───────────────────────┤
│ SafetyTask  (P5)    │ HUDTask (P2)          │
│ - 100 Hz            │ - 30 Hz               │
│ - ABS/TCS           │ - Display             │
│ - Heartbeat Monitor │ - Touch Input         │
│                     │                       │
│ ControlTask (P4)    │ TelemetryTask (P1)    │
│ - 100 Hz            │ - 10 Hz               │
│ - Motors            │ - Logging             │
│ - Heartbeat Update  │ - Telemetry           │
│                     │                       │
│ PowerTask   (P3)    │                       │
│ - 10 Hz             │                       │
│ - Power Mgmt        │                       │
│ - Sensors (I2C)     │                       │
└─────────────────────┴───────────────────────┘
```

## Critical Parameters

| Parameter | Value | Location |
|-----------|-------|----------|
| Heartbeat Timeout | 200ms | SafetyManagerEnhanced.cpp:10 |
| I2C Timeout | 50ms | SensorManagerEnhanced.cpp:8 |
| Safety Task Frequency | 100 Hz | rtos_tasks.cpp:118 |
| Control Task Frequency | 100 Hz | rtos_tasks.cpp:133 |
| Sensor Update Frequency | 10 Hz | rtos_tasks.cpp:158 |
| HUD Update Frequency | 30 Hz | rtos_tasks.cpp:178 |
| Mutex Timeout | 50ms | shared_data.cpp:48,62,76,90 |

## Emergency Stops

### Heartbeat Failsafe
**Trigger**: ControlManager not responding for >200ms  
**Action**:
```cpp
Traction::setDemand(0.0f);        // Stop traction motors
SteeringMotor::setDemandAngle(0.0f); // Stop steering motor
```
**Recovery**: Automatic when heartbeat resumes

### I2C Failure
**Trigger**: 5 consecutive I2C timeouts  
**Action**: Mark I2C bus unhealthy, continue with degraded data  
**Recovery**: Automatic when I2C responds

## Memory Usage

### Internal RAM (512 KB total)
- Used: ~33 KB (6.4%)
- Free: ~479 KB (93.6%) ✅

**Breakdown**:
- Task Stacks: 22.25 KB
- Shared Data: 0.42 KB
- FastLED: 0.13 KB
- System: ~10 KB

### PSRAM (8 MB total)
- Used: ~1.17 MB (14.6%)
- Free: ~6.83 MB (85.4%) ✅

**Breakdown**:
- Display Layer 0: 307.2 KB
- Display Layer 1: 307.2 KB
- Display Layer 2: 307.2 KB
- Display Layer 3: 307.2 KB

## How to Build & Test

### Build
```bash
pio run -e esp32-s3-n16r8
```

### Upload
```bash
pio run -e esp32-s3-n16r8 -t upload
```

### Monitor
```bash
pio device monitor -p /dev/ttyUSB0 -b 115200
```

### Expected Boot Messages
```
[BOOT] Starting vehicle firmware...
[INIT] Initializing shared data structures...
SharedData: Initialization complete
[INIT] Creating FreeRTOS tasks...
RTOSTasks: All tasks created successfully
SafetyTask: Started on Core 0
ControlTask: Started on Core 0
PowerTask: Started on Core 0
HUDTask: Started on Core 1
TelemetryTask: Started on Core 1
```

## Quick Tests

### Test 1: Boot Check (30 seconds)
✅ All 5 tasks report "Started"  
✅ No "Failed to create" errors  
✅ Memory logs appear

### Test 2: Heartbeat Check (Manual)
1. Comment out heartbeat update in ControlTask
2. Upload and monitor
3. Should see: "Heartbeat timeout - EMERGENCY MOTOR STOP" within 220ms

### Test 3: I2C Check (Manual)
1. Disconnect one I2C device
2. Monitor serial output
3. Should see: "I2C operations timing out" after 5 errors
4. System should continue operating

## Troubleshooting

### "Failed to create task"
- **Cause**: Insufficient RAM
- **Fix**: Reduce stack sizes in rtos_tasks.h

### "Heartbeat timeout" on normal operation
- **Cause**: ControlTask blocked or delayed
- **Fix**: Check for mutex contention, increase timeout to 300ms

### "Timeout reading/writing sensor data"
- **Cause**: Mutex held too long
- **Fix**: Increase mutex timeout to 100ms

### Build errors about FreeRTOS
- **Cause**: Platform version mismatch
- **Fix**: Verify platformio.ini uses espressif32 platform

## Performance Expectations

| Metric | Target | Acceptable | Critical |
|--------|--------|------------|----------|
| Control Loop | 10ms | 11ms | >15ms |
| Free Heap | >450 KB | >400 KB | <350 KB |
| Free PSRAM | >6 MB | >5 MB | <4 MB |
| Heartbeat Response | 200ms | 220ms | >250ms |

## Backward Compatibility

### Safe Mode ✅
- FreeRTOS tasks NOT created
- Falls back to simple loop
- Critical systems only

### STANDALONE_DISPLAY ✅
- FreeRTOS tasks NOT created
- Only HUD updates at 30 FPS
- No motor control

## Documentation

For detailed information, see:
1. **FREERTOS_ARCHITECTURE_v2.18.0.md** - Complete architecture
2. **MEMORY_OPTIMIZATION_REPORT_v2.18.0.md** - Memory analysis
3. **TESTING_VALIDATION_GUIDE_v2.18.0.md** - Full test suite
4. **AUDITORIA_SEGURIDAD_RENDIMIENTO_v2.18.0.md** - Executive summary

## Next Steps

1. ✅ **Implementation Complete** - All code written
2. ⏳ **Build Verification** - Compile with PlatformIO
3. ⏳ **Hardware Upload** - Load onto ESP32-S3
4. ⏳ **Validation Suite** - Run 10 tests from guide
5. ⏳ **Extended Runtime** - Monitor for 24+ hours
6. ⏳ **Production Deploy** - After all tests pass

## Questions?

Check the full documentation or search for these keywords in the code:
- `RTOSTasks::` - Task management
- `SharedData::` - Thread-safe data
- `updateWithHeartbeat()` - Failsafe mechanism
- `updateNonBlocking()` - Non-blocking I2C
- `PSRAM_ENABLE` - Memory allocation

---
**Version**: v2.18.0  
**Date**: 2026-01-25  
**Status**: ✅ Implementation Complete, ⏳ Hardware Validation Pending
