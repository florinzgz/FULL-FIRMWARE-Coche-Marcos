# ESP32-S3 Bootloop Fix - STANDALONE_DISPLAY Mode

## Problem Description

The `esp32-s3-n32r16v-standalone` environment was causing an infinite bootloop on ESP32-S3 N32R16V hardware with:
- Flash: 32MB OPI (Octal SPI)
- PSRAM: 16MB OPI (Octal SPI)

### Boot Log Symptoms

```
rst:0x3 (RTC_SW_SYS_RST),boot:0x8 (SPI_FAST_FLASH_BOOT)
Saved PC:0x403cdb0a
Octal Flash Mode Enabled
For OPI Flash, Use Default Flash Boot Mode
mode:SLOW_RD, clock div:1
load...
entry 0x403c98d0
```

**Key observations:**
- Bootloader loads successfully
- OPI Flash mode is correct
- Firmware crashes immediately after entry point (before Serial output appears)
- System resets and repeats infinitely

## Root Cause Analysis

### Primary Issue: Global TFT_eSPI Constructor

**Location:** `src/hud/hud_manager.cpp:26`

```cpp
// BEFORE (BROKEN):
TFT_eSPI tft = TFT_eSPI();
```

**Problem:** The explicit constructor call `TFT_eSPI()` executes complex initialization code during C++ global construction phase, which happens **before** `main()` or `setup()`. At this point:
- ESP32-S3 OPI PSRAM may not be fully initialized
- OPI Flash may still be in bootloader mode
- SPI bus configuration may be incomplete
- Memory allocations can fail or access invalid addresses

**Fix:**
```cpp
// AFTER (FIXED):
TFT_eSPI tft;  // Use default constructor, defer complex init to tft.init()
```

### Secondary Issue: Missing DISABLE_SENSORS Guards

**Location:** `src/managers/SensorManager.h`

The `DISABLE_SENSORS` flag was defined in `platformio.ini` but not implemented in code. Sensor initialization was still attempted even in standalone display mode.

**Fix:** Added compile-time guards:
```cpp
#ifdef DISABLE_SENSORS
    Serial.println("[SensorManager] DISABLE_SENSORS mode - skipping all sensor init");
    return true;
#else
    // Normal sensor initialization
#endif
```

### Tertiary Issue: Lack of Early Boot Diagnostics

**Problem:** Crash occurred before Serial.begin() completed, making debugging impossible.

**Fix:** Added early UART output with explicit flush and delays in `src/main.cpp`:
```cpp
#ifdef STANDALONE_DISPLAY
    Serial.begin(115200);
    delay(100);
    Serial.println("\n\n=== ESP32-S3 EARLY BOOT ===");
    Serial.println("[STANDALONE] Mode active");
    Serial.flush();
#endif
```

## Files Modified

### 1. `src/hud/hud_manager.cpp`
- **Line 26:** Changed `TFT_eSPI tft = TFT_eSPI()` to `TFT_eSPI tft`
- **Lines 49-61:** Added Serial.flush() and delay() calls before TFT init to ensure diagnostic output is visible

### 2. `src/main.cpp`
- **Lines 32-44:** Added early UART diagnostic output for STANDALONE_DISPLAY mode
- **Lines 92-97:** Added Serial.flush() calls in initializeSystem() to ensure progress messages are visible

### 3. `src/managers/SensorManager.h`
- **Lines 11-29:** Added `#ifdef DISABLE_SENSORS` guards to skip all sensor initialization
- **Lines 31-38:** Added guards to skip sensor update calls

### 4. `platformio.ini`
- **Lines 175-204:** Added new `esp32-s3-n32r16v-standalone-debug` environment with:
  - CORE_DEBUG_LEVEL=5 (maximum verbosity)
  - Bootloader verbose logging
  - UART console logging enabled
  - 10-second watchdog timeout for faster iteration

## Testing Procedure

### Step 1: Test Fixed Standalone Environment

```bash
pio run -e esp32-s3-n32r16v-standalone --target upload
pio device monitor -e esp32-s3-n32r16v-standalone
```

**Expected output:**
```
=== ESP32-S3 EARLY BOOT ===
[STANDALONE] Mode active
[BOOT] Starting vehicle firmware...
[BOOT] Firmware version: 2.11.6
...
🧪 STANDALONE DISPLAY MODE
[INIT] HUD Manager initialization...
[HUD] Starting HUDManager initialization...
[HUD] Initializing TFT_eSPI...
[HUD] TFT_eSPI init SUCCESS
...
🧪 STANDALONE: Skipping other managers
[BOOT] System initialization complete
```

### Step 2: If Bootloop Persists, Use Debug Environment

```bash
pio run -e esp32-s3-n32r16v-standalone-debug --target upload
pio device monitor -e esp32-s3-n32r16v-standalone-debug -f esp32_exception_decoder
```

The debug environment provides:
- Maximum verbosity at all levels
- Detailed bootloader logging
- Exception decoder for crash analysis
- Shorter watchdog timeout (10s) for faster iteration

### Step 3: Verify Display Functionality

Once boot is successful, the display should show:
1. Black screen during initialization
2. "Mercedes AMG GT" title
3. "Esperando sensores..." message (in standalone mode)
4. Dashboard with simulated data (speed, RPM, gauges)

