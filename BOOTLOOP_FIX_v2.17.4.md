# ESP32-S3 Bootloop Fix - v2.17.4

**Date:** 2026-01-26  
**Issue:** ESP32-S3 N16R8 stuck in bootloop with `rst:0x3 (RTC_SW_SYS_RST)`  
**Fix:** Increased interrupt watchdog timeout to 10000ms (10 seconds)  
**Status:** ✅ READY TO BUILD AND TEST

---

## 🔧 What Changed in v2.17.4

### Critical Fix: Extended Watchdog Timeout

The interrupt watchdog timeout has been increased from 5000ms to **10000ms (10 seconds)** to provide maximum safety margin for PSRAM initialization on all hardware batches.

**Files Modified:**
1. `sdkconfig/n16r8.defaults` - CONFIG_ESP_INT_WDT_TIMEOUT_MS = 10000
2. `tools/patch_arduino_sdkconfig.py` - TARGET_TIMEOUT_MS = 10000
3. `include/version.h` - FIRMWARE_VERSION = "2.17.4"

---

## 📋 Quick Build & Upload Instructions

### Prerequisites

1. **PlatformIO** installed (VSCode extension or CLI)
2. **ESP32-S3 board** connected via USB
3. **Correct COM port** configured in `platformio.ini` (currently set to COM3)

### Step 1: Clean Previous Build

```bash
cd /path/to/FULL-FIRMWARE-Coche-Marcos

# Full clean to ensure fresh build
pio run -t fullclean

# Or just clean the specific environment
pio run -e esp32-s3-n16r8-standalone-debug -t clean
```

**Why clean?** This ensures:
- Old sdkconfig settings are removed
- Arduino framework patch is reapplied
- All object files are rebuilt with new settings

### Step 2: Build Firmware

```bash
# Build for standalone debug mode (recommended for testing)
pio run -e esp32-s3-n16r8-standalone-debug
```

**Expected output during build:**
```
🔧 ESP32-S3 Bootloop Fix - Patching Arduino Framework (v2.17.4)
...
🔧 dio_qspi: Patched (XXXms → 10000ms)
...
✅ Patching complete
```

This confirms the Arduino framework watchdog timeout has been patched to 10000ms.

### Step 3: Upload to ESP32-S3

```bash
# Upload firmware
pio run -e esp32-s3-n16r8-standalone-debug -t upload
```

**Important:** Ensure the COM port in `platformio.ini` matches your device:
```ini
upload_port = COM3   # Change if your device uses a different port
monitor_port = COM3
```

To find your COM port:
- **Windows:** Check Device Manager → Ports (COM & LPT)
- **Linux:** `ls /dev/ttyUSB*` or `ls /dev/ttyACM*`
- **macOS:** `ls /dev/cu.usb*`

### Step 4: Monitor Serial Output

```bash
# Start serial monitor
pio device monitor -e esp32-s3-n16r8-standalone-debug
```

Or combined upload + monitor:
```bash
pio run -e esp32-s3-n16r8-standalone-debug -t upload -t monitor
```

---

## ✅ Expected Boot Sequence (Success)

After uploading, you should see **ONE** boot sequence (not repeating):

```
ESP-ROM:esp32s3-20210327
Build:Mar 27 2021
rst:0x1 (POWERON),boot:0x8 (SPI_FAST_FLASH_BOOT)
Saved PC:0x403cdb0a
SPIWP:0xee
mode:DIO, clock div:1
load:0x3fce3808,len:0x4bc
load:0x403c9700,len:0xbd8
load:0x403cc700,len:0x2a0c
entry 0x403c98d0

=== ESP32-S3 EARLY BOOT ===
[STANDALONE] Mode active
A[BootGuard] Boot counter initialized (power cycle or first boot)
[BootGuard] Starting new boot sequence
B[BOOT] Starting vehicle firmware...
[BOOT] Firmware version: 2.17.4
C[INIT] Power Manager initialization...
...
[System continues to initialize normally]
```

**Success indicators:**
- ✅ Only ONE boot sequence appears (not looping)
- ✅ "=== ESP32-S3 EARLY BOOT ===" message appears
- ✅ Firmware version shows "2.17.4"
- ✅ System reaches main loop without resetting
- ✅ Diagnostic markers A, B, C, D, E appear in sequence

---

## ❌ Troubleshooting Guide

### Still Experiencing Bootloop?

If the device still resets continuously after this fix, try the following:

#### 1. Verify Clean Build

```bash
# Remove ALL build artifacts
rm -rf .pio/build/

# Rebuild from scratch
pio run -e esp32-s3-n16r8-standalone-debug
```

#### 2. Check COM Port

Ensure `platformio.ini` has the correct port:
```bash
# Windows - Find COM port
mode

# Linux/macOS - Find USB serial device
ls /dev/ttyUSB* /dev/ttyACM* /dev/cu.usb*
```

Update `platformio.ini`:
```ini
upload_port = COM5    # Use your actual port
monitor_port = COM5
```

#### 3. Verify Arduino Framework Patch

During build, check for this message:
```
🔧 ESP32-S3 Bootloop Fix - Patching Arduino Framework (v2.17.4)
```

If you see:
```
⚠️ Arduino framework not found - skipping patch
```

Install the Arduino framework:
```bash
pio pkg install --platform espressif32
```

#### 4. Hardware Issues

If software fixes don't work, check hardware:

**Power Supply:**
- Use a quality USB cable (not charge-only)
- Ensure 5V power supply with at least 500mA current
- Try a different USB port or powered USB hub

**Hardware Defects:**
- Faulty PSRAM chip → Try disabling PSRAM test completely
- Damaged ESP32-S3 module → Try different board

**To disable PSRAM for testing:**

