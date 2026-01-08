# ESP32-S3 BOOTLOOP FIX - QUICK REFERENCE

## Problem
- ESP32-S3-WROOM-2 N32R16V (32MB OPI Flash, 16MB OPI PSRAM)
- Infinite bootloop with `RTC_SW_SYS_RST`
- No Serial output, setup() never reached

## Root Cause
Wrong ESP-IDF SDK variant: `qio_qspi` instead of `opi_opi`

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
