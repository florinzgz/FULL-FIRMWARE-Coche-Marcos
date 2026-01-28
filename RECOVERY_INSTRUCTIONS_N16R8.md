# ESP32-S3 N16R8 Recovery Instructions

## Critical Bootloop Fix Applied

This document provides recovery steps to transition the ESP32-S3 N16R8 device from the problematic DIO mode to the correct QIO mode for proper OPI PSRAM operation.

## Problem Summary

- **Device**: ESP32-S3 DevKitC-1 N16R8 (16MB Flash + 8MB PSRAM OPI)
- **Symptom**: RTC_SW_SYS_RST bootloop
- **Root Cause**: Boot log showing `mode:DIO`, but N16R8 hardware requires `mode:QIO` (Flash) to enable `mode:OPI` (PSRAM)
- **Conflict**: Redundant board configurations and flash mode stored in device flash header

## Changes Applied

### 1. Directory Cleanup
- ✅ Removed redundant `boards/esp32s3_n16r8.json`
- ✅ Kept only `boards/esp32-s3-devkitc1-n16r8.json` with strict QIO configuration

### 2. Board JSON Hardening (`boards/esp32-s3-devkitc1-n16r8.json`)
- ✅ **ADDED**: `"flash_mode": "qio"` in upload section (NEW)
- ℹ️ **RETAINED**: `"flash_mode": "qio"` in build section (already present)
- ℹ️ **RETAINED**: `"arduino.memory_type": "qio_opi"` (already present)
- ℹ️ **RETAINED**: `"psram_type": "opi"` (already present)

### 3. PlatformIO Configuration Hardening (`platformio.ini`)
Added explicit overrides to bypass any JSON ambiguity:
```ini
board_build.flash_mode = qio
board_upload.flash_mode = qio
board_build.arduino.memory_type = qio_opi
```

Note: The ESP32-S3 architecture handles PSRAM cache correctly without additional compiler flags (unlike the original ESP32).

### 4. SDK Configuration (`sdkconfig/n16r8.defaults`)
Critical changes to force Octal PSRAM mode:
- ✅ **CHANGED**: `CONFIG_SPIRAM_TYPE_AUTO=n` (was y - now disables auto-detect)
- ✅ **ADDED**: `CONFIG_SPIRAM_MODE_OCT=y` (Force Octal Mode - NEW)
- ✅ **CHANGED**: `CONFIG_SPIRAM_MEMTEST=n` (was y - now disabled to prevent WDT reset)
- ✅ **CHANGED**: `CONFIG_SPIRAM_IGNORE_NOTFOUND=n` (was y - now fails if PSRAM not found)
- ℹ️ **RETAINED**: `CONFIG_SPIRAM_SPEED_80M=y` (already present)
- ℹ️ **RETAINED**: `CONFIG_ESPTOOLPY_FLASHMODE_QIO=y` (already present)

### 5. Scripts Verified
- ✅ `tools/patch_arduino_sdkconfig.py` - Only touches watchdog timeout (3000ms), does NOT modify SPIRAM flags
- ✅ `tools/preflight_validator.py` - Only validates hardware initialization order, does NOT modify flash mode

## 🚨 MANDATORY RECOVERY STEPS

**IMPORTANT**: A standard "Upload" is **INSUFFICIENT** to fix an already-flashed device showing DIO mode.

### Step 1: Erase Flash (Required)
The DIO-mode flag is stored in the flash header and must be erased:

```bash
# Using PlatformIO CLI
pio run -t erase

# OR using esptool.py directly
esptool.py --port COM3 erase_flash
```

**Why this is necessary**: The flash mode is stored in the bootloader header. Without erasing, the old DIO flag will persist even with new firmware.

### Step 2: Upload Firmware
After erasing, upload the fixed firmware:

```bash
pio run -t upload
```

### Step 3: Monitor Boot Log
Connect to serial monitor to verify the fix:

```bash
pio device monitor
```

### Expected Boot Log (Success Indicators)

✅ **CORRECT Boot Log:**
```
mode:QIO, clock div:1
load:0x3fce2810,len:0x178c
...
PSRAM initialized, heap: 8388584
opi psram spi_num: 1 psram cs io: 26
...
PSRAM: 8192 KB
```

❌ **INCORRECT Boot Log (if still in DIO mode):**
```
mode:DIO, clock div:2
...
RTC_SW_SYS_RST
```

## Success Criteria

After applying these changes and performing the recovery steps:

1. ✅ Boot log shows `mode:QIO` (NOT `mode:DIO`)
2. ✅ PSRAM successfully initialized and mapped (check for "opi psram" messages)
3. ✅ Boot log shows `PSRAM: 8192 KB` (8MB)
4. ✅ No RTC_SW_SYS_RST bootloop
5. ✅ System boots successfully into main application

## Troubleshooting

### Still showing mode:DIO after upload?
- ✅ Did you run `pio run -t erase` **before** uploading? This is mandatory.
- ✅ Check that you're uploading to the correct port (COM3 in platformio.ini)
- ✅ Verify the correct board is selected: `esp32-s3-devkitc1-n16r8`

### PSRAM not detected or showing wrong size?
- ✅ Verify hardware: N16R8 variant has 8MB OPI PSRAM
- ✅ Check boot log for `CONFIG_SPIRAM_MODE_OCT=y`
- ✅ Ensure sdkconfig is being loaded: look for "Loading SDK config from sdkconfig/n16r8.defaults"

### Still experiencing bootloops?
- ✅ Check watchdog timeout in boot log (should be 3000ms)
- ✅ Verify PSRAM memtest is disabled (should not show "Testing PSRAM..." message)
- ✅ Monitor for WDT reset during PSRAM initialization phase

## Technical Background

### Why QIO Flash Mode is Required

The ESP32-S3 N16R8 variant uses:
- **Flash**: 16MB in QIO mode (Quad I/O - 4 data lines)
- **PSRAM**: 8MB in OPI mode (Octal - 8 data lines)

The hardware multiplexes pins between Flash and PSRAM. QIO flash mode is required to properly configure the pin multiplexing for OPI PSRAM operation.

Using DIO mode (Dual I/O - 2 data lines) prevents the PSRAM from initializing correctly, causing:
1. PSRAM initialization timeout
2. Watchdog reset
3. RTC_SW_SYS_RST bootloop

### Why Erase is Mandatory

The ESP32 bootloader reads the flash mode from the bootloader image header stored in flash. This header is written during the first upload and is NOT automatically updated on subsequent uploads unless:
1. Flash is erased first, OR
2. The bootloader partition is explicitly reflashed

The safest approach is to erase flash completely before uploading the fixed firmware.

## Reference Documents

- Original issue: `BOOTLOOP_STATUS_2026-01-18.md`
- Board configuration: `boards/esp32-s3-devkitc1-n16r8.json`
- SDK defaults: `sdkconfig/n16r8.defaults`
- PlatformIO config: `platformio.ini`

## Support

If issues persist after following these steps:
1. Capture full boot log via serial monitor
2. Verify hardware variant is actually N16R8 (16MB Flash + 8MB PSRAM)
3. Check for hardware damage or connection issues
4. Consider testing with a minimal sketch to isolate firmware vs. hardware issues

---

**Version**: 2.18.2+
**Date**: 2026-01-28
**Status**: Recovery procedures defined and validated
