# eFuse-Based PSRAM Configuration Correction

**Date:** 2026-01-28  
**Tool Used:** espefuse.py v4.8.1  
**Hardware:** ESP32-S3 N16R8  
**Status:** ✅ **CORRECTED** - Board configuration now matches actual hardware eFuses

---

## 🎯 Issue Summary

The board configuration file `boards/esp32-s3-devkitc1-n16r8.json` was incorrectly configured with **QSPI PSRAM** settings, but the actual hardware eFuse data shows the device has **OPI PSRAM** (AP_3v3 vendor).

This mismatch could cause:
- ❌ Reduced PSRAM bandwidth (50% performance loss)
- ❌ Potential boot failures or instability
- ❌ Incorrect SDK variant selection

---

## 📊 eFuse Analysis

### PSRAM eFuse Data (from espefuse.py v4.8.1)

```
PSRAM_CAP (BLOCK1)         = 8M R/W (0b01)
PSRAM_TEMP (BLOCK1)        = 85C R/W (0b10)
PSRAM_VENDOR (BLOCK1)      = AP_3v3 R/W (0b01)  ← KEY INDICATOR
PSRAM_CAP_3 (BLOCK1)       = False R/W (0b0)
PSRAM_CAPACITY (calculated) = 1 R/W (0b001)
```

### Flash eFuse Data

```
FLASH_TYPE (BLOCK0)        = 4 data lines R/W (0b0)  ← QIO mode
FLASH_ECC_MODE (BLOCK0)    = 16to18 byte R/W (0b0)
```

### Voltage Configuration

```
VDD_SPI_XPD (BLOCK0)       = True R/W (0b1)
VDD_SPI_TIEH (BLOCK0)      = VDD_SPI connects to VDD3P3_RTC_IO R/W (0b1)
VDD_SPI_FORCE (BLOCK0)     = True R/W (0b1)
PIN_POWER_SELECTION (BLOCK0) = VDD_SPI R/W (0b1)

Flash voltage (VDD_SPI) set to 3.3V by efuse.
```

---

## 🔍 Critical Finding: AP_3v3 Vendor = OPI PSRAM

**PSRAM_VENDOR = AP_3v3 (0b01)** is the definitive indicator that this device has:

| Vendor Code | Vendor Name | Type | Interface |
|-------------|-------------|------|-----------|
| `0b01` | **AP_3v3** | Apmemory 3.3V | **Octal (OPI)** - 8 data lines |

**NOT:**
| Vendor Code | Type | Interface |
|-------------|------|-----------|
| Standard QSPI | Generic | **Quad (QSPI)** - 4 data lines |

---

## ⚙️ Configuration Changes

### Before (INCORRECT)

**File:** `boards/esp32-s3-devkitc1-n16r8.json`

```json
{
  "build": {
    "arduino": {
      "memory_type": "qio_qspi"  ❌ WRONG - Mismatch with eFuses
    },
    "flash_mode": "qio",
    "psram_type": "qspi"  ❌ WRONG - Should be OPI
  }
}
```

**Impact:**
- ESP-IDF SDK variant: `qio_qspi` → Uses 4-bit PSRAM interface
- PSRAM bandwidth: **50% of actual capability**
- Configuration mismatch with hardware reality

### After (CORRECT)

**File:** `boards/esp32-s3-devkitc1-n16r8.json`

```json
{
  "build": {
    "arduino": {
      "memory_type": "qio_opi"  ✅ CORRECT - Matches eFuses
    },
    "flash_mode": "qio",
    "psram_type": "opi"  ✅ CORRECT - Octal mode
  }
}
```

**Impact:**
- ESP-IDF SDK variant: `qio_opi` → Uses 8-bit PSRAM interface
- PSRAM bandwidth: **100% (full Octal bandwidth)**
- Configuration matches hardware eFuses

---

## 📝 Documentation Updates

### Files Updated

1. **`boards/esp32-s3-devkitc1-n16r8.json`**
   - Changed `memory_type` from `qio_qspi` → `qio_opi`
   - Changed `psram_type` from `qspi` → `opi`
   - Updated board name for clarity

2. **`platformio.ini`**
   - Updated header comments to reflect OPI PSRAM
   - Clarified PSRAM is Octal mode (8 data lines)
   - Added AP_3v3 vendor reference

3. **`README.md`**
   - Corrected all QSPI references to OPI
   - Updated hardware specifications
   - Fixed PHASE 14 novedades section

