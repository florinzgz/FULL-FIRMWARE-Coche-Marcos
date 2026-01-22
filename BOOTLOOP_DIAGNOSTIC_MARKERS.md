# ESP32-S3 Bootloop Diagnostic Markers

**Date:** 2026-01-18  
**Purpose:** Binary markers to identify exact bootloop failure point  
**Status:** Active Diagnostic Tool

---

## Overview

This system uses binary character markers (`Serial.write()`) to identify exactly where the ESP32-S3 hangs during boot. These markers are more reliable than `Serial.println()` because they:

1. **Don't depend on buffer flushing** - Single byte writes are immediate
2. **Survive partial serial buffer loss** - Simple characters are easier to capture
3. **Work even if crash occurs mid-print** - No string formatting overhead

In addition, the firmware now persists a **reset marker** in RTC memory. When a
critical error exceeds the maximum retry count, the system stops feeding the
watchdog to force a reset and records `RESET_MARKER_WATCHDOG_LOOP` for the next
boot log. This behavior is intentional and should be accounted for in
telemetry/monitoring workflows.

---

## Marker Map

### Main Boot Sequence (src/main.cpp)

| Marker | Location | Meaning |
|--------|----------|---------|
| **A** | After Serial.begin() | Serial communication initialized |
| **B** | After BootGuard init | Boot counter and safety systems ready |
| **C** | After System/Storage/Watchdog init | Core systems initialized |
| **D** | Before initializeSystem() | About to start subsystem initialization |
| **E** | After initializeSystem() | All systems initialized successfully |

### HUD Initialization (src/hud/hud_manager.cpp)

| Marker | Location | Meaning |
|--------|----------|---------|
| **F** | Start of HUDManager::init() | HUD initialization started |
| **G** | After queue creation | Render event queue created |
| **H** | Before tft.init() | **CRITICAL** - About to initialize display |
| **I** | After tft.init() | **SUCCESS** - Display initialized |
| **J** | After setRotation() | Display rotation configured |
| **K** | After dashboard components | All HUD components ready |

---

## How to Use

### 1. Upload Firmware

```bash
pio run -e esp32-s3-n16r8-standalone-debug -t upload
```

### 2. Monitor Serial Output

```bash
pio device monitor
```

### 3. Interpret Results

Look for the **last marker** before the reset:

#### Nothing (no markers)
- **Problem:** Reset before `setup()`
- **Cause:** Very early crash (ROM bootloader, PSRAM init, global constructors)
- **Action:** Check watchdog timeouts in `sdkconfig/n16r8.defaults`

#### Only 'A'
- **Problem:** Crash during early boot after Serial init
- **Cause:** BootGuard or System::init() failure
- **Action:** Check RTC memory, NVS, or storage initialization

#### A, B
- **Problem:** Crash during core system initialization
- **Cause:** System::init(), Storage::init(), or Watchdog::init()
- **Action:** Check system initialization code

#### A, B, C
- **Problem:** Crash before or during Logger init
- **Cause:** Logger initialization or initializeSystem() entry
- **Action:** Check logging subsystem

#### A, B, C, D
- **Problem:** Crash inside initializeSystem()
- **Cause:** One of the subsystem managers (check standalone vs full mode)
- **Action:** Review initializeSystem() in main.cpp

#### A, B, C, D, F
- **Problem:** Crash very early in HUD initialization
- **Cause:** HUDManager constructor or early init code
- **Action:** Check HUDManager static initialization

#### A, B, C, D, F, G
- **Problem:** Crash after queue creation
- **Cause:** FreeRTOS queue or pre-TFT setup
- **Action:** Check queue configuration

#### A, B, C, D, F, G, H
- **Problem:** ⚠️ **CRASH IN tft.init()** ⚠️
- **Cause:** Display initialization hanging (most likely)
- **Possible causes:**
  - SPI bus issue
  - Display power problem
  - Incorrect pin configuration
  - Display hardware fault
- **Actions:**
  1. Check power supply voltage (must be stable ≥3.0V)
  2. Verify display connections (especially RST, CS, DC pins)
  3. Try reducing SPI_FREQUENCY to 20MHz
  4. Check for loose wiring

