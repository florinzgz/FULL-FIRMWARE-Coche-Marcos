# Fix: Exception Decoder Path Mismatch - Updated 2026-01-27

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

## ✅ Solution Implemented (Updated)

**Created custom board manifest and simplified platformio.ini:**

1. **New Board Manifest:** `boards/esp32-s3-devkitc1-n16r8.json`
   - Proper USB Serial configuration
   - 16MB Flash QIO + 8MB PSRAM OPI
   - Correct ARDUINO_USB_CDC_ON_BOOT handling

2. **Updated platformio.ini:**
```ini
[platformio]
boards_dir = boards
default_envs = esp32-s3-devkitc1-n16r8

[env:esp32-s3-devkitc1-n16r8]
platform = espressif32
board = esp32-s3-devkitc1-n16r8  # ← Custom board manifest
framework = arduino
```

3. **Deactivated other environments** - Only one active board configuration to avoid confusion

### Why Custom Board Manifest?

The custom board manifest (`esp32-s3-devkitc1-n16r8.json`) provides:
- **Correct USB Serial routing** based on `ARDUINO_USB_CDC_ON_BOOT` flag
- **Proper PSRAM configuration** (8MB OPI)
- **Flash configuration** (16MB QIO @ 80MHz)
- **No path mismatches** - Single environment = single build path

## 📦 How to Use After Fix

Now you can build and monitor with the simplified configuration:

```bash
# Build (uses esp32-s3-devkitc1-n16r8 by default)
pio run

# Build and upload
pio run -t upload

# Monitor (will correctly find firmware at .pio/build/esp32-s3-devkitc1-n16r8/firmware.elf)
pio device monitor
```

### USB Serial Configuration

The board manifest handles USB Serial routing automatically based on `ARDUINO_USB_CDC_ON_BOOT`:

| ARDUINO_USB_CDC_ON_BOOT | UART 0 (RX/TX) | OTG (USB nativo) |
|-------------------------|----------------|------------------|
| 0 | `Serial` | `USBSerial` |
| 1 | `Serial0` | `Serial` |

The current configuration has `ARDUINO_USB_CDC_ON_BOOT` **not explicitly set** in the board manifest, so it uses the Arduino framework default.

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

- **Change:** Created custom board manifest `boards/esp32-s3-devkitc1-n16r8.json`
- **Change:** Set `default_envs = esp32-s3-devkitc1-n16r8` in platformio.ini
- **Change:** Deactivated all other board environments to avoid confusion
- **Impact:** Single build environment = no path mismatches
- **Compatibility:** Custom board manifest provides proper USB Serial configuration
- **Board Config:** ESP32-S3-DevKitC-1 with 16MB Flash QIO + 8MB PSRAM OPI

## ✅ Verification

To verify the fix is working:

```bash
# Build
pio run

# The firmware should now exist at the correct path:
# .pio/build/esp32-s3-devkitc1-n16r8/firmware.elf

# Monitor should now find the firmware
pio device monitor
```

You should NOT see the error: `firmware at ... does not exist`

---

**Status:** ✅ **FIXED**  
**Date:** 2026-01-27  
**Impact:** Exception decoder now works, enabling proper crash debugging
