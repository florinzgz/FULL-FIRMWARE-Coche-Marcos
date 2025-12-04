# Touch Screen Calibration Guide

## Overview
The ESP32-S3 Car Control System now includes a dynamic touch screen calibration feature for the XPT2046 touch controller. This allows users to calibrate their touch screen for accurate touch input.

## Why Calibration is Needed
Different ST7796S display units with XPT2046 touch controllers may have slightly different offsets and ranges. Calibration ensures that touch inputs accurately map to the displayed content.

## How to Access Calibration

### Method 1: Via Hidden Menu (Recommended)
1. Touch and hold the **battery icon** on the dashboard
2. Enter the access code: **8989** (using the directional buttons or touch)
3. From the hidden menu, select **Option 3: Calibrar touch**

### Method 2: STANDALONE_DISPLAY Mode
In standalone display mode (test mode), you can access the hidden menu by:
1. Long-press (1.5 seconds) the demo button in the bottom-right corner
2. Select **Option 3: Calibrar touch** from the menu

## Calibration Process

### Step 1: Instructions Screen
- The screen will display calibration instructions
- **Touch anywhere on the screen** to begin

### Step 2: First Calibration Point (Top-Left)
- A **RED crosshair target** will appear in the top-left corner
- **Touch and hold** the center of the red target
- The system will collect **10 samples** (progress bar shown)
- Keep touching until the progress bar is complete

### Step 3: Second Calibration Point (Bottom-Right)
- A **RED crosshair target** will appear in the bottom-right corner
- **Touch and hold** the center of the red target
- The system will collect **10 samples** (progress bar shown)
- Keep touching until the progress bar is complete

### Step 4: Verification
- The system will calculate the calibration values
- Calibration data will be displayed on screen:
  - Min X, Max X, Min Y, Max Y values
- The calibration is **automatically saved** to non-volatile storage
- Touch anywhere to return to the menu

## Calibration Data

The calibration process determines:
- **min_x, max_x**: Horizontal touch range (0-4095)
- **min_y, max_y**: Vertical touch range (0-4095)
- **rotation**: Screen rotation setting (3 for landscape mode)

### Default Values
If no calibration has been performed, the system uses these defaults:
- Min X: 200
- Max X: 3900
- Min Y: 200
- Max Y: 3900
- Rotation: 3

### Storage
Calibration data is stored in:
- **Location**: NVS (Non-Volatile Storage)
- **Namespace**: `vehicle`
- **Key**: `config`
- **Fields**: 
  - `cfg.touchCalibration[5]` - Array of calibration values
  - `cfg.touchCalibrated` - Boolean flag indicating calibration status

## Troubleshooting

### Touch is Inaccurate After Calibration
1. Re-run the calibration process
2. Ensure you touch the **exact center** of each target
3. Keep your finger steady during the sampling process

### Calibration Times Out
- Default timeout: **30 seconds** per point
- If timeout occurs, the system returns to the menu
- Try again with quicker, more decisive touches

### Calibration Failed Message
- Check that the touch screen is properly connected
- Verify pin connections:
  - TOUCH_CS: GPIO 21
  - TOUCH_IRQ: GPIO 47
- Ensure no SPI bus conflicts

### Restoring Default Calibration
1. Access the hidden menu
2. Select **Option 7: Restaurar fabrica** (Factory Reset)
3. This will reset all configuration, including touch calibration

## Technical Details

### Calibration Algorithm
1. **Two-point calibration**: Uses corner points to determine range
2. **Sample averaging**: Collects 10 samples per point for accuracy
3. **Margin compensation**: Accounts for edge proximity (30 pixels margin)
4. **Extrapolation**: Calculates full screen range from corner samples

### Code Structure
- **Header**: `include/touch_calibration.h`
- **Implementation**: `src/hud/touch_calibration.cpp`
- **Storage**: Integrated with `Storage::Config` system
- **Version**: Config structure v7 (updated for touch calibration)

### Hardware Specifications
- **Touch Controller**: XPT2046 (SPI)
- **ADC Resolution**: 12-bit (0-4095 range)
- **Practical Range**: 200-3900 (excludes edge zones)
- **SPI Frequency**: 2.5 MHz (XPT2046 requirement)
- **Screen Size**: 480x320 (landscape, rotation 3)

## Best Practices

1. **Perform calibration**:
   - After first installation
   - If touch accuracy degrades over time
   - After replacing the display module

2. **During calibration**:
   - Use a stylus or fingertip for precision
   - Touch the exact center of each target
   - Hold steady until progress bar completes
   - Avoid touching near the edges

3. **Verification**:
   - After calibration, test touch accuracy
   - Try touching different parts of the screen
   - Re-calibrate if needed

## Version History

- **v2.9.0** (2024-12-04): Initial implementation of dynamic touch calibration
  - Two-point calibration routine
  - Visual feedback with progress indicators
  - Persistent storage of calibration data
  - Integration with hidden menu system
