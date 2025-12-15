# Build Instructions - Firmware v2.11.0

## Quick Start

### Prerequisites
- PlatformIO installed (via VS Code or CLI)
- USB cable for ESP32-S3 connection
- ESP32-S3-DevKitC-1 board

### Build Commands

#### Production Release Build (Recommended)
```bash
pio run -e esp32-s3-devkitc-release
```

#### Upload to Hardware
```bash
pio run -e esp32-s3-devkitc-release --target upload
```

## Available Environments

### 1. esp32-s3-devkitc-release (Production)
**Best for:** Production deployment, best performance

```bash
pio run -e esp32-s3-devkitc-release --target upload
```

**Features:**
- Maximum optimization (-O3)
- Debug logging disabled
- No console UART output
- Smallest binary size
- Best performance

**Use when:**
- Deploying to production vehicle
- Need maximum performance
- Don't need debug output

---

### 2. esp32-s3-devkitc-no-touch (Hardware Issues)
**Best for:** Touch hardware problems or debugging SPI conflicts

```bash
pio run -e esp32-s3-devkitc-no-touch --target upload
```

**Features:**
- Touch completely disabled
- Display works normally
- No touch controller initialization
- Uses base environment settings

**Use when:**
- Touch screen is not responding
- Suspect SPI bus conflicts
- Touch hardware is faulty
- Testing display-only functionality

---

### 3. esp32-s3-devkitc-touch-debug (Touch Troubleshooting)
**Best for:** Debugging touch issues, calibration problems

```bash
pio run -e esp32-s3-devkitc-touch-debug --target upload
```

**Features:**
- Slower touch SPI (1MHz for reliability)
- Verbose touch logging enabled
- Lower pressure threshold (more sensitive)
- Maximum debug output

**Use when:**
- Touch is intermittent
- Need to see touch raw values
- Calibrating touch screen
- Diagnosing touch problems

---

### 4. esp32-s3-devkitc (Base/Development)
**Best for:** Development with full debug output

```bash
pio run -e esp32-s3-devkitc --target upload
```

**Features:**
- Full debug logging (CORE_DEBUG_LEVEL=5)
- Standard optimizations
- All sensors enabled
- Touch enabled

**Use when:**
- Developing new features
- Need full debug output
- Troubleshooting system issues

## Clean Build

If you encounter build issues, clean first:

```bash
pio run --target clean
pio run -e esp32-s3-devkitc-release
```

## Monitor Serial Output

```bash
pio device monitor -e esp32-s3-devkitc-release
```

Or specify baud rate:
```bash
pio device monitor -b 115200
```

## Common Issues

### Issue: "command not found: pio"
**Solution:** Install PlatformIO CLI or use VS Code PlatformIO extension

### Issue: "Cannot open COM port"
**Solution:** 
1. Check USB cable connection
2. Verify COM port in platformio.ini (default: COM4)
3. Close any other serial monitor programs

### Issue: "Library not found"
**Solution:** PlatformIO will auto-download on first build. If issues persist:
```bash
pio lib install
```

### Issue: "Upload failed"
**Solution:**
1. Hold BOOT button on ESP32 during upload
2. Press RESET button after upload
3. Check USB driver installed

## Library Versions (Pinned)

All libraries are pinned to exact versions for reproducibility:

- bodmer/TFT_eSPI @ 2.5.43
- dfrobot/DFRobotDFPlayerMini @ 1.0.6
- milesburton/DallasTemperature @ 4.0.5
- paulstoffregen/OneWire @ 2.3.8
- adafruit/Adafruit PWM Servo Driver Library @ 3.0.2
- adafruit/Adafruit BusIO @ 1.17.4
- robtillaart/INA226 @ 0.6.5
- fastled/FastLED @ 3.6.0
- adafruit/Adafruit MCP23017 Arduino Library @ ^2.3.2
- stm32duino/VL53L5CX @ 1.2.3

## Build Size Comparison

| Environment | Binary Size (approx) | RAM Usage |
|-------------|---------------------|-----------|
| release | ~1.2 MB | Minimal |
| base | ~1.5 MB | Standard |
| no-touch | ~1.4 MB | Standard |
| touch-debug | ~1.6 MB | Higher |

## Troubleshooting Build Errors

### Error: Stack Overflow
Already fixed in v2.10.3+ with increased stack sizes.

### Error: WiFi.h not found
This is expected - WiFi support removed in v2.11.0. All WiFi code has been removed.

### Error: wifi_manager.h not found
This is expected - file deleted in v2.11.0. All references removed.

## Next Steps After Build

1. **Test Display:** Verify HUD shows correctly
2. **Test Touch:** (if enabled) Tap screen to access menus
3. **Test Sensors:** Check temperature, current, speed readings
4. **Test Controls:** Verify pedal, steering, shifter respond
5. **Test Audio:** Check DFPlayer audio feedback

## Firmware Version

Current version: **v2.11.0**

Check version in code:
- File: `include/version.h`
- Constant: `FIRMWARE_VERSION`

---

**Need Help?**
- Check CHANGELOG_v2.11.0.md for detailed changes
- Review platformio.ini for configuration details
- See docs/ folder for additional documentation
