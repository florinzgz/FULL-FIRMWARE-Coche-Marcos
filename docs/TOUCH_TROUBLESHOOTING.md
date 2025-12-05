# Touch Screen Troubleshooting Guide

## XPT2046 Touch Controller with TFT_eSPI Library

This guide helps diagnose and fix touch screen issues with the ST7796S display and XPT2046 touch controller.

---

## Quick Diagnostics

### 1. Check Hardware Connections

Verify the following pins are correctly connected:

| Pin | GPIO | Function | Notes |
|-----|------|----------|-------|
| TOUCH_CS | 21 | Touch Chip Select | Must be different from TFT_CS (16) |
| TFT_MISO | 12 | SPI MISO | Shared between display and touch |
| TFT_MOSI | 11 | SPI MOSI | Shared between display and touch |
| TFT_SCLK | 10 | SPI Clock | Shared between display and touch |
| TOUCH_IRQ | 47 | Touch Interrupt | Optional, not used in polling mode |

**Common Issues:**
- TOUCH_CS not connected or wrong pin
- MISO line not connected (touch reads need this)
- Loose connections or bad solder joints
- Display and touch sharing same CS pin (they must be different!)

### 2. Check Serial Monitor Output

Connect to serial monitor at 115200 baud and look for:

```
[HUD] Initializing HUD components...
Touch: Using default calibration [offset_x=200, range_x=3700, offset_y=200, range_y=3700, flags=0]
Touchscreen XPT2046 integrado TFT_eSPI inicializado OK
Touch: Testing touch controller response...
Touch: Controller responding, raw values: X=0, Y=0
```

**Good Signs:**
- "Touchscreen XPT2046 integrado TFT_eSPI inicializado OK"
- "Touch: Controller responding"
- When you touch screen: "Touch detected at (X, Y)"

**Bad Signs:**
- "Touch: Controller not responding to getTouchRaw()"
- No touch messages when touching screen
- "Touch: Invalid values detected - possible hardware or SPI issue"

---

## Step-by-Step Troubleshooting

### Step 1: Enable Touch Debug Mode

Edit `platformio.ini` and uncomment the TOUCH_DEBUG flag:

```ini
; Touch debug - uncomment to enable verbose touch logging
-DTOUCH_DEBUG
```

Or use the pre-configured debug environment:

```bash
pio run -e esp32-s3-devkitc-touch-debug --target upload
```

This environment has:
- Slower SPI frequency (1MHz instead of 2.5MHz) - more reliable
- Lower Z threshold (250 instead of 350) - more sensitive
- Verbose logging enabled

### Step 2: Check Raw Touch Values

When you touch the screen, you should see in serial monitor:

```
Touch detected at (240, 160)
Touch RAW: X=2048, Y=2048, Z=450
```

**What to check:**
- **X and Y both 0**: Touch controller not responding, check wiring
- **X and Y > 4095**: SPI communication error
- **Z too low (< 200)**: Increase sensitivity by lowering Z_THRESHOLD
- **Z always 0 or 4095**: Hardware issue or SPI problem

### Step 3: Try Lower SPI Frequency

If touch is unreliable, edit `platformio.ini`:

```ini
-DSPI_TOUCH_FREQUENCY=1000000  ; Try 1MHz (was 2.5MHz)
```

Or even lower for maximum compatibility:

```ini
-DSPI_TOUCH_FREQUENCY=100000   ; 100kHz - slowest but most reliable
```

**Note:** Lower frequency = more reliable but slower response

### Step 4: Adjust Touch Sensitivity

If touch works but is not sensitive enough, lower the Z_THRESHOLD:

```ini
-DZ_THRESHOLD=250   ; More sensitive (was 350)
```

Or if too sensitive (false touches):

```ini
-DZ_THRESHOLD=500   ; Less sensitive
```

**Recommended range:** 200-600

### Step 5: Run Touch Calibration

If touch is detected but coordinates are wrong:

1. Touch battery icon 4 times rapidly
2. Enter code: 8989
3. Select option 3: "Calibrar touch"
4. Follow on-screen instructions to touch calibration points
5. Calibration is saved automatically

---

## Common Problems and Solutions

### Problem: Touch not detected at all

**Symptoms:**
- No touch messages in serial monitor
- Visual crosshair never appears on screen

**Solutions:**
1. Check TOUCH_CS pin connection (GPIO 21)
2. Check MISO pin connection (GPIO 12) - essential for reading
3. Try lower SPI frequency: `-DSPI_TOUCH_FREQUENCY=100000`
4. Check if touch is enabled in configuration
5. Use touch-debug environment for diagnostics

### Problem: Touch detected but coordinates wrong

**Symptoms:**
- Touch crosshair appears far from where you touched
- Touch seems inverted or rotated

**Solutions:**
1. Run touch calibration (battery icon x4, code 8989, option 3)
2. Verify display rotation matches calibration (should be rotation 3)
3. Check calibration values in serial monitor
4. Try different Z_THRESHOLD values

### Problem: Touch works intermittently

**Symptoms:**
- Sometimes detects touch, sometimes doesn't
- Z values fluctuate wildly

**Solutions:**
1. Lower SPI_TOUCH_FREQUENCY to 1MHz or 100kHz
2. Check for loose connections
3. Lower Z_THRESHOLD for higher sensitivity
4. Enable SPI transactions (already enabled in platformio.ini)
5. Check power supply - insufficient power can cause issues

### Problem: False touches / Ghost touches

**Symptoms:**
- Touch detected when not touching screen
- Random touch events

