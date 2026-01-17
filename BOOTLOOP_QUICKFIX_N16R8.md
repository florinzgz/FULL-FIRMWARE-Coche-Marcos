# Bootloop Quick Fix - ESP32-S3 N16R8

## Problem
Device stuck in bootloop with `rst:0x3 (RTC_SW_SYS_RST)` - no Serial output appears.

## Solution
Increased Interrupt Watchdog timeout from 800ms to 3000ms to allow PSRAM initialization to complete.

## How to Apply

### Step 1: Clean Build
```bash
pio run -e esp32-s3-n16r8-standalone-debug --target clean
rm -rf .pio/build/esp32-s3-n16r8-standalone-debug
```

### Step 2: Upload Firmware
```bash
pio run -e esp32-s3-n16r8-standalone-debug --target upload
```

### Step 3: Monitor
```bash
pio device monitor -e esp32-s3-n16r8-standalone-debug
```

## Expected Result
You should see:
```
=== ESP32-S3 EARLY BOOT ===
[STANDALONE] Mode active
[BOOT] Starting vehicle firmware...
[BOOT] Firmware version: 2.17.2
[BOOT] System initialization complete
```

## What Changed

**File:** `sdkconfig/n16r8.defaults`

- Interrupt watchdog timeout: **from 800ms to 3000ms**
- Added: `CONFIG_SPIRAM_IGNORE_NOTFOUND=y`

## Why It Works

The PSRAM initialization + memory test was taking longer than 800ms, causing the interrupt watchdog to reset the device before boot could complete. The new 3000ms timeout provides sufficient time for PSRAM to initialize properly.

## Troubleshooting

### Still bootlooping?
1. Try power cycling the device (unplug and replug USB)
2. Check PSRAM is properly seated (hardware issue)
3. Try the release build: `pio run -e esp32-s3-n16r8-release --target upload`

### PSRAM not detected?
The system will now boot anyway (with reduced memory). Check Serial output for PSRAM warnings.

### Need more help?
See detailed documentation: `BOOTLOOP_FIX_N16R8_v2.17.2.md`

---

**Version:** 2.17.2  
**Date:** 2026-01-17  
**Status:** ✅ FIXED
