# ESP32-S3 Bootloop Fix - Testing Guide
## Quick Start for Hardware Validation

---

## Pre-Testing Checklist

Before flashing the fixed firmware:

- [ ] ESP32-S3 DevKitC-1 N16R8 board available
- [ ] USB-C cable connected
- [ ] PlatformIO installed
- [ ] Serial monitor ready (115200 baud)

---

## Flashing Instructions

### 1. Enter Bootloader Mode (if needed)

If the device is currently in a bootloop:

1. Hold **BOOT** button
2. Press and release **RESET** button
3. Release **BOOT** button
4. Device should enumerate as USB Serial/JTAG

### 2. Flash Firmware

```bash
cd /path/to/FULL-FIRMWARE-Coche-Marcos
platformio run -e esp32-s3-n16r8 --target upload
```

Or use the PlatformIO IDE upload button.

### 3. Monitor Serial Output

```bash
platformio device monitor -e esp32-s3-n16r8
```

---

## Expected Boot Sequence

### Diagnostic Markers (in order)

If all fixes are working, you should see these markers WITHOUT crashes:

```
A - Serial initialized
B - Boot counter initialized
C - System init started
D - I2C initialized
E - Storage initialized
F - HUD init started
G - Render queue created
H - Before tft.init()
I - tft.init() SUCCESS
J - Rotation set
K - Dashboard components initialized
```

### Success Messages

Look for these initialization messages:

```
[HUD] Allocating TFT_eSPI object...
[HUD] TFT_eSPI object allocated successfully
[HUD] TFT_eSPI init SUCCESS
[Temperature] Sensors initialized: X/Y OK
[DFPlayer] Initialized on UART1 (GPIO18/17)
```

---

## Testing Scenarios

### Test 1: Basic Boot (CRITICAL)

**Purpose:** Verify no bootloop occurs

**Steps:**
1. Flash firmware
2. Press RESET button
3. Watch serial output

**Expected:**
- ✅ Device boots without crash
- ✅ COM port stays enumerated
- ✅ All diagnostic markers appear
- ✅ TFT backlight turns on

**Failure Indicators:**
- ❌ "Stack canary watchpoint triggered"
- ❌ "Guru Meditation Error"
- ❌ COM port disappears
- ❌ Infinite reset loop

### Test 2: Power Cycle (CRITICAL)

**Purpose:** Verify consistent boot behavior

**Steps:**
1. Power off device (unplug USB)
2. Wait 5 seconds
3. Power on device (plug USB)
4. Repeat 5 times

**Expected:**
- ✅ All 5 boots successful
- ✅ Boot counter stays low (< 3)
- ✅ No random crashes

### Test 3: TFT Display Initialization

**Purpose:** Verify display works with pointer-based init

**Steps:**
1. Boot device
2. Observe TFT screen

**Expected:**
- ✅ Backlight turns on
- ✅ Boot screen appears (blue background)
- ✅ "Ready" message displays
- ✅ Dashboard loads

**Failure Indicators:**
- ❌ Black screen (backlight off)
- ❌ White/random pixels
- ❌ Crash after "Before tft.init()"

### Test 4: Temperature Sensor Initialization

**Purpose:** Verify OneWire/DallasTemperature work with pointers

**Steps:**
1. Boot device with DS18B20 sensors connected to PIN_ONEWIRE
2. Check serial output

**Expected:**
```
[Temperature] DS18B20 0: ROM=0x28XXXXXXXXXXXXXXXX
[Temperature] DS18B20 1: ROM=0x28XXXXXXXXXXXXXXXX
[Temperature] Sensors initialized: 2/2 OK
```

**Failure Indicators:**
- ❌ "Failed to allocate OneWire object" (error 395)
- ❌ "Failed to allocate DallasTemperature object" (error 396)
- ❌ No temperature readings

### Test 5: DFPlayer Audio Initialization

**Purpose:** Verify HardwareSerial/DFPlayer work with pointers

**Steps:**
1. Boot device with DFPlayer Mini connected to GPIO17/18
2. Check serial output

**Expected:**
```
[DFPlayer] Initialized on UART1 (GPIO18/17)
```