4. **`BOOTLOOP_FIX_OPI_FLASH_EFUSE.md`**
   - Updated current hardware specs
   - Clarified OPI PSRAM usage

---

## 🔬 Technical Explanation

### Memory Type Selection

PlatformIO/ESP-IDF selects the SDK variant based on `memory_type`:

| memory_type | Flash Mode | PSRAM Mode | SDK Variant Path |
|-------------|------------|------------|------------------|
| `qio_qspi` | QIO (4-bit) | QSPI (4-bit) | `.../sdk/esp32s3/qio_qspi/` |
| **`qio_opi`** | **QIO (4-bit)** | **OPI (8-bit)** | **`.../sdk/esp32s3/qio_opi/`** ✅ |
| `opi_opi` | OPI (8-bit) | OPI (8-bit) | `.../sdk/esp32s3/opi_opi/` |

### Why This Matters

**OPI PSRAM Performance:**
- Data width: 8 bits per transfer (vs 4 bits for QSPI)
- Theoretical bandwidth: **2x QSPI** at same clock speed
- Better performance for:
  - Frame buffer operations (TFT display)
  - Sprite buffers
  - Large data structures
  - FreeRTOS task stacks

**Wrong Configuration Impact:**
```c
// With qio_qspi (WRONG):
PSRAM bandwidth = 80 MHz × 4 bits = 320 Mbps

// With qio_opi (CORRECT):
PSRAM bandwidth = 80 MHz × 8 bits = 640 Mbps
```

---

## ✅ Validation

### How eFuse Data Confirms OPI PSRAM

1. **PSRAM_VENDOR = AP_3v3 (0b01)**
   - Apmemory is a vendor of **Octal PSRAM chips**
   - The "3v3" indicates 3.3V operation
   - This is **NOT** a QSPI chip vendor code

2. **PSRAM_CAP = 8M**
   - 8MB capacity
   - Consistent with ESPPSRAM64 (64 Mbit = 8 MB) OPI chips

3. **Voltage Configuration**
   - VDD_SPI = 3.3V
   - OPI PSRAM operates at 3.3V on this hardware

### ESP-IDF Compatibility

The ESP-IDF framework expects:
- `qio_opi` for: QIO Flash + OPI PSRAM configurations
- Correct SDK variant ensures proper initialization
- Bootloader uses eFuse data to configure PSRAM correctly

---

## 🚀 Performance Impact

### Expected Improvements

With correct `qio_opi` configuration:

1. **Display Rendering**
   - Faster frame buffer updates
   - Smoother animations
   - Better sprite handling

2. **FreeRTOS Tasks**
   - Faster context switches when using PSRAM-allocated stacks
   - Better multitasking performance

3. **Large Data Operations**
   - Faster copy operations
   - Better cache performance

### No Breaking Changes

This fix is **purely corrective**:
- Hardware already operates in OPI mode (eFuses configure this)
- We're just updating software configuration to **match reality**
- No functional regressions expected
- Only performance improvements

---

## 📚 Reference

### espefuse.py Command Used

```bash
espefuse.py --port <PORT> summary
```

Output version: v4.8.1

### Key eFuse Blocks

- **BLOCK0**: System configuration, voltage settings
- **BLOCK1**: Calibration, PSRAM/Flash vendor/capacity
- **BLOCK2**: Additional calibration, ADC, temperature

### Related ESP32-S3 Documentation

- [ESP32-S3 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf)
- [eFuse Controller](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/system/efuse.html)
- [PSRAM Support](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/external-ram.html)

---

## 🎯 Conclusion

The board configuration has been corrected to accurately reflect the **actual hardware eFuse programming**. The device has **OPI PSRAM** (8-bit Octal mode) with AP_3v3 vendor, not QSPI PSRAM (4-bit Quad mode).

**Changes Made:**
- ✅ Board JSON: `qio_qspi` → `qio_opi`
- ✅ PSRAM type: `qspi` → `opi`
- ✅ Documentation: Updated to reflect OPI PSRAM
- ✅ Comments: Clarified Octal mode (8 data lines)

**Result:**
- Configuration now matches eFuse data
- Full PSRAM bandwidth available (640 Mbps vs 320 Mbps)
- Correct ESP-IDF SDK variant selection
- Better performance for graphics and multitasking

---

**Last Updated:** 2026-01-28  
**Validated By:** espefuse.py v4.8.1 eFuse data analysis  
**Hardware:** ESP32-S3 N16R8 with 16MB QIO Flash + 8MB OPI PSRAM @ 3.3V
