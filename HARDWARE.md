# ESP32-S3 N16R8 - Official Hardware Specification

**Version:** 2.17.1  
**Last Updated:** 2026-01-12  
**Status:** ✅ OFFICIAL - Single Source of Truth

---

## 🎯 Hardware Identity

This firmware is designed **exclusively** for:

```
ESP32-S3 N16R8
16MB QIO Flash + 8MB QSPI PSRAM @ 3.3V
```

### Official Specifications

| Component | Specification | Details |
|-----------|--------------|---------|
| **Module** | ESP32-S3 DevKitC-1 N16R8 | Official Espressif module with WROOM-2 |
| **Development Board** | ESP32-S3-DevKitC-1 | 44-pin variant |
| **CPU** | Dual-core Xtensa LX7 | 240 MHz |
| **Flash Memory** | 16 MB | **QIO mode** (Quad I/O, 4 data lines) |
| **Flash Voltage** | 3.3V | **NOT 1.8V** |
| **Flash Speed** | 80 MHz | Safe, stable frequency |
| **PSRAM** | 8 MB | **QSPI mode** (Quad SPI, 4 data lines) |
| **PSRAM Type** | ESPPSRAM32, AP_3v3 | **NOT OPI/Octal** |
| **PSRAM Voltage** | 3.3V | **NOT 1.8V** |
| **PSRAM Speed** | 80 MHz | Safe, stable frequency |
| **SDK Variant** | qio_qspi | Flash QIO + PSRAM QSPI |

---

## 📦 Memory Configuration

### Flash Layout (16MB Total)

The 16MB Flash is partitioned as follows (see `partitions/n16r8_ota.csv`):

- **Bootloader**: 0x1000 - 0x8000
- **Partition Table**: 0x8000 - 0x9000
- **NVS**: 0x9000 - 0xF000 (24KB)
- **OTA Data**: 0xF000 - 0x11000 (8KB)
- **Factory App**: 0x10000 - 0x810000 (~8MB)
- **OTA_0**: 0x810000 - 0x1010000 (~8MB)
- **SPIFFS**: Remaining space for data

### PSRAM Configuration (8MB)

- **Total Available**: 8,388,608 bytes
- **Used for**: Frame buffers, sprite buffers, large allocations
- **Mode**: QSPI (4-bit, safe and stable)
- **Integration**: Fully managed by Arduino-ESP32 framework

---

## 🚫 Why NOT OPI / 1.8V?

### Historical Context

This project was **previously** configured for:
- ❌ N32R16V (32MB OPI Flash + 16MB OPI PSRAM @ 1.8V)

### Migration to N16R8

The firmware has been **completely migrated** to N16R8 because:

1. **Hardware Availability**: N16R8 is more readily available and cost-effective
2. **Voltage Stability**: 3.3V operation is more stable than 1.8V
3. **Compatibility**: Better compatibility with development boards
4. **Sufficient Memory**: 16MB Flash + 8MB PSRAM is adequate for this application
5. **Proven Stability**: QIO/QSPI modes are well-tested and reliable

### Why QIO/QSPI is Required

| Parameter | Reason |
|-----------|--------|
| **QIO Flash** | 4-bit interface provides good speed/stability balance |
| **QSPI PSRAM** | 4-bit PSRAM interface compatible with 3.3V operation |
| **80MHz Speed** | Safe frequency for both Flash and PSRAM |
| **3.3V Logic** | Standard voltage, no level shifting required |
| **Arduino-ESP32** | Framework expects `qio_qspi` for this configuration |

**IMPORTANT:** Using incorrect modes (like `opi_opi` or `qio_opi`) will cause boot failures and instability.

---

## ⚙️ PlatformIO Configuration

### Board Definition

File: `boards/esp32s3_n16r8.json`

```json
{
  "name": "ESP32-S3 N16R8 (16MB QIO Flash, 8MB QSPI PSRAM)",
  "build": {
    "flash_mode": "qio",
    "psram_type": "qspi",
    "memory_type": "qio_qspi"
  },
  "upload": {
    "flash_size": "16MB"
  }
}
```

### Platform Configuration

File: `platformio.ini`

```ini
[env:esp32-s3-n16r8]
platform = espressif32@6.12.0
board = esp32s3_n16r8
framework = arduino

board_build.partitions = partitions/n16r8_ota.csv
board_build.sdkconfig = sdkconfig/n16r8.defaults
board_build.arduino.memory_type = qio_qspi
```

### SDK Configuration

File: `sdkconfig/n16r8.defaults`

Key settings:
```
CONFIG_ESPTOOLPY_FLASHMODE_QIO=y
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
CONFIG_SPIRAM_MODE_QUAD=y
CONFIG_SPIRAM_SIZE=8388608
```

---

## 🔍 Verification

### Hardware Detection

Upon boot, the firmware verifies:

1. ✅ Flash size = 16MB
2. ✅ PSRAM size = 8MB
3. ✅ Flash mode = QIO
4. ✅ PSRAM mode = QSPI

### Boot Logs

Expected output:
```
[BOOT] ESP32-S3 N16R8 detected
[BOOT] Flash: 16MB QIO @ 80MHz
[BOOT] PSRAM: 8MB QSPI @ 80MHz
[BOOT] Free PSRAM: ~8MB
```

If you see different values, **STOP** and verify your hardware.

---

## 📚 Related Documentation

- **Boot Certification**: [PHASE14_N16R8_BOOT_CERTIFICATION.md](PHASE14_N16R8_BOOT_CERTIFICATION.md)
- **Quick Reference**: [PHASE14_QUICK_REFERENCE.md](PHASE14_QUICK_REFERENCE.md)
- **Hardware Reference**: [docs/REFERENCIA_HARDWARE.md](docs/REFERENCIA_HARDWARE.md)
- **Pin Mapping**: [docs/PIN_MAPPING_DEVKITC1.md](docs/PIN_MAPPING_DEVKITC1.md)
- **Build Instructions**: [BUILD_INSTRUCTIONS_v2.11.0.md](BUILD_INSTRUCTIONS_v2.11.0.md)

---

## ⚠️ Important Notes

1. **This is N16R8 ONLY**: Do not attempt to use N32R16V boards or configurations
2. **No OPI Mode**: OPI/Octal mode is NOT supported or needed
3. **3.3V Only**: All flash and PSRAM operate at 3.3V
4. **QIO_QSPI Required**: Using other memory types will cause failures
5. **Verified Configuration**: This setup has been tested and certified in PHASE 14

---

## 🔧 Troubleshooting

### Boot Failures

If the device fails to boot:
1. Verify you have an N16R8 module (not N32R16V, N8R8, etc.)
2. Check that `board_build.arduino.memory_type = qio_qspi` in platformio.ini
3. Erase flash completely: `pio run -t erase`
4. Re-flash firmware: `pio run -t upload`

### Memory Detection Issues

If PSRAM is not detected:
1. Verify module marking shows "N16R8"
2. Check soldering/connections on development board
3. Verify voltage is 3.3V (not 1.8V)

---

## 📝 Changelog

### 2026-01-12 - v2.17.1
- ✅ Created official HARDWARE.md specification
- ✅ Established N16R8 as single hardware target
- ✅ Removed all references to N32R16V, OPI, 1.8V

---

**For questions about hardware configuration, refer to this document first.**
