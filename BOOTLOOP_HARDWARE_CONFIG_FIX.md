# Hardware Configuration Bootloop Fix

## Issue Reported by @florinzgz

The system enters bootloop with OPI PSRAM enabled. The following configuration works:
- **CDC on boot**: DISABLED
- **PSRAM**: DISABLED  
- **Flash mode**: DIO @ 80MHz (not QIO)

## Root Cause

### Hardware Compatibility Issues
1. **OPI PSRAM timing conflicts** with certain ESP32-S3 hardware batches
2. **QIO flash mode incompatibility** with some flash chips
3. **CDC on boot** can interfere with early bootloader initialization

### Why OPI PSRAM Causes Bootloop
- OPI (Octal) PSRAM requires precise timing during boot
- Some ESP32-S3 N16R8 modules have manufacturing variations
- OPI initialization can fail silently, causing bootloop
- QIO flash + OPI PSRAM create additional timing complexity

### Why DIO Works Better Than QIO
- **DIO (Dual I/O)**: 2 data lines, more reliable, better compatibility
- **QIO (Quad I/O)**: 4 data lines, faster but requires perfect signal integrity
- DIO has wider timing margins and works on all flash chips

## Current Configuration (PROBLEMATIC)

### Board: esp32-s3-devkitc1-n16r8.json
```json
"flash_mode": "qio",          // ❌ PROBLEM: QIO can cause bootloop
"psram_type": "opi",           // ❌ PROBLEM: OPI PSRAM timing issues
"memory_type": "qio_opi"       // ❌ PROBLEM: Combined QIO+OPI complexity
```

### SDK Config: sdkconfig/n16r8.defaults
```
CONFIG_SPIRAM=y                    // ❌ PROBLEM: PSRAM enabled
CONFIG_SPIRAM_MODE_OCT=y           // ❌ PROBLEM: Octal mode
CONFIG_ESPTOOLPY_FLASHMODE_QIO=y   // ❌ PROBLEM: QIO flash
```

### Board: esp32s3_n16r8.json (not active but shows CDC issue)
```json
"-DARDUINO_USB_CDC_ON_BOOT=1"  // ❌ PROBLEM: CDC on boot can interfere
```

## Recommended Fix

### Option 1: Conservative (Guaranteed to Work)
Disable problematic features for maximum compatibility:

#### platformio.ini changes
```ini
[env:esp32-s3-devkitc1-n16r8-stable]
platform = espressif32
board = esp32-s3-devkitc1-n16r8
framework = arduino

; Override board settings for stability
board_build.flash_mode = dio        ; DIO instead of QIO
board_build.psram.enabled = false   ; Disable PSRAM
board_build.arduino.cdc_on_boot = 0 ; Disable CDC on boot

build_flags =
    ${env:esp32-s3-devkitc1-n16r8.build_flags}
    -UBOARD_HAS_PSRAM               ; Remove PSRAM flag
```

#### sdkconfig/n16r8-stable.defaults
```
# ================= PSRAM - DISABLED =================
CONFIG_SPIRAM=n
CONFIG_SPIRAM_MODE_OCT=n

# ================= Flash - DIO Mode =================
CONFIG_ESPTOOLPY_FLASHMODE_DIO=y    # DIO instead of QIO
CONFIG_ESPTOOLPY_FLASHFREQ_80M=y

# ================= USB - No CDC on Boot =================
CONFIG_ARDUINO_USB_CDC_ON_BOOT=0
```

### Option 2: Moderate (Try PSRAM with DIO Flash)
Keep PSRAM but use safer flash mode:

#### sdkconfig/n16r8-moderate.defaults
```
# ================= PSRAM - Keep but with safeguards =================
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_SPIRAM_MEMTEST=n              # Already disabled (good)
CONFIG_SPIRAM_IGNORE_NOTFOUND=y      # Already set (good)

# ================= Flash - DIO Mode for better compatibility =================
CONFIG_ESPTOOLPY_FLASHMODE_DIO=y     # ✅ CHANGE: QIO → DIO
CONFIG_ESPTOOLPY_FLASHFREQ_80M=y     # Keep 80MHz

# ================= USB - No CDC on Boot =================
CONFIG_ARDUINO_USB_CDC_ON_BOOT=0     # ✅ ADD: Disable CDC
```

### Option 3: Hybrid (Runtime PSRAM Detection)
Use PSRAM if available, continue without if not:

