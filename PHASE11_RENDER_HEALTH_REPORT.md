# PHASE 11 — GRAPHICS AUDIT & RENDER HEALTH VALIDATION REPORT

**Status:** PENDING HARDWARE VALIDATION  
**Date:** [To be filled in during testing]  
**Firmware Version:** v2.17.x  
**Build Target:** esp32-s3-n32r16v (32MB Flash + 16MB PSRAM)

---

## EXECUTIVE SUMMARY

**Objective:** Verify that the PSRAM compositor, dirty rectangle engine, shadow mode, and telemetry together form a correct, efficient, and stable rendering pipeline.

**Scope:** This phase does NOT add features. It only measures, validates, and proves correctness.

**Status:** ⏳ AWAITING HARDWARE TESTING

---

## SYSTEMS UNDER TEST

The following systems are being validated:

- ✅ **PSRAM Sprite Compositor** (Phase 5)
  - Layer-based rendering architecture
  - Multi-sprite management in PSRAM
  - Deterministic composition order

- ✅ **Dirty Rectangle Engine** (Phase 8)
  - Selective region updates
  - Rectangle merging and clipping
  - Minimal SPI bandwidth usage

- ✅ **Shadow Mode Validation** (Phase 7)
  - Block-by-block pixel comparison
  - Mismatch detection and logging
  - Validation sprite integrity

- ✅ **Telemetry HUD** (Phase 9)
  - Real-time performance metrics
  - FPS and frame time tracking
  - Bandwidth and PSRAM monitoring

- ✅ **BASE Layer Damage Tracking** (Phase 10)
  - Component-level granular dirty tracking
  - Change detection thresholds
  - Optimized update regions

---

## TEST CONFIGURATION

### Hardware Setup
- **Board:** ESP32-S3-WROOM-2-N32R16V
- **Flash:** 32MB QIO mode
- **PSRAM:** 16MB OPI mode
- **Display:** ST7796 480×320 TFT

### Firmware Settings
The following settings must be enabled for testing:

**Option 1: Via Configuration File (Requires Rebuild)**
```cpp
// File: include/config_storage.h
// Line ~99: TEMPORARILY change default from false to true (for testing only)
const Config DEFAULT_CONFIG = {
    // ... other settings ...
    true, // shadowHudEnabled (TEMPORARY - revert to false for production)
    // ... other settings ...
};
```

**⚠️ IMPORTANT:** Revert to `false` after testing completes for production builds.

**Option 2: Via Hidden Menu (Runtime, No Rebuild) — RECOMMENDED**
```
During testing, shadow mode can be toggled on/off via the Hidden Menu:
1. Long press button for 3 seconds to open Hidden Menu
2. Navigate to Diagnostics → Shadow Mode
3. Toggle shadow mode ON

This option is preferred as it doesn't require code changes and
automatically uses production-safe defaults after device reset.
```

To enable graphics telemetry overlay:
```cpp
// During testing, open Hidden Menu to display telemetry
// Telemetry appears automatically at position (10, 10)
```

### Build Commands
```bash
# Build firmware
pio run -e esp32-s3-n32r16v

# Upload to device
pio run -e esp32-s3-n32r16v -t upload

# Monitor serial output
pio device monitor -e esp32-s3-n32r16v
```

---

## TELEMETRY DATA SOURCES

### HUD Graphics Telemetry Overlay (Top Left)

**Location:** X=10, Y=10, Width=220, Height=120

**Access:** Open Hidden Menu during testing

**Metrics Displayed:**
- **FPS** - Frames per second (color-coded: GREEN>25, YELLOW 15-25, RED<15)
- **Frame time** - Milliseconds per frame
- **Dirty rects** - Number of dirty rectangles per frame
- **Dirty area** - Total pixels in dirty rectangles
- **Bandwidth** - KB/s pushed to display
- **PSRAM** - PSRAM usage by sprites (KB)
- **Shadow** - Shadow mode status (ON/OFF)
- **Shadow blocks** - Blocks compared in shadow mode
- **Shadow errors** - Mismatch count (RED if > 0)

### Hidden Menu → Shadow Diagnostics

**Access:** Navigate to Hidden Menu → Diagnostics → Shadow Stats

**Data Available:**
- Block mismatch count
- Last mismatch coordinates
- Frame comparison counters
- Validation statistics

---

## TEST SCENARIOS & RESULTS

### Test 1: IDLE HUD
**Scenario:** Car stopped, no input, no movement