#### A, B, C, D, F, G, H, I
- **Problem:** Crash after tft.init() but before rotation
- **Cause:** Display initialization succeeded but post-init fails
- **Action:** Check setRotation() implementation

#### A, B, C, D, F, G, H, I, J
- **Problem:** Crash during dashboard component initialization
- **Cause:** Icons, Gauges, WheelsDisplay, or MenuHidden init
- **Action:** Check component initialization order

#### A, B, C, D, F, G, H, I, J, K
- **Problem:** Crash after dashboard init but before fillScreen
- **Cause:** Display drawing operations
- **Action:** Check TFT operations after component init

#### A, B, C, D, E (all main markers)
- **SUCCESS!** Boot completed, issue is elsewhere (not in boot sequence)
- Check loop() or runtime behavior

---

## Common Failure Patterns

### Pattern 1: Stuck at H (Before tft.init)

**Most Common Issue:** This is where ~80% of display-related bootloops occur.

**Quick Fixes to Try:**

1. **Reduce SPI frequency** - Edit `platformio.ini`:
   ```ini
   -DSPI_FREQUENCY=20000000  # Try 20MHz instead of 40MHz
   ```

2. **Increase watchdog timeout** - Edit `sdkconfig/n16r8.defaults`:
   ```ini
   CONFIG_ESP_INT_WDT_TIMEOUT_MS=5000  # Increase from 3000 to 5000
   ```

3. **Check power supply:**
   - Measure voltage during boot (should stay >3.0V)
   - Use ≥1A power supply
   - Display + backlight can draw >500mA

### Pattern 2: No markers at all

**Common Issue:** Very early crash before Serial is ready.

**Quick Fixes:**

1. **Increase bootloader watchdog** - Edit `sdkconfig/n16r8.defaults`:
   ```ini
   CONFIG_BOOTLOADER_WDT_TIME_MS=60000  # Increase from 40s to 60s
   ```

2. **Check PSRAM:**
   - Verify PSRAM is detected
   - Check for bad PSRAM chip
   - Try disabling PSRAM memory test temporarily

### Pattern 3: Stuck between D and F

**Issue:** Crash during STANDALONE_DISPLAY conditional init or before HUD.

**Check:** Review conditional compilation and STANDALONE_DISPLAY flag.

---

## Expected Normal Boot Sequence

When everything works correctly, you should see:

```
A B C D F G H I J K E
```

Followed by:
```
[BOOT] System initialization complete
```

---

## Removing Diagnostic Markers

Once the issue is identified and fixed, these markers can be removed or commented out to clean up the code. They add ~110 bytes of code and minimal boot delay (10ms per marker).

To disable without removing:
```cpp
// #define ENABLE_BOOT_DIAGNOSTICS
#ifdef ENABLE_BOOT_DIAGNOSTICS
  Serial.write('A');
  Serial.flush();
  delay(10);
#endif
```

---

## Technical Notes

### Why Serial.write() instead of Serial.println()?

1. **Atomic operation:** Single byte write is atomic, less likely to be interrupted
2. **No formatting:** No string processing or buffer allocation
3. **Immediate:** Bypasses some buffering layers
4. **Small footprint:** Single byte vs. multi-byte strings

### Why delay(10) after each marker?

Ensures the byte is physically transmitted before continuing. Without this, a crash immediately after the marker might lose the byte in the UART FIFO.

### Marker Letter Choice

Using sequential letters (A-K) makes it easy to:
- Identify the sequence in the serial output
- Quickly spot where it stops
- Distinguish from other serial noise

---

## Related Documentation

- **BOOTLOOP_FIX_N16R8_v2.17.2.md** - Watchdog timeout fix details
- **BOOTLOOP_STATUS_2026-01-18.md** - Complete bootloop status report
- **sdkconfig/n16r8.defaults** - SDK configuration with watchdog settings

---

**Version:** 1.0  
**Last Updated:** 2026-01-18  
**Maintainer:** ESP32-S3 N16R8 Boot Diagnostics
