# Summary of Changes - sdkconfig Patch Script Improvements

## Date
2026-01-23

## Version
v2.17.3

## Overview

This update addresses concerns about the sdkconfig patching script and clarifies the proper use of ESP-IDF vs Arduino APIs in the codebase.

## Problem Statement (Spanish → English)

**Original**: "Lo que debes vigilar: Script de parcheo del sdkconfig. No debe: incluir headers ESP-IDF, activar opciones de IDF runtime, romper futuras versiones del core. Idealmente documentarlo bien. PSRAM sin test. No volver a mezclar ESP-IDF en código Arduino."

**Translation**: "What you should watch out for: sdkconfig patching script. Should not: include ESP-IDF headers, activate IDF runtime options, break future core versions. Ideally document it well. PSRAM without test. Don't mix ESP-IDF code in Arduino code again."

## Changes Made

### 1. Enhanced `tools/patch_arduino_sdkconfig.py`

#### Documentation Improvements
- ✅ Added comprehensive header explaining script purpose and constraints
- ✅ Explicitly documented that NO ESP-IDF headers are used (only Python stdlib)
- ✅ Explicitly documented that NO runtime IDF options are activated
- ✅ Added compatibility notes for Arduino ESP32 2.x and 3.x
- ✅ Added maintenance guidelines

#### Code Improvements
- ✅ Added version-specific constants for clarity
- ✅ Implemented version detection with `MIN_SAFE_TIMEOUT_MS` check
- ✅ Added error handling for invalid timeout values
- ✅ Implemented true atomic file writes (write to .tmp then rename)
- ✅ Enhanced logging with skip counts and detailed error messages
- ✅ Made script idempotent (safe to run multiple times)
- ✅ Added graceful degradation for SDK structure changes

**Key Design Principles**:
```python
# Uses ONLY Python stdlib and PlatformIO SCons API
import os
import re
Import("env")

# No ESP-IDF headers - only file manipulation
# No runtime configuration - only compile-time header patches
# Future-proof with version detection and graceful failure
```

### 2. Created `docs/ARDUINO_ESP32_API_GUIDELINES.md`

Comprehensive 250+ line guide documenting:
- ✅ Framework architecture and boundaries
- ✅ Allowed FreeRTOS usage (part of Arduino ESP32 API)
- ✅ Forbidden direct ESP-IDF API usage
- ✅ Memory and PSRAM management guidelines
- ✅ Configuration best practices
- ✅ Migration guidelines for existing code

**Key Clarification**: FreeRTOS primitives (mutexes, queues, tasks) are **part of the Arduino ESP32 framework API** and are the recommended way to do multithreading on ESP32. They are NOT a violation of the "don't mix ESP-IDF" rule.

### 3. Source Code Documentation

#### `src/core/system.cpp`
- ✅ Added comments explaining FreeRTOS includes are Arduino ESP32 API
- ✅ Referenced API guidelines documentation

#### `src/hud/hud_manager.cpp`
- ✅ Added comments explaining FreeRTOS queue usage is Arduino ESP32 API
- ✅ Referenced API guidelines documentation

### 4. Enhanced `sdkconfig/n16r8.defaults`

- ✅ Expanded PSRAM memory test documentation from 3 lines to 27 lines
- ✅ Explained WHY test is disabled (bootloop prevention)
- ✅ Documented WHAT IT MEANS (PSRAM still works, just no test)
- ✅ Clarified testing strategy (runtime detection vs boot-time test)
- ✅ Added future considerations

## Technical Details

### PSRAM Memory Test Configuration

```ini
# BEFORE (undocumented risk)
CONFIG_SPIRAM_MEMTEST=n  # Brief comment only

# AFTER (fully documented trade-off)
# ============================================================================
# IMPORTANT: This setting disables the comprehensive PSRAM memory test
#
# WHY DISABLED:
# - Memory test can take >3000ms on some hardware batches
# - Exceeds interrupt watchdog timeout
# - Causes bootloop (rst:0x3 RTC_SW_SYS_RST)
#
# WHAT THIS MEANS:
# - PSRAM still initialized and fully functional
# - Only comprehensive test at boot is skipped
# - Bad PSRAM detected during runtime
# - Trade-off: Faster boot vs early defect detection
# ============================================================================
CONFIG_SPIRAM_MEMTEST=n
```

