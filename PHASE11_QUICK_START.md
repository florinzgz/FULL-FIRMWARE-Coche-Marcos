# PHASE 11 — QUICK START GUIDE

**Graphics Audit & Render Health Validation**

This guide provides a streamlined approach to executing Phase 11 validation tests.

---

## PRE-FLIGHT CHECKLIST

### Hardware Required
- [ ] ESP32-S3-WROOM-2-N32R16V board
- [ ] ST7796 480×320 TFT display
- [ ] USB cable for programming and monitoring
- [ ] Vehicle chassis with all sensors connected
- [ ] Camera/phone for screenshots (optional)

### Software Setup
- [ ] PlatformIO installed
- [ ] Repository cloned and updated
- [ ] Build environment: `esp32-s3-n32r16v`

---

## STEP 1: ENABLE SHADOW MODE

You have two options to enable shadow mode:

**Option A: Enable via Code (Requires Rebuild)**

Edit the configuration to enable shadow mode validation:

**File:** `include/config_storage.h`

Find the `DEFAULT_CONFIG` constant (around line 64-102) and change:

```cpp
const Config DEFAULT_CONFIG = {
    // ... (many other settings) ...
    true, // shadowHudEnabled (Line ~99: CHANGE from false to true)
    // ... other settings ...
};
```

Then rebuild and reflash firmware.

**Option B: Enable via Hidden Menu (Runtime, No Rebuild)**

If the firmware supports runtime configuration:

1. Power on the device
2. Long press the designated button for 3+ seconds
3. Hidden Menu will open
4. Navigate to: Diagnostics → Shadow Mode
5. Toggle shadow mode to ON
6. Telemetry will show "Shadow: ON"

**Note:** Option B is preferred as it doesn't require rebuilding firmware.

---

## STEP 2: BUILD AND FLASH

```bash
# Build firmware
pio run -e esp32-s3-n32r16v

# Upload to device
pio run -e esp32-s3-n32r16v -t upload

# Start serial monitor
pio device monitor -e esp32-s3-n32r16v
```

Watch for successful boot and HUD initialization.

---

## STEP 3: ACTIVATE TELEMETRY OVERLAY

**During Testing:**

1. **Trigger Hidden Menu:**
   - Press the designated button/gesture to open Hidden Menu
   - Or use touch interface if configured

2. **Verify Telemetry Display:**
   - Telemetry overlay appears at top-left (10, 10)
   - Shows FPS, frame time, dirty rects, bytes/frame
   - Border is white (or red if shadow errors)

3. **Keep Menu Open:**
   - Leave Hidden Menu active during all tests
   - Telemetry will update in real-time

---

## STEP 4: EXECUTE TEST SCENARIOS

Run each test scenario in sequence. Record results in `PHASE11_RENDER_HEALTH_REPORT.md`.

### Test 1: IDLE HUD (2 minutes)
```
Action: Car stopped, no input
Observe: Telemetry for 30-60 seconds
Record: FPS, bytes/frame, dirty rects, shadow errors
Expected: FPS ≥ 28, bytes ≤ 5 KB, dirty rects 0-1
```

### Test 2: PEDAL ONLY (3 minutes)
```
Action: Press pedal smoothly 0% → 100% → 0%
Repeat: 3 times
Record: Min/max/avg telemetry values
Expected: FPS ≥ 28, bytes 10-30 KB, dirty rects 1-3
```

### Test 3: STEERING ONLY (3 minutes)
```
Action: Turn wheel left → right → center
Repeat: 3 times
Record: Min/max/avg telemetry values
Expected: FPS ≥ 25, bytes 15-40 KB, dirty rects 1-4
```

### Test 4: SPEED + PEDAL (5 minutes)
```
Action: Accelerate 0 → 20 km/h → 0
Repeat: 3 times
Record: Min/max/avg telemetry values
Expected: FPS ≥ 25, bytes 30-60 KB, dirty rects 5-10
```

### Test 5: MENU OVERLAY (3 minutes)
```
Action: Open menu → navigate → close menu
Repeat: 2-3 times
Record: Min/max/avg telemetry values
Expected: FPS ≥ 20, bytes 40-80 KB
Verify: HUD intact after menu close
```

### Test 6: STRESS TEST (5 minutes)
```
Action: Drive + steer + indicators + all features
Duration: 30 seconds continuous
Record: Min/max/avg telemetry values
Expected: FPS ≥ 20, bytes ≤ 80 KB, dirty rects 8-15
```

---

## STEP 5: CHECK SHADOW DIAGNOSTICS

**Access Shadow Stats:**

1. Open Hidden Menu
2. Navigate to: Diagnostics → Shadow Stats
3. Record:
   - Total comparisons
   - Mismatch count
   - Last mismatch coordinates (if any)

**Expected Results:**
- Shadow mismatch count: 0
- No errors logged

---

## STEP 6: CAPTURE SCREENSHOTS

**Recommended Screenshots:**

1. **Idle with telemetry** - Shows baseline metrics
2. **Acceleration** - Shows dynamic update metrics
3. **Steering** - Shows wheel/gauge update metrics
4. **Stress test** - Shows maximum load metrics
5. **Shadow diagnostics** - Shows validation stats

Save screenshots to: `docs/phase11/`

---

## STEP 7: FILL IN REPORT

**Update:** `PHASE11_RENDER_HEALTH_REPORT.md`

