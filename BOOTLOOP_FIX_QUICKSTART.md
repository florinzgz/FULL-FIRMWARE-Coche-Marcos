# ESP32-S3 BOOTLOOP FIX - QUICK REFERENCE

**Status:** 📜 **HISTORICAL** - For N32R16V hardware (obsolete)  
**Current Hardware:** ESP32-S3 N16R8 (16MB QIO Flash + 8MB QSPI PSRAM @ 3.3V)

## Historical Problem (N32R16V)
- ESP32-S3-WROOM-2 N32R16V (32MB OPI Flash, 16MB OPI PSRAM)
- Infinite bootloop with `RTC_SW_SYS_RST`
- No Serial output, setup() never reached

## Historical Root Cause (N32R16V)
Wrong ESP-IDF SDK variant: `qio_qspi` instead of `opi_opi`

## Current Status
✅ Firmware migrated to N16R8 - uses standard QIO/QSPI modes @ 3.3V  
✅ No OPI mode required  
✅ No bootloop issues with current hardware  

See [HARDWARE.md](HARDWARE.md) for current hardware specification.

---

## Historical Fix (N32R16V - No Longer Applicable)

## Solution
Custom board definition with `"memory_type": "opi_opi"`

## Files Changed
1. `boards/esp32-s3-wroom-2-n32r16v.json` - NEW
2. `platformio.ini` - Line 12: `board = esp32-s3-wroom-2-n32r16v`

## Build & Upload
```bash
# Clean build
pio run -e esp32-s3-n32r16v --target clean

# Build
pio run -e esp32-s3-n32r16v

# Upload
pio run -e esp32-s3-n32r16v --target upload

# Monitor
pio device monitor -e esp32-s3-n32r16v
```

## Expected Boot Output
```
=== ESP32-S3 EARLY BOOT ===
[BOOT] Starting vehicle firmware...
[System init: ✅ PSRAM DETECTADA Y HABILITADA]
[System init: PSRAM Total: 16777216 bytes (16.00 MB)]
[HUD] TFT_eSPI init SUCCESS
```

## Success Criteria
- ✅ No bootloop
- ✅ Serial output appears
- ✅ PSRAM: 16MB detected
- ✅ Display initializes
- ✅ Runs >60 seconds stable

## Documentation
- Full analysis: `FORENSIC_AUTOPSY_REPORT.md`
- Board config: `boards/esp32-s3-wroom-2-n32r16v.json`

## Status
✅ Code ready for hardware validation
