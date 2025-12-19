# Touch Screen Calibration - Quick Guide

**Firmware Version:** 2.11.1+  
**Last Updated:** 2025-12-19

---

## Why Calibrate?

The touch screen needs calibration to accurately map your finger position to screen coordinates. This is necessary because:
- Manufacturing variations between touch panels
- Different screen orientations (rotation)
- Edge alignment accuracy

**When to calibrate:**
- First time using the system
- After rotating the display
- If touch seems misaligned (touching one place, system registers different location)
- After replacing the touch screen hardware

---

## Two Ways to Start Calibration

### Method 1: Using Physical Button (Easier)

1. **Press and hold** the 4X4 button for **5 seconds**
2. You'll hear a confirmation beep
3. Calibration starts automatically
4. Follow on-screen instructions

**Advantages:**
- ✅ Works even if touch is not responding
- ✅ No menu navigation needed
- ✅ Quick and simple

---

### Method 2: Using Hidden Menu (If touch works)

1. **Tap the battery icon** 4 times rapidly
2. Enter code: **8-9-8-9** (using on-screen keypad)
3. Select option **3: "Calibrar touch"**
4. Follow on-screen instructions

**Advantages:**
- ✅ More calibration options available
- ✅ Can see current calibration values
- ✅ Can test calibration before saving

---

## Calibration Process (Both Methods)

### Step 1: Read Instructions
- Yellow screen with instructions appears
- You have **60 seconds** to read and start
- Touch anywhere to proceed

### Step 2: Touch Top-Left Corner
- Red target appears in top-left corner
- **Touch accurately** at the center of the target
- Hold for about **0.5 seconds** (until progress bar fills)
- System collects 5 samples (takes ~50ms)
- Green progress bar shows collection progress

**Tips:**
- Use a stylus or fingernail for better accuracy
- Don't touch too lightly (must exceed Z_THRESHOLD)
- Don't touch too hard (can damage screen)
- Touch at the exact center of the red circle

### Step 3: Touch Bottom-Right Corner
- Red target moves to bottom-right corner
- Same process as Step 2
- Touch accurately and hold briefly

### Step 4: Review Results
- Green "CALIBRATION COMPLETE!" message
- Calibration values displayed:
  - Min X, Max X (horizontal range)
  - Min Y, Max Y (vertical range)
  - Rotation (should match display orientation)
- Calibration is **automatically saved** to EEPROM
- Screen returns to normal after 3 seconds

---

## Understanding Calibration Values

### Example Good Calibration:
```
Min X: 3900
Max X: 200
Min Y: 200
Max Y: 3900
Rotation: 3
```

**What this means:**
- **X-axis inverted** (Max → Min): Normal for this hardware
- **Y-axis normal** (Min → Max): Correct orientation
- **Range ~200-3900**: Good coverage of touch panel
- **Rotation 3**: Landscape mode (480×320)

### Warning Signs (Bad Calibration):

❌ **Values too close together:**
```
Min X: 2000
Max X: 2100  ← Only 100 units apart (should be ~3700)
```
**Problem:** Didn't touch corners accurately  
**Solution:** Recalibrate and touch exact corners

❌ **Values outside normal range:**
```
Min X: 50    ← Too low (should be ~200)
Max X: 4050  ← Too high (should be ~3900)
```
**Problem:** Touch hardware issue or weak touch  
**Solution:** Touch harder, check connections, try again

❌ **Both axes inverted:**
```
Min X > Max X: ✅ OK (expected)
Min Y > Max Y: ❌ WRONG (should be normal)
```
**Problem:** Touched corners in wrong order  
**Solution:** Recalibrate carefully

---

## Troubleshooting

### Problem: Calibration times out
**Symptoms:** "Timeout on point 1" or "Timeout on point 2"  
**Causes:**
- Not touching hard enough
- Touch hardware not working
- Touching wrong area

**Solutions:**
1. Touch harder (but not too hard)
2. Enable touch debug mode (see below)
3. Check hardware connections
4. Try touch-debug build environment

---

### Problem: "Sample out of expected range"
**Symptoms:** Yellow flash on target, "please try again" message  
**Causes:**
- Too light touch (Z pressure too low)
- Touch hardware issue
- Finger slipped off target

**Solutions:**
1. Touch harder and hold steady
2. Use stylus for better precision
3. Check TOUCH_CS connection (GPIO 21)
4. Verify MISO connection (GPIO 12)

---

### Problem: Touch works but coordinates are wrong
**Symptoms:** Touch detected but in wrong location  
**Causes:**
- Bad calibration
- Display rotation changed after calibration
- Hardware defect

**Solutions:**
1. Recalibrate carefully
2. Check rotation matches calibration (see logs)
3. Try touch-debug environment
4. Verify display is actually 480×320 (not 320×480)

---

### Problem: "Rotation mismatch" warning in logs
**Symptoms:** Logs show "Calibration rotation (X) != display rotation (Y)"  
**Cause:** Display rotation was changed after calibration  
**Solution:** 
- Recalibrate with current rotation
- Touch mapping will be incorrect until recalibrated

