# Phase 2 Implementation Guide: Fixing Coordinate Space Corruption

## Status: IN PROGRESS

This document provides the systematic approach to fix all rendering functions that suffer from coordinate space corruption bugs.

## The Pattern to Fix

### UNSAFE Pattern (CAUSES CRASHES)
```cpp
void someDrawFunction(int cx, int cy, ..., TFT_eSprite *sprite) {
    TFT_eSPI *drawTarget = sprite ? (TFT_eSPI *)sprite : tft;  // ❌ UNSAFE!
    
    // These use SCREEN coordinates (e.g., 195, 115) but write to SPRITE
    drawTarget->fillRect(cx - 10, cy - 10, 20, 20, color);  // OUT OF BOUNDS!
    drawTarget->drawCircle(cx, cy, 15, color);              // OUT OF BOUNDS!
}
```

**Why this crashes:**
- `cx=195, cy=115` are screen coordinates
- If sprite is 100×100, valid coords are 0-99
- Writing at (195,115) goes FAR beyond sprite buffer
- Corrupts heap → corrupts FreeRTOS IPC stack → "Stack canary watchpoint triggered (ipc0)"

### SAFE Pattern (PREVENTS CRASHES)

#### Option 1: Use SafeDraw wrapper (RECOMMENDED for simple shapes)
```cpp
void someDrawFunction(int screenCX, int screenCY, ..., const HudLayer::RenderContext &ctx) {
    // SafeDraw automatically translates coordinates and clips to bounds
    SafeDraw::fillRect(ctx, screenCX - 10, screenCY - 10, 20, 20, color);  // ✅ SAFE
    SafeDraw::drawCircle(ctx, screenCX, screenCY, 15, color);               // ✅ SAFE
}
```

#### Option 2: Manual translation (for complex shapes like triangles)
```cpp
void someDrawFunction(int screenCX, int screenCY, ..., const HudLayer::RenderContext &ctx) {
    if (ctx.sprite) {
        // Translate to sprite-local coordinates
        int16_t localCX = ctx.toLocalX(screenCX);
        int16_t localCY = ctx.toLocalY(screenCY);
        
        // Bounds check
        if (localCX < -20 || localCX > ctx.width + 20 ||
            localCY < -20 || localCY > ctx.height + 20) {
            return;  // Clipped out
        }
        
        ctx.sprite->fillRect(localCX - 10, localCY - 10, 20, 20, color);
        ctx.sprite->drawCircle(localCX, localCY, 15, color);
    } else {
        // Drawing to screen - use screen coordinates directly
        tft->fillRect(screenCX - 10, screenCY - 10, 20, 20, color);
        tft->drawCircle(screenCX, screenCY, 15, color);
    }
}
```

#### Option 3: For very complex functions with many draw calls
Create a local helper to get the safe draw target:
```cpp
void complexDrawFunction(int screenCX, int screenCY, ..., const HudLayer::RenderContext &ctx) {
    // For functions that need setTextColor, setTextDatum, etc.
    TFT_eSPI *drawTarget = SafeDraw::getDrawTarget(ctx);
    drawTarget->setTextColor(color, bgcolor);
    drawTarget->setTextDatum(MC_DATUM);
    
    // But ALWAYS translate coordinates for actual drawing
    if (ctx.sprite) {
        int16_t localX = ctx.toLocalX(screenCX);
        int16_t localY = ctx.toLocalY(screenCY);
        
        if (ctx.isInBounds(screenCX, screenCY)) {
            ctx.sprite->drawString("Text", localX, localY, 2);
        }
    } else {
        drawTarget->drawString("Text", screenCX, screenCY, 2);
    }
}
```

## Files to Fix - Priority Order

### 🔴 CRITICAL (Causes frequent crashes)

#### 1. src/hud/wheels_display.cpp
- **Function**: `drawWheel3D()` (static helper)
  - Status: ⏳ PARTIAL - Started but incomplete
  - Lines: 60-250 (~60 drawTarget calls)
  - Strategy: Create RenderContext version, manually translate all coordinates
  
- **Function**: `drawWheel()` (public API)
  - Status: ⏳ NOT STARTED
  - Lines: 263-417
  - Issue: Calls unsafe `drawWheel3D()` and has many direct drawTarget calls
  - Strategy: Convert to use RenderContext, call safe drawWheel3D

#### 2. src/hud/gauges.cpp  
- **Functions**: `drawGaugeSpeed()`, `drawGaugeRPM()`
  - Status: ⏳ NOT STARTED
  - Pattern: Uses `TFT_eSPI *drawTarget = sprite ? (TFT_eSPI *)sprite : tft;`
  - Strategy: Replace with SafeDraw for circles/arcs, manual translation for complex geometry

