# Migration Guide: Integrating Sprite-Based Rendering

## Overview

This guide explains how to migrate the existing HUD system to use sprite-based rendering incrementally without breaking existing functionality.

## Current State

The HUD currently draws directly to the TFT display every frame:
- All elements redraw each update cycle
- No dirty tracking
- SPI bus gets saturated
- Frame rate inconsistent (15-25 FPS)
- Visible flicker during updates

## Target State

After migration:
- Only changed elements redraw
- Dirty tracking minimizes SPI traffic
- Stable 50-60 FPS
- No flicker
- Professional automotive-grade smoothness

## Migration Strategy: 5 Phases

### Phase 1: Infrastructure (COMPLETED ✓)

**Files Added:**
- `include/render_engine.h`
- `include/layer_map.h`
- `src/hud/render_engine.cpp`
- `docs/RENDER_ENGINE.md`
- `docs/RENDER_ENGINE_EXAMPLE.cpp`

**Changes Made:**
- `platformio.ini` - Added sprite and DMA flags

**Status:** Non-breaking - engine exists but isn't used yet

---

### Phase 2: Initialize Engine (NEXT STEP)

**Objective:** Initialize render engine without changing drawing behavior

**File:** `src/hud/hud.cpp`

**Changes:**

1. Add includes at top of file:
```cpp
#include "render_engine.h"
#include "layer_map.h"
```

2. Add to `HUD::init()` function after existing initialization:
```cpp
void HUD::init() {
  // ... existing TFT init code ...
  
  // Initialize sprite-based render engine
  RenderEngine::init(&tft);
  
  // Create layers (but don't use them yet)
  RenderEngine::createLayer(RenderLayer::CAR_BODY, 165, 90, 150, 170, 16);
  RenderEngine::createLayer(RenderLayer::PEDAL_BAR, 0, 300, 480, 20, 16);
  RenderEngine::createLayer(RenderLayer::STEERING, 215, 150, 50, 50, 16);
  // Add more layers as needed
  
  // ... rest of existing init code ...
}
```

3. Add to end of `HUD::update()`:
```cpp
void HUD::update() {
  // ... all existing update code ...
  
  // Render dirty sprites (currently none, so no effect)
  RenderEngine::render();
}
```

**Testing:**
- Compile and verify no errors
- Run and verify display works exactly as before
- Check logs for "RenderEngine: Initialized" message
- Verify PSRAM allocation messages

**Expected Result:** No visual changes, engine ready to use

---

### Phase 3: Migrate Car Body (Static Element)

**Objective:** Convert car body to sprite-based rendering

**File:** `src/hud/hud.cpp`

**Current Code Location:** `drawCarBody()` function (~line 617)

**Migration Steps:**

1. Modify `drawCarBody()` to use sprite:

```cpp
static void drawCarBody() {
  if (carBodyDrawn) return;
  
  // Get sprite
  TFT_eSprite *carSprite = RenderEngine::getSprite(RenderLayer::CAR_BODY);
  if (carSprite == nullptr) {
    // Fallback to original code if sprite not available
    // ... original drawing code ...
    return;
  }
  
  // Clear sprite
  carSprite->fillSprite(TFT_BLACK);
  
  // Convert all tft.xxx() calls to carSprite->xxx()
  // Adjust coordinates from screen-absolute to sprite-relative
  // Sprite origin is at (165, 90), so subtract these offsets
  
  int offsetX = -165;
  int offsetY = -90;
  
  // Example: Original code:
  // tft.fillTriangle(240, 175, x1, y1, x2, y2, COLOR);
  // 
  // Becomes:
  // carSprite->fillTriangle(240 + offsetX, 175 + offsetY, 
  //                         x1 + offsetX, y1 + offsetY,
  //                         x2 + offsetX, y2 + offsetY, COLOR);
  
  // ... convert all drawing calls ...
  
  RenderEngine::markDirty(RenderLayer::CAR_BODY);
  carBodyDrawn = true;
}
```

