# PHASE 3: MIRROR RENDERING VALIDATION
**Date:** 2026-01-10  
**Status:** IN PROGRESS (Partial Implementation)  

---

## OBJECTIVE

Create a validation layer that mirrors TFT rendering to the STEERING_SHADOW sprite for pixel-perfect comparison before migration.

**Critical Rule:** NO changes to production rendering behavior. All mirroring code is inside `#ifdef RENDER_SHADOW_MODE` blocks.

---

## IMPLEMENTATION STATUS

### ✅ COMPLETED MODULES

#### 1. Shadow Rendering Helper Library
**File:** `include/shadow_render.h`  
**Size:** 6,421 bytes  
**Status:** COMPLETE  

**Functionality:**
- 18 mirroring macros for common TFT_eSPI primitives
- Zero overhead when RENDER_SHADOW_MODE undefined (macros become no-ops)
- Type-safe sprite access with null pointer protection

**Available Macros:**
```cpp
SHADOW_MIRROR_drawLine(x1, y1, x2, y2, color)
SHADOW_MIRROR_fillRect(x, y, w, h, color)
SHADOW_MIRROR_drawCircle(x, y, r, color)
SHADOW_MIRROR_fillCircle(x, y, r, color)
SHADOW_MIRROR_drawTriangle(x1, y1, x2, y2, x3, y3, color)
SHADOW_MIRROR_fillTriangle(x1, y1, x2, y2, x3, y3, color)
SHADOW_MIRROR_drawRoundRect(x, y, w, h, r, color)
SHADOW_MIRROR_fillRoundRect(x, y, w, h, r, color)
SHADOW_MIRROR_drawFastHLine(x, y, w, color)
SHADOW_MIRROR_drawFastVLine(x, y, h, color)
SHADOW_MIRROR_drawString(text, x, y, font)
SHADOW_MIRROR_setTextDatum(datum)
SHADOW_MIRROR_setTextColor(fg, bg)
SHADOW_MIRROR_drawArc(x, y, r_start, r_end, start_angle, end_angle, fg, bg, smooth)
... and more
```

#### 2. Gauges Module
**File:** `src/hud/gauges.cpp`  
**Status:** COMPLETE  
**Screen Area:** Left gauge (70, 175), Right gauge (410, 175)  
**Estimated Pixels:** ~39,200 pixels per gauge  

**Mirrored Functions:**
- `drawThickArc()` - Gauge arc segments (background)
- `drawScaleMarks()` - Scale lines and numeric labels
- `drawNeedle3D()` - Needle drawing and erasing
- `drawGaugeBackground()` - Gauge rings and unit labels
- `drawSpeed()` - Speed value text
- `drawRPM()` - RPM value text

**Drawing Operations Mirrored:**
- Arc drawing (thick arcs for gauge background)
- Line drawing (scale marks)
- Circle drawing (gauge rings, needle center)
- Triangle drawing (needle shape)
- Text drawing (scale numbers, value display, unit labels)

**Mismatch Risk:** LOW  
- All drawing is deterministic
- No antialiasing differences expected
- Text rendering should match pixel-perfect

#### 3. Pedal Bar
**File:** `src/hud/hud.cpp` (function: `drawPedalBar()`)  
**Status:** COMPLETE  
**Screen Area:** Bottom bar (0, 300, 480×18)  
**Estimated Pixels:** 8,640 pixels  

**Mirrored Operations:**
- Background fill (rounded rectangle)
- Progress bar fill with 3D effects
- Text with shadow (percentage display)
- Reference marks (25%, 50%, 75% lines)

**Mismatch Risk:** LOW  
- Simple geometric shapes
- Solid colors, no gradients
- Text should match exactly

#### 4. WheelsDisplay Module
**File:** `src/hud/wheels_display.cpp`  
**Status:** PARTIAL (text and bars complete, wheel geometry pending)  
**Screen Area:** 4 wheels at (195,115), (285,115), (195,235), (285,235)  
**Estimated Pixels:** ~10,000 pixels per wheel  

**Mirrored Functions:**
- Temperature display (text and background)
- Effort percentage display (text and background)
- Effort progress bar (fill and 3D effects)
- Wheel clear area (background erase)

**NOT YET MIRRORED:**
- `drawWheel3D()` - Complex rotated wheel geometry
  - Triangular wheel shape with rotation
  - Multiple overlapping fills
  - 3D shading effects
  