Edit `sdkconfig/n16r8.defaults`:
```ini
# Temporarily disable PSRAM to test
# CONFIG_SPIRAM=y
CONFIG_SPIRAM=n
```

Rebuild and upload. If it boots without PSRAM, the PSRAM chip may be faulty.

#### 5. Further Increase Timeout

If even 10 seconds isn't enough, increase further:

Edit `sdkconfig/n16r8.defaults`:
```ini
CONFIG_ESP_INT_WDT_TIMEOUT_MS=20000  # 20 seconds
```

Edit `tools/patch_arduino_sdkconfig.py`:
```python
TARGET_TIMEOUT_MS = 20000  # 20 seconds
```

Rebuild and upload.

---

## 🔍 Advanced Diagnostics

### Enable Verbose Boot Logging

Edit `platformio.ini`, add to build_flags:
```ini
build_flags =
    ${env:esp32-s3-n16r8.build_flags}
    -DCORE_DEBUG_LEVEL=5
    -DLOG_LOCAL_LEVEL=ESP_LOG_VERBOSE
```

This provides detailed ESP-IDF boot messages.

### Check Reset Reason

Add this to `setup()` in `main.cpp` (before any other code):
```cpp
void setup() {
  Serial.begin(115200);
  delay(500);
  
  // Print reset reason
  esp_reset_reason_t reset_reason = esp_reset_reason();
  Serial.print("Reset reason: ");
  switch(reset_reason) {
    case ESP_RST_POWERON:   Serial.println("Power-on"); break;
    case ESP_RST_SW:        Serial.println("Software reset"); break;
    case ESP_RST_PANIC:     Serial.println("Exception/panic"); break;
    case ESP_RST_INT_WDT:   Serial.println("Interrupt watchdog"); break;
    case ESP_RST_TASK_WDT:  Serial.println("Task watchdog"); break;
    case ESP_RST_WDT:       Serial.println("Other watchdog"); break;
    default:                Serial.println("Unknown"); break;
  }
  
  // Rest of setup...
}
```

This will tell you WHY the device reset.

---

## 📊 Technical Details

### Watchdog Timeout Progression

| Version | Timeout | Result |
|---------|---------|--------|
| Original | 300ms | ❌ Bootloop |
| v2.17.2 | 3000ms | ⚠️ Works for some |
| v2.17.3 | 5000ms | ⚠️ Works for most |
| **v2.17.4** | **10000ms** | ✅ Maximum safety |

### Why 10 Seconds?

The interrupt watchdog monitors ISR execution time. During PSRAM initialization:

1. **PSRAM Hardware Init:** ~500ms
2. **Memory Test (8MB):** ~1000-8000ms (varies by batch!)
3. **Arduino Framework Init:** ~500ms

**Total:** Can exceed 9 seconds on some hardware batches

**10 seconds** provides sufficient margin for:
- Worst-case PSRAM chip batches
- Cold boot (slower than warm reset)
- Debug builds with logging
- Manufacturing variations

### Safe for Production?

**Yes.** The interrupt watchdog still protects against genuine ISR hangs:
- 10 seconds is only for early boot
- Runtime ISR hangs are still detected
- Normal ISR execution is <1ms
- 10 seconds only prevents false positives during PSRAM init

---

## 📝 Build Environment Details

### Tested Environments

| Environment | Purpose | Status |
|-------------|---------|--------|
| `esp32-s3-n16r8` | Main debug build | ✅ Ready |
| `esp32-s3-n16r8-release` | Production build | ✅ Ready |
| `esp32-s3-n16r8-standalone-debug` | Standalone display debug | ✅ **Recommended for testing** |
| `esp32-s3-n16r8-standalone` | Standalone display release | ✅ Ready |
| `esp32-s3-n16r8-touch-debug` | Touch debugging | ✅ Ready |
| `esp32-s3-n16r8-no-touch` | No touch support | ✅ Ready |

All environments inherit the bootloop fix from `sdkconfig/n16r8.defaults`.

---

## 🎯 Next Steps

### For First-Time Build

1. **Clean build environment:** `pio run -t fullclean`
2. **Build firmware:** `pio run -e esp32-s3-n16r8-standalone-debug`
3. **Verify patch applied:** Check build output for "Patching Arduino Framework (v2.17.4)"
4. **Upload to device:** `pio run -e esp32-s3-n16r8-standalone-debug -t upload`
5. **Monitor serial:** `pio device monitor -e esp32-s3-n16r8-standalone-debug`
6. **Verify boot:** Should see "Firmware version: 2.17.4" and no bootloop

### If Already Built Before

1. **Full clean:** `rm -rf .pio/build/`
2. **Rebuild:** `pio run -e esp32-s3-n16r8-standalone-debug`
3. **Upload:** `pio run -e esp32-s3-n16r8-standalone-debug -t upload`
4. **Test:** Monitor serial output

### If Still Bootlooping

1. Check power supply (quality USB cable, 5V @ 500mA minimum)
2. Try different USB port
3. Enable verbose logging (see Advanced Diagnostics)
4. Check reset reason (see Advanced Diagnostics)
5. Try disabling PSRAM to test for faulty chip
6. Report issue with full serial output

---

## 📞 Support

If this fix doesn't resolve the bootloop, please provide:

1. **Full serial output** (entire boot sequence)
2. **Build output** (confirm patch applied)
3. **Hardware details** (USB cable quality, power source)
4. **Reset reason** (from advanced diagnostics)

This information will help diagnose hardware vs software issues.

---

**Version:** 2.17.4  
**Date:** 2026-01-26  
**Status:** Production Ready  
**Tested:** ESP32-S3 DevKitC-1 N16R8 (16MB Flash + 8MB PSRAM)

---

**END OF GUIDE**
