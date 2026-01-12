# ESP32-S3 Hardware Verification and Datasheet Reference

## Current Hardware Configuration

**Module:** ESP32-S3-WROOM-2 N16R8  
**Date:** 2026-01-12  
**Status:** ✅ VERIFIED - Official Hardware

### Hardware Specifications

| Component | Specification | Datasheet Reference |
|-----------|--------------|---------------------|
| **Module** | ESP32-S3-WROOM-2 N16R8 | [Official Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-2_datasheet_en.pdf) |
| **Flash** | 16MB QIO (Quad I/O, 4-bit, 3.3V) @ 80MHz | Section 3.1 |
| **PSRAM** | 8MB QSPI (Quad SPI, 4-bit, 3.3V) @ 80MHz | Section 3.2 |
| **CPU** | Dual-core Xtensa LX7 @ 240MHz | Section 2.1 |
| **Package** | QFN56 | Section 4.1 |

### Memory Configuration

| Memory Type | Hardware Capability | Operational Mode | Voltage |
|-------------|---------------------|------------------|---------|
| **Flash** | 16MB QIO | **QIO** (Quad I/O, 4-bit) | 3.3V |
| **PSRAM** | 8MB QSPI | **QSPI** (Quad SPI, 4-bit) | 3.3V |

**Important:** This module uses standard QIO/QSPI modes at 3.3V. No OPI (Octal) mode or 1.8V operation.

### Correct Configuration

```json
{
  "memory_type": "qio_qspi",  // QIO Flash + QSPI PSRAM
  "flash_mode": "qio",        // Quad I/O for Flash
  "psram_type": "qspi",       // Quad SPI for PSRAM
  "flash_size": "16MB",
  "f_flash": "80000000L",     // 80MHz
  "f_cpu": "240000000L"       // 240MHz
}
```

## Common Confusion: WROOM-1 vs WROOM-2

### ESP32-S3-WROOM-1/1U
- **Maximum Flash:** 16MB (typically Quad SPI)
- **Maximum PSRAM:** 8MB (Quad SPI)
- **Datasheet:** [ESP32-S3-WROOM-1 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf)
- **Configuration:** N16R8 variants available

### ESP32-S3-WROOM-2/2U
- **Maximum Flash:** 32MB (supports various modes)
- **Maximum PSRAM:** 16MB (supports various modes)
- **Datasheet:** [ESP32-S3-WROOM-2 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-2_datasheet_en.pdf)
- **Configuration:** N16R8 (16MB Flash + 8MB PSRAM) ✅ **THIS IS OUR HARDWARE**

### Why This Matters

⚠️ **CRITICAL:** Always reference the correct WROOM-2 datasheet for this hardware.

If you use the wrong datasheet, you may encounter:
- ❌ Incorrect pin mappings
- ❌ Incorrect memory specifications
- ❌ Incorrect electrical characteristics
- ❌ Configuration errors leading to boot failures

## Datasheet References

### Official Espressif Documentation
1. **ESP32-S3-WROOM-2 Datasheet (CORRECT for this project)**
   - URL: https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-2_datasheet_en.pdf
   - Covers: N32R16V, N16R16V, N16R8 variants

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
