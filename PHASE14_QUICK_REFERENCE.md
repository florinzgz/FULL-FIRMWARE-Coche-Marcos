# PHASE 14 Quick Reference Guide

## Hardware Migration Summary

**Old Hardware (DEPRECATED):** ESP32-S3-N32R16V
- 32MB Flash (QIO mode)
- 16MB PSRAM (OPI/Octal mode @ 1.8V)

**New Hardware (CURRENT):** ESP32-S3-N16R8
- 16MB Flash (QIO mode @ 3.3V)
- 8MB PSRAM (QSPI/Quad mode @ 3.3V)

---

## Quick Start

### 1. Build Firmware
```bash
# For standard OTA configuration
pio run -e esp32-s3-n16r8

# For release build
pio run -e esp32-s3-n16r8-release

# For standalone (no OTA)
pio run -e esp32-s3-n16r8-standalone
```

### 2. Flash Firmware
```bash
# Standard flash
pio run -e esp32-s3-n16r8 -t upload

# Monitor serial output
pio device monitor -e esp32-s3-n16r8
```

### 3. Verify Boot
Expected boot sequence:
1. ✅ Bootloader detects QIO flash (16MB)
2. ✅ PSRAM initialized as QSPI (8MB)
3. ✅ PSRAM memory test passes
4. ✅ Application starts normally

---

## Available Build Environments

| Environment | Purpose | Partition Table |
|-------------|---------|-----------------|
| `esp32-s3-n16r8` | Standard debug build with OTA | n16r8_ota.csv |
| `esp32-s3-n16r8-release` | Optimized release build | n16r8_ota.csv |
| `esp32-s3-n16r8-touch-debug` | Touch debugging | n16r8_ota.csv |
| `esp32-s3-n16r8-no-touch` | No touch support | n16r8_ota.csv |
| `esp32-s3-n16r8-standalone` | Standalone (no OTA) | n16r8_standalone.csv |
| `esp32-s3-n16r8-standalone-debug` | Standalone debug | n16r8_standalone.csv |

---

## Configuration Files

### Board Definition
**File:** `boards/esp32s3_n16r8.json`
- Flash: 16MB QIO
- PSRAM: 8MB QSPI
- Memory Type: qio_qspi

### SDK Configuration
**File:** `sdkconfig/n16r8.defaults`
- `CONFIG_ESPTOOLPY_FLASHMODE_QIO=y`
- `CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y`
- `CONFIG_SPIRAM_MODE_QUAD=y` (not OCT!)

### Partition Tables
**OTA:** `partitions/n16r8_ota.csv`
- 2x 6MB OTA partitions
- 3.7MB SPIFFS
- 256KB margin

**Standalone:** `partitions/n16r8_standalone.csv`
- 12MB single app partition
- 3.7MB SPIFFS
- 256KB margin

---

## Troubleshooting

### Boot Failure
If the device fails to boot:
1. Verify hardware is ESP32-S3 with 16MB flash + 8MB PSRAM
2. Confirm PSRAM operates at 3.3V (not 1.8V)
3. Check serial output for errors
4. Try erasing flash: `pio run -e esp32-s3-n16r8 -t erase`

### Build Errors
If build fails:
1. Clean build: `pio run -e esp32-s3-n16r8 -t clean`
2. Update platform: `pio pkg update`
3. Rebuild: `pio run -e esp32-s3-n16r8`

### PSRAM Not Detected
If PSRAM is not detected:
1. Check `CONFIG_SPIRAM_MODE_QUAD=y` in sdkconfig
2. Verify `memory_type = qio_qspi` in platformio.ini
3. Ensure board is esp32s3_n16r8

---

## What Changed?

### Files Created
- `boards/esp32s3_n16r8.json` - New board definition
- `sdkconfig/n16r8.defaults` - New SDK config (QSPI PSRAM)
- `partitions/n16r8_ota.csv` - OTA partition table for 16MB
- `partitions/n16r8_standalone.csv` - Standalone partition table for 16MB
- `PHASE14_N16R8_BOOT_CERTIFICATION.md` - Detailed certification report

### Files Deprecated
- `boards/esp32s3_n32r16v.json` → `.deprecated`
- `sdkconfig/n32r16v.defaults` → `.deprecated`
- `partitions/n32r16v.csv` → `.deprecated`
- `partitions_32mb.csv` → `.deprecated`
- `partitions_32mb_standalone.csv` → `.deprecated`

### Files Updated
- `platformio.ini` - All 6 environments updated to use N16R8
- `.gitignore` - Exclude deprecated files

---

## Key Differences from N32R16V

| Aspect | N32R16V (Old) | N16R8 (New) |
|--------|---------------|-------------|
| Flash Size | 32MB | 16MB |
| Flash Mode | QIO | QIO |
| PSRAM Size | 16MB | 8MB |
| PSRAM Mode | OPI (Octal, 8-bit) | QSPI (Quad, 4-bit) |
| PSRAM Voltage | 1.8V | 3.3V |
| Memory Type | qio_opi | qio_qspi |
| Complexity | High (dual voltage) | Low (single voltage) |
| eFuse Required | Yes (for OPI) | No |
| Boot Reliability | Lower | Higher |

---

## Migration Benefits

1. **Simpler Hardware:** Single 3.3V voltage domain
2. **Better Reliability:** No OPI complexity, no voltage switching
3. **No eFuse Dependency:** Standard QIO + QSPI modes
4. **Proven Configuration:** Widely used, well-tested
5. **Sufficient Resources:** 16MB flash + 8MB PSRAM adequate for application

---

## Next Steps

1. ✅ Configuration complete
2. ⏭️ Flash firmware to N16R8 hardware
3. ⏭️ Verify first boot success
4. ⏭️ Test all application features
5. ⏭️ Validate OTA updates (if using OTA environment)

---

## Support

For detailed technical information, see:
- `PHASE14_N16R8_BOOT_CERTIFICATION.md` - Complete certification report
- `platformio.ini` - Build configuration
- `boards/esp32s3_n16r8.json` - Board definition

---

**Status:** ✅ MIGRATION COMPLETE  
**Hardware:** ESP32-S3-N16R8 (16MB QIO + 8MB QSPI @ 3.3V)  
**Date:** 2026-01-12
