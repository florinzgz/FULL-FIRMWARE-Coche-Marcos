# Touch Screen Quick Reference Card

Quick solutions for common XPT2046 touch screen issues.

## 🚨 Touch Not Working At All

```ini
# In platformio.ini, try these in order:

# 1. Lower SPI frequency (most common fix)
-DSPI_TOUCH_FREQUENCY=1000000   # Change from 2500000 to 1000000

# 2. If still not working, go even lower
-DSPI_TOUCH_FREQUENCY=100000    # Slowest but most reliable

# 3. Increase sensitivity
-DZ_THRESHOLD=250               # Change from 350 to 250
```

**Then rebuild:**
```bash
pio run -e esp32-s3-devkitc --target upload
```

## 🎯 Touch Detected But Wrong Position

Run touch calibration:
1. Touch battery icon 4 times rapidly
2. Enter code: **8989**
3. Select option **3**: "Calibrar touch"
4. Follow on-screen instructions

## 🐛 Use Debug Mode

```bash
# Build with debug environment
pio run -e esp32-s3-devkitc-touch-debug --target upload

# Watch serial monitor
pio device monitor -b 115200
```

This environment automatically:
- Reduces SPI frequency to 1MHz
- Increases sensitivity (Z=250)
- Enables verbose logging

## 🔍 Check Hardware

| Pin | GPIO | Check |
|-----|------|-------|
| TOUCH_CS | 21 | Connected and different from TFT_CS (16) |
| MISO | 12 | **Critical** - Must be connected for reading |
| MOSI | 11 | Connected |
| SCLK | 10 | Connected |

**Most common issue**: MISO not connected

## 📊 Serial Monitor Messages

### ✅ Good Signs
```
Touch: Controller responding, raw values: X=0, Y=0
Touch: Initial test successful
Touch detected at (240, 160)
```

### ❌ Bad Signs
```
Touch: Controller not responding to getTouchRaw()
Touch: Invalid values detected
```
**Solution**: Check TOUCH_CS and MISO connections

## ⚙️ Quick Settings Reference

| Issue | Setting | Value |
|-------|---------|-------|
| Not working | SPI_TOUCH_FREQUENCY | 1000000 or 100000 |
| Not sensitive | Z_THRESHOLD | 250 or lower |
| Too sensitive | Z_THRESHOLD | 500 or higher |
| Wrong position | Run calibration | Battery icon x4 → 8989 → option 3 |

## 📖 Full Documentation

For detailed troubleshooting: [TOUCH_TROUBLESHOOTING.md](TOUCH_TROUBLESHOOTING.md)

## 🎯 90% Solution

**Most touch issues are fixed by lowering SPI frequency:**

1. Edit `platformio.ini`
2. Find: `-DSPI_TOUCH_FREQUENCY=2500000`
3. Change to: `-DSPI_TOUCH_FREQUENCY=1000000`
4. Save and upload

```bash
pio run --target upload
```

---

**Still stuck?** Check [TOUCH_TROUBLESHOOTING.md](TOUCH_TROUBLESHOOTING.md) for comprehensive guide.