---

## Advanced: Touch Debug Mode

For troubleshooting hardware issues:

### Enable Touch Debug Build:
```bash
pio run -e esp32-s3-devkitc-touch-debug --target upload
```

**What it does:**
- Slows down touch SPI (1 MHz instead of 2.5 MHz) - more reliable
- Enables verbose logging of all touch events
- Lowers Z_THRESHOLD (more sensitive: 250 instead of 300)
- Shows raw X/Y/Z values in serial monitor

**How to use:**
1. Upload touch-debug build
2. Open Serial Monitor (115200 baud)
3. Touch the screen
4. Watch for messages like:
   ```
   Touch detected at (240, 160) - RAW: X=2048, Y=2048, Z=450
   ```

**Interpret results:**
- **X=0, Y=0**: Touch controller not responding (check wiring)
- **Z too low**: Increase touch pressure or lower Z_THRESHOLD
- **X/Y > 4095**: SPI communication error
- **Erratic values**: Hardware issue or interference

---

## Performance Metrics (v2.11.1)

**Calibration Speed:**
- Point sampling: **50 milliseconds** (was 500ms in older versions)
- Release detection: **200 milliseconds** (was 500ms)
- Total calibration time: **~8 seconds** (was ~34 seconds)

**Touch Responsiveness:**
- Latency: **33-83 milliseconds** (was 33-533ms)
- Frame rate: 30 FPS
- Sample averaging: 5 samples @ 10ms intervals

**Improvements over v2.9:**
- ✅ **10× faster** calibration point sampling
- ✅ **2.5× faster** touch release detection
- ✅ **4× faster** overall calibration process
- ✅ **6× faster** touch response latency

---

## Tips for Best Results

### During Calibration:
1. ✅ **Clean screen** - dirt/grease affects accuracy
2. ✅ **Use stylus** - more accurate than finger
3. ✅ **Take your time** - you have 60 seconds per point
4. ✅ **Touch center** - aim for exact center of red circle
5. ✅ **Steady pressure** - don't wobble or slide finger

### After Calibration:
1. ✅ **Test accuracy** - touch all corners and edges
2. ✅ **Verify icons** - battery, lights, media icons should work
3. ✅ **Check rotation** - if you change display rotation, recalibrate
4. ✅ **Save to EEPROM** - calibration is auto-saved, but verify in logs

### General Maintenance:
1. ✅ **Clean regularly** - use screen cleaner and microfiber cloth
2. ✅ **Avoid pressure** - don't press too hard (can damage panel)
3. ✅ **Protect from scratches** - use screen protector if desired
4. ✅ **Recalibrate if needed** - after hardware changes or rotation changes

---

## Technical Reference

### Touch Controller: XPT2046
- Type: 4-wire resistive touch
- Resolution: 12-bit ADC (0-4095 theoretical)
- Practical range: 100-4000 (varies by panel)
- Pressure sensing: Z-axis threshold
- Interface: SPI (shared with display)

### Pin Configuration:
```
TOUCH_CS  = GPIO 21
TOUCH_IRQ = GPIO 47 (not used in polling mode)
SPI_MOSI  = GPIO 11 (shared with TFT)
SPI_MISO  = GPIO 12 (shared with TFT)
SPI_SCLK  = GPIO 10 (shared with TFT)
```

### SPI Frequencies:
- Normal mode: 2.5 MHz
- Debug mode: 1.0 MHz

### Z_THRESHOLD:
- Normal: 300
- Debug mode: 250
- Range: 200-600 (typical)

---

## Getting Help

If calibration still doesn't work after following this guide:

1. **Check documentation:**
   - `docs/TOUCH_TROUBLESHOOTING.md` - Detailed troubleshooting
   - `docs/TOUCH_INSPECTION_REPORT_v2.11.1.md` - Technical analysis
   - `PRUEBAS_TOUCH_DIAGNOSTICO.md` - Diagnostic tests

2. **Enable debug logging:**
   - Use touch-debug environment
   - Save serial monitor output
   - Look for error messages

3. **Verify hardware:**
   - Check all connections with multimeter
   - Verify 3.3V power supply
   - Test with known-good display if available

4. **Report issue:**
   - Include serial monitor output
   - Describe calibration behavior
   - List troubleshooting steps tried
   - Provide photos of connections (if possible)

---

## Version History

- **v2.11.1** (2025-12-19): Major performance improvements
  - 10× faster calibration sampling (50ms vs 500ms)
  - 2.5× faster touch release detection
  - Added sample validation and rejection
  - Increased timeouts to 60 seconds
  - Added rotation mismatch detection
  - Improved user feedback

- **v2.9.2** (2025-12-05): Enhanced diagnostics
  - Added comprehensive touch logging
  - Created touch-debug environment
  - Z_THRESHOLD configuration

- **v2.9.0** (2025-11-30): Initial calibration system
  - 2-point calibration routine
  - EEPROM storage
  - X-axis inversion fix

---

**Need more help?** See `docs/TOUCH_TROUBLESHOOTING.md` for comprehensive troubleshooting guide.
