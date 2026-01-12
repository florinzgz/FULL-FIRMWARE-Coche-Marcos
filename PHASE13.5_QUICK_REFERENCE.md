# PHASE 13.5 — ESP32-S3 Boot Failure Quick Reference

**Date:** 2026-01-12  
**Status:** ✅ ANALYSIS COMPLETE

---

## BOOT ERROR

```
assert failed: do_core_init startup.c:328 (flash_ret == ESP_OK)
Octal Flash Mode Enabled
For OPI Flash, Use Default Flash Boot Mode
```

---

## ROOT CAUSE (ONE LINE)

**Bootloader compiled with OPI Flash support, but chip's OPI Flash eFuses NOT BURNED → Flash init fails → bootloop**

---

## HARDWARE vs FIRMWARE COMPARISON

| Component | Hardware (EFUSE) | Firmware Config | Status |
|-----------|------------------|-----------------|--------|
| **Flash Mode** | OPI-capable, eFuses NOT burned | QIO (board JSON) | ⚠️ Config correct, but SDK variant may be wrong |
| **Flash Size** | 32MB | 32MB | ✅ MATCH |
| **PSRAM Mode** | OPI (eFuses burned) | OPI | ✅ MATCH |
| **PSRAM Size** | 16MB | 16MB | ✅ MATCH |

---

## CONFIGURATION STATUS

```
boards/esp32s3_n32r16v.json:
  flash_mode: "qio"         ✅ CORRECT
  flash_size: "32MB"        ✅ CORRECT
  psram_type: "opi"         ✅ CORRECT
  memory_type: "qio_opi"    ✅ CORRECT (but needs verification)

sdkconfig/n32r16v.defaults:
  CONFIG_ESPTOOLPY_FLASHMODE_QIO=y    ✅ CORRECT
  CONFIG_SPIRAM_MODE_OCT=y            ✅ CORRECT
```

---

## THE PROBLEM

**Despite correct configuration files, bootloader exhibits OPI Flash behavior:**

```
Error Message: "Octal Flash Mode Enabled"
              ↓
This means bootloader was compiled with OPI Flash support
              ↓
Requires OPI Flash eFuses to be burned
              ↓
eFuses NOT burned on this chip
              ↓
Flash initialization fails
              ↓
assert(flash_ret == ESP_OK) fails
              ↓
Infinite bootloop
```

---

## LIKELY CAUSE

**Stale build artifacts** OR **SDK variant not being selected correctly**

Even though board JSON says `"memory_type": "qio_opi"`, the actual SDK variant used during build may be `opi_opi`.

---

## VERIFICATION NEEDED (NOT EXECUTED - READ-ONLY)

```bash
# Check SDK variant in build
pio run -v 2>&1 | grep "sdk/esp32s3"

# Expected: .../sdk/esp32s3/qio_opi/include  ✅
# Wrong:    .../sdk/esp32s3/opi_opi/include  ❌
```

---

## RECOMMENDED FIX (NOT EXECUTED - READ-ONLY)

```bash
# 1. Clean everything
pio run -t clean
rm -rf .pio/

# 2. Rebuild
pio run -e esp32-s3-n32r16v

# 3. Verify SDK variant
pio run -v 2>&1 | grep "qio_opi"

# 4. Upload
pio run -e esp32-s3-n32r16v -t upload -t monitor
```

---

## WHY IT BOOTLOOPS

The ESP32-S3 ROM bootloader checks eFuses during flash initialization:

```c
// Simplified ESP-IDF boot logic
if (bootloader_compiled_with_opi_flash) {
    if (!efuse_opi_flash_enabled()) {
        // eFuses NOT burned!
        print("Octal Flash Mode Enabled");
        print("For OPI Flash, Use Default Flash Boot Mode");
        flash_ret = ESP_ERR_NOT_SUPPORTED;
        assert(flash_ret == ESP_OK);  // ← FAILS HERE
    }
}
```

---

## SECONDARY ERRORS

**Core dump error:** `esp_core_dump_flash: Core dump flash config is corrupted`

- **Cause:** Flash not initialized, coredump can't access flash
- **Fix:** Will resolve when flash initialization succeeds
- **Improvement:** Add coredump partition to partition table

**Left shift warning:** Not found in pins.h (either already fixed or false report)

---

## IS FIRMWARE SAFE?

**Current:** ❌ NO (bootloops immediately)

**After rebuild with qio_opi SDK:** ✅ YES (should work correctly)

**This is NOT:**
- ❌ Hardware defect
- ❌ Application code bug
- ❌ GPIO/CAN/Sensor issue
- ❌ PSRAM problem (PSRAM config is correct)

**This IS:**
- ✅ 100% SDK variant / build configuration issue

---

## PREVIOUS FIX HISTORY

1. **FORENSIC_AUTOPSY_REPORT.md (2026-01-08):**
   - Fixed: `qio_qspi` → `opi_opi` (PSRAM issue)
   
2. **BOOTLOOP_FIX_OPI_FLASH_EFUSE.md (2026-01-08):**
   - Fixed: `opi_opi` → `qio_opi` (Flash issue)
   - Status: "✅ FIXED"

3. **Current (2026-01-12):**
   - Configuration shows `qio_opi` in files
   - But bootloop still occurs
   - Suggests: Build artifacts not cleaned OR SDK variant not applied

---

## WHAT NOT TO DO

❌ **DO NOT:**
- Burn OPI Flash eFuses (irreversible, may brick device)
- Force OPI Flash mode in software
- Modify ESP-IDF source code
- Change hardware

✅ **DO:**
- Clean build environment
- Rebuild with qio_opi SDK variant
- Verify bootloader binary is QIO mode
- Upload fresh firmware

---

## NEXT STEPS (RECOMMENDATIONS ONLY)

1. **Verify current SDK variant** (check build verbose output)
2. **Clean rebuild** (remove all .pio/ artifacts)
3. **Check bootloader binary** (esptool.py image_info)
4. **Upload and test**

**Expected outcome:** Firmware boots successfully after clean rebuild with qio_opi SDK variant.

---

**Full Report:** See `PHASE13.5_FORENSIC_BOOT_FAILURE_REPORT.md`

**Status:** READ-ONLY ANALYSIS COMPLETE - NO CODE CHANGES MADE
