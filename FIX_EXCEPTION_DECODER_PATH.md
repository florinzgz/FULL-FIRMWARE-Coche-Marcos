# Fix: Exception Decoder Path Mismatch - 2026-01-27

## 🔧 Problem

When uploading firmware to ESP32-S3, the serial monitor showed an error:
```
Esp32ExceptionDecoder: firmware at c:\FIRMWARE-Coche-Marcos\.pio\build\esp32-s3-n16r8\firmware.elf does not exist, rebuild the project?
```

Additionally, the device was boot-looping (continuously resetting):
```
ESP-ROM:esp32s3-20210327
Build:Mar 27 2021
rst:0x3 (RTC_SW_SYS_RST),boot:0x8 (SPI_FAST_FLASH_BOOT)
```

## 🎯 Root Cause

**Path Mismatch Between Build and Monitor:**
- The firmware was built for environment `esp32-s3-devkitc-1`
  - Build output: `.pio/build/esp32-s3-devkitc-1/firmware.elf`
- But the serial monitor (without `-e` flag) defaulted to the first environment in `platformio.ini`: `esp32-s3-n16r8`
  - Expected path: `.pio/build/esp32-s3-n16r8/firmware.elf` ❌

This caused the exception decoder to fail, making it impossible to debug crashes causing the boot loop.

## ✅ Solution Implemented

**Set default environment in platformio.ini:**

```ini
[platformio]
boards_dir = boards
default_envs = esp32-s3-devkitc-1  # ← ADDED THIS LINE
```

### Why `esp32-s3-devkitc-1`?

According to the platformio.ini configuration (line 143):
```ini
; ===================================================================
; STANDARD ESP32-S3 DEVKITC-1 BOARD
; Flash: 16MB QD, PSRAM: 8MB OT
; Solution for reboot issues - esta es la solución para los reinicios
; ===================================================================
[env:esp32-s3-devkitc-1]
```

This environment is **the official solution for reboot issues**.

## 📦 How to Use After Fix

Now you can build and monitor without specifying the environment explicitly:

```bash
# Build (uses esp32-s3-devkitc-1 by default)
pio run

# Build and upload
pio run -t upload

# Monitor (will correctly find firmware at .pio/build/esp32-s3-devkitc-1/firmware.elf)
pio device monitor
```

### Optional: Explicitly Specify Environment

You can still use other environments if needed:

```bash
# Use the N16R8 custom board
pio run -e esp32-s3-n16r8

# Monitor with specific environment
pio device monitor -e esp32-s3-n16r8
```

## 🔍 What This Fix Enables

With the exception decoder working correctly:
1. **Stack traces are decoded** - You'll see function names and line numbers instead of raw addresses
2. **Easier debugging** - Crashes will show readable error messages
3. **Find boot loop root cause** - You can identify which code is causing the resets

## 📊 Expected Serial Monitor Output

After this fix, if there's a crash, you'll see decoded stack traces like:

```
Backtrace: 0x4200f8e4:0x3fc9a2b0 0x4200a7c5:0x3fc9a2d0 0x4200a7f1:0x3fc9a2f0
  #0  0x4200f8e4 in functionName() at src/file.cpp:123
  #1  0x4200a7c5 in setup() at src/main.cpp:45
  #2  0x4200a7f1 in loop() at src/main.cpp:67
```

Instead of just:
```
Backtrace: 0x4200f8e4:0x3fc9a2b0 0x4200a7c5:0x3fc9a2d0 0x4200a7f1:0x3fc9a2f0
```

## 🚀 Next Steps for Boot Loop Investigation

With exception decoding now working:
1. Clean and rebuild: `pio run -t clean && pio run`
2. Upload firmware: `pio run -t upload`
3. Start monitor: `pio device monitor`
4. Check for decoded error messages in the crash dump
5. Fix the specific issue causing the crash
6. Repeat until boot loop is resolved

## 📝 Technical Details

- **Change:** Added `default_envs = esp32-s3-devkitc-1` to `[platformio]` section
- **Impact:** Both build and monitor now use the same environment by default
- **Compatibility:** Existing environment-specific commands still work with `-e` flag
- **Board Config:** Uses standard ESP32-S3-DevKitC-1 with 16MB Flash QIO + 8MB PSRAM OPI

## ✅ Verification

To verify the fix is working:

```bash
# Build
pio run

# The firmware should now exist at the correct path:
# .pio/build/esp32-s3-devkitc-1/firmware.elf

# Monitor should now find the firmware
pio device monitor
```

You should NOT see the error: `firmware at ... does not exist`

---

**Status:** ✅ **FIXED**  
**Date:** 2026-01-27  
**Impact:** Exception decoder now works, enabling proper crash debugging
