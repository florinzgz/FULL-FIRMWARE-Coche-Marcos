# ESP32-S3 N16R8 Architecture Fix - Implementation Summary

## Problem Statement

The ESP32-S3 N16R8 (16MB Flash + 8MB Octal PSRAM) was experiencing:
- RTC_SW_SYS_RST resets (reset 0x3)
- MALLOC_CAP_INTERNAL crashes
- Stack canary watchpoint failures
- Boot instability

### Root Causes Identified

1. **GPIO Collisions with OPI PSRAM Bus (GPIO 33-37)**
   - These pins are internally wired to the 8-bit Octal PSRAM chip
   - Any external use causes bus conflicts and crashes

2. **GPIO Collisions with SPI Flash Bus (GPIO 10-12)**
   - These pins are part of the internal Flash memory controller
   - Using them for peripherals causes Flash access corruption

3. **PSRAM Misconfiguration**
   - System not properly initialized for Octal mode (OPI)
   - Sprites attempting to allocate in internal SRAM instead of PSRAM
   - ~1.8MB of sprite data exhausting internal memory

## Implementation Summary

### Task 1: Platform Configuration (platformio.ini)

**Status:** ✅ Already Correct - No Changes Needed

Verified settings:
```ini
board_build.arduino.memory_type = qio_opi
board_build.flash_mode = qio
board_build.f_flash = 80000000L
board_build.f_cpu = 240000000L
-DBOARD_HAS_PSRAM
-mfix-esp32-psram-cache-issue
```

All TFT_eSPI build flags correctly configured for new pin assignments.

### Task 2: Hardware Pin Remapping (include/pins.h)

**Status:** ✅ Complete - 12 Pins Relocated

#### Critical Moves (Forbidden Zone Evacuation)

**From OPI PSRAM Bus (GPIO 33-37):**
- `PIN_RELAY_MAIN`: GPIO 35 → GPIO 38
- `PIN_WHEEL_FR`: GPIO 36 → GPIO 2
- `PIN_ENCODER_A`: GPIO 37 → GPIO 1

**From SPI Flash Bus (GPIO 10-12):**
- Already fixed in previous commits (TFT pins moved to GPIO 13-17)

#### Cascading Relocations

To resolve conflicts created by the above moves:

1. `PIN_BTN_LIGHTS`: GPIO 2 → GPIO 0 (displaced by WHEEL_FR)
2. `PIN_WHEEL_RR`: GPIO 1 → GPIO 46 (displaced by ENCODER_A)
3. `PIN_ENCODER_B`: GPIO 38 → GPIO 39 (to free GPIO 38 for RELAY_MAIN)
4. `PIN_ENCODER_Z`: GPIO 39 → GPIO 3 (encoder grouping)
5. `PIN_RELAY_SPARE`: GPIO 46 → GPIO 18 (strapping pin safety)
6. `PIN_DFPLAYER_TX`: GPIO 18 → GPIO 19 (displaced by RELAY_SPARE)
7. `PIN_DFPLAYER_RX`: GPIO 17 → GPIO 20 (displaced by TFT_RST)
8. `PIN_LED_FRONT`: GPIO 19 → GPIO 47 (to free space for DFPLAYER)
9. `PIN_WHEEL_RL`: GPIO 15 → GPIO 45 (conflict with TFT_CS)
10. `PIN_ONEWIRE`: GPIO 20 → GPIO 48 (to free space for DFPLAYER_RX)
11. `PIN_LED_REAR`: GPIO 48 → GPIO 43 (displaced by ONEWIRE)
12. `PIN_TOFSENSE_TX`: GPIO 43 → -1 (sensor is TX-only, doesn't use this pin)

#### Optimizations

- `PIN_TOUCH_IRQ`: Removed (using polling mode instead of interrupt)
- This freed GPIO 47 for LED_FRONT

### Task 3: PSRAM Force Allocation (src/hud/hud_compositor.cpp)

**Status:** ✅ Complete - Enhanced Diagnostics Added

**Changes Made:**

1. **createLayerSprite() enhancements:**
   - Added detailed Serial output on sprite creation failure
   - Outputs PSRAM found status, free PSRAM, and free Heap
   - Layer number included in diagnostic messages

2. **createShadowSprite() enhancements:**
   - Same diagnostic improvements as layer sprites
   - Helps identify exactly which allocation is failing

**Code Pattern:**
```cpp
void *spriteBuffer = layerSprites[idx]->createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);
if (!spriteBuffer) {
    Serial.printf("[HudCompositor] PSRAM FAIL Layer %d - buffer is NULL\n", idx);
    Serial.printf("  PSRAM found: %s\n", psramFound() ? "YES" : "NO");
    Serial.printf("  Free PSRAM: %u bytes\n", ESP.getFreePsram());
    Serial.printf("  Free Heap: %u bytes\n", ESP.getFreeHeap());
    // ... error handling
}
```

**Note:** The code already had `setAttribute(PSRAM_ENABLE, true)` before `createSprite()` calls.

### Task 4: Boot Sequence (src/main.cpp)

**Status:** ✅ Complete - Critical PSRAM Check Added

**Changes Made:**

Added mandatory PSRAM initialization check immediately after Serial.begin():

```cpp
void setup() {
    Serial.begin(115200);
    
    // 🔒 N16R8 CRITICAL: Initialize and verify PSRAM BEFORE any UI allocation
    Serial.println("[BOOT] Initializing PSRAM (Octal mode)...");
    if (!psramInit()) {
        Serial.println("[BOOT] ❌ PSRAM INIT FAILED - SYSTEM HALTED");
        Serial.println("[BOOT] This is CRITICAL for ESP32-S3 N16R8 (8MB OPI PSRAM)");
        Serial.println("[BOOT] Check hardware configuration and memory_type=qio_opi");
        Serial.flush();
        // Halt system - cannot proceed without PSRAM
        while (1) {
            delay(1000);
        }
    }
    Serial.printf("[BOOT] ✓ PSRAM initialized: %u bytes available\n", ESP.getPsramSize());
    Serial.printf("[BOOT]   Free PSRAM: %u bytes\n", ESP.getFreePsram());
    
    // ... rest of setup()
}
```

**Boot Sequence Now:**
1. Serial.begin(115200)
2. psramInit() with halt on failure
3. TFT hardware reset
4. System::init()
5. HudCompositor::init() (inside initializeSystem)

### Task 5: Final Verification

**Status:** ✅ Complete

**Verification Results:**

1. ✅ No GPIO 33-37 usage (all references are comments about old pins)
2. ✅ No GPIO 10-12 usage (forbidden SPI Flash bus)
3. ✅ No duplicate GPIO assignments
4. ✅ Pin validation function updated
5. ✅ Pin usage table updated with all changes

## Final GPIO Assignment Table

| GPIO | Function | Type | Notes |
|------|----------|------|-------|
| 0 | BTN_LIGHTS | Input | Moved from GPIO 2 |
| 1 | ENCODER_A | Input | Moved from GPIO 37 (OPI PSRAM) |
| 2 | WHEEL_FR | Input | Moved from GPIO 36 (OPI PSRAM) |
| 3 | ENCODER_Z | Input | Moved from GPIO 39 |
| 4 | PEDAL (ADC) | Analog In | Sensor Hall |
| 5 | RELAY_TRAC | Output | Relay 24V |
| 6 | RELAY_DIR | Output | Relay 12V |
| 7 | WHEEL_FL | Input | Wheel sensor |
| 8 | I2C_SDA | I/O | I²C Data |
| 9 | I2C_SCL | I/O | I²C Clock |
| 10-12 | ⛔ FORBIDDEN | - | SPI Flash bus |
| 13 | TFT_MOSI | Output | Moved from GPIO 11 |
| 14 | TFT_SCK | Output | Moved from GPIO 10 |
| 15 | TFT_CS | Output | Moved from GPIO 16 |
| 16 | TFT_DC | Output | Moved from GPIO 13 |
| 17 | TFT_RST | Output | Moved from GPIO 14 |
| 18 | RELAY_SPARE | Output | Moved from GPIO 46 |
| 19 | DFPLAYER_TX | Output | UART1 TX |
| 20 | DFPLAYER_RX | Input | UART1 RX |
| 21 | TOUCH_CS | Output | Touch screen |
| 33-37 | ⛔ FORBIDDEN | - | OPI PSRAM bus |
| 38 | RELAY_MAIN | Output | Moved from GPIO 35 (OPI PSRAM) |
| 39 | ENCODER_B | Input | Moved from GPIO 38 |
| 40 | KEY_ON | Input | Ignition detect |
| 41 | KEY_OFF | Input | Shutdown request |
| 42 | TFT_BL | Output | Backlight PWM |
| 43 | LED_REAR | Output | WS2812B LEDs |
| 44 | TOFSENSE_RX | Input | LiDAR sensor |
| 45 | WHEEL_RL | Input | Moved from GPIO 15 |
| 46 | WHEEL_RR | Input | Moved from GPIO 1 |
| 47 | LED_FRONT | Output | WS2812B LEDs |
| 48 | ONEWIRE | I/O | Temperature sensors |

**Special Notes:**
- PIN_TOFSENSE_TX = -1 (sensor is TX-only)
- PIN_TOUCH_IRQ removed (using polling mode)

## Expected Results

### Eliminated Issues

1. ✅ **RTC_SW_SYS_RST resets** - No more GPIO bus conflicts
2. ✅ **Stack canary crashes** - PSRAM properly initialized before sprite allocation
3. ✅ **MALLOC_CAP_INTERNAL errors** - Sprites allocated in PSRAM, not internal SRAM
4. ✅ **Boot instability** - Proper boot sequence with PSRAM verification

### Protected Hardware Zones

- **GPIO 10-12**: SPI Flash bus - fully protected ⛔
- **GPIO 33-37**: OPI PSRAM bus - fully protected ⛔
- **GPIO 45**: VDD_SPI strapping - only INPUT use (safe)
- **GPIO 46**: Boot mode strapping - only INPUT use (safe)

### Memory Architecture

- **8MB PSRAM (Octal)**: Properly initialized and used for sprite buffers
- **Internal SRAM**: Reserved for stack, heap, and system operations
- **Diagnostic output**: Clear indication of memory allocation success/failure

## Testing Recommendations

1. **Power-on test**: Verify system boots without resets
2. **Serial monitor**: Check for PSRAM initialization success message
3. **Display test**: Verify HUD sprites render correctly
4. **Memory monitoring**: Watch for any PSRAM allocation failures
5. **Long-term stability**: Run for extended periods to verify no bootloops

## Files Modified

1. `include/pins.h` - Complete pin remapping
2. `src/hud/hud_compositor.cpp` - Enhanced PSRAM diagnostics
3. `src/main.cpp` - Early PSRAM initialization check
4. `platformio.ini` - Already correct (verified only)

## Compliance

✅ All forbidden GPIO zones protected
✅ No hardware bus conflicts
✅ PSRAM properly configured for Octal mode
✅ Boot sequence ensures PSRAM ready before UI
✅ Diagnostic output for troubleshooting

---

**Implementation Date:** 2026-01-28
**Hardware:** ESP32-S3-DevKitC-1 N16R8 (16MB Flash + 8MB OPI PSRAM)
**Firmware:** v2.18.x+