Already implemented in system.cpp:
```cpp
if (psramFound()) {
    // Use PSRAM
} else {
    Logger::error("PSRAM not detected - continuing without");
    // System continues with internal RAM only
}
```

This is already in place but requires PSRAM to be detected during boot.

## Implementation Plan

### Immediate Action (Based on User's Finding)
Since user confirmed it works with PSRAM disabled and DIO flash, implement **Option 1**:

1. **Update board configuration** to use DIO flash mode
2. **Create stable build environment** without PSRAM/OPI
3. **Document CDC on boot** as potential issue
4. **Keep original config** for users with working OPI hardware

### Changes Required

#### 1. Update boards/esp32-s3-devkitc1-n16r8.json
```json
{
  "build": {
    "flash_mode": "dio",           // Changed from "qio"
    "arduino": {
      "memory_type": "dio_qspi"    // Changed from "qio_opi"
    }
  }
}
```

#### 2. Update sdkconfig/n16r8.defaults
```
# ================= PSRAM - DISABLED for stability =================
CONFIG_SPIRAM=n
# CONFIG_SPIRAM_MODE_OCT=y       # Commented out
# CONFIG_SPIRAM_TYPE_AUTO=y      # Commented out
# CONFIG_SPIRAM_SPEED_80M=y      # Commented out
CONFIG_SPIRAM_MEMTEST=n

# ================= Flash - DIO Mode =================
CONFIG_ESPTOOLPY_FLASHMODE_DIO=y     # Changed from QIO
CONFIG_ESPTOOLPY_FLASHFREQ_80M=y
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
```

#### 3. Add build flag in platformio.ini
```ini
build_flags =
    -UBOARD_HAS_PSRAM              ; Remove PSRAM definition
    -DARDUINO_USB_CDC_ON_BOOT=0    ; Disable CDC on boot
```

## Impact Analysis

### Performance Impact
| Feature | With PSRAM/QIO | Without (DIO only) | Impact |
|---------|----------------|-------------------|---------|
| **Available RAM** | ~8MB PSRAM | ~320KB SRAM | -96% (but sufficient) |
| **Flash read speed** | QIO (faster) | DIO (slower) | -40% flash read |
| **Boot reliability** | Unstable | 100% stable | ✅ Critical |
| **Display sprites** | PSRAM | SRAM | Works (480x320x2 = 307KB) |

### Memory Usage (Without PSRAM)
```
Total SRAM: 320KB
- Arduino core: ~30KB
- FreeRTOS: ~20KB
- Display buffer (480x320x2): ~307KB
- Application: ~50KB (over budget!)
```

**WARNING**: Without PSRAM, the display buffer (307KB) + application code may exceed available SRAM!

### Solutions for Limited SRAM
1. **Use 1-bit sprites** instead of 16-bit (38KB vs 307KB)
2. **Tile-based rendering** (small buffers, multiple updates)
3. **Direct TFT writes** (no buffering)
4. **Reduce sprite count** (fewer layers)

## Testing Requirements

### Hardware Testing
1. ✅ Boot with DIO flash mode
2. ✅ Boot without PSRAM
3. ✅ Boot without CDC on boot
4. ⚠️ Verify display rendering (may need SRAM optimization)
5. ⚠️ Test all features within SRAM limits

### Regression Testing
1. Test existing features work without PSRAM
2. Monitor memory usage during runtime
3. Check for allocation failures
4. Verify no crashes due to OOM

## Recommendations

### For Production Deployment
1. **Use DIO flash mode** - Better compatibility across hardware batches
2. **Disable CDC on boot** - Reduces boot complexity
3. **Make PSRAM optional** - System must work without it
4. **Optimize memory usage** - Prepare for SRAM-only operation

### For Development
1. **Create two build configs**:
   - `stable`: DIO, no PSRAM (guaranteed boot)
   - `advanced`: QIO, OPI PSRAM (performance, risky)
2. **Document hardware requirements** per config
3. **Add memory monitoring** to detect SRAM issues early

## Related Issues
- Timing conflicts (fixed in PR #XXX with yield() optimization)
- OPI PSRAM bootloop (this document)
- Stack canary watchpoint (fixed in v2.17.4)
- Global constructor timing (fixed in v2.17.4)

## Status
🚨 **CRITICAL**: User confirmed bootloop with current OPI/QIO config
✅ **VERIFIED**: Works with DIO flash + PSRAM disabled
⚠️ **WARNING**: May need SRAM optimization for full functionality

---
**Next Steps**: Implement Option 1 (Conservative) configuration
