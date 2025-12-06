# ESP32-S3 Stack Overflow Fix

## Problem Description

When compiling and uploading the firmware with `pio run -e esp32-s3-devkitc-test -t upload --upload-port COM4`, the device experienced a stack overflow crash:

```
Guru Meditation Error: Core 0 panic'ed (Unhandled debug exception). 
Debug exception reason: Stack canary watchpoint triggered (ipc0)
```

This error indicates that the stack was corrupted due to overflow, triggering the stack canary watchpoint on the IPC (Inter-Processor Communication) task running on Core 0.

## Root Cause

The ESP32-S3 default stack sizes were insufficient for this complex firmware, especially in test mode where multiple features are enabled simultaneously:

- **Default Arduino Loop Stack**: 8192 bytes (8 KB)
- **Default Main Task Stack**: 4096 bytes (4 KB)

The `esp32-s3-devkitc-test` environment has the following features enabled:
- `TEST_MODE` - Testing mode
- `STANDALONE_DISPLAY` - Standalone display mode
- `TEST_ALL_LEDS` - Test all LED functionality
- `TEST_ALL_SENSORS` - Test all sensor functionality
- `CORE_DEBUG_LEVEL=5` - Maximum debug output

All these features running simultaneously during initialization caused stack overflow.

## Solution

Increased the stack sizes for both the base environment and test environment:

### Base Environment (`esp32-s3-devkitc`)
```ini
-DCONFIG_ARDUINO_LOOP_STACK_SIZE=12288  ; Increased from 8192 to 12288 (12 KB)
-DCONFIG_ESP_MAIN_TASK_STACK_SIZE=8192  ; Increased from 4096 to 8192 (8 KB)
```

### Test Environment (`esp32-s3-devkitc-test`)
```ini
-DCONFIG_ARDUINO_LOOP_STACK_SIZE=16384  ; Increased from 8192 to 16384 (16 KB)
-DCONFIG_ESP_MAIN_TASK_STACK_SIZE=8192  ; Increased from 4096 to 8192 (8 KB)
```

The test environment gets a larger Arduino loop stack (16 KB) due to the additional testing features.

## Technical Details

### What is a Stack Canary?

A stack canary is a security feature that helps detect stack buffer overflows:
1. A known value (the "canary") is placed on the stack between local variables and return addresses
2. Before a function returns, the canary value is checked
3. If the canary has been modified, it indicates a stack overflow occurred
4. The system triggers a panic to prevent potential security exploits or crashes

### IPC Task

The IPC (Inter-Processor Communication) task is used by ESP32-S3 for communication between cores. When the stack canary is triggered on this task, it typically indicates that initialization code on Core 0 consumed too much stack space.

## Impact

- **Stack Memory Allocation**: The increase in stack sizes reserves additional RAM for stack usage:
  - Base environment: Increases from 12 KB (8KB loop + 4KB main) to 20 KB (12KB loop + 8KB main) = +8 KB
  - Test environment: Increases from 12 KB (8KB loop + 4KB main) to 24 KB (16KB loop + 8KB main) = +12 KB
- **ESP32-S3 has 512 KB SRAM total** (actually 327,680 bytes available), so this increase is acceptable
- The reserved stack space is pre-allocated at task creation but may not all be used

## Verification

Both environments build successfully after the fix:

- **`esp32-s3-devkitc`** (Full mode with all sensors): RAM usage 17.4% (57,148 bytes), Flash 74.0%
- **`esp32-s3-devkitc-test`** (Standalone display mode): RAM usage 9.0% (29,360 bytes), Flash 39.4%

Note: The test environment shows lower total RAM usage because it runs in `STANDALONE_DISPLAY` mode, which skips sensor initialization and other hardware components. However, it still needs the larger stack during initialization to handle test features safely.

The firmware should now boot without stack overflow errors.

## References

- [ESP-IDF Stack Configuration](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/performance/ram-usage.html)
- [Arduino-ESP32 Configuration](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/arduino.html)
- [FreeRTOS Stack Overflow Detection](https://www.freertos.org/Stacks-and-stack-overflow-checking.html)