**Expected Performance:**
- Dirty Rects: 0–1
- Bytes/frame: 0–5 KB
- FPS: ≥ 28
- Shadow mismatches: 0

**Test Procedure:**
1. Start vehicle in idle state
2. Open Hidden Menu to enable telemetry overlay
3. Observe telemetry for 30 seconds
4. Record min/max values

**Results:**
```
Status: [ ] PENDING / [ ] PASS / [ ] FAIL

FPS:
  Min: ___ fps
  Max: ___ fps
  Avg: ___ fps

Frame Time:
  Min: ___ ms
  Max: ___ ms
  Avg: ___ ms

Dirty Rectangles:
  Min: ___
  Max: ___
  Avg: ___

Bytes per Frame:
  Min: ___ KB
  Max: ___ KB
  Avg: ___ KB

Shadow Mismatches:
  Total: ___
  
Visual Quality:
  [ ] No flickering
  [ ] No ghosting
  [ ] No artifacts
  [ ] HUD stable

Notes:
_______________________________________________
_______________________________________________
```

**Pass Criteria:**
- ✅ Bytes/frame ≤ 5 KB
- ✅ Dirty rects ≤ 1
- ✅ FPS ≥ 28
- ✅ Shadow mismatches = 0

---

### Test 2: PEDAL ONLY
**Scenario:** Press accelerator pedal smoothly (no steering)

**Expected Performance:**
- Dirty Rects: 1–3
- Bytes/frame: 10–30 KB
- FPS: ≥ 28
- Shadow mismatches: 0
- Dirty area: Pedal bar only (~480×18 = 17 KB)

**Test Procedure:**
1. Start with vehicle idle
2. Smoothly press pedal from 0% to 100%
3. Release pedal smoothly to 0%
4. Repeat 3 times
5. Record telemetry data

**Results:**
```
Status: [ ] PENDING / [ ] PASS / [ ] FAIL

FPS:
  Min: ___ fps
  Max: ___ fps
  Avg: ___ fps

Frame Time:
  Min: ___ ms
  Max: ___ ms
  Avg: ___ ms

Dirty Rectangles:
  Min: ___
  Max: ___
  Avg: ___

Bytes per Frame:
  Min: ___ KB
  Max: ___ KB
  Avg: ___ KB

Shadow Mismatches:
  Total: ___

Visual Quality:
  [ ] Pedal bar updates smoothly
  [ ] No full-screen redraws
  [ ] Other HUD elements static
  [ ] No artifacts

Notes:
_______________________________________________
_______________________________________________
```

**Pass Criteria:**
- ✅ Bytes/frame: 10–30 KB
- ✅ Dirty rects: 1–3
- ✅ FPS ≥ 28
- ✅ Shadow mismatches = 0

---

### Test 3: STEERING ONLY
**Scenario:** Turn steering wheel left and right slowly (no acceleration)

**Expected Performance:**
- Dirty Rects: 1–4
- Bytes/frame: 15–40 KB
- FPS: ≥ 25
- Shadow mismatches: 0
- Dirty areas: Wheels + steering gauge only

**Test Procedure:**
1. Start with vehicle idle, wheels centered
2. Turn wheel slowly to full left
3. Turn wheel slowly to full right
4. Return to center
5. Repeat 3 times
6. Record telemetry data

**Results:**
```
Status: [ ] PENDING / [ ] PASS / [ ] FAIL

FPS:
  Min: ___ fps
  Max: ___ fps
  Avg: ___ fps

Frame Time:
  Min: ___ ms
  Max: ___ ms
  Avg: ___ ms

Dirty Rectangles:
  Min: ___
  Max: ___
  Avg: ___

Bytes per Frame:
  Min: ___ KB
  Max: ___ KB
  Avg: ___ KB

Shadow Mismatches:
  Total: ___

Visual Quality:
  [ ] Wheels update correctly
  [ ] Steering angle displayed
  [ ] No full-screen redraws
  [ ] Speed/RPM gauges static
  [ ] No artifacts

Notes:
_______________________________________________
_______________________________________________
```

**Pass Criteria:**
- ✅ Bytes/frame: 15–40 KB
- ✅ Dirty rects: 1–4
- ✅ FPS ≥ 25
- ✅ Shadow mismatches = 0

---

### Test 4: SPEED + PEDAL
**Scenario:** Drive forward, accelerate and decelerate