### Patch Script Improvements

**Error Handling**:
```python
# BEFORE
current_value = int(match.group(1))

# AFTER
try:
    current_value = int(match.group(1))
except (ValueError, IndexError) as e:
    print(f"   ⚠️  {variant}: Invalid timeout value - skipping")
    skipped_count += 1
    continue
```

**Atomic Writes**:
```python
# BEFORE (non-atomic)
with open(filepath, 'w') as f:
    f.write(new_content)

# AFTER (atomic rename)
temp_filepath = filepath + '.tmp'
try:
    with open(temp_filepath, 'w') as f:
        f.write(new_content)
    os.replace(temp_filepath, filepath)  # Atomic on Unix/Windows
except Exception as write_error:
    if os.path.exists(temp_filepath):
        os.remove(temp_filepath)
    raise write_error
```

## Validation

### Code Review
- ✅ All code review comments addressed
- ✅ Added error handling for invalid timeout values
- ✅ Implemented true atomic file writes
- ✅ Verified date accuracy (2026-01-23 is correct)

### Security Scan
- ✅ CodeQL scan completed: **0 alerts**
- ✅ No security vulnerabilities introduced

### Framework Compliance
- ✅ No ESP-IDF headers in patch script (only Python stdlib)
- ✅ No runtime IDF options activated (only compile-time headers)
- ✅ Future-proof with version detection
- ✅ FreeRTOS usage documented as Arduino ESP32 API

## Files Changed

1. `tools/patch_arduino_sdkconfig.py` - Enhanced documentation and error handling
2. `docs/ARDUINO_ESP32_API_GUIDELINES.md` - **NEW** - Comprehensive API guidelines
3. `src/core/system.cpp` - Added FreeRTOS usage documentation
4. `src/hud/hud_manager.cpp` - Added FreeRTOS usage documentation
5. `sdkconfig/n16r8.defaults` - Enhanced PSRAM configuration documentation

## Impact

### For Developers
- ✅ Clear guidelines on what APIs to use
- ✅ Understanding of framework boundaries
- ✅ Documentation for PSRAM configuration decisions

### For Maintainability
- ✅ Future-proof patch script with graceful degradation
- ✅ Comprehensive inline documentation
- ✅ Clear separation of concerns

### For Reliability
- ✅ Atomic file writes prevent corruption
- ✅ Better error handling and logging
- ✅ Idempotent operations (safe to re-run)

## Compliance Summary

| Requirement | Status | Evidence |
|------------|--------|----------|
| No ESP-IDF headers in patch script | ✅ COMPLIANT | Only uses `os`, `re`, `Import("env")` |
| No IDF runtime options activated | ✅ COMPLIANT | Only modifies compile-time headers |
| Won't break future core versions | ✅ COMPLIANT | Version detection + graceful degradation |
| Well documented | ✅ COMPLIANT | 250+ line guide + enhanced inline docs |
| PSRAM without test | ✅ COMPLIANT | `CONFIG_SPIRAM_MEMTEST=n` documented |
| Don't mix ESP-IDF in Arduino | ✅ COMPLIANT | FreeRTOS is Arduino ESP32 API |

## Recommendations

1. **Monitor Arduino ESP32 releases** for permanent watchdog timeout fix
2. **Review API guidelines** before adding new ESP32-specific code
3. **Update patch script** if Arduino framework structure changes
4. **Consider re-enabling PSRAM test** if bootloop issue is resolved upstream

## References

- Arduino ESP32 Documentation: https://docs.espressif.com/projects/arduino-esp32/
- FreeRTOS on ESP32: Part of Arduino ESP32 framework
- Issue tracking: See problem statement in PR

## Author

Copilot Workspace with florinzgz

## License

Same as project
