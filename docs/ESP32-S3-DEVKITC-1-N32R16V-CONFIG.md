# ESP32-S3-DevKitC-1 N32R16V Configuration

## Overview

This document describes the configuration for ESP32-S3-DevKitC-1 board with N32R16V variant (32MB Flash, 16MB PSRAM) added to the platformio.ini file.

## Hardware Specifications

- **Board:** ESP32-S3-DevKitC-1
- **Flash Memory:** 32MB (Quad SPI - QIO mode)
- **PSRAM:** 16MB (Octal SPI - OPI mode)
- **Flash Frequency:** 80MHz
- **Memory Configuration:** QIO_OPI (QIO Flash + OPI PSRAM)

## Configuration Details

### Board Definition File

Location: `boards/esp32-s3-devkitc1-n32r16v.json`

Key settings:
```json
{
  "name": "ESP32-S3-DevKitC-1 N32R16V (32MB QIO Flash, 16MB OPI PSRAM)",
  "build": {
    "flash_mode": "qio",
    "psram_type": "opi",
    "f_flash": "80000000L",
    "arduino": {
      "memory_type": "qio_opi"
    }
  },
  "upload": {
    "flash_size": "32MB"
  }
}
```

### PlatformIO Environment

Environment name: `[env:esp32-s3-devkitc-1]`

Key configuration:
```ini
platform = espressif32
board = esp32-s3-devkitc1-n32r16v
framework = arduino

board_upload.flash_size = 32MB
board_build.arduino.memory_type = qio_opi
board_build.f_flash = 80000000L

build_flags = 
    -DBOARD_HAS_PSRAM
    -mfix-esp32-psram-cache-issue
```

## Build Flags

### PSRAM Support
- **`-DBOARD_HAS_PSRAM`**: Enables PSRAM support at compile time
- **`-mfix-esp32-psram-cache-issue`**: Applies compiler workaround for ESP32-S3 PSRAM cache issues

### Debug Level
- **`-DCORE_DEBUG_LEVEL=3`**: Set to level 3 for development (can be adjusted as needed)

## Memory Type: QIO_OPI

The `qio_opi` memory type is crucial for this hardware configuration:
- **QIO (Quad I/O)**: For Flash memory access (4 data lines)
- **OPI (Octal SPI)**: For PSRAM access (8 data lines)

This configuration tells the ESP-IDF SDK to use:
- QIO mode for the 32MB Flash
- OPI mode for the 16MB PSRAM

## Differences from N32R16V Configuration

The existing `esp32-s3-n32r16v` configuration targets:
- **Flash:** 32MB QIO
- **PSRAM:** 16MB OPI

The new `esp32-s3-devkitc-1` configuration targets:
- **Flash:** 32MB QIO
- **PSRAM:** 16MB OPI

## Usage

### Compile
```bash
pio run -e esp32-s3-devkitc-1
```

### Compile and Upload
```bash
pio run -e esp32-s3-devkitc-1 -t upload
```

### Monitor Serial Output
```bash
pio device monitor
```

### Clean Build
```bash
pio run -e esp32-s3-devkitc-1 -t clean
```

## Verification

The configuration has been verified to:
1. ✅ Parse correctly in platformio.ini
2. ✅ Have valid board definition JSON
3. ✅ Include PSRAM support flags
4. ✅ Include PSRAM cache fix flag
5. ✅ Set correct flash size (32MB)
6. ✅ Set correct flash frequency (80MHz)
7. ✅ Set correct memory type (qio_opi)

## Notes

- This configuration is appropriate for ESP32-S3-DevKitC-1 boards with the N32R16V module
- The memory_type setting (qio_opi) is critical for proper PSRAM initialization
- The `-mfix-esp32-psram-cache-issue` flag is recommended for ESP32-S3 stability with PSRAM
- All TFT_eSPI display settings and peripheral configurations are inherited from the base configuration

## Related Documentation

- **Main platformio.ini:** Configuration for all build environments
- **Board definition:** `boards/esp32-s3-devkitc1-n32r16v.json`
- **Existing N32R16V config:** See `[env:esp32-s3-n32r16v]` section in platformio.ini
- **PSRAM troubleshooting:** See `ANALISIS_PSRAM_COMPLETO.md`
- **Forensic analysis:** See `FORENSIC_AUTOPSY_REPORT.md` for understanding memory type selection

## Troubleshooting

If you experience boot loops or PSRAM initialization failures:

1. Verify your hardware has the N32R16V module (32MB Flash + 16MB PSRAM)
2. Ensure the flash eFuses are configured for QIO mode
3. Check that PSRAM is OPI-capable
4. Review serial output at 115200 baud during boot for error messages
5. Consult `BOOTLOOP_FIX_OPI_FLASH_EFUSE.md` for detailed PSRAM troubleshooting

---

*Configuration added: 2026-01-08*
*Verified: platformio.ini syntax valid, board definition valid*
