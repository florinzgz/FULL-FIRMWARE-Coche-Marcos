# BOOTLOADER FILE REQUIRED

The file `bootloader_qio_80m.bin` must be placed in this directory for the build to work.

## How to obtain the bootloader

The bootloader has been generated using ESP-IDF 5.5.2 and should be provided separately.

If you need to regenerate it:

1. Install ESP-IDF 5.5.2
2. Create a minimal ESP-IDF project
3. Configure with `idf.py menuconfig`:
   - Component config → ESP32S3-Specific → Support for external, SPI-connected RAM
     - SPI RAM config → Set RAM clock speed → 80MHz
     - SPI RAM config → Type of SPI RAM chip in use → ESP-PSRAM32
   - Serial flasher config → Flash size → 16 MB
   - Serial flasher config → Flash SPI mode → QIO
   - Serial flasher config → Flash SPI speed → 80 MHz
4. Build: `idf.py bootloader`
5. Copy: `build/bootloader/bootloader.bin` to this directory as `bootloader_qio_80m.bin`

## File Requirements

- **Size**: ~28-32 KB (typical ESP32-S3 bootloader size)
- **Format**: Binary file
- **Target**: ESP32-S3
- **Configuration**: QIO mode, 80MHz, 16MB Flash, 8MB QSPI PSRAM

## Verification

After placing the file, verify with:
```bash
file bootloader_qio_80m.bin
# Should show: bootloader_qio_80m.bin: data
```

Size check:
```bash
ls -lh bootloader_qio_80m.bin
# Should be approximately 28-32 KB
```

---

**NOTE FOR REPOSITORY MAINTAINER**: 
The actual `bootloader_qio_80m.bin` file should be committed to the repository to ensure consistent builds across all development environments.
