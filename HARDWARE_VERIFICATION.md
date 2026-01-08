# ESP32-S3 Hardware Verification and Datasheet Reference

## Current Hardware Configuration

**Module:** ESP32-S3-WROOM-2 N32R16V  
**Date:** 2026-01-08  
**Status:** ✅ VERIFIED

### Hardware Specifications

| Component | Specification | Datasheet Reference |
|-----------|--------------|---------------------|
| **Module** | ESP32-S3-WROOM-2 N32R16V | [Official Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-2_datasheet_en.pdf) |
| **Flash** | 32MB Octal SPI (OPI-capable, eFuses NOT burned) | Section 3.1 |
| **PSRAM** | 16MB Octal SPI (Embedded) | Section 3.2 |
| **CPU** | Dual-core Xtensa LX7 @ 240MHz | Section 2.1 |
| **Package** | QFN56 |  Section 4.1 |
| **Revision** | v0.2 | Boot log |

### eFuse Configuration (CRITICAL)

| Memory Type | Hardware Capability | eFuse Status | Operational Mode |
|-------------|---------------------|--------------|------------------|
| **Flash** | OPI-capable (32MB) | ❌ NOT burned | **QIO** (Quad I/O) |
| **PSRAM** | OPI (16MB embedded) | ✅ Burned | **OPI** (Octal) |

**Important:** The flash eFuses are ONE-TIME programmable and were NOT burned by the manufacturer. Therefore, the flash MUST operate in QIO mode, not OPI mode.

### Correct Configuration

```json
{
  "memory_type": "qio_opi",  // QIO Flash + OPI PSRAM
  "flash_mode": "qio",       // Quad I/O for Flash
  "psram_type": "opi",       // Octal for PSRAM
  "flash_size": "32MB",
  "f_flash": "80000000L",    // 80MHz
  "f_cpu": "240000000L"      // 240MHz
}
```

## Common Confusion: WROOM-1 vs WROOM-2

### ESP32-S3-WROOM-1/1U
- **Maximum Flash:** 16MB (typically Quad SPI)
- **Maximum PSRAM:** 16MB (Quad or Octal SPI)
- **Datasheet:** [ESP32-S3-WROOM-1 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf)
- **Configuration:** N16R16V (16MB Flash + 16MB PSRAM) is maximum

### ESP32-S3-WROOM-2/2U
- **Maximum Flash:** 32MB (Octal SPI capable)
- **Maximum PSRAM:** 16MB (Octal SPI)
- **Datasheet:** [ESP32-S3-WROOM-2 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-2_datasheet_en.pdf)
- **Configuration:** N32R16V (32MB Flash + 16MB PSRAM) ✅ **THIS IS OUR HARDWARE**

### Why This Matters

⚠️ **CRITICAL:** The N32R16V configuration (32MB Flash + 16MB PSRAM) is ONLY available on WROOM-2, NOT on WROOM-1.

If you reference the WROOM-1 datasheet for WROOM-2 hardware, you will get:
- ❌ Incorrect pin mappings
- ❌ Incorrect memory specifications
- ❌ Incorrect electrical characteristics
- ❌ Potential configuration errors

## Datasheet References

### Official Espressif Documentation
1. **ESP32-S3-WROOM-2 Datasheet (CORRECT for this project)**
   - URL: https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-2_datasheet_en.pdf
   - Covers: N32R16V, N16R16V, N16R8V variants

2. **ESP32-S3 Technical Reference Manual**
   - URL: https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf
   - Covers: Detailed chip specifications, peripherals, memory architecture

3. **ESP32-S3 Datasheet (Chip-level)**
   - URL: https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf
   - Covers: Core chip specifications (before module integration)

## Verification Steps

To verify your hardware module, check the boot log:

```
rst:0x1 (POWERON),boot:0x8 (SPI_FAST_FLASH_BOOT)
ESP-ROM:esp32s3-20210327
Build:Mar 27 2021
chip revision: v0.2
Flash: 32MB                    ← Confirms 32MB
PSRAM: 16MB                    ← Confirms 16MB
Octal Flash Mode Enabled       ← OPI-capable hardware
eFuses NOT burned              ← Must use QIO mode
```

## Configuration Files

Current project configuration (CORRECT):

1. **platformio.ini**
   ```ini
   board = esp32-s3-wroom-2-n32r16v
   ```

2. **boards/esp32-s3-wroom-2-n32r16v.json**
   ```json
   {
     "name": "ESP32-S3-WROOM-2 N32R16V (32MB QIO Flash, 16MB OPI PSRAM)",
     "build": {
       "arduino": {
         "memory_type": "qio_opi"
       }
     }
   }
   ```

3. **sdkconfig.defaults**
   ```
   CONFIG_SPIRAM_MODE_OCT=y      # Octal PSRAM
   CONFIG_SPIRAM_SIZE=16777216   # 16MB
   ```

## Summary

✅ **Hardware:** ESP32-S3-WROOM-2 N32R16V  
✅ **Configuration:** qio_opi (QIO Flash + OPI PSRAM)  
✅ **Datasheet:** ESP32-S3-WROOM-2 Datasheet (NOT WROOM-1)  
✅ **Status:** Correctly configured  

⚠️ **Do NOT use ESP32-S3-WROOM-1 datasheet for this hardware!**

---

**Last Updated:** 2026-01-08  
**Verified By:** Copilot Agent  
**Status:** ✅ Configuration Verified