#### 3. src/hud/icons.cpp
- **Functions**: ALL icon drawing functions (~12 functions)
  - `drawSystemState()`
  - `drawGear()`
  - `drawFeatures()`
  - `drawBattery()`
  - `drawErrorWarning()`
  - `drawSensorStatus()`
  - `drawTemperatureWarning()`
  - `drawAmbientTemp()`
  - More...
  - Status: ⏳ NOT STARTED
  - Strategy: Each function needs RenderContext parameter, use SafeDraw

### 🟡 HIGH PRIORITY (Can cause crashes in specific scenarios)

#### 4. src/hud/hud_limp_indicator.cpp
- **Function**: `drawIndicator()`
  - Status: ⏳ NOT STARTED
  - Pattern: Uses drawTarget
  - Strategy: SafeDraw for fillRect calls

#### 5. src/hud/hud_limp_diagnostics.cpp
- **Function**: `drawDiagnostics()`
  - Status: ⏳ NOT STARTED
  - Pattern: Uses drawTarget
  - Strategy: SafeDraw for fillRect calls

#### 6. src/hud/hud_graphics_telemetry.cpp
- **Function**: Telemetry overlay drawing
  - Status: ⏳ NOT STARTED
  - Pattern: Uses drawTarget
  - Strategy: SafeDraw

### 🟢 MEDIUM PRIORITY (Less frequent but still needed)

#### 7. src/hud/hud.cpp
- **Function**: `drawPedalBar()`, wheel rendering
  - Status: ⏳ NOT STARTED
  - Has some RenderContext support but may have unsafe patterns

#### 8. src/hud/touch_calibration.cpp
- **Function**: Calibration overlay drawing
  - Status: ⏳ NOT STARTED
  - Draws full-screen, but might be called with sprite

#### 9. src/hud/menu_hidden.cpp
- **Function**: Menu rendering
  - Status: ⏳ NOT STARTED
  - Uses direct tft calls (may need sprite support)

## Implementation Checklist

- [x] Phase 1: Infrastructure
  - [x] Enhanced RenderContext with origin/bounds
  - [x] Created SafeDraw wrapper
  - [x] Added coordinate translation methods
  - [x] Added bounds checking/clipping

- [ ] Phase 2: Critical Fixes
  - [ ] Fix wheels_display.cpp
    - [x] Added includes (safe_draw.h, hud_layer.h)
    - [ ] Complete drawWheel3D() conversion
    - [ ] Fix drawWheel() public API
  - [ ] Fix gauges.cpp
  - [ ] Fix icons.cpp (all 12+ functions)
  - [ ] Fix hud_limp_indicator.cpp
  - [ ] Fix hud_limp_diagnostics.cpp

- [ ] Phase 3: Additional Fixes
  - [ ] Fix hud_graphics_telemetry.cpp
  - [ ] Fix hud.cpp
  - [ ] Fix touch_calibration.cpp
  - [ ] Fix menu_hidden.cpp

- [ ] Phase 4: Testing & Validation
  - [ ] Enable DEBUG_SAFE_DRAW
  - [ ] Test with small sprites
  - [ ] Verify no out-of-bounds draws logged
  - [ ] Verify no crashes
  - [ ] Code review

## Progress Tracking

**Commit 1bb68c9**: Phase 1 complete - Infrastructure
- ✅ Enhanced RenderContext
- ✅ Created SafeDraw wrapper
- ✅ Documentation

**Next Commit**: wheels_display.cpp fixes
- ⏳ Started drawWheel3D() conversion
- Need to complete all drawTarget→SafeDraw conversions
- Need to fix drawWheel() public API

## Estimated Remaining Work

- **wheels_display.cpp**: ~2 hours (complex geometry, many draw calls)
- **gauges.cpp**: ~1 hour (circular geometry)
- **icons.cpp**: ~3 hours (12+ functions, each needs conversion)
- **overlays**: ~2 hours (limp mode, diagnostics, telemetry)
- **additional files**: ~2 hours (hud, calibration, menus)
- **Testing**: ~2 hours (enable debug, validate, fix issues)

**Total**: ~12 hours of focused work

## Key Learnings

1. **SafeDraw is sufficient for most cases**: fillRect, drawCircle, drawLine cover 80% of use cases
2. **Complex shapes need manual translation**: fillTriangle requires translating all vertices
3. **Text drawing**: Can use SafeDraw::drawString or manual translation
4. **Style functions**: setTextColor, setTextDatum can use getDrawTarget() but coords must still be translated

## References

- Infrastructure: `include/hud_layer.h`, `include/safe_draw.h`
- Examples: `docs/COORDINATE_SPACE_FIX.md`
- This guide: `docs/PHASE2_IMPLEMENTATION_GUIDE.md`
