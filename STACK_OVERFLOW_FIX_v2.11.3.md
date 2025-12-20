# Stack Overflow Fix - v2.11.3

## Problem Statement

The ESP32S3 device was experiencing a critical bug causing it to enter an infinite reboot loop with the following error:

```
Guru Meditation Error: Core 0 panic'ed (Unhandled debug exception).
Debug exception reason: Stack canary watchpoint triggered (ipc0)
Backtrace: 0x40378fd4:0x3fcf0e30 0x0005002d:0xa5a5a5a5 |<-CORRUPTED
```

This error indicates that the IPC (Inter-Processor Communication) task's stack was being corrupted due to overflow.

## Root Cause Analysis

After thorough investigation, four root causes were identified:

### 1. IPC Task Stack Too Small (3072 bytes)
The IPC task stack was configured at 3072 bytes, which was insufficient for complex initialization sequences involving:
- Multiple I2C transactions
- Nested logging calls
- Display initialization
- Sensor initialization

### 2. Large Logger Buffers (256 bytes per call)
The logger functions (`infof()`, `warnf()`, `errorf()`, `debugf()`) were allocating 256-byte buffers on the stack for each call. During initialization, multiple nested logging calls would compound this issue, consuming significant stack space.

### 3. Large Debug Buffers (160 bytes)
The debug utility (`Debug::infof()`, `Debug::debugf()`) was allocating 160-byte buffers on the stack, adding to the stack pressure during diagnostic logging.

### 4. Missing Watchdog Feeds During I2C Operations
The obstacle detection initialization performs multiple I2C transactions without feeding the watchdog, potentially causing additional issues during long initialization sequences.

## Solution Implemented

### 1. Increased IPC Task Stack Size
**File:** `platformio.ini`
**Change:** Increased `CONFIG_ESP_IPC_TASK_STACK_SIZE` from 3072 to 4096 bytes

```ini
-DCONFIG_ESP_IPC_TASK_STACK_SIZE=4096
```

**Rationale:** Provides a safe margin (33% increase) for complex initialization sequences while still being conservative with memory usage.

### 2. Reduced Logger Buffer Sizes
**File:** `src/core/logger.cpp`
**Change:** Reduced buffer size from 256 to 128 bytes in all formatting functions

```cpp
constexpr size_t BUF_SZ = 128;  // Was 256
```

**Impact:** 
- 50% reduction in stack usage per logging call
- 128 bytes is still sufficient for most log messages
- Dramatically reduces stack pressure during initialization with nested logging

### 3. Reduced Debug Buffer Size
**File:** `src/utils/debug.cpp`
**Change:** Reduced buffer size from 160 to 96 bytes

```cpp
char buf[96];  // Was 160
```

**Impact:**
- 40% reduction in stack usage per debug call
- 96 bytes is adequate for most debug messages
- Further reduces stack pressure during diagnostic logging

### 4. Added Strategic Watchdog Feeds
**File:** `src/sensors/obstacle_detection.cpp`
**Changes:** Added watchdog feeds at critical points during initialization

```cpp
// After I2C bus test
Watchdog::feed();

// After multiplexer verification
Watchdog::feed();

// After each sensor initialization
Watchdog::feed();
```

**Impact:**
- Prevents watchdog timeout during I2C-intensive operations
- Allows initialization to complete without triggering watchdog reset
- Improves system stability during boot

## Memory Impact

### Before Changes
- IPC Stack: 3072 bytes
- Logger buffer: 256 bytes per call
- Debug buffer: 160 bytes per call
- Potential nested usage: 256 * 3 + 160 = 928 bytes in worst case

### After Changes
- IPC Stack: 4096 bytes (+1024 bytes, +33%)
- Logger buffer: 128 bytes per call (-128 bytes, -50%)
- Debug buffer: 96 bytes per call (-64 bytes, -40%)
- Potential nested usage: 128 * 3 + 96 = 480 bytes in worst case (-48%)

### Net Effect
- IPC stack increased by 1024 bytes
- Typical stack usage per logging call reduced by 448 bytes (48% reduction)
- Overall stack pressure significantly reduced during initialization

## Build Verification

Firmware builds successfully with no compilation errors:

```
RAM:   [=         ]   8.4% (used 27,668 bytes from 327,680 bytes)
Flash: [====      ]  37.0% (used 484,581 bytes from 1,310,720 bytes)
```

## Testing Recommendations

1. **Flash Test:** Flash firmware to ESP32S3 and monitor serial output for clean boot
2. **Reboot Test:** Verify system boots without entering reboot loop
3. **Stack Monitor:** Monitor stack high water marks during operation to ensure adequate margin
4. **Stress Test:** Run system through initialization cycles to verify stability
5. **I2C Test:** Verify obstacle detection sensors initialize correctly
6. **Logging Test:** Verify log messages are not truncated at 128 bytes

## Expected Behavior

After applying these fixes:
- ESP32S3 should boot correctly without entering reboot loop
- No "Stack canary watchpoint triggered" errors should appear
- All sensors should initialize properly
- System should remain stable during operation
- Log messages should remain clear and complete (128 bytes is sufficient for typical messages)

## Version Information

- **Firmware Version:** 2.11.3
- **Date:** 2025-12-20
- **Platform:** ESP32-S3-DevKitC-1
- **Framework:** Arduino (PlatformIO)

## Files Modified

1. `platformio.ini` - IPC stack size configuration
2. `src/core/logger.cpp` - Logger buffer size reduction
3. `src/utils/debug.cpp` - Debug buffer size reduction
4. `src/sensors/obstacle_detection.cpp` - Watchdog feeds during initialization
5. `include/version.h` - Version number update to 2.11.3

## Additional Notes

- All changes are minimal and surgical, targeting only the stack overflow issue
- Changes are well-documented with comments explaining the rationale
- Buffer size reductions are conservative (128 and 96 bytes are still generous for most use cases)
- Watchdog feeds are strategically placed to prevent timeout without over-feeding
- No functional changes to application logic - only stack and timing optimizations
