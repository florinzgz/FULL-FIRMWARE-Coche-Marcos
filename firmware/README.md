# Custom Bootloader for ESP32-S3 N16R8

## bootloader_qio_80m.bin

This directory contains a custom bootloader for the ESP32-S3-WROOM-1U-N16R8 hardware.

### Why a custom bootloader is needed

The default Arduino-ESP32 bootloader is configured for DIO flash mode, which causes compatibility issues with the N16R8 hardware (16MB Flash + 8MB PSRAM). The custom bootloader ensures:

- **Flash Mode**: QIO (Quad I/O - 4 data lines)
- **Flash Speed**: 80 MHz
- **Flash Size**: 16 MB
- **PSRAM**: QSPI (Quad SPI - 4 data lines)
- **PSRAM Size**: 8 MB

### How the bootloader was generated

The bootloader was created using ESP-IDF 5.5.2:

1. Create a minimal ESP-IDF project
2. Configure with `idf.py menuconfig`:
   - Target: ESP32-S3
   - Flash mode: QIO
   - Flash speed: 80 MHz
   - Flash size: 16 MB
   - PSRAM: QSPI mode
   - PSRAM speed: 80 MHz
3. Build with: `idf.py bootloader`
4. Extract: `build/bootloader/bootloader.bin`

### Installation

The bootloader is referenced in `platformio.ini`:

```ini
board_build.bootloader = firmware/bootloader_qio_80m.bin
board_build.flash_mode = qio
board_build.arduino.memory_type = qio_qspi
```

### Flashing manually

If needed, the bootloader can be flashed directly:

```bash
esptool.py --chip esp32s3 write_flash 0x0 firmware/bootloader_qio_80m.bin
```

### Boot verification

After flashing, the serial output should show:
```
mode:QIO, clock div:1
I (44) boot.esp32s3: SPI Mode       : QIO
I (48) boot.esp32s3: SPI Flash Size : 16MB
```

## Important Notes

- **DO NOT** delete this file - it's required for correct hardware operation
- The bootloader file must be committed to the repository
- If the bootloader is missing, the build will fail
- DIO mode will cause PSRAM initialization failures and bootloops
