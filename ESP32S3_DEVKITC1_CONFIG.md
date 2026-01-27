# ESP32-S3 DevKitC-1 Configuration Guide

## Overview

This configuration provides a solution for reboot issues on ESP32-S3 boards with 16MB Flash and 8MB PSRAM.

## Configuration Details

### Hardware Specifications
- **Board**: ESP32-S3 DevKitC-1 (standard board)
- **Flash**: 16MB QD (Quad) with QIO mode @ 80MHz
- **PSRAM**: 8MB OT (Octal) with OPI mode
- **Memory Type**: qio_opi (QIO Flash + OPI PSRAM)

### PlatformIO Environment

Use the `esp32-s3-devkitc-1` environment:

```bash
# Compile
pio run -e esp32-s3-devkitc-1

# Upload
pio run -e esp32-s3-devkitc-1 -t upload

# Monitor
pio device monitor
```

### Key Configuration Parameters

```ini
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino

board_build.arduino.memory_type = qio_opi
board_build.flash_mode = qio
board_build.psram_type = opi
board_upload.flash_size = 16MB
board_upload.maximum_size = 16777216
board_build.partitions = partitions/default_16MB.csv
board_build.extra_flags = -DBOARD_HAS_PSRAM
```

### Partition Layout

The `partitions/default_16MB.csv` file provides:

| Partition | Type    | Subtype  | Offset    | Size      | Usage        |
|-----------|---------|----------|-----------|-----------|--------------|
| nvs       | data    | nvs      | 0x9000    | 0x5000    | NVS Storage  |
| coredump  | data    | coredump | 0xE000    | 0x10000   | Core Dumps   |
| app0      | app     | factory  | 0x20000   | 0xA00000  | Firmware     |
| spiffs    | data    | spiffs   | 0xA20000  | 0x5E0000  | File System  |

Total: 16MB (0x1000000)

## Reboot Issue Solution

This configuration includes:

1. **BOARD_HAS_PSRAM flag**: Ensures proper PSRAM initialization
2. **Bootloop prevention**: Automatic patching via `patch_arduino_sdkconfig.py`
3. **Extended watchdog timeout**: Increased from 300ms to 5000ms
4. **Hardware validation**: Pre-flight checks via `preflight_validator.py`

## Differences from Custom Board

This environment uses the standard `esp32-s3-devkitc-1` board instead of the custom `esp32s3_n16r8` board. Both configurations are equivalent in functionality, but this one uses PlatformIO's standard board definition.

### When to Use Each Environment

- **esp32-s3-devkitc-1**: Standard board, universal compatibility
- **esp32-s3-n16r8**: Custom board definition with pre-configured settings

## Troubleshooting

### If compilation fails
```bash
pio run -e esp32-s3-devkitc-1 -t clean
rm -rf .pio
pio run -e esp32-s3-devkitc-1
```

### If upload fails
- Verify USB connection (use data cable, not charge-only)
- Check COM port in platformio.ini
- Try lower upload speed: `upload_speed = 115200`

### If device still reboots
1. Verify power supply (5V, ≥2A)
2. Check for stable 3.3V on the board
3. Review serial output for error messages
4. Ensure proper hardware connections

## References

- **GUIA_RAPIDA_CONFIGURACION_ESP32S3.md** - Quick start guide
- **SOLUCION_BOOTLOOP_ESP32S3.md** - Bootloop solution details
- **HARDWARE.md** - Hardware specifications
- **platformio.ini** - Complete configuration file

---

**Created**: 2026-01-27  
**Purpose**: Solution for ESP32-S3 reboot issues  
**Status**: ✅ Validated and tested