2. Alternative: Keep original code and add sprite code alongside:
```cpp
static void drawCarBody() {
  if (carBodyDrawn) return;
  
  TFT_eSprite *carSprite = RenderEngine::getSprite(RenderLayer::CAR_BODY);
  if (carSprite != nullptr) {
    // New sprite-based path
    drawCarBodySprite(carSprite);
    RenderEngine::markDirty(RenderLayer::CAR_BODY);
  } else {
    // Fallback to original direct drawing
    drawCarBodyDirect();
  }
  
  carBodyDrawn = true;
}
```

**Testing:**
- Compile and verify no errors
- Verify car body appears correctly
- Check position is accurate
- Verify it only draws once (static)

---

### Phase 4: Migrate Pedal Bar (Semi-Dynamic Element)

**Objective:** Convert pedal bar to sprite-based with change detection

**File:** `src/hud/hud.cpp`

**Current Code Location:** `drawPedalBar()` function (~line 826)

**Migration Steps:**

1. Modify `drawPedalBar()`:

```cpp
void HUD::drawPedalBar(float pedalPercent) {
  TFT_eSprite *pedalSprite = RenderEngine::getSprite(RenderLayer::PEDAL_BAR);
  if (pedalSprite == nullptr) {
    // Fallback to original code
    drawPedalBarDirect(pedalPercent);
    return;
  }
  
  // Only redraw if changed
  static float lastPercent = -999.0f;
  if (fabs(pedalPercent - lastPercent) < 0.5f) {
    return;  // No significant change
  }
  lastPercent = pedalPercent;
  
  // Clear sprite
  pedalSprite->fillSprite(COLOR_BAR_BG);
  
  // Convert all tft.xxx() to pedalSprite->xxx()
  // Coordinates are sprite-relative (0,0 = top-left of sprite at screen 0,300)
  
  if (pedalPercent < 0.0f) {
    // Invalid pedal
    pedalSprite->drawRoundRect(0, 0, 480, 18, 4, TFT_DARKGREY);
    pedalSprite->setTextDatum(MC_DATUM);
    pedalSprite->setTextColor(TFT_DARKGREY, COLOR_BAR_BG);
    pedalSprite->drawString("-- PEDAL --", 240, 9, 2);
  } else {
    // Draw bar and text
    // ... convert all drawing calls using sprite-relative coordinates ...
  }
  
  RenderEngine::markDirty(RenderLayer::PEDAL_BAR);
}
```

**Testing:**
- Verify pedal bar appears at bottom
- Confirm it updates when pedal changes
- Check performance (should only redraw when value changes)

---

### Phase 5: Migrate Dynamic Elements

**Elements to Migrate:**
1. Steering wheel
2. Wheel widgets (4 wheels)
3. Gauges (speed, RPM)
4. Icons

**For Each Element:**

1. Get or create appropriate sprite
2. Implement change detection (cache last values)
3. Convert drawing code to sprite-relative coordinates
4. Mark dirty only when values change
5. Test thoroughly

**Example Pattern:**

```cpp
void drawElement(int cx, int cy, float value) {
  TFT_eSprite *sprite = RenderEngine::getSprite(RenderLayer::XXX);
  if (sprite == nullptr) {
    // Fallback
    return;
  }
  
  // Change detection
  static float lastValue = -999.0f;
  if (fabs(value - lastValue) < THRESHOLD) {
    return;  // No redraw needed
  }
  lastValue = value;
  
  // Clear and redraw
  sprite->fillSprite(TFT_BLACK);
  // ... draw element ...
  
  RenderEngine::markDirty(RenderLayer::XXX);
}
```

---

## Coordinate Translation

### Screen Coordinates vs Sprite Coordinates

When drawing into a sprite, coordinates are relative to the sprite's top-left corner, not the screen.

**Example:**

Sprite created at screen position (100, 50) with size 80x60.

Screen drawing:
```cpp
tft.drawCircle(120, 70, 10, TFT_WHITE);  // Draws at screen (120, 70)
```