**Expected Performance:**
- Dirty Rects: 5–10
- Bytes/frame: 30–60 KB
- FPS: ≥ 25
- Shadow mismatches: 0
- Speed & RPM gauges tracked separately (not full screen)

**Test Procedure:**
1. Accelerate from 0 to 20 km/h
2. Maintain speed for 5 seconds
3. Decelerate to 0 km/h
4. Repeat 3 times
5. Record telemetry data

**Results:**
```
Status: [ ] PENDING / [ ] PASS / [ ] FAIL

FPS:
  Min: ___ fps
  Max: ___ fps
  Avg: ___ fps

Frame Time:
  Min: ___ ms
  Max: ___ ms
  Avg: ___ ms

Dirty Rectangles:
  Min: ___
  Max: ___
  Avg: ___

Bytes per Frame:
  Min: ___ KB
  Max: ___ KB
  Avg: ___ KB

Shadow Mismatches:
  Total: ___

Visual Quality:
  [ ] Speed gauge updates smoothly
  [ ] RPM gauge updates correctly
  [ ] Pedal bar reflects input
  [ ] No full-screen redraws
  [ ] Gauges update independently
  [ ] No artifacts

Notes:
_______________________________________________
_______________________________________________
```

**Pass Criteria:**
- ✅ Bytes/frame: 30–60 KB
- ✅ Dirty rects: 5–10
- ✅ FPS ≥ 25
- ✅ Shadow mismatches = 0
- ✅ Gauges tracked separately

---

### Test 5: MENU OVERLAY
**Scenario:** Open hidden menu

**Expected Performance:**
- Dirty Rects: Large but localized
- Bytes/frame: 40–80 KB
- FPS: ≥ 20
- Shadow mismatches: 0
- Base HUD remains intact under menu

**Test Procedure:**
1. Start with HUD in normal state
2. Open hidden menu
3. Navigate through menu items
4. Close hidden menu
5. Verify HUD restoration
6. Record telemetry data

**Results:**
```
Status: [ ] PENDING / [ ] PASS / [ ] FAIL

FPS:
  Min: ___ fps
  Max: ___ fps
  Avg: ___ fps

Frame Time:
  Min: ___ ms
  Max: ___ ms
  Avg: ___ ms

Dirty Rectangles:
  Min: ___
  Max: ___
  Avg: ___

Bytes per Frame:
  Min: ___ KB
  Max: ___ KB
  Avg: ___ KB

Shadow Mismatches:
  Total: ___

Visual Quality:
  [ ] Menu opens cleanly
  [ ] Menu items render correctly
  [ ] Menu closes cleanly
  [ ] HUD restored correctly
  [ ] No ghosting
  [ ] No artifacts

Notes:
_______________________________________________
_______________________________________________
```

**Pass Criteria:**
- ✅ Bytes/frame: 40–80 KB
- ✅ FPS ≥ 20
- ✅ Shadow mismatches = 0
- ✅ HUD intact under menu

---

### Test 6: STRESS TEST
**Scenario:** Drive, steer, regen, indicators, errors simultaneously

**Expected Performance:**
- Dirty Rects: 8–15
- Bytes/frame: ≤ 80 KB
- FPS: ≥ 20
- Shadow mismatches: 0
- No flicker, ghosting, or corruption

**Test Procedure:**
1. Accelerate to 15 km/h
2. Turn steering left and right
3. Enable regenerative braking
4. Activate turn indicators
5. Trigger warning conditions (optional)
6. Maintain activity for 30 seconds
7. Record telemetry data

**Results:**
```
Status: [ ] PENDING / [ ] PASS / [ ] FAIL

FPS:
  Min: ___ fps
  Max: ___ fps
  Avg: ___ fps

Frame Time:
  Min: ___ ms
  Max: ___ ms
  Avg: ___ ms

Dirty Rectangles:
  Min: ___
  Max: ___
  Avg: ___

Bytes per Frame:
  Min: ___ KB
  Max: ___ KB
  Avg: ___ KB

Shadow Mismatches:
  Total: ___

Visual Quality:
  [ ] All HUD elements update
  [ ] No flickering
  [ ] No ghosting
  [ ] No corruption
  [ ] System stable
  [ ] No visual glitches

Notes:
_______________________________________________
_______________________________________________
```

**Pass Criteria:**
- ✅ Bytes/frame ≤ 80 KB
- ✅ Dirty rects: 8–15
- ✅ FPS ≥ 20
- ✅ Shadow mismatches = 0
- ✅ No visual artifacts

