# ESP32-S3 Boot Fix - Historical OPI Flash eFuse Issue (N32R16V)

**Date:** 2026-01-08  
**Status:** 📜 **HISTORICAL** - This document describes a fix for previous N32R16V hardware  
**Current Hardware:** ESP32-S3 N16R8 (16MB QIO Flash + 8MB OPI PSRAM @ 3.3V)

---

## ⚠️ IMPORTANT NOTICE

**This document is HISTORICAL and describes an issue that affected the previous N32R16V hardware.**

The firmware has been fully migrated to **ESP32-S3 N16R8** which uses:
- 16MB QIO Flash (4-bit, 3.3V)
- 8MB OPI PSRAM (8-bit, 3.3V, AP_3v3 vendor)
- **No OPI mode for Flash** - standard QIO only
- **No eFuse issues** - uses standard configuration

See [HARDWARE.md](HARDWARE.md) for current hardware specification.

---

## HISTORICAL ERROR RESOLVED (N32R16V Only)

**Error Message:**
```
Octal Flash option selected, but EFUSE not configured!
```

**Root Cause (N32R16V):**
The board JSON was configured with `memory_type: "opi_opi"`, which instructs ESP-IDF to use OPI (Octal SPI) mode for BOTH Flash and PSRAM. However, the ESP32-S3-WROOM-2 N32R16V hardware had **OPI Flash eFuses NOT burned**, making OPI Flash mode impossible.

---

## HISTORICAL HARDWARE CONFIGURATION (N32R16V)

| Component | Capability | eFuse Status | Working Mode |
|-----------|-----------|--------------|--------------|
| **Flash** | 32MB (OPI-capable) | ❌ NOT burned | **QIO** (Quad I/O) |
| **PSRAM** | 16MB (OPI embedded) | ✅ Burned | **OPI** (Octal) |

**Critical Understanding:**
- The Flash chip HARDWARE supports OPI mode
- BUT the ESP32-S3 eFuses are NOT configured to enable OPI Flash
- eFuses are ONE-TIME programmable and cannot be changed
- Therefore, Flash MUST use QIO mode
- PSRAM eFuses ARE configured, so OPI PSRAM works

---

## THE FIX

### Changed File: `boards/esp32-s3-wroom-2-n32r16v.json`

**Before (WRONG):**
```json
{
  "build": {
    "arduino": {
      "memory_type": "opi_opi"  // ❌ CAUSES BOOT CRASH
    },
    "flash_mode": "qio",
    "psram_type": "opi"
  }
}
```

**After (CORRECT):**
```json
{
  "build": {
    "arduino": {
      "memory_type": "qio_opi"  // ✅ MATCHES HARDWARE REALITY
    },
    "flash_mode": "qio",
    "psram_type": "opi"
  }
}
```

### ESP-IDF SDK Variant Selection

PlatformIO selects the ESP-IDF SDK variant based on `memory_type`:
- `opi_opi` → `packages/.../esp32s3/opi_opi/include` (FAILS on this hardware)
- `qio_opi` → `packages/.../esp32s3/qio_opi/include` (CORRECT for this hardware)

---

## CLEAN & REBUILD INSTRUCTIONS

### Step 1: Clean Build Environment
```bash
# Remove ALL build artifacts
pio run -t clean

# Or use PlatformIO IDE: Project Tasks → Clean

# Verify .pio directory is cleaned
rm -rf .pio/build/
```

### Step 2: Rebuild Specific Environment
```bash
# Build the standalone debug environment
pio run -e esp32-s3-n32r16v-standalone-debug

# Or use PlatformIO IDE: 
# Project Tasks → esp32-s3-n32r16v-standalone-debug → Build
```

### Step 3: Verify SDK Path
After build, check the SDK variant being used:

**Correct path should contain:**
```
.platformio/packages/framework-arduinoespressif32/tools/sdk/esp32s3/qio_opi/include
```

**If you see this, it's WRONG:**
```
.platformio/packages/framework-arduinoespressif32/tools/sdk/esp32s3/opi_opi/include
```

### Step 4: Upload and Monitor
```bash
# Upload firmware
pio run -e esp32-s3-n32r16v-standalone-debug -t upload

# Monitor serial output
pio device monitor -b 115200

# Or combined:
pio run -e esp32-s3-n32r16v-standalone-debug -t upload -t monitor
```

---

## VALIDATION CRITERIA