**Reason for Deferral:**
The wheel 3D drawing is complex with ~30 drawing calls per wheel including rotated triangles. Mirroring this requires careful handling of coordinate transformations. Marked as TODO for complete Phase 3.

**Mismatch Risk:** MEDIUM (for mirrored parts), UNKNOWN (for wheel geometry)

---

### ⏸️ DEFERRED MODULES

#### 5. Icons Module
**File:** `src/hud/icons.cpp`  
**Status:** HEADER INCLUDED, NOT YET MIRRORED  
**Screen Area:** Top and sides (various locations)  

**Functions Requiring Mirroring:**
1. `drawSystemState()` - System status indicator (top-left)
2. `drawGear()` - Gear selector panel (center-top)
3. `drawFeatures()` - 4x4 mode and REGEN icons
4. `drawBattery()` - Battery icon and voltage (top-right)
5. `drawAmbientTemp()` - Ambient temperature display
6. `drawErrorWarning()` - Error warning triangle
7. `drawSensorStatus()` - Sensor LED indicators (I/T/W)
8. `drawTempWarning()` - Critical temperature warning

**Estimated Total Pixels:** ~20,000 pixels  
**Complexity:** Medium (various icon shapes, some conditional)  

**Deferral Reason:** Time constraints, lower priority than gauges/wheels

---

## VALIDATION OUTPUT

### Expected Comparison Results

When `RenderEngine::compareShadowSprites()` is called:

**Perfect Match Modules (0 mismatches expected):**
- Gauges (speed and RPM)
- Pedal bar

**Acceptable Mismatch (<100 pixels):**
- WheelsDisplay (temperature and effort text)
  - Minor antialiasing differences possible

**High Mismatch (>1000 pixels):**
- WheelsDisplay (wheel 3D geometry not mirrored yet)
- Icons (not mirrored yet)

### Interpreting Mismatch Statistics

```cpp
#ifdef RENDER_SHADOW_MODE
uint32_t mismatches = RenderEngine::compareShadowSprites();

if (mismatches == 0) {
  // Perfect match - module is sprite-safe
  Logger::info("Perfect pixel match - ready for migration");
  
} else if (mismatches < 100) {
  // Acceptable - likely antialiasing or rounding differences
  Logger::infof("Minor mismatch: %u pixels (acceptable)", mismatches);
  
} else {
  // Significant difference - investigate
  Logger::warnf("Significant mismatch: %u pixels - needs investigation", mismatches);
}
#endif
```

---

## SCREEN AREA COVERAGE

### Mirrored Regions (as of current implementation)

```
┌─────────────────────────────────────────────────────────────┐
│  Icons (NOT MIRRORED)        Battery (NOT MIRRORED)        │
│  System State                Ambient Temp                   │
│                                                              │
│        Gear Selector (NOT MIRRORED)                         │
│                                                              │
│  ┌───────────┐                            ┌───────────┐     │
│  │  SPEED    │    Wheels (PARTIAL)        │    RPM    │     │
│  │  GAUGE    │    Temp/Effort: YES        │   GAUGE   │     │
│  │   ✅      │    Geometry: NO            │    ✅     │     │
│  └───────────┘                            └───────────┘     │
│                                                              │
│                  Car Body (SPRITE - not TFT)                │
│                  Steering Wheel (SPRITE - not TFT)          │
│                                                              │
│  ┌─────────────────────────────────────────────────────┐    │
│  │           Pedal Bar (100% mirrored) ✅             │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
```

**Coverage Statistics:**
- Fully mirrored: Gauges (2), Pedal bar (1)
- Partially mirrored: WheelsDisplay (text only, 4 wheels)
- Not mirrored: Icons (8 functions)

**Estimated Pixel Coverage:**
- Mirrored pixels: ~100,000 / frame (gauges + pedal + wheel text)
- Not mirrored: ~150,000 / frame (icons + wheel geometry)
- Total TFT pixels: ~250,000 / frame (estimated)
- **Coverage: ~40%**

---

## COMPILATION & TESTING

### Production Build (Default)
```bash
pio run -e esp32-s3-devkitc1
```

**Result:**
- All `#ifdef RENDER_SHADOW_MODE` blocks omitted
- Zero overhead (macros compile to `((void)0)`)
- Binary identical to pre-Phase 3 version

