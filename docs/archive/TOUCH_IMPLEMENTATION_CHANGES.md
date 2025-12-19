# Touch Screen Implementation - Changes Summary

**Date**: 2025-12-05  
**Version**: v2.9.2  
**Issue**: Touch screen not working with XPT2046 controller

---

## Problem Statement

The touch screen was not functioning properly despite being configured. The user requested implementation of proper TFT_eSPI library touch support based on the official GitHub repository.

---

## Root Causes Identified

1. **Incorrect Calibration Format**: The calibration data was using min/max values instead of offset/range format required by TFT_eSPI
2. **Missing Configuration**: Z_THRESHOLD (pressure threshold) was not defined
3. **Missing SPI Transaction Support**: Required flags for reliable SPI bus sharing were missing
4. **Insufficient Diagnostics**: No way to debug touch issues
5. **No Documentation**: Lack of troubleshooting guidance

---

## Changes Made

### 1. Fixed Calibration Data Format (`src/hud/hud.cpp`)

**Before:**
```cpp
calData[0] = TouchConstants::RAW_MIN;   // 200
calData[1] = TouchConstants::RAW_MAX;   // 3900
calData[2] = TouchConstants::RAW_MIN;   // 200
calData[3] = TouchConstants::RAW_MAX;   // 3900
calData[4] = 3;  // Rotation
```

**After:**
```cpp
uint16_t xMin = TouchConstants::RAW_MIN;   // 200
uint16_t xMax = TouchConstants::RAW_MAX;   // 3900
uint16_t yMin = TouchConstants::RAW_MIN;   // 200
uint16_t yMax = TouchConstants::RAW_MAX;   // 3900

calData[0] = xMin;              // x offset
calData[1] = xMax - xMin;       // x range (3700)
calData[2] = yMin;              // y offset
calData[3] = yMax - yMin;       // y range (3700)
calData[4] = 0;                 // No rotation/inversion
```

**Why**: TFT_eSPI's `setTouch()` expects [offset, range] not [min, max]

### 2. Added Z_THRESHOLD Configuration (`platformio.ini`)

**Added:**
```ini
-DZ_THRESHOLD=350
```

**Why**: Touch controller uses pressure (Z value) to validate touches. Without this, library uses default which might not match hardware.

### 3. Added SPI Transaction Support (`platformio.ini`)

**Added:**
```ini
-DSPI_HAS_TRANSACTION
-DSUPPORT_TRANSACTIONS
```

**Why**: Ensures proper SPI bus arbitration when touch and display share the same SPI bus.

### 4. Added Touch Controller Diagnostics (`src/hud/hud.cpp`)

**Added on initialization:**
```cpp
// Test touch immediately after initialization
uint16_t testX = 0, testY = 0;
bool touchResponding = tft.getTouchRaw(&testX, &testY);
// Log results and validate
```

**Added during operation:**
```cpp
// Periodically check raw touch when calibrated touch fails
if (!touchDetected && touchInitialized) {
    uint16_t rawX = 0, rawY = 0;
    if (tft.getTouchRaw(&rawX, &rawY)) {
        uint16_t rawZ = tft.getTouchRawZ();
        Logger::infof("Touch: Raw values available - X=%d, Y=%d, Z=%d", rawX, rawY, rawZ);
    }
}
```

**Why**: Helps diagnose hardware vs configuration issues

### 5. Added Touch Debug Mode (`platformio.ini`)

**Added:**
```ini
; Touch debug flag
-DTOUCH_DEBUG  # Optional, for verbose logging

; New build environment
[env:esp32-s3-devkitc-touch-debug]
# Slower SPI (1MHz), lower threshold (250), verbose logging
```

**Why**: Enables troubleshooting without modifying code

### 6. Documentation

Created three new documentation files:

| File | Purpose |
|------|---------|
| `TOUCH_QUICK_FIX.md` | Quick reference for common fixes (90% of issues) |
| `TOUCH_TROUBLESHOOTING.md` | Comprehensive step-by-step troubleshooting guide |
| Updated `docs/README.md` | Added Display & Touch section |

---

## Implementation Details

### Calibration Format Explanation

TFT_eSPI uses this format for touch calibration:

```cpp
parameters[0] = x_offset;      // Minimum X raw value
parameters[1] = x_range;       // Maximum - Minimum X
parameters[2] = y_offset;      // Minimum Y raw value  
parameters[3] = y_range;       // Maximum - Minimum Y
parameters[4] = flags;         // Rotation and inversion bits
```

