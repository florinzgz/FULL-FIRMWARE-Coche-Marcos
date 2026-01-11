# PHASE 8 — DIRTY RECTANGLE ENGINE TESTING GUIDE

## Quick Start

This guide helps you validate the Phase 8 dirty rectangle implementation on real hardware.

## Pre-Testing Checklist

### Build Verification
- [x] All 6 environments compile successfully
- [x] No compilation errors or warnings (except pre-existing ones)
- [x] Binary size unchanged (~4.3% Flash, 0.1% RAM)

### Code Review
- [x] DirtyRect structure implemented
- [x] RenderContext extended with markDirty()
- [x] Compositor dirty rect tracking added
- [x] compositeLayers() optimized for partial updates
- [x] Shadow mode optimized for dirty regions only
- [x] Layer integrations completed (indicator, diagnostics)

## Testing Phases

### Phase 1: Basic Functionality (30 minutes)

#### 1.1 Initial Boot Test
**Goal:** Verify HUD renders correctly on first boot

```
Steps:
1. Flash firmware to ESP32-S3
2. Power on and observe boot sequence
3. Verify HUD appears identical to Phase 7

Expected Result:
✅ HUD shows speed, RPM, battery, temperatures
✅ No visual artifacts or glitches
✅ No missing elements
✅ Colors and positions correct
```

#### 1.2 Static HUD Test
**Goal:** Verify idle state with minimal dirty regions

```
Steps:
1. Let vehicle sit idle (no movement)
2. Observe HUD for 1 minute
3. Monitor display stability

Expected Result:
✅ HUD remains stable
✅ No flickering or tearing
✅ No unnecessary redraws
✅ FPS should be high (minimal work)
```

#### 1.3 Limp Indicator Test
**Goal:** Verify small dirty region updates work

```
Steps:
1. Trigger limp mode (simulate sensor failure)
2. Watch for limp indicator to appear
3. Clear limp condition
4. Watch indicator disappear

Expected Result:
✅ Indicator appears cleanly in top-right
✅ No artifacts around indicator
✅ Indicator disappears cleanly
✅ Rest of HUD unaffected
```

#### 1.4 Diagnostics Panel Test
**Goal:** Verify medium dirty region updates

```
Steps:
1. Trigger degraded/limp/critical state
2. Observe diagnostics panel appearance
3. Change limp state
4. Observe panel updates

Expected Result:
✅ Panel appears at correct position
✅ Content renders correctly
✅ Updates reflect state changes
✅ No visual artifacts
```

### Phase 2: Performance Testing (30 minutes)

#### 2.1 FPS Baseline (Shadow OFF)
**Goal:** Measure FPS without shadow mode

```
Steps:
1. Ensure shadow mode is disabled
2. Drive vehicle at various speeds
3. Observe HUD responsiveness
4. Monitor via hidden menu stats

Target FPS:
✅ Idle: 30+ FPS
✅ Active driving: 25-30 FPS
✅ Rapid changes: 20-25 FPS

How to Check:
- Open hidden menu (button combination)
- Look for FPS counter
- Average over 30 seconds
```

#### 2.2 FPS with Shadow Mode
**Goal:** Verify shadow mode overhead is minimal

```
Steps:
1. Enable shadow mode via hidden menu
2. Drive vehicle at various speeds
3. Monitor FPS counter

Target FPS:
✅ Idle: 25+ FPS
✅ Active driving: 20-25 FPS
✅ Should not drop below 15 FPS

Compare to Phase 7:
- Phase 7 Shadow ON: 8-12 FPS
- Phase 8 Shadow ON: 20-25 FPS target
- Improvement: 2-3x faster
```

#### 2.3 Shadow Validation Test
**Goal:** Ensure shadow mode still detects corruption

```
Steps:
1. Enable shadow mode
2. Drive for 5 minutes
3. Check for shadow mismatches in logs

Expected Result:
✅ Shadow statistics update (frame count increases)
✅ No mismatches logged (M = 0)
✅ If mismatch occurs, gets logged correctly

Shadow Stats Display:
- T = Total comparisons
- M = Mismatch count
- Should be: T > 0, M = 0
```

### Phase 3: Stress Testing (30 minutes)

#### 3.1 Rapid State Changes
**Goal:** Test many dirty regions in quick succession

```
Scenario 1: Rapid Limp State Cycling
1. Rapidly trigger/clear limp conditions
2. Watch for visual corruption
3. Monitor FPS stability

Scenario 2: Menu Overlay Spam
1. Rapidly open/close hidden menu
2. Verify clean transitions
3. Check for artifacts

Expected Result:
✅ No ghosting or artifacts
✅ FPS remains stable (>15 FPS)
✅ All elements render correctly
```