**Failure Indicators:**
- ❌ "Failed to allocate HardwareSerial object" (error 698)
- ❌ "Failed to allocate DFRobotDFPlayerMini object" (error 699)
- ❌ "DFPlayer init failed on UART1" (error 700)

### Test 6: Memory Allocation Stress Test

**Purpose:** Verify pointer allocations don't fragment heap

**Steps:**
1. Boot device
2. Let it run for 5 minutes
3. Monitor free heap via serial

**Expected:**
- ✅ Heap remains stable
- ✅ No "allocation failed" errors
- ✅ No crashes over time

---

## Troubleshooting

### Issue: Still Getting Bootloop

**Possible Causes:**
1. Old firmware still cached in flash
2. Partition table mismatch
3. Hardware defect

**Solutions:**
```bash
# Full erase and reflash
platformio run -e esp32-s3-n16r8 --target erase
platformio run -e esp32-s3-n16r8 --target upload
```

### Issue: TFT Stays Black

**Check:**
- Backlight GPIO (PIN_TFT_BL = 42)
- TFT connections (SPI pins)
- Serial output for "tft.init() SUCCESS"

**Debug:**
```cpp
// Add after tft->init()
Serial.printf("TFT width=%d, height=%d\n", tft->width(), tft->height());
```

Should print: `TFT width=480, height=320`

### Issue: Error Code 395/396 (Temperature)

**Meaning:**
- 395: OneWire allocation failed
- 396: DallasTemperature allocation failed

**Causes:**
- Out of memory (unlikely with 8MB PSRAM)
- Heap fragmentation

**Solution:**
Check available heap before allocation:
```cpp
Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
```

Should show > 100KB free.

### Issue: Error Code 698/699 (DFPlayer)

**Meaning:**
- 698: HardwareSerial allocation failed
- 699: DFRobotDFPlayerMini allocation failed

**Causes:**
- Out of memory
- UART1 already in use

**Solution:**
Verify no other code uses UART1 (GPIO 17/18).

---

## Performance Validation

### Memory Usage

Run this command after boot:
```cpp
Serial.printf("RAM: %d / %d bytes\n", ESP.getHeapSize() - ESP.getFreeHeap(), ESP.getHeapSize());
Serial.printf("PSRAM: %d / %d bytes\n", ESP.getPsramSize() - ESP.getFreePsram(), ESP.getPsramSize());
```

**Expected:**
- RAM: < 100KB used (out of 8MB)
- PSRAM: Variable (depends on sprites)

### Boot Time

Measure time from power-on to "Ready" screen:

**Expected:** < 5 seconds

**Benchmark:**
- Serial init: ~100ms
- TFT init: ~500ms
- Sensor init: ~1000ms
- Total: ~2-3 seconds

---

## Success Criteria

The bootloop fix is considered **SUCCESSFUL** if:

- [x] Device boots without "Stack canary watchpoint triggered"
- [x] COM port stays enumerated through entire boot
- [x] All diagnostic markers A-K appear in serial output
- [x] TFT display initializes and shows content
- [x] No crashes during first 5 minutes of operation
- [x] Power cycling 5x produces consistent results
- [x] All error codes (395, 396, 601, 698, 699) never appear
- [x] Free heap remains > 100KB after boot

---

## Reporting Results

If testing is successful, report:
```
✅ BOOTLOOP FIX VERIFIED
- Hardware: ESP32-S3 N16R8 (16MB Flash + 8MB PSRAM)
- Firmware: v2.17.4
- Boot time: X seconds
- Free heap: X KB
- All systems initialized successfully
```

If testing fails, report:
```
❌ BOOTLOOP STILL PRESENT
- Last diagnostic marker: X
- Error code: XXX (if any)
- Serial output: [attach log]
- Crash backtrace: [if available]
```

---

## Next Steps After Successful Testing

1. Update firmware version to 2.17.4 in SystemConfig.h
2. Merge PR to main branch
3. Tag release as v2.17.4-bootloop-fix
4. Update CHANGELOG.md
5. Close related GitHub issues

---

**Version:** 2.17.4  
**Date:** 2026-01-22  
**Status:** Ready for Hardware Testing
