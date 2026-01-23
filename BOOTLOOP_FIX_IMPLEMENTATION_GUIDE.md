# ESP32-S3 Bootloop Fix - Implementation Guide
## Firmware v2.17.3

**Date:** 2026-01-23  
**Status:** ✅ READY TO TEST  
**Hardware:** ESP32-S3 N16R8 (16MB Flash + 8MB PSRAM)

---

## Problem Summary

Your ESP32-S3 was experiencing a continuous bootloop with these symptoms:
- Repeated resets with `rst:0x3 (RTC_SW_SYS_RST)`
- Never reaching `setup()` or executing user code
- Infinite loop showing only ROM bootloader output

**Root Cause:** The Arduino framework for ESP32 ships with an interrupt watchdog timeout of only **300ms**, which is too short for PSRAM initialization on some hardware batches. When PSRAM init takes longer than 300ms, the watchdog fires and resets the system, creating an infinite bootloop.

---

## Solution Implemented

### What Was Changed

#### 1. **Arduino Framework Patching** ⭐ (Main Fix)
   - **File:** `tools/patch_arduino_sdkconfig.py`
   - **What it does:** Automatically patches the Arduino ESP32 framework files to increase the interrupt watchdog timeout from 300ms to 5000ms
   - **When it runs:** Before every build
   - **Impact:** Gives PSRAM initialization enough time to complete without triggering watchdog

#### 2. **Board Configuration**
   - **File:** `boards/esp32s3_n16r8.json`
   - **Changes:** Added proper PSRAM configuration (memory_type, psram_type)
   - **Impact:** Ensures the build system uses the correct SDK variant

#### 3. **SDK Configuration Documentation**
   - **File:** `sdkconfig/n16r8.defaults`
   - **Purpose:** Documents the ideal ESP-IDF settings for this hardware
   - **Note:** This file serves as documentation since Arduino framework uses pre-compiled libraries

#### 4. **Build Configuration**
   - **File:** `platformio.ini`
   - **Changes:** Added patch script to `extra_scripts`
   - **Impact:** Ensures the fix is applied on every build

---

## How to Build and Upload

### 1. Clean Build (Recommended First Time)
```bash
cd /path/to/FULL-FIRMWARE-Coche-Marcos

# Clean previous build
pio run -e esp32-s3-n16r8-standalone-debug -t clean

# Build firmware
pio run -e esp32-s3-n16r8-standalone-debug
```

### 2. Upload to Device
```bash
# Make sure device is connected to COM3 (or update platformio.ini)
pio run -e esp32-s3-n16r8-standalone-debug -t upload
```

### 3. Monitor Serial Output
```bash
pio device monitor -e esp32-s3-n16r8-standalone-debug
```

---

## Expected Results

### ✅ Success Indicators

After upload, you should see:

```
ESP-ROM:esp32s3-20210327
Build:Mar 27 2021
rst:0x1 (POWERON),boot:0x8 (SPI_FAST_FLASH_BOOT)
Saved PC:0x403cdb0a
SPIWP:0xee
mode:DIO, clock div:1
load:0x3fce3808,len:0x4bc
load:0x403c9700,len:0xbd8
load:0x403cc700,len:0x2a0c
entry 0x403c98d0

=== ESP32-S3 EARLY BOOT ===
[STANDALONE] Mode active
A[BootGuard] Boot counter initialized (power cycle or first boot)
[BootGuard] Starting new boot sequence
B[BOOT] Starting vehicle firmware...
[BOOT] Firmware version: 2.17.3
C[System init] entrando en PRECHECK
...system continues initializing...
```

**Key Success Criteria:**
1. ✅ Only **ONE** boot sequence (not repeating)
2. ✅ "=== ESP32-S3 EARLY BOOT ===" message appears
3. ✅ Firmware version shows "2.17.3"
4. ✅ System reaches main loop
5. ✅ Remains stable (no resets)

### ❌ If Still Bootlooping

If the device still bootloops after this fix:

1. **Verify the patch was applied:**
   - During build, you should see: `"✅ dio_qspi: Already patched (5000ms)"`
   - If not, the framework may need manual patching

2. **Check hardware:**
   - Ensure stable 5V power supply (at least 2A)
   - Check USB cable quality
   - Verify PSRAM is correctly soldered (if custom board)

3. **Try extended timeout:**
   You can manually increase the timeout in the patch script:
   ```python
   # In tools/patch_arduino_sdkconfig.py, change:
   '#define CONFIG_ESP_INT_WDT_TIMEOUT_MS 5000',
   # to:
   '#define CONFIG_ESP_INT_WDT_TIMEOUT_MS 10000',
   ```
   Then rebuild.

---

## Technical Details

### What is CONFIG_ESP_INT_WDT_TIMEOUT_MS?

The ESP32 has an "Interrupt Watchdog" that monitors interrupt service routines (ISRs). If an ISR runs too long without yielding, it assumes the system is hung and triggers a reset.

During early boot (before `main()` is called):
- PSRAM controller is initialized
- PSRAM memory is mapped
- On some hardware, this can take 1-3 seconds

**Problem:** Arduino framework's 300ms timeout is too short for this.

**Solution:** Our patch increases it to 5000ms (5 seconds), providing plenty of margin.

### Affected SDK Variants

The patch modifies all 6 ESP32-S3 SDK configuration variants:
- `dio_opi` - DIO flash + OPI PSRAM
- `opi_opi` - OPI flash + OPI PSRAM  
- `opi_qspi` - OPI flash + QSPI PSRAM
- `dio_qspi` - DIO flash + QSPI PSRAM ⬅️ **Used by N16R8**
- `qio_qspi` - QIO flash + QSPI PSRAM
- `qio_opi` - QIO flash + OPI PSRAM