---

## SHADOW MODE VALIDATION

### Shadow Integrity Checks

**Observations During Testing:**

1. **Mismatch Count:**
   ```
   Total shadow mismatches across all tests: ___
   
   Expected: 0
   Actual: ___
   Status: [ ] PASS / [ ] FAIL
   ```

2. **Mismatch Logging:**
   ```
   When mismatches occur:
   [ ] Block coordinates logged
   [ ] Mismatch count recorded
   [ ] Serial output available
   ```

3. **Corruption Duration:**
   ```
   If corruption detected:
   Persistence: ___ frames
   
   Expected: ≤ 1 frame
   Actual: ___ frames
   Status: [ ] PASS / [ ] FAIL / [ ] N/A
   ```

**Shadow Mode Pass Criteria:**
- ✅ Shadow mismatch count = 0 across all tests
- ✅ If mismatch occurs, coordinates are logged
- ✅ Corruption does not persist > 1 frame

---

## OVERALL PASS/FAIL CRITERIA

### PASS Criteria ✅

The rendering system is **APPROVED** if ALL of the following are true:

- [ ] **Average bytes/frame < 60 KB** during normal driving (Tests 2-4)
- [ ] **FPS ≥ 25** with shadow mode enabled (all tests)
- [ ] **Shadow mismatches = 0** across all test scenarios
- [ ] **No full-screen redraws** during normal driving operations
- [ ] **No visual artifacts:**
  - [ ] No flickering
  - [ ] No tearing
  - [ ] No ghosting
  - [ ] No corruption

### FAIL Conditions ❌

The system **FAILS** if ANY of the following occur:

- [ ] Bytes/frame > 120 KB during normal driving (Tests 2-4)
- [ ] FPS < 20 in any test scenario
- [ ] Dirty rects constantly = 1 full screen (480×320)
- [ ] Shadow mismatches persistent (> 0 across multiple frames)
- [ ] HUD visually corrupts or becomes unstable

---

## PERFORMANCE SUMMARY

### Aggregate Metrics

```
Overall FPS (Shadow ON):
  Minimum: ___ fps
  Maximum: ___ fps
  Average: ___ fps

Overall Bytes/Frame:
  Minimum: ___ KB
  Maximum: ___ KB
  Average: ___ KB

Overall Dirty Rect Count:
  Minimum: ___
  Maximum: ___
  Average: ___

Total Shadow Mismatches: ___
```

### Performance Comparison

| Metric | Expected | Actual | Status |
|--------|----------|--------|--------|
| Avg FPS (Shadow ON) | ≥ 25 | ___ | [ ] PASS / [ ] FAIL |
| Avg Bytes/Frame | < 60 KB | ___ KB | [ ] PASS / [ ] FAIL |
| Avg Dirty Rects | 3-10 | ___ | [ ] PASS / [ ] FAIL |
| Shadow Mismatches | 0 | ___ | [ ] PASS / [ ] FAIL |
| Visual Quality | Perfect | ___ | [ ] PASS / [ ] FAIL |

---

## VISUAL DOCUMENTATION

### Screenshots/Photos

**Recommended captures:**

1. **Idle HUD with Telemetry Overlay**
   - [ ] Captured
   - Location: `docs/phase11/idle_telemetry.jpg`

2. **Telemetry During Acceleration**
   - [ ] Captured
   - Location: `docs/phase11/acceleration_telemetry.jpg`

3. **Telemetry During Steering**
   - [ ] Captured
   - Location: `docs/phase11/steering_telemetry.jpg`

4. **Menu Overlay with Telemetry**
   - [ ] Captured
   - Location: `docs/phase11/menu_telemetry.jpg`

5. **Stress Test Telemetry**
   - [ ] Captured
   - Location: `docs/phase11/stress_test_telemetry.jpg`

6. **Shadow Diagnostics Screen**
   - [ ] Captured
   - Location: `docs/phase11/shadow_diagnostics.jpg`

---

## ISSUES IDENTIFIED

### Critical Issues
```
None identified / List any critical issues below:

1. _______________________________________________
2. _______________________________________________
```

### Warnings
```
None identified / List any warnings below:

1. _______________________________________________
2. _______________________________________________
```

### Minor Issues
```
None identified / List any minor issues below:

1. _______________________________________________
2. _______________________________________________
```

---

## RECOMMENDATIONS

