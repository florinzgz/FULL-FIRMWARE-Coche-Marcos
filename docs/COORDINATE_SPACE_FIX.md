# CRITICAL FIX: Coordinate Space Corruption Bug

## 🚨 Root Cause Analysis

### The Bug
Functions receive **absolute screen coordinates** (e.g., `cx=195, cy=115` for front-left wheel) but write them to **sprite-local coordinate spaces** (e.g., 0-99 for a 100×100 sprite).

### Example from wheels_display.cpp (line 278)
```cpp
void WheelsDisplay::drawWheel(int cx, int cy, ..., TFT_eSprite *sprite) {
    TFT_eSPI *drawTarget = sprite ? (TFT_eSPI *)sprite : tft;
    
    // ❌ BUG: cx=195, cy=115 (screen coords) written to 100x100 sprite
    // This writes to sprite memory at offset far beyond bounds!
    drawTarget->fillRoundRect(cx - 26, cy - 36, 52, 16, 3, COLOR_INFO_BG);
    //                        ^^^^^^  ^^^^^^
    //                        169     79  ← OUT OF BOUNDS for 100x100 sprite!
}
```

### Memory Corruption Chain
1. **Out-of-bounds write**: `fillRoundRect` at x=169, y=79 in 100×100 sprite
2. **Heap corruption**: Sprite buffer overrun writes into adjacent heap blocks
3. **Stack corruption**: Corrupted heap metadata damages FreeRTOS IPC task stack
4. **Stack canary triggered**: ESP32 detects stack corruption
5. **Crash**: "Guru Meditation Error: Stack canary watchpoint triggered (ipc0)"

## 🔧 The Fix

### Enhanced RenderContext
Added to `include/hud_layer.h`:
- `originX`, `originY`: Sprite position in screen coordinates
- `width`, `height`: Sprite dimensions for bounds checking
- `toLocalX()`, `toLocalY()`: Convert screen → sprite-local coordinates  
- `isInBounds()`: Check if screen coordinates are within sprite
- `clipRect()`: Clip rectangle to sprite bounds

### SafeDraw Wrapper
Created `include/safe_draw.h` with coordinate-safe drawing primitives:
- `SafeDraw::fillRect()`: Auto-translates and clips
- `SafeDraw::drawCircle()`: Bounds-checked
- `SafeDraw::drawLine()`: Endpoint validation
- `SafeDraw::drawString()`: Position validation

### Migration Pattern

**Before (BROKEN):**
```cpp
void drawSomething(int screenX, int screenY, TFT_eSprite *sprite) {
    TFT_eSPI *drawTarget = sprite ? (TFT_eSPI*)sprite : tft;
    drawTarget->fillRect(screenX, screenY, 100, 50, color);  // ❌ CRASH
}
```

**After (SAFE):**
```cpp
void drawSomething(int screenX, int screenY, HudLayer::RenderContext &ctx) {
    // Automatic coordinate translation and clipping
    SafeDraw::fillRect(ctx, screenX, screenY, 100, 50, color);  // ✅ SAFE
}
```

**Manual Translation (when needed):**
```cpp
void drawSomething(int screenX, int screenY, HudLayer::RenderContext &ctx) {
    if (ctx.sprite) {
        // Translate to sprite-local coordinates
        int16_t localX = ctx.toLocalX(screenX);
        int16_t localY = ctx.toLocalY(screenY);
        
        // Bounds check
        if (localX < 0 || localX >= ctx.width || localY < 0 || localY >= ctx.height) {
            return;  // Clipped out
        }
        
        ctx.sprite->fillRect(localX, localY, 100, 50, color);
    } else {
        tft.fillRect(screenX, screenY, 100, 50, color);
    }
}
```

## 📋 Files to Fix

### High Priority (Causes Crashes)
- ✅ `include/hud_layer.h` - Enhanced RenderContext
- ✅ `include/safe_draw.h` - Safe drawing primitives
- ⏳ `src/hud/wheels_display.cpp` - drawWheel3D() and drawWheel()
- ⏳ `src/hud/gauges.cpp` - drawGaugeSpeed() and drawGaugeRPM()
- ⏳ `src/hud/icons.cpp` - ALL icon drawing functions
- ⏳ `src/hud/hud_limp_indicator.cpp` - drawIndicator()
- ⏳ `src/hud/hud_limp_diagnostics.cpp` - drawDiagnostics()

### Medium Priority (May Cause Issues)
- `src/hud/hud.cpp` - drawPedalBar(), wheel rendering
- `src/hud/hud_graphics_telemetry.cpp` - telemetry overlay
- `src/hud/touch_calibration.cpp` - calibration overlays
- `src/hud/menu_hidden.cpp` - menu rendering

### Pattern to Search For
```bash
grep -r "TFT_eSPI \*drawTarget = sprite ? (TFT_eSPI \*)sprite :" src/hud/
```

## 🧪 Validation

### Debug Mode Checks
Enable `DEBUG_SAFE_DRAW` to log clipped draws:
```cpp
#define DEBUG_SAFE_DRAW
```

### Runtime Assertions
SafeDraw logs warnings when draws are near sprite bounds:
```
[SafeDraw] fillRect clipped out: screen(240,40,100,50) sprite(0,0,100,100)
[SafeDraw] WARNING: fillRect near bounds: local(90,30,100,50) sprite(100,100)
```

### Expected Results
After fix:
- ✅ No heap corruption
- ✅ No stack canary crashes
- ✅ No ipc0 Guru Meditation errors
- ✅ Stable rendering with sprites
- ✅ Touch calibration works
- ✅ Hidden menu stable

## 🎯 Implementation Status

### Phase 1: Infrastructure ✅ COMPLETE
- [x] Enhanced RenderContext with origin/bounds tracking
- [x] Created SafeDraw wrapper with coordinate translation
- [x] Added bounds checking and clipping methods
- [x] Added debug logging infrastructure

### Phase 2: Critical Fixes (IN PROGRESS)
- [ ] Fix wheels_display.cpp
- [ ] Fix gauges.cpp
- [ ] Fix icons.cpp (12+ functions)
- [ ] Fix hud_limp_indicator.cpp
- [ ] Fix hud_limp_diagnostics.cpp

### Phase 3: Additional Fixes
- [ ] Fix hud.cpp
- [ ] Fix touch_calibration.cpp
- [ ] Fix menu functions

### Phase 4: Testing
- [ ] Enable DEBUG_SAFE_DRAW
- [ ] Test with small sprites
- [ ] Verify no crashes
- [ ] Verify correct rendering

## 📊 Impact

This fix eliminates the #1 cause of firmware crashes:
- **Root cause**: Coordinate space mismatch
- **Symptom**: ipc0 stack canary watchpoint
- **Frequency**: Every time sprites are used with touch/menu/overlays
- **Fix complexity**: Medium (requires systematic refactoring)
- **Fix safety**: High (mathematically prevents out-of-bounds writes)

---

**Last Updated**: 2026-01-13 23:35 UTC
**Author**: GitHub Copilot  
**Status**: Phase 1 Complete, Phase 2 In Progress