**Solutions:**
1. Increase Z_THRESHOLD to 450 or higher
2. Check for electrical interference
3. Ensure proper grounding
4. Check display cable routing (away from power cables)

---

## Configuration Reference

### platformio.ini Touch Settings

```ini
; Touch pins
-DTOUCH_CS=21                    ; Touch chip select (must be unique)

; SPI frequency for touch
-DSPI_TOUCH_FREQUENCY=2500000    ; 2.5MHz (standard)
; -DSPI_TOUCH_FREQUENCY=1000000  ; 1MHz (more reliable)
; -DSPI_TOUCH_FREQUENCY=100000   ; 100kHz (maximum compatibility)

; Touch sensitivity
-DZ_THRESHOLD=350                ; Standard sensitivity
; -DZ_THRESHOLD=250              ; More sensitive
; -DZ_THRESHOLD=500              ; Less sensitive

; Debug mode
; -DTOUCH_DEBUG                  ; Uncomment for verbose logging

; SPI transaction support (required for reliable operation)
-DSPI_HAS_TRANSACTION
-DSUPPORT_TRANSACTIONS
```

### Build Environments

| Environment | Purpose | Settings |
|-------------|---------|----------|
| `esp32-s3-devkitc` | Normal operation | SPI: 2.5MHz, Z: 350 |
| `esp32-s3-devkitc-touch-debug` | Touch troubleshooting | SPI: 1MHz, Z: 250, Debug ON |
| `esp32-s3-devkitc-no-touch` | Disable touch completely | Touch disabled |

### Usage Examples

```bash
# Normal build
pio run -e esp32-s3-devkitc --target upload

# Touch debug build
pio run -e esp32-s3-devkitc-touch-debug --target upload

# No touch build (for testing if touch causes issues)
pio run -e esp32-s3-devkitc-no-touch --target upload
```

---

## Understanding Touch Debug Output

### Normal Operation

```
Touch: Using default calibration [offset_x=200, range_x=3700, offset_y=200, range_y=3700, flags=0]
Touchscreen XPT2046 integrado TFT_eSPI inicializado OK
Touch: Testing touch controller response...
Touch: Controller responding, raw values: X=0, Y=0
Touch: Initial test successful, values in valid range
```

### When Touching Screen

```
Touch: Screen touched
Touch detected at (240, 160)
Touch RAW: X=2048, Y=2048, Z=450
```

### When Releasing

```
Touch: Screen released
```

### Diagnostic Messages

```
Touch: Raw values available but getTouch() failed - Raw X=2048, Y=2048, Z=150
Touch: Calibration may be incorrect or Z threshold too high
```
This means touch hardware works but Z threshold is too high - lower it.

---

## Hardware Verification

### Using Multimeter

1. **Check continuity** between ESP32 pins and touch controller:
   - GPIO 21 → Touch CS
   - GPIO 12 → Touch SO (MISO)
   - GPIO 11 → Touch SI (MOSI)
   - GPIO 10 → Touch SCK

2. **Check voltage** on touch CS pin:
   - Should be HIGH (3.3V) when not accessing touch
   - Should pulse LOW when reading touch

3. **Check power** on touch controller:
   - VCC should be 3.3V
   - GND should be 0V

### Using Logic Analyzer (Optional)

If you have a logic analyzer, monitor:
- CS line: Should pulse LOW when reading touch
- MISO line: Should show data when CS is LOW
- SCK line: Should show clock pulses during transfer

---

## Still Not Working?

### Try These Last Resort Options

1. **Swap display** - Test with a known-good display/touch combo
2. **Check for hardware damage** - Inspect touch controller IC
3. **Test with simple TFT_eSPI example** - Verify library works standalone
4. **Disable touch** - Use no-touch environment to verify rest of system works
5. **Check ESP32-S3 pins** - Ensure GPIO pins are not damaged

### Report an Issue

Include this information:
1. Serial monitor output with TOUCH_DEBUG enabled
2. Hardware connection details
3. Display and touch controller model
4. What troubleshooting steps you've tried
5. Photos of connections (if possible)

---

## Advanced Configuration

### Custom Calibration Values

If you have calibration values from another project:

Edit `src/hud/hud.cpp` and modify:

```cpp
calData[0] = YOUR_X_OFFSET;    // e.g., 200
calData[1] = YOUR_X_RANGE;     // e.g., 3700
calData[2] = YOUR_Y_OFFSET;    // e.g., 200
calData[3] = YOUR_Y_RANGE;     // e.g., 3700
calData[4] = YOUR_FLAGS;       // e.g., 0
```

### Using Different Touch Controller

If using a different touch controller (not XPT2046):
1. Modify touch initialization in `src/hud/hud.cpp`
2. Update pin definitions in `platformio.ini`
3. May need to use different library

---

## References

- [TFT_eSPI Library](https://github.com/Bodmer/TFT_eSPI)
- [TFT_eSPI Touch Documentation](https://github.com/Bodmer/TFT_eSPI/blob/master/examples/Generic/Touch_calibrate/Touch_calibrate.ino)
- [XPT2046 Datasheet](https://datasheetspdf.com/pdf/1049874/XPTEK/XPT2046/1)
- [ESP32-S3 Pin Reference](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/gpio.html)

---

## Version History

- **v2.9.2** (2025-12-05): Added comprehensive touch improvements
  - Fixed calibration data format
  - Added Z_THRESHOLD configuration
  - Added SPI transaction support
  - Added touch diagnostics
  - Created touch-debug environment