### Immediate Actions Required
```
None / List recommendations:

1. _______________________________________________
2. _______________________________________________
```

### Performance Optimizations
```
None / List optimizations:

1. _______________________________________________
2. _______________________________________________
```

### Future Enhancements
```
Optional improvements for consideration:

1. _______________________________________________
2. _______________________________________________
```

---

## FINAL VERDICT

**Graphics Rendering Pipeline Status:**

```
[ ] ✅ APPROVED - Automotive-grade stable and performance-correct
[ ] ⚠️ CONDITIONAL PASS - Minor issues require attention
[ ] ❌ FAILED - Critical issues prevent approval
[ ] ⏳ PENDING - Testing not yet completed
```

**Certification Statement:**
```
The graphics engine has been validated against all Phase 11 criteria.
The system demonstrates:

[ ] Correct rendering behavior
[ ] Efficient resource utilization
[ ] Stable performance under load
[ ] Zero shadow validation errors
[ ] Production-ready quality

Tested by: _______________
Date: _______________
Firmware version: _______________
```

**Approver Signature:**
```
Name: _______________
Role: _______________
Date: _______________
```

---

## APPENDIX A: TEST ENVIRONMENT DETAILS

### Hardware Configuration
```
Board: ESP32-S3-WROOM-2-N32R16V
Flash: 32MB (QIO mode)
PSRAM: 16MB (OPI mode)
Display: ST7796 480×320
SPI Frequency: 40 MHz
Touch Frequency: 2.5 MHz
```

### Software Configuration
```
Firmware Version: _______________
PlatformIO Environment: esp32-s3-n32r16v
Build Flags:
  - BOARD_HAS_PSRAM
  - CORE_DEBUG_LEVEL=3
  - SPI_FREQUENCY=40000000
  
Shadow Mode: ENABLED
Graphics Telemetry: ENABLED
```

### Test Conditions
```
Temperature: ___ °C
Battery Voltage: ___ V
Test Duration: ___ minutes
Test Date: _______________
Tester: _______________
```

---

## APPENDIX B: TELEMETRY DATA LOG

### Raw Telemetry Samples

**Format:** timestamp, fps, frame_ms, dirty_rects, bytes_kb, shadow_errors

```
Example (replace with actual data):
00:00:00, 28, 35, 1, 2.1, 0
00:00:01, 27, 37, 3, 18.5, 0
00:00:02, 26, 38, 5, 42.3, 0
...
```

### Data Collection Script

For automated data collection, use the following approach:

1. **Serial Monitor Logging:**
   ```bash
   pio device monitor -e esp32-s3-n32r16v > telemetry_log.txt
   ```

2. **Manual Recording:**
   - Open Hidden Menu to show telemetry
   - Record values every 5 seconds
   - Note any anomalies

3. **Post-Processing:**
   - Calculate averages
   - Identify min/max values
   - Generate performance graphs (optional)

---

## APPENDIX C: TROUBLESHOOTING

### Common Issues

**Issue: Telemetry overlay not visible**
- Solution: Open Hidden Menu to activate telemetry
- Verify: Check `HudGraphicsTelemetry::isVisible()` returns true

**Issue: Shadow mode not active**
- Solution: Enable in config: `Config.shadowHudEnabled = true`
- Verify: Telemetry shows "Shadow: ON"

**Issue: FPS lower than expected**
- Check: Shadow mode overhead (~10ms/frame)
- Check: SPI frequency (should be 40 MHz)
- Check: Display connection quality

**Issue: High bytes/frame in idle**
- Check: Dirty rect engine active
- Check: BASE layer using granular tracking
- Check: No full-screen redraws

**Issue: Shadow mismatches detected**
- Action: Record coordinates from diagnostics menu
- Action: Check serial output for mismatch logs
- Action: Investigate specific HUD components

---

## DOCUMENT REVISION HISTORY

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2026-01-11 | System | Initial template creation |

---

**End of Report**

---

## USAGE INSTRUCTIONS

This report template should be used as follows:

1. **Pre-Test:** Review all test scenarios and prepare hardware
2. **During Test:** Fill in results for each test scenario as you execute them
3. **Post-Test:** Complete performance summary and final verdict
4. **Documentation:** Add screenshots to `docs/phase11/` directory
5. **Review:** Have a second person verify results
6. **Archive:** Save completed report for project records

The report certifies that the graphics engine is **automotive-grade stable** and **performance-correct** for production deployment.
