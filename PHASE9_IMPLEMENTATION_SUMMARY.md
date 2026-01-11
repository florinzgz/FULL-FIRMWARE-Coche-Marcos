# PHASE 9 — Runtime Graphics Telemetry & Performance HUD
**Implementation Summary**

## Overview
Phase 9 adds a real-time performance telemetry system that exposes how the HUD render pipeline, PSRAM sprites, dirty-rect engine, and shadow mode are behaving on real hardware.

## Implementation Details

### 1. Compositor Telemetry Core
**File:** `include/hud_compositor.h`, `src/hud/hud_compositor.cpp`

Added `RenderStats` structure to track:
- `frameCount` - Total frames rendered
- `lastFrameTimeMs` - Last frame render time
- `avgFrameTimeMs` - Smoothed average frame time (exponential moving average)
- `fps` - Frames per second (calculated from average)
- `dirtyRectCount` - Number of dirty rectangles per frame
- `dirtyPixels` - Total pixels in dirty rectangles
- `bytesPushed` - Bytes pushed to TFT (dirtyPixels * 2)
- `shadowEnabled` - Shadow mode status
- `shadowBlocksCompared` - Total shadow comparisons
- `shadowMismatches` - Frames with shadow errors
- `psramUsedBytes` - PSRAM used by all sprites

**Updates:** Once per frame in `HudCompositor::render()`

**Access:** Via `HudCompositor::getRenderStats()`

### 2. Graphics Telemetry Overlay
**Files:** `include/hud_graphics_telemetry.h`, `src/hud/hud_graphics_telemetry.cpp`

**Location:** X=10, Y=10, W=220, H=120

**Displays:**
```
FPS:            xx     (color-coded: GREEN>25, YELLOW 15-25, RED<15)
Frame time:     xx ms
Dirty rects:    n
Dirty area:     xxxx px
Bandwidth:      xxxx KB/s
PSRAM:          xxxx KB
Shadow:         ON / OFF
Shadow blocks:  n      (if shadow enabled)
Shadow errors:  n      (RED if > 0, RED border on panel)
```

### 3. Integration
**File:** `src/hud/hud_manager.cpp`

- Created `CombinedDiagnosticsRenderer` that renders both:
  - Limp diagnostics (260, 60) - shown when limp mode active
  - Graphics telemetry (10, 10) - shown when hidden menu active
- Both share DIAGNOSTICS layer (no spatial overlap)
- Telemetry visibility tied to hidden menu state
- Zero cost when hidden (renderer returns `isActive() = false`)

### 4. Performance Characteristics

**Zero Performance Penalty When Hidden:**
- ✅ No heap allocations (fixed structures only)
- ✅ No `String` class usage
- ✅ No `malloc` or `new`
- ✅ No floating point arithmetic (uses integer math)
- ✅ No rendering when `visible = false`
- ✅ Renderer returns `isActive() = false` when hidden
- ✅ One update per frame (efficient)

**When Visible:**
- Single dirty rect (220×120 = 26,400 pixels)
- ~53 KB/frame at 16-bit color
- Negligible CPU cost (text rendering only)

### 5. Usage

**Enable telemetry:**
```cpp
// Telemetry is shown automatically when hidden menu is active
HUDManager::activateHiddenMenu(true);
```

**Disable telemetry:**
```cpp
// Telemetry is hidden automatically when hidden menu is closed
HUDManager::activateHiddenMenu(false);
```

**Manual control:**
```cpp
HudGraphicsTelemetry::setVisible(true);  // Show
HudGraphicsTelemetry::setVisible(false); // Hide
```

## Testing Checklist

- [x] RenderStats structure defined with all required fields
- [x] Telemetry updated once per frame in render()
- [x] PSRAM usage tracked correctly
- [x] Overlay displays at correct position (10, 10, 220, 120)
- [x] FPS color-coded (GREEN/YELLOW/RED)
- [x] Shadow errors highlighted with red border
- [x] Zero performance penalty when hidden
- [x] No heap allocations
- [x] No String class usage
- [x] No floating point arithmetic
- [x] Integrated with compositor DIAGNOSTICS layer
- [x] Visibility tied to hidden menu activation

## Success Criteria ✅

After Phase 9, you can see in real-time:
- ✅ FPS (color-coded for quick visual feedback)
- ✅ Frame time (average, smoothed)
- ✅ Dirty-rect efficiency (count and pixel area)
- ✅ PSRAM usage (all sprites)
- ✅ Shadow-mode cost (blocks compared)
- ✅ Shadow integrity (error count with visual alert)

This makes the system:
- ✅ **Tunable** - See immediate impact of changes
- ✅ **Debuggable** - Identify performance bottlenecks
- ✅ **Safe** - Verify shadow mode integrity
- ✅ **Observable** - Real-time metrics without external tools

## Architecture Notes

The telemetry system is designed to be:
1. **Non-invasive** - Observes compositor without changing its behavior
2. **Efficient** - Integer-only math, minimal memory footprint
3. **Safe** - No dynamic allocation, no risk of memory fragmentation
4. **Integrated** - Follows existing HUD layer pattern
5. **Conditional** - Zero cost when not needed

The combined diagnostics renderer pattern allows multiple diagnostic overlays to coexist in the same layer without conflicts, as long as they don't overlap spatially.