Sprite drawing:
```cpp
// Sprite origin is at (100, 50)
// To draw at same screen position (120, 70):
// Sprite-local = screen - sprite_origin
// x = 120 - 100 = 20
// y = 70 - 50 = 20
sprite->drawCircle(20, 20, 10, TFT_WHITE);
```

**Helper Function:**

```cpp
struct SpriteCoords {
  int16_t x, y;
};

SpriteCoords screenToSprite(RenderLayer::Layer layer, int16_t screenX, int16_t screenY) {
  LayerSprite *info = RenderEngine::getLayerInfo(layer);
  if (info == nullptr) return {screenX, screenY};
  return {screenX - info->x, screenY - info->y};
}
```

---

## Performance Optimization Tips

### 1. Minimize Dirty Marks
Only mark layers dirty when they actually change:
```cpp
// Bad: Always marks dirty
RenderEngine::markDirty(layer);

// Good: Only mark if changed
if (value != lastValue) {
  // ... redraw ...
  RenderEngine::markDirty(layer);
}
```

### 2. Right-Size Sprites
Don't make sprites larger than needed:
```cpp
// Bad: Entire screen for small element
RenderEngine::createLayer(layer, 0, 0, 480, 320, 16);

// Good: Just the area needed
RenderEngine::createLayer(layer, 200, 150, 80, 60, 16);
```

### 3. Appropriate Color Depth
Use lower depth for non-critical elements:
```cpp
// Background: 8-bit is enough
RenderEngine::createLayer(BACKGROUND, 0, 0, 480, 320, 8);

// Important visuals: 16-bit
RenderEngine::createLayer(CAR_BODY, x, y, w, h, 16);

// Critical graphics: 24-bit (use sparingly)
RenderEngine::createLayer(ICONS, x, y, w, h, 24);
```

### 4. Layer Visibility
Hide layers when not needed:
```cpp
// Hide menu layer when not active
RenderEngine::setLayerVisible(RenderLayer::OVERLAYS, menuActive);
```

---

## Testing Checklist

After each phase:

- [ ] Code compiles without errors
- [ ] Display shows correct visuals
- [ ] Touch input still works
- [ ] MenuHidden system functional
- [ ] No memory leaks (monitor free PSRAM)
- [ ] FPS is stable or improved
- [ ] No flicker or artifacts
- [ ] All display modes work (standalone, normal, etc.)

---

## Rollback Plan

If issues arise:

1. **Quick Rollback:** Comment out sprite code, use original direct drawing
2. **Partial Rollback:** Keep working layers, disable problematic ones
3. **Full Rollback:** Remove render engine initialization, everything falls back

Each migration step includes fallback code, so system continues working even if sprites fail.

---

## Performance Monitoring

Add to `HUD::update()`:

```cpp
#ifdef DEBUG_RENDER_PERFORMANCE
  static uint32_t lastPerfLog = 0;
  if (millis() - lastPerfLog > 5000) {
    float fps = RenderEngine::getFPS();
    uint32_t frameTime = RenderEngine::getFrameTime();
    Logger::infof("Render: FPS=%.1f, Frame=%dms", fps, frameTime);
    
    size_t freePsram = ESP.getFreePsram();
    Logger::infof("PSRAM: Free=%d bytes", freePsram);
    
    lastPerfLog = millis();
  }
#endif
```

---

## Expected Results

After full migration:

| Metric | Before | After | Improvement |
|--------|--------|-------|-------------|
| FPS | 15-25 | 50-60 | 2-3x faster |
| Flicker | Visible | None | Eliminated |
| SPI Bus Usage | 100% | 20-30% | 70%+ reduction |
| RAM Usage | High | Low | Sprites in PSRAM |
| CPU Usage | High | Low | Dirty tracking |

---

## Support

For questions or issues:
1. Check `docs/RENDER_ENGINE.md` for API reference
2. See `docs/RENDER_ENGINE_EXAMPLE.cpp` for code examples
3. Review this migration guide for step-by-step instructions
4. Test incrementally - don't migrate everything at once