The library converts raw touch coordinates using:
```cpp
screen_x = (raw_x - x_offset) * screen_width / x_range
screen_y = (raw_y - y_offset) * screen_height / y_range
```

### Z_THRESHOLD Explanation

XPT2046 measures pressure as a "Z" value (0-4095). The threshold determines minimum pressure to register a valid touch:

- **Too low** (< 200): False touches, noise sensitivity
- **Optimal** (250-400): Good balance
- **Too high** (> 500): Hard to trigger, missed touches

Default 350 is a good starting point for most hardware.

### SPI Transaction Support

When display and touch share SPI bus:
1. Display uses fast SPI (40MHz)
2. Touch uses slow SPI (2.5MHz or lower)
3. Without transactions, they can interfere
4. `SPI_HAS_TRANSACTION` ensures proper switching

---

## Testing Procedure

### 1. Basic Test
```bash
pio run --target upload
pio device monitor -b 115200
```

Look for:
```
Touchscreen XPT2046 integrado TFT_eSPI inicializado OK
Touch: Controller responding
```

### 2. Touch Test
Touch the screen and look for:
```
Touch detected at (X, Y)
```

Visual crosshair should appear at touch point.

### 3. If Not Working
```bash
# Use debug environment
pio run -e esp32-s3-devkitc-touch-debug --target upload
pio device monitor -b 115200
```

Check serial output for diagnostics.

### 4. Calibration Test
1. Touch battery icon 4 times
2. Enter code 8989
3. Select option 3
4. Follow calibration procedure

---

## Common Issues and Solutions

### Issue 1: Touch not detected at all

**Symptoms**: No touch messages in serial monitor

**Solutions**:
1. Check TOUCH_CS pin (GPIO 21)
2. Check MISO pin (GPIO 12) - critical for reading
3. Lower SPI frequency to 1MHz or 100kHz
4. Verify touch is enabled in config

### Issue 2: Touch detected but wrong position

**Symptoms**: Touch crosshair far from actual touch point

**Solution**: Run touch calibration

### Issue 3: Intermittent touch

**Symptoms**: Sometimes works, sometimes doesn't

**Solutions**:
1. Lower SPI_TOUCH_FREQUENCY
2. Check connections
3. Lower Z_THRESHOLD

---

## Files Modified

```
platformio.ini                      # Added touch config flags
src/hud/hud.cpp                    # Fixed calibration, added diagnostics
docs/README.md                     # Added Display & Touch section
docs/TOUCH_TROUBLESHOOTING.md     # New comprehensive guide
docs/TOUCH_QUICK_FIX.md          # New quick reference
```

---

## Build Environments

### Default: `esp32-s3-devkitc`
- SPI Touch: 2.5MHz
- Z Threshold: 350
- Debug: Off

### Debug: `esp32-s3-devkitc-touch-debug`
- SPI Touch: 1MHz (more reliable)
- Z Threshold: 250 (more sensitive)
- Debug: On (verbose logging)

### No Touch: `esp32-s3-devkitc-no-touch`
- Touch completely disabled
- For testing if touch causes other issues

---

## References

Based on official TFT_eSPI library:
- [Touch.cpp](https://github.com/Bodmer/TFT_eSPI/blob/master/Extensions/Touch.cpp)
- [Touch_calibrate example](https://github.com/Bodmer/TFT_eSPI/blob/master/examples/Generic/Touch_calibrate/Touch_calibrate.ino)
- [Library documentation](https://github.com/Bodmer/TFT_eSPI)

---

## Success Criteria

✅ Touch controller responds on initialization  
✅ Touch detected and logged to serial  
✅ Visual crosshair appears at touch point  
✅ Coordinates within screen bounds (0-479, 0-319)  
✅ Touch calibration accessible and functional  
✅ Comprehensive documentation provided  

---

## Next Steps for User

1. Flash updated firmware
2. Check serial monitor for touch initialization
3. Test touch functionality
4. Run calibration if coordinates are off
5. Adjust settings if needed (see TOUCH_QUICK_FIX.md)
6. Consult TOUCH_TROUBLESHOOTING.md if issues persist

---

## Version History

- **v2.9.2** (2025-12-05): Complete touch implementation based on TFT_eSPI
  - Fixed calibration format
  - Added Z_THRESHOLD
  - Added SPI transaction support
  - Added diagnostics and debug mode
  - Created comprehensive documentation