#### 3.2 Full HUD Activity
**Goal:** Test maximum dirty region count

```
Steps:
1. Drive at high speed (high RPM)
2. Enable limp diagnostics
3. Open menu overlay
4. Change multiple states simultaneously

Expected Result:
✅ All elements visible and correct
✅ No overflow to full-screen (check logs)
✅ FPS acceptable (>15 FPS)
```

#### 3.3 Extended Runtime
**Goal:** Verify memory stability

```
Steps:
1. Let system run for 1 hour
2. Perform various HUD changes
3. Monitor for memory leaks

Expected Result:
✅ No memory leaks (PSRAM stable)
✅ FPS doesn't degrade over time
✅ No crashes or hangs
```

### Phase 4: Edge Cases (20 minutes)

#### 4.1 Overlapping Dirty Regions
**Goal:** Test rectangle merging

```
Scenario:
1. Trigger limp indicator (top-right)
2. Immediately trigger diagnostics (center)
3. Should merge into 2 dirty rects

Expected Result:
✅ Both render correctly
✅ No visual overlap issues
✅ Efficient merging (few rects)
```

#### 4.2 Boundary Clipping
**Goal:** Test screen edge handling

```
Test Cases:
1. Indicator at screen edge
2. Diagnostics panel near edge
3. Menu overlay at corners

Expected Result:
✅ All elements clipped correctly
✅ No out-of-bounds rendering
✅ No corruption at edges
```

#### 4.3 Maximum Dirty Rects
**Goal:** Test overflow protection

```
Simulated Scenario:
- Need to trigger 17+ dirty regions simultaneously
- Hard to test in practice
- Should fallback to full-screen

Expected Result:
✅ Graceful fallback (no crash)
✅ Full screen rendered if overflow
✅ System remains stable
```

## Performance Metrics

### Key Performance Indicators (KPIs)

| Metric | Phase 7 | Phase 8 Target | Pass Threshold |
|--------|---------|----------------|----------------|
| FPS (Shadow OFF, Idle) | 15-20 | 30+ | > 25 |
| FPS (Shadow OFF, Active) | 15-20 | 25-30 | > 20 |
| FPS (Shadow ON, Idle) | 8-12 | 25+ | > 20 |
| FPS (Shadow ON, Active) | 8-12 | 20-25 | > 15 |
| PSRAM Usage | Baseline | Similar | < Baseline + 5% |
| Shadow Overhead | ~50% FPS drop | ~20% FPS drop | < 30% drop |

### Bandwidth Reduction Estimate

**Small Change (Indicator only):**
- Old: 480×320×2 = 307,200 bytes
- New: 110×40×2 = 8,800 bytes
- Savings: 97%

**Typical Frame (3-5 regions):**
- Estimate: 15-20% of screen updated
- Savings: 80-85%

## Debug Output Monitoring

### Expected Log Messages

**Normal Operation:**
```
[HUD] Starting HUDManager initialization...
[HUD] Initializing TFT_eSPI...
[HudCompositor] Initialized successfully
[HudCompositor] Created sprite for layer 0
[HudCompositor] Created sprite for layer 1
...
```

**Shadow Mode Enabled:**
```
[HudCompositor] Shadow mode ENABLED
[HudCompositor] Shadow sprite created (PSRAM remaining: XXXXX bytes)
```

**Shadow Comparison (No Errors):**
```
(No output = good, only logs on mismatch)
```

**Shadow Mismatch (Unexpected - Should Investigate):**
```
[ERROR] HUD SHADOW MISMATCH: Frame 1234, 5 blocks differ, first at block(10,5) px(160,80)
```

### What to Look For

**Good Signs:**
- No ERROR messages related to compositor
- Shadow stats show M=0 (no mismatches)
- FPS meets targets
- PSRAM usage stable

**Warning Signs:**
- Shadow mismatches (M > 0) - possible rendering bug
- FPS below 15 (slower than Phase 7)
- PSRAM usage growing over time
- Visual artifacts or corruption

## Troubleshooting

### Problem: FPS Below Target

**Possible Causes:**
1. Too many dirty regions → check merging logic
2. Full-screen fallback happening → reduce dirty rect sources
3. Shadow mode too expensive → verify dirty-only comparison

**Debug Steps:**
```cpp
// Add temporary logging in compositor:
Logger::debugf("Dirty rects this frame: %d", dirtyRectCount);
for (int i = 0; i < dirtyRectCount; i++) {
  Logger::debugf("  Rect %d: (%d,%d) %dx%d", 
    i, dirtyRects[i].x, dirtyRects[i].y,
    dirtyRects[i].w, dirtyRects[i].h);
}
```

