# ESP32-S3 Bootloop Fix - Quick Reference v2.17.3

**Date:** 2026-01-18  
**Issue:** ESP32-S3 N16R8 stuck in bootloop with `rst:0x3 (RTC_SW_SYS_RST)`  
**Fix:** Disable PSRAM memory test + Increase INT_WDT timeout  
**Status:** ✅ READY TO TEST

---

## Quick Summary

**Problem:** Device boots in infinite loop, never reaches `setup()`

**Root Cause:** PSRAM memory test takes >3000ms → Interrupt watchdog timeout → Reset

**Solution:**
1. Disabled PSRAM memory test (`CONFIG_SPIRAM_MEMTEST=n`)
2. Increased INT_WDT timeout to 5000ms (was 3000ms)

---

## Build & Test (Quick Steps)

### 1. Clean Build
```bash
cd /path/to/FULL-FIRMWARE-Coche-Marcos
pio run -e esp32-s3-n16r8-standalone-debug -t clean
```

### 2. Build Firmware
```bash
pio run -e esp32-s3-n16r8-standalone-debug
```

### 3. Upload to Device
```bash
pio run -e esp32-s3-n16r8-standalone-debug -t upload
```

### 4. Monitor Serial Output
```bash
pio device monitor -e esp32-s3-n16r8-standalone-debug --filter esp32_exception_decoder
```

---

## Expected Output (Success)

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
C[... continues normally ...]
```

**Key indicators of success:**
- ✅ Only ONE boot sequence (not repeating)
- ✅ "=== ESP32-S3 EARLY BOOT ===" appears
- ✅ Firmware version shows "2.17.3"
- ✅ System continues to initialize

---

## What Changed

### File: `sdkconfig/n16r8.defaults`

**Before:**
```ini
CONFIG_SPIRAM_MEMTEST=y              # ❌ Enabled (slow)
CONFIG_ESP_INT_WDT_TIMEOUT_MS=3000   # ❌ Too short
```

**After:**
```ini
CONFIG_SPIRAM_MEMTEST=n              # ✅ Disabled (fast)
CONFIG_ESP_INT_WDT_TIMEOUT_MS=5000   # ✅ Increased
```

### File: `src/core/boot_guard.cpp`

**Before:**
```cpp
Logger::info("BootGuard: ...");  // ❌ Logger not initialized yet
```

**After:**
```cpp
Serial.println("[BootGuard] ...");  // ✅ Serial already initialized
```

### File: `include/version.h`

```cpp
#define FIRMWARE_VERSION "2.17.3"  // Updated from 2.11.3
```

---

## Troubleshooting

### Still Bootlooping?

1. **Verify clean build:**
   ```bash
   pio run -e esp32-s3-n16r8-standalone-debug -t fullclean
   pio run -e esp32-s3-n16r8-standalone-debug
   ```

2. **Check sdkconfig is applied:**
   ```bash
   # After build, check build directory
   cat .pio/build/esp32-s3-n16r8-standalone-debug/sdkconfig | grep SPIRAM_MEMTEST
   # Should show: CONFIG_SPIRAM_MEMTEST=n
   ```

3. **Try even higher timeout:**
   Edit `sdkconfig/n16r8.defaults`:
   ```ini
   CONFIG_ESP_INT_WDT_TIMEOUT_MS=10000  # 10 seconds
   ```

### No Serial Output?

1. **Check USB connection:**
   - Reconnect USB cable
   - Try different USB port
   - Verify COM port in platformio.ini matches actual device

2. **Check Serial monitor settings:**
   ```bash
   pio device monitor -e esp32-s3-n16r8-standalone-debug -b 115200
   ```

3. **Power cycle device:**
   - Disconnect USB
   - Wait 5 seconds
   - Reconnect USB

### PSRAM Not Detected?

This is expected if hardware doesn't have PSRAM. Check serial output:
```
[System init] ❌ PSRAM NO DETECTADA
```

The system should still boot (thanks to `CONFIG_SPIRAM_IGNORE_NOTFOUND=y`).

---

## Test All Environments

After confirming standalone-debug works, test other environments:

```bash
# Main debug build
pio run -e esp32-s3-n16r8 -t upload

# Release build
pio run -e esp32-s3-n16r8-release -t upload

# Touch debug
pio run -e esp32-s3-n16r8-touch-debug -t upload
```

All should boot successfully without loop.

---

## Rollback (If Needed)

If this fix causes issues (unlikely):

```bash
git checkout HEAD~1 sdkconfig/n16r8.defaults include/version.h src/core/boot_guard.cpp
pio run -e esp32-s3-n16r8-standalone-debug -t clean
pio run -e esp32-s3-n16r8-standalone-debug -t upload
```

---

## Success Checklist

- [ ] Clean build completes without errors
- [ ] Upload succeeds
- [ ] Device boots (no infinite loop)
- [ ] Serial output appears within 2 seconds
- [ ] "EARLY BOOT" message visible
- [ ] Firmware version shows "2.17.3"
- [ ] PSRAM detected (if hardware has it)
- [ ] System reaches main loop
- [ ] Stable for 5+ minutes

---

## Documentation

Full technical details: `BOOTLOOP_FIX_v2.17.3.md`

---

## Support

If issues persist after this fix:
1. Check hardware connections
2. Verify PSRAM is correctly installed
3. Try different ESP32-S3 board
4. Check for hardware defects

---

**Last Updated:** 2026-01-18  
**Version:** 2.17.3  
**Status:** Ready to Deploy
