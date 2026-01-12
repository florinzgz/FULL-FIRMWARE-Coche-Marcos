# PHASE 12 — QUICK START GUIDE

## Overview

Phase 12 validates that the MarcosDashboard firmware can boot safely and reliably without hardware. This guide shows you how to run the validation.

---

## Prerequisites

- **Python 3.7+**
- **PlatformIO Core** (will be installed automatically)
- **Git** (to clone/check repository)

---

## Quick Validation (5 minutes)

### 1. Install PlatformIO (if needed)

```bash
pip install platformio
```

### 2. Run Phase 12 Validator

```bash
cd /path/to/FULL-FIRMWARE-Coche-Marcos
python3 phase12_boot_validator.py
```

### 3. Check Results

The validator will:
- ✅ Build all 3 firmware targets
- ✅ Analyze boot chain (87 init functions)
- ✅ Simulate runtime behavior
- ✅ Test failure modes
- ✅ Validate graphics safety
- ✅ Audit memory configuration
- ✅ Generate certification report

**Expected Output:**
```
======================================================================
                            FINAL VERDICT                             
======================================================================

✓ ✓ PHASE 12 VALIDATION PASSED
✓ ✓ Firmware is certified for hardware deployment
```

### 4. Review Certification Report

```bash
cat PHASE12_BOOT_CERTIFICATION_REPORT.md
```

---

## What Gets Validated

### ✅ Build Matrix
- `esp32-s3-n32r16v` (Debug)
- `esp32-s3-n32r16v-release` (Optimized)
- `esp32-s3-n32r16v-standalone` (Display only)

### ✅ Boot Chain
- System::init → Storage → Watchdog → Logger
- I2C Recovery → Sensors → Pedal → Steering
- LimpMode → HUD → Compositor → Managers

### ✅ Failure Modes
- No PSRAM detected
- I2C bus failure
- Missing sensors
- Invalid inputs
- Low battery

### ✅ Graphics Safety
- Sprites allocated before use
- DirtyRect engine initialized
- Shadow mode safe
- No crashes on startup

### ✅ Memory
- 32KB loop stack (✓)
- 16MB PSRAM configured (✓)
- No malloc in render paths (✓)

---

## Using Boot Test on Hardware

To add boot validation to your firmware:

### 1. Enable Boot Tracking

Edit `src/main.cpp`:

```cpp
#include "boot_sequence_test.h"

void setup() {
  // Start boot test
  BootSequenceTest::init();
  
  // Track System init
  BootSequenceTest::markStageStart(BootSequenceTest::BootStage::SYSTEM_INIT);
  System::init();
  BootSequenceTest::markStageComplete(BootSequenceTest::BootStage::SYSTEM_INIT, true);
  
  // Track Storage init
  BootSequenceTest::markStageStart(BootSequenceTest::BootStage::STORAGE_INIT);
  Storage::init();
  BootSequenceTest::markStageComplete(BootSequenceTest::BootStage::STORAGE_INIT, true);
  
  // ... track other stages ...
  
  // Final validation
  BootSequenceTest::markStageStart(BootSequenceTest::BootStage::BOOT_COMPLETE);
  BootSequenceTest::markStageComplete(BootSequenceTest::BootStage::BOOT_COMPLETE, true);
  
  // Run comprehensive check
  bool bootOk = BootSequenceTest::runComprehensiveCheck();
  if (!bootOk) {
    Logger::error("Boot validation failed!");
  }
}
```

### 2. Build and Flash

```bash
pio run -e esp32-s3-n32r16v -t upload
```

### 3. Monitor Serial Output

```bash
pio device monitor -b 115200
```

**Expected Output:**
```
[BootTest] Boot sequence validation initialized
[BootTest] Stage started: SYSTEM_INIT
[BootTest] Stage completed: SYSTEM_INIT (125ms)
[BootTest] Stage started: STORAGE_INIT
[BootTest] Stage completed: STORAGE_INIT (45ms)
...
[BootTest] ✓ All stages passed
[BootTest] ✓ Boot time acceptable
[BootTest] ✓ All critical stages executed
[BootTest] ✓ Stage ordering correct
[BootTest] ✓✓✓ BOOT SEQUENCE VALIDATION PASSED ✓✓✓
```

---

## Troubleshooting

### Validation Fails

If the validator reports failures:

1. **Check build output:**
   ```bash
   pio run -e esp32-s3-n32r16v -v
   ```

2. **Review error messages** in validator output

3. **Check certification report** for details:
   ```bash
   cat PHASE12_BOOT_CERTIFICATION_REPORT.md
   ```

### Build Timeout

If builds timeout (>10 minutes):

1. Clean build cache:
   ```bash
   pio run -t clean
   ```

2. Run again:
   ```bash
   python3 phase12_boot_validator.py
   ```

### Missing Dependencies

If PlatformIO complains about libraries:

```bash
pio lib install
```

---

## Continuous Integration

Add to `.github/workflows/phase12.yml`:

```yaml
name: Phase 12 Boot Validation

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main, develop ]

jobs:
  validate-boot:
    runs-on: ubuntu-latest
    
    steps:
    - uses: actions/checkout@v2
    
    - name: Set up Python
      uses: actions/setup-python@v2
      with:
        python-version: '3.9'
    
    - name: Install PlatformIO
      run: pip install platformio
    
    - name: Run Phase 12 Validation
      run: python3 phase12_boot_validator.py
    
    - name: Upload Certification Report
      uses: actions/upload-artifact@v2
      with:
        name: certification-report
        path: PHASE12_BOOT_CERTIFICATION_REPORT.md
```

---

## Success Criteria

✅ **PASS** if:
- All 3 builds succeed
- 0 circular dependencies
- 0 double-init risks
- 0 null-before-init issues
- All failure modes handled
- Graphics safe startup
- Boot time < 10s

❌ **FAIL** if:
- Any build fails
- Critical initialization issue found
- Failure mode not handled
- Graphics can crash on startup

---

## What's Next?

After Phase 12 passes:

1. ✅ **Flash to hardware** - Firmware is certified
2. 📊 **Monitor first boot** - Check serial output
3. 🔄 **Verify bootloop protection** - Boot counter clears
4. 🚗 **Test vehicle functions** - All systems operational

---

## Support

If you encounter issues:

1. Check `PHASE12_IMPLEMENTATION_SUMMARY.md` for details
2. Review `PHASE12_BOOT_CERTIFICATION_REPORT.md`
3. Open an issue on GitHub with validator output

---

## Quick Commands Reference

```bash
# Run validator
python3 phase12_boot_validator.py

# Build specific target
pio run -e esp32-s3-n32r16v

# Clean build
pio run -t clean

# Flash to hardware
pio run -e esp32-s3-n32r16v -t upload

# Monitor serial
pio device monitor -b 115200

# View certification
cat PHASE12_BOOT_CERTIFICATION_REPORT.md
```

---

**Phase 12 makes sure your car will boot. Every time. 🚗✨**