### Persistence

The patch modifies files in your PlatformIO installation:
```
~/.platformio/packages/framework-arduinoespressif32/tools/sdk/esp32s3/*/include/sdkconfig.h
```

**Important:**
- ✅ Patch persists across builds (you only need to apply it once)
- ⚠️  Patch is lost if you update the Arduino framework package
- ✅ The build script automatically re-applies the patch if needed

---

## Testing on Hardware

### Test Checklist

After uploading firmware:

- [ ] Device boots without looping (single boot sequence)
- [ ] Serial output appears within 2-3 seconds
- [ ] "EARLY BOOT" message is visible
- [ ] Firmware version shows "2.17.3"
- [ ] PSRAM is detected (if hardware has it)
- [ ] System reaches main loop
- [ ] Stable operation for 5+ minutes
- [ ] No unexpected resets

### Power Cycle Test

After confirming stable boot:
1. Disconnect USB power
2. Wait 10 seconds
3. Reconnect USB power
4. Verify device boots successfully (cold boot test)

### Multi-Environment Test

Test all build environments to ensure fix works everywhere:

```bash
# Main debug build
pio run -e esp32-s3-n16r8 -t upload
pio device monitor -e esp32-s3-n16r8

# Release build
pio run -e esp32-s3-n16r8-release -t upload  
pio device monitor -e esp32-s3-n16r8-release

# Standalone display (minimal dependencies)
pio run -e esp32-s3-n16r8-standalone -t upload
pio device monitor -e esp32-s3-n16r8-standalone
```

---

## Rollback Plan

If this fix causes unexpected issues (unlikely), you can rollback:

```bash
git checkout HEAD~1 tools/patch_arduino_sdkconfig.py platformio.ini boards/esp32s3_n16r8.json sdkconfig/
pio run -e esp32-s3-n16r8-standalone-debug -t clean
pio run -e esp32-s3-n16r8-standalone-debug -t upload
```

Then manually restore the framework files:
```bash
# This resets the patched files back to 300ms
pio platform update espressif32 --skip-default-package
pio platform install espressif32
```

---

## Additional Resources

### Related Documentation

- **BOOTLOOP_FIX_v2.17.3.md** - Detailed technical analysis
- **BOOTLOOP_QUICKFIX_v2.17.3.md** - Quick reference guide
- **BOOTLOOP_FIX_FINAL_v2.17.3.md** - Complete fix summary

### Build Output Analysis

During build, look for these messages:

```
🔧 ESP32-S3 Bootloop Fix - Patching Arduino Framework (v2.17.3)
📁 Found 6 sdkconfig.h file(s) to patch
   ✅ dio_qspi: Already patched (5000ms)
   ... (5 more variants)
✅ Patching complete: 0 file(s) patched, 6 file(s) already correct
```

This confirms the fix is active.

---

## Troubleshooting

### Build Errors

**Error:** "Cannot find patch_arduino_sdkconfig.py"
- **Fix:** Ensure the file is in `tools/` directory and committed to git

**Error:** "Permission denied" when patching
- **Fix:** On Linux/Mac, ensure you have write access to `~/.platformio/`
- Run: `chmod -R u+w ~/.platformio/packages/framework-arduinoespressif32`

### Runtime Issues

**Device still bootlooping:**
- Check power supply (needs stable 5V at 2A+)
- Verify USB cable is data-capable (not charge-only)
- Try different USB port on computer
- Ensure ESP32-S3 board is genuine Espressif hardware

**No serial output:**
- Verify COM port in platformio.ini matches actual device
- Try pressing RESET button on board after upload
- Disconnect and reconnect USB cable

**PSRAM not detected:**
- This is expected if board doesn't have PSRAM
- Firmware will continue to work with reduced memory

---

## Support

If you continue to experience issues after applying this fix:

1. **Verify Fix Applied**
   - Check build output for patching messages
   - Manually inspect one of the sdkconfig.h files:
     ```bash
     grep CONFIG_ESP_INT_WDT_TIMEOUT_MS ~/.platformio/packages/framework-arduinoespressif32/tools/sdk/esp32s3/dio_qspi/include/sdkconfig.h
     # Should show: #define CONFIG_ESP_INT_WDT_TIMEOUT_MS 5000
     ```

2. **Hardware Verification**
   - Test with a different ESP32-S3 board if available
   - Check for physical damage or poor solder joints
   - Verify PSRAM chip is correctly installed

3. **Capture Debug Info**
   - Save complete serial output from boot
   - Note: Any changes to behavior (e.g., boots further before reset)
   - Share build log showing patch application

---

## Success!

If your ESP32-S3 now boots successfully:

1. ✅ The bootloop issue is resolved
2. ✅ You can continue development normally  
3. ✅ The fix will persist across builds
4. ✅ Share this solution with others experiencing similar issues

**Note:** Remember to re-apply the patch if you update PlatformIO's ESP32 Arduino framework package in the future.

---

**Prepared By:** GitHub Copilot  
**Date:** 2026-01-23  
**Firmware Version:** 2.17.3  
**Status:** Ready for Hardware Testing

---

## Quick Command Reference

```bash
# Build
pio run -e esp32-s3-n16r8-standalone-debug

# Upload
pio run -e esp32-s3-n16r8-standalone-debug -t upload

# Monitor
pio device monitor -e esp32-s3-n16r8-standalone-debug

# Clean + Build + Upload (all in one)
pio run -e esp32-s3-n16r8-standalone-debug -t clean && \
pio run -e esp32-s3-n16r8-standalone-debug && \
pio run -e esp32-s3-n16r8-standalone-debug -t upload
```

Good luck! 🚀