In standalone mode, the system uses hardcoded demo values:
- Speed: 12.0 km/h
- RPM: 850
- Pedal: 50%
- All sensors: OK (simulated)
- Gear: P (Park)

## Additional Diagnostic Environments

### Production Environment (Optimized)
```bash
pio run -e esp32-s3-n32r16v-release --target upload
```
- Minimal logging
- Optimizations enabled (-O3)
- Best performance

### Touch Debug Environment
```bash
pio run -e esp32-s3-n32r16v-touch-debug --target upload
```
- Touch diagnostics enabled
- Slower touch SPI frequency (1 MHz)
- Verbose touch logging

### No-Touch Environment
```bash
pio run -e esp32-s3-n32r16v-no-touch --target upload
```
- Touch completely disabled
- Use if touch controller causes SPI conflicts

## Technical Deep Dive

### Why Global Constructors Are Dangerous on ESP32-S3

The ESP32-S3 boot sequence:

1. **ROM Bootloader** (in mask ROM)
   - Initializes basic UART for logging
   - Configures flash mode (QIO/OPI)
   - Loads 2nd stage bootloader from flash

2. **2nd Stage Bootloader** (from flash partition)
   - Initializes OPI PSRAM (if enabled)
   - Verifies app partitions
   - Loads app firmware into RAM
   - Jumps to app entry point

3. **C Runtime Initialization** (before main)
   - Initializes .data and .bss sections
   - **Calls global C++ constructors** ← CRASH POINT
   - Sets up FreeRTOS
   - Calls app_main() → setup()

4. **Application** (main/setup/loop)
   - User code runs
   - Arduino framework initialized
   - setup() called
   - loop() runs forever

**The Problem:** At step 3, OPI PSRAM is initialized but may not be stable. Large memory allocations or complex SPI transactions (like TFT_eSPI constructor) can crash the system.

**The Solution:** Keep global constructors minimal. Defer complex initialization to explicit init() functions called from setup().

### OPI (Octal SPI) Flash/PSRAM Considerations

OPI mode uses 8 data lines instead of 1 (SPI) or 4 (QPI/QIO):
- **Higher bandwidth:** Up to 8x faster than standard SPI
- **More sensitive to timing:** Clock delays, signal integrity matter more
- **Initialization complexity:** Requires precise boot sequence

OPI peripherals must be initialized in this order:
1. Flash controller (by bootloader)
2. PSRAM controller (by 2nd stage bootloader)
3. Application code can use both

Any SPI transactions before PSRAM is stable can cause crashes, especially if they trigger PSRAM allocations (like large framebuffers in TFT_eSPI).

## Verification Checklist

- [ ] No bootloop (system completes boot sequence)
- [ ] Serial output appears within 2 seconds
- [ ] "STANDALONE DISPLAY MODE" message visible
- [ ] HUD Manager initializes successfully
- [ ] Display shows "Mercedes AMG GT" title
- [ ] Dashboard renders with simulated data
- [ ] No crash or reset for at least 60 seconds
- [ ] Watchdog doesn't trigger
- [ ] Memory diagnostics show healthy PSRAM usage

## Common Issues and Solutions

### Issue: Still bootloops after fix

**Solution 1:** Check if there are other global constructors
```bash
nm -C .pio/build/esp32-s3-n32r16v-standalone/firmware.elf | grep "global constructors"
```

**Solution 2:** Verify OPI Flash/PSRAM configuration in platformio.ini
```ini
board_build.flash_mode = qio    # OPI handled by bootloader
board_build.psram = enabled
board_build.psram_size = 16MB
-DCONFIG_SPIRAM_MODE_OCT=1      # Enable OPI PSRAM
```

**Solution 3:** Check for stack overflow
```bash
# Use debug environment to see stack usage
pio run -e esp32-s3-n32r16v-standalone-debug --target upload
```

### Issue: Display stays black

**Possible causes:**
1. TFT_BL (backlight) not configured
2. Wrong SPI pins
3. Display not powered
4. SPI bus conflict with touch controller

**Debug:**
```cpp
// Add to setup() before HUDManager::init()
pinMode(PIN_TFT_BL, OUTPUT);
digitalWrite(PIN_TFT_BL, HIGH);
delay(100);
```

### Issue: Touch doesn't work in standalone mode

**Expected behavior:** Touch is enabled in standalone mode and should work. A "MENU" button appears in bottom-right corner for accessing demo features.

**If touch fails:**
1. Use `esp32-s3-n32r16v-no-touch` environment
2. Calibrate touch using hidden menu (battery icon x4, code 8989)
3. Check touch calibration in storage

## References

- ESP32-S3 Technical Reference Manual: https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf
- TFT_eSPI Library: https://github.com/Bodmer/TFT_eSPI
- PlatformIO ESP32 Documentation: https://docs.platformio.org/en/latest/platforms/espressif32.html

## Version History

- **v2.11.6:** Fixed bootloop in standalone mode (global TFT constructor, DISABLE_SENSORS guards)
- **v2.11.5:** Added fault tolerance for display init
- **v2.11.0:** Initial STANDALONE_DISPLAY mode implementation

---

**Author:** Copilot AI Assistant  
**Date:** 2026-01-07  
**Firmware Version:** 2.11.6-BOOTLOOP-FIX