### Problem: Visual Artifacts

**Possible Causes:**
1. Dirty region not marked correctly
2. Rectangle clipping bug
3. Layer not clearing properly

**Debug Steps:**
1. Verify markDirty() calls in layers
2. Check clipRect() implementation
3. Ensure sprites clear dirty regions

### Problem: Shadow Mismatches

**Possible Causes:**
1. Non-deterministic rendering
2. Race condition in layer rendering
3. Dirty rect not covering all changes

**Debug Steps:**
1. Disable shadow mode to verify visual correctness
2. Check if mismatches are consistent or random
3. Verify both sprites render identically

### Problem: Memory Issues

**Possible Causes:**
1. Memory leak (unlikely - no dynamic allocation)
2. Stack overflow (deep recursion)
3. PSRAM corruption

**Debug Steps:**
1. Monitor PSRAM with `ESP.getFreePsram()`
2. Check for stack overflows
3. Verify no external corruption

## Success Criteria

### Minimum Viable (Must Pass)
- [ ] HUD visually identical to Phase 7
- [ ] No crashes or hangs
- [ ] FPS > 15 in all scenarios
- [ ] Shadow mode functional (M = 0)
- [ ] No memory leaks

### Target Performance (Should Pass)
- [ ] FPS > 25 with Shadow OFF
- [ ] FPS > 20 with Shadow ON
- [ ] No visual artifacts
- [ ] Smooth transitions

### Optimal Performance (Nice to Have)
- [ ] FPS > 30 idle with Shadow OFF
- [ ] FPS > 25 active with Shadow ON
- [ ] < 10 dirty rects typical frame
- [ ] Shadow overhead < 20%

## Test Report Template

```markdown
# Phase 8 Testing Report

**Date:** YYYY-MM-DD
**Tester:** [Name]
**Firmware:** [Git commit hash]
**Hardware:** ESP32-S3 N32R16V

## Test Results

### Phase 1: Basic Functionality
- [ ] Initial boot: PASS / FAIL
- [ ] Static HUD: PASS / FAIL  
- [ ] Limp indicator: PASS / FAIL
- [ ] Diagnostics panel: PASS / FAIL

### Phase 2: Performance
- FPS (Shadow OFF, Idle): ____ FPS (Target: 30+)
- FPS (Shadow OFF, Active): ____ FPS (Target: 25-30)
- FPS (Shadow ON, Idle): ____ FPS (Target: 25+)
- FPS (Shadow ON, Active): ____ FPS (Target: 20-25)

### Phase 3: Stress Testing
- [ ] Rapid state changes: PASS / FAIL
- [ ] Full HUD activity: PASS / FAIL
- [ ] Extended runtime: PASS / FAIL

### Phase 4: Edge Cases
- [ ] Overlapping regions: PASS / FAIL
- [ ] Boundary clipping: PASS / FAIL
- [ ] Overflow handling: PASS / FAIL

## Issues Found

1. [Issue description]
   - Severity: Critical / Major / Minor
   - Steps to reproduce:
   - Expected vs Actual:

## Performance Comparison

| Metric | Phase 7 | Phase 8 | Improvement |
|--------|---------|---------|-------------|
| FPS (Shadow OFF) | | | |
| FPS (Shadow ON) | | | |
| Shadow Overhead | | | |

## Conclusion

Overall Status: PASS / FAIL / NEEDS WORK

Recommendation:
- [ ] Ready for production
- [ ] Needs minor fixes
- [ ] Needs major revision

Notes:
[Additional observations]
```

## Next Steps After Testing

### If Tests Pass
1. Merge to main branch
2. Update documentation
3. Plan Phase 9 (animations/transitions)
4. Consider granular BASE layer tracking

### If Tests Fail
1. Document all failures in detail
2. Analyze root causes
3. Implement fixes
4. Re-test affected areas
5. Full regression test before merge

## Contact / Support

**Questions or Issues:**
- Review `PHASE8_DIRTY_RECT_IMPLEMENTATION.md` for technical details
- Check git commit history for implementation details
- Examine compositor logs for runtime behavior

**Performance Tuning:**
- Adjust `MAX_DIRTY_RECTS` if needed (currently 16)
- Tune merging heuristics in `mergeDirtyRects()`
- Profile specific bottlenecks with timing logs

---

**Happy Testing! 🚗💨**

The dirty rectangle engine represents a major performance milestone. Thorough testing will ensure it's production-ready for high-performance real-time HUD rendering.