### ✅ Success Indicators:
1. No error: "Octal Flash option selected, but EFUSE not configured!"
2. Serial output shows:
   ```
   rst:0xX (POWERON_RESET),boot:0x8 (SPI_FAST_FLASH_BOOT)
   [... normal ESP-IDF boot sequence ...]
   ESP32-S3 Car Control System v2.11.x
   [... setup() executes ...]
   ```
3. No reboot loop
4. Arduino `setup()` function executes successfully
5. Main application starts

### ❌ Failure Indicators:
- Boot crash with eFuse error
- Immediate reset after bootloader
- No Serial output
- Guru Meditation errors during boot
- Constant reset PC address

---

## TECHNICAL EXPLANATION

### Why `opi_opi` Failed:

1. **ESP-IDF Boot Process:**
   ```
   ROM Bootloader
       ↓
   2nd Stage Bootloader (checks eFuses)
       ↓
   Detects memory_type = "opi_opi"
       ↓
   Checks Flash eFuses for OPI configuration
       ↓
   eFuses NOT burned → ABORT with error
   ```

2. **eFuse Check:**
   ```c
   // ESP-IDF bootloader code (simplified)
   if (memory_type == OPI_OPI) {
       if (!efuse_flash_opi_enabled()) {
           esp_rom_printf("Octal Flash option selected, but EFUSE not configured!\n");
           abort();
       }
   }
   ```

### Why `qio_opi` Works:

1. **Correct Boot Process:**
   ```
   ROM Bootloader
       ↓
   2nd Stage Bootloader (checks eFuses)
       ↓
   Detects memory_type = "qio_opi"
       ↓
   Flash: Uses QIO (no eFuse check needed)
   PSRAM: Uses OPI (eFuse configured)
       ↓
   Successfully boots to application
   ```

2. **Memory Access:**
   - Flash accessed via QIO (4-bit data lines)
   - PSRAM accessed via OPI (8-bit data lines)
   - Both work correctly with this configuration

---

## WHAT NOT TO DO

### ❌ DO NOT:
- Burn eFuses to enable OPI Flash (irreversible and may brick the device)
- Add CONFIG_* macros to override ESP-IDF settings
- Force OPI Flash mode via sdkconfig
- Suppress the error without fixing the configuration
- Use `opi_qio` (wrong combination)
- Use `qio_qspi` (PSRAM would fail)

### ✅ DO:
- Use `qio_opi` memory_type
- Let board JSON control the configuration
- Keep platformio.ini clean (no CONFIG_* overrides)
- Verify SDK variant path after build

---

## ADDITIONAL NOTES

### platformio.ini Review:
- ✅ No CONFIG_* overrides present
- ✅ BOARD_HAS_PSRAM is application-level (safe)
- ✅ build_src_filter excludes test/ directory
- ✅ All environments inherit from base correctly

### Source Tree Review:
- ✅ Only ONE setup() in src/main.cpp
- ✅ Only ONE loop() in src/main.cpp
- ✅ test/ directory properly excluded
- ✅ test_display.cpp uses conditional compilation

### Board JSON Final State:
```json
{
  "build": {
    "arduino": {
      "memory_type": "qio_opi"
    },
    "flash_mode": "qio",
    "psram_type": "opi"
  }
}
```

---

## REFERENCE

**ESP-IDF Memory Types:**
- `qio_qspi` - QIO Flash + QSPI PSRAM (standard ESP32-S3)
- `qio_opi` - QIO Flash + OPI PSRAM (THIS HARDWARE)
- `opi_opi` - OPI Flash + OPI PSRAM (requires eFuses burned)
- `opi_qspi` - OPI Flash + QSPI PSRAM (unusual)

**PlatformIO SDK Selection Logic:**
```python
memory_type = board.get("build.arduino.memory_type", 
    f"{flash_mode}_{psram_type}")
sdk_path = f"sdk/esp32s3/{memory_type}/include"
```

---

## CONCLUSION

The boot crash was caused by a configuration mismatch between software settings and hardware eFuse programming. The fix is surgical: change one line in the board JSON from `opi_opi` to `qio_opi`. This correctly reflects the hardware reality where Flash eFuses are NOT burned for OPI mode, but PSRAM eFuses ARE configured for OPI.

**No application code changes required.**  
**No CONFIG_* overrides needed.**  
**No eFuse burning required.**  
**Pure configuration fix.**