### Shadow Validation Build
```bash
pio run -e esp32-s3-devkitc1 -DRENDER_SHADOW_MODE
```

**Result:**
- Shadow sprite allocated (+300 KB PSRAM)
- Mirroring code active
- Calls to `compareShadowSprites()` functional
- Screen output IDENTICAL to production

---

## KNOWN LIMITATIONS

### 1. Incomplete Coverage
Only ~40% of TFT drawing is currently mirrored:
- Icons module not started
- WheelsDisplay wheel geometry deferred
- ObstacleDisplay HUD already uses sprites (no mirroring needed)

### 2. Performance Impact (Shadow Mode Only)
- Additional sprite drawing calls (~100% overhead)
- Pixel comparison: 153,600 pixel reads per frame
- Frame rate impact: estimated 5-10ms additional processing

### 3. Antialiasing Differences
TFT_eSPI may use different antialiasing for TFT vs sprite:
- Arc drawing
- Circle drawing
- Text rendering

Expected mismatch: <100 pixels for fully mirrored modules

---

## FUTURE WORK (Complete Phase 3)

### To Achieve 100% Coverage:

1. **Complete WheelsDisplay mirroring:**
   - Mirror `drawWheel3D()` function (~30 drawing calls per wheel)
   - Handle rotated coordinate transformations
   - Test with various wheel angles

2. **Add Icons mirroring:**
   - Mirror all 8 icon drawing functions
   - Handle conditional rendering (icons appear/disappear)
   - Test with various system states

3. **Add automatic comparison:**
   - Call `compareShadowSprites()` in `HUD::update()` (after RenderEngine::render())
   - Log statistics every 30 frames (avoid spam)
   - Expose counters via Logger

4. **Document mismatch patterns:**
   - Identify which mismatches are acceptable (antialiasing)
   - Which are bugs requiring investigation
   - Create threshold guidelines

---

## SAFETY VERIFICATION

### ✅ Production Safety Confirmed

**No Behavior Changes:**
- All TFT drawing calls unchanged
- All original code paths intact
- No logic modifications

**Compilation Tests:**
- Production build: compiles without warnings
- Shadow build: compiles without warnings
- Macro expansion verified (manual inspection)

**Memory Impact:**
- Production: 0 bytes overhead
- Shadow: +300 KB PSRAM (as designed)

---

## MISMATCH CAUSES (Expected)

### Acceptable Mismatches

1. **Antialiasing Differences:**
   - Sprite rendering may use different antialiasing than TFT
   - Typical mismatch: 1-2 pixels per edge
   - Acceptablefor arcs, circles, diagonal lines

2. **Text Rendering:**
   - Font rendering may differ slightly
   - Typical mismatch: <10 pixels per character
   - Acceptable if text is readable

3. **Rounding Errors:**
   - Coordinate calculations may round differently
   - Typical mismatch: 1 pixel offset
   - Acceptable for most graphics

### Unacceptable Mismatches

1. **Missing Drawing Calls:**
   - If a TFT call is not mirrored
   - Result: thousands of pixels different
   - Requires code fix

2. **Wrong Parameters:**
   - If mirroring uses wrong coordinates/colors
   - Result: visible artifacts
   - Requires code fix

3. **Logic Errors:**
   - If conditional drawing is not mirrored correctly
   - Result: elements appear/disappear incorrectly
   - Requires code fix

---

## CONCLUSION

Phase 3 mirror rendering is **PARTIALLY COMPLETE** with ~40% pixel coverage.

**Completed:**
- ✅ Shadow rendering helper library
- ✅ Gauges module (full mirroring)
- ✅ Pedal bar (full mirroring)
- ✅ WheelsDisplay text/bars (partial mirroring)

**Remaining:**
- ⏸️ WheelsDisplay wheel geometry
- ⏸️ Icons module (8 functions)
- ⏸️ Automatic comparison integration
- ⏸️ Mismatch threshold tuning

**Safety:**
- ✅ Zero production impact verified
- ✅ Compile-time conditional compilation working
- ✅ No behavior changes detected

**Next Steps:**
- Complete remaining modules for 100% coverage
- Integrate comparison calls in main loop
- Generate validation report (Phase 6)

---

**END OF PHASE 3 DOCUMENTATION (PARTIAL)**