For each test scenario:
1. Mark status as PASS/FAIL
2. Fill in FPS min/max/avg
3. Fill in bytes/frame min/max/avg
4. Fill in dirty rect count
5. Check visual quality checkboxes
6. Add notes if needed

---

## STEP 8: DETERMINE VERDICT

**Review Overall Pass Criteria:**

- [ ] Average bytes/frame < 60 KB
- [ ] FPS ≥ 25 (shadow ON)
- [ ] Shadow mismatches = 0
- [ ] No full-screen redraws
- [ ] No flicker/tearing/ghosting

**Mark Final Verdict:**
- ✅ APPROVED
- ⚠️ CONDITIONAL PASS
- ❌ FAILED
- ⏳ PENDING

---

## QUICK REFERENCE: TELEMETRY METRICS

### FPS (Frames Per Second)
- **GREEN:** > 25 fps (excellent)
- **YELLOW:** 15-25 fps (acceptable)
- **RED:** < 15 fps (poor)

### Frame Time
- **Good:** < 40 ms
- **Acceptable:** 40-65 ms
- **Poor:** > 65 ms

### Dirty Rectangles
- **Idle:** 0-1
- **Single component:** 1-3
- **Multiple components:** 3-10
- **Menu/overlay:** 10-15
- **Full screen:** 1 (480×320)

### Bytes per Frame
- **Idle:** 0-5 KB
- **Single component:** 10-30 KB
- **Normal driving:** 30-60 KB
- **Heavy activity:** 60-80 KB
- **Full screen:** 300 KB (BAD!)

### Shadow Mismatches
- **Expected:** 0
- **Warning:** > 0 (investigate)
- **Critical:** Persistent (> 5 frames)

---

## TROUBLESHOOTING

### Telemetry not visible
→ Open Hidden Menu  
→ Check `HudGraphicsTelemetry::isVisible()`

### Shadow mode not active
→ Check config: `shadowHudEnabled = true`  
→ Rebuild and reflash firmware

### FPS too low
→ Verify SPI frequency: 40 MHz  
→ Check shadow mode overhead  
→ Review dirty rect counts

### High bytes/frame in idle
→ Check for full-screen redraws  
→ Verify dirty rect engine active  
→ Review BASE layer tracking

### Shadow mismatches detected
→ Record coordinates from diagnostics  
→ Check serial output for logs  
→ Investigate specific components

---

## TESTING TIME ESTIMATE

| Test Scenario | Duration | Notes |
|---------------|----------|-------|
| Setup & build | 10 min | One-time |
| Test 1: Idle | 2 min | Baseline |
| Test 2: Pedal | 3 min | Single component |
| Test 3: Steering | 3 min | Multiple components |
| Test 4: Speed + Pedal | 5 min | Normal driving |
| Test 5: Menu | 3 min | Overlay validation |
| Test 6: Stress | 5 min | Maximum load |
| Shadow diagnostics | 2 min | Validation check |
| Screenshots | 5 min | Documentation |
| Report fill-in | 10 min | Results recording |
| **TOTAL** | **~45 min** | After initial setup |

---

## SUCCESS INDICATORS

✅ **You're on track if:**
- Telemetry displays correctly
- FPS stays green (> 25)
- Bytes/frame in expected ranges
- Shadow errors remain at 0
- HUD visually perfect

⚠️ **Warning signs:**
- FPS yellow/red frequently
- Bytes/frame > 100 KB
- Shadow errors appear
- Visual artifacts present

❌ **Critical failures:**
- FPS < 20 sustained
- Bytes/frame > 120 KB
- Shadow errors persistent
- HUD corrupts or crashes

---

## NEXT STEPS AFTER TESTING

### If PASS ✅
1. Complete report with all metrics
2. Add screenshots to documentation
3. Archive report for records
4. Mark Phase 11 as complete
5. System certified production-ready

### If FAIL ❌
1. Document all failures in report
2. Create detailed issue tickets
3. Investigate root causes
4. Fix identified issues
5. Re-run Phase 11 validation

### If CONDITIONAL PASS ⚠️
1. Document minor issues
2. Assess impact on production
3. Create improvement tickets
4. Determine deployment readiness
5. Plan follow-up validation

---

## HELPFUL COMMANDS

```bash
# Build only (no upload)
pio run -e esp32-s3-n32r16v

# Upload only (after build)
pio run -e esp32-s3-n32r16v -t upload

# Clean build
pio run -e esp32-s3-n32r16v -t clean

# Monitor serial output
pio device monitor -e esp32-s3-n32r16v

# Build + upload + monitor (all in one)
pio run -e esp32-s3-n32r16v -t upload && pio device monitor

# Save serial output to file
pio device monitor -e esp32-s3-n32r16v > telemetry_log.txt
```

---

## CONTACT / SUPPORT

If you encounter issues during testing:

1. Check the main report: `PHASE11_RENDER_HEALTH_REPORT.md`
2. Review previous phase docs: `PHASE7_*.md`, `PHASE8_*.md`, `PHASE9_*.md`, `PHASE10_*.md`
3. Check troubleshooting section above
4. Review serial output for error messages

---

**Good luck with testing!**

**Remember:** This phase validates months of graphics pipeline work.  
Take your time, be thorough, and document everything.

The goal is to certify the system as **automotive-grade stable**.

---

**Document Version:** 1.0  
**Created:** 2026-01-11  
**Last Updated:** 2026-01-11
