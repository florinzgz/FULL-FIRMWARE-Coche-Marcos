# COORDINATE-SPACE SAFETY QUICK REFERENCE

**Last Updated**: 2026-01-14  
**Status**: ✅ Certified Safe (Zero Violations)

---

## 🎯 GOLDEN RULES

1. **ALWAYS use SafeDraw** when rendering to sprites
2. **NEVER mix screen and sprite-local coordinates**
3. **Fullscreen sprites (480×320) are safe** for direct drawing
4. **Menu/calibration code** can use direct TFT (no sprites)

---

## ✅ SAFE PATTERNS

### Pattern 1: SafeDraw (RECOMMENDED)
```cpp
void myRender(HudLayer::RenderContext &ctx) {
  // ✅ SAFE: SafeDraw handles coordinate translation automatically
  SafeDraw::fillRect(ctx, x, y, w, h, color);
  SafeDraw::drawString(ctx, text, x, y, font);
  SafeDraw::fillCircle(ctx, x, y, radius, color);
}
```

### Pattern 2: Manual Translation (Complex Shapes)
```cpp
void myRender(HudLayer::RenderContext &ctx) {
  if (ctx.sprite) {
    // ✅ SAFE: Manual coordinate translation
    int16_t local_x = ctx.toLocalX(screen_x);
    int16_t local_y = ctx.toLocalY(screen_y);
    ctx.sprite->fillTriangle(local_x, local_y, ...);
  } else {
    // Drawing to screen
    tft->fillTriangle(screen_x, screen_y, ...);
  }
}
```

### Pattern 3: Text Properties Setup
```cpp
void myRender(HudLayer::RenderContext &ctx) {
  TFT_eSPI *drawTarget = SafeDraw::getDrawTarget(ctx);
  
  // ✅ SAFE: Only for text properties
  drawTarget->setTextColor(TFT_WHITE, TFT_BLACK);
  drawTarget->setTextDatum(MC_DATUM);
  drawTarget->setFreeFont(&FreeSans12pt7b);
  
  // ✅ Then use SafeDraw for actual drawing
  SafeDraw::drawString(ctx, "Text", x, y, font);
}
```

### Pattern 4: Direct TFT (Menu/Calibration Only)
```cpp
void drawMenu() {
  // ✅ SAFE: Direct TFT for fullscreen overlays (no sprites)
  tft->fillScreen(TFT_BLACK);
  tft->drawString("Menu", 240, 50, 4);
  tft->fillRect(60, 40, 360, 240, TFT_BLUE);
}
```

---

## ❌ UNSAFE PATTERNS (DO NOT USE)

### Anti-Pattern 1: Screen Coords to Sprite
```cpp
void myRender(HudLayer::RenderContext &ctx) {
  // ❌ UNSAFE: Screen coordinates written directly to sprite!
  ctx.sprite->fillRect(screen_x, screen_y, w, h, color);
  //                   ^^^^^^^^  ^^^^^^^^
  //                   These need translation!
}
```
**Fix**: Use SafeDraw or ctx.toLocalX/Y

### Anti-Pattern 2: Undefined RenderContext
```cpp
void myRender() {
  // ❌ UNSAFE: ctx is not defined!
  SafeDraw::fillRect(ctx, x, y, w, h, color);
  //                 ^^^ undefined
}
```
**Fix**: Create RenderContext or use direct TFT

### Anti-Pattern 3: Mixed Coordinates
```cpp
void myRender(HudLayer::RenderContext &ctx) {
  int local_x = ctx.toLocalX(screen_x);
  // ❌ UNSAFE: Mixing local_x with screen_y!
  ctx.sprite->fillRect(local_x, screen_y, w, h, color);
  //                   ^^^^^^^  ^^^^^^^^ mixing!
}
```
**Fix**: Translate BOTH coordinates

---

## 🔍 SAFEDRAW API REFERENCE

### Drawing Functions
```cpp
// Rectangles
SafeDraw::fillRect(ctx, x, y, w, h, color)
SafeDraw::drawRect(ctx, x, y, w, h, color)
SafeDraw::fillRoundRect(ctx, x, y, w, h, radius, color)
SafeDraw::drawRoundRect(ctx, x, y, w, h, radius, color)

// Circles
SafeDraw::fillCircle(ctx, x, y, radius, color)
SafeDraw::drawCircle(ctx, x, y, radius, color)

// Lines
SafeDraw::drawLine(ctx, x0, y0, x1, y1, color)
SafeDraw::drawFastHLine(ctx, x, y, w, color)
SafeDraw::drawFastVLine(ctx, x, y, h, color)

// Triangles
SafeDraw::fillTriangle(ctx, x0, y0, x1, y1, x2, y2, color)
SafeDraw::drawTriangle(ctx, x0, y0, x1, y1, x2, y2, color)

// Arcs (for gauges)
SafeDraw::drawArc(ctx, x, y, r1, r2, start, end, fg, bg)

// Text
SafeDraw::drawString(ctx, text, x, y, font)

// Pixel
SafeDraw::drawPixel(ctx, x, y, color)

// Helpers
TFT_eSPI *target = SafeDraw::getDrawTarget(ctx) // For text properties only
```

### RenderContext Helpers
```cpp
// Coordinate translation
int16_t local_x = ctx.toLocalX(screen_x)
int16_t local_y = ctx.toLocalY(screen_y)

// Bounds checking
bool inBounds = ctx.isInBounds(screen_x, screen_y)
bool intersects = ctx.intersectsBounds(screen_x, screen_y, w, h)
bool visible = ctx.clipRect(screen_x, screen_y, w, h)  // Modifies params
```

---

## 📐 SPRITE DIMENSIONS

| Sprite | Dimensions | Notes |
|--------|------------|-------|
| CAR_BODY | 480×320 | Fullscreen - screen coords OK |
| STEERING | 480×320 | Fullscreen - screen coords OK |

**Important**: Both sprites are fullscreen, so screen coordinates == sprite-local coordinates. However, **ALWAYS use SafeDraw** to maintain forward compatibility in case sprite architecture changes.

---

## 🚨 COMMON MISTAKES

### Mistake 1: Forgetting to Create RenderContext
**Problem**: SafeDraw calls fail without context
```cpp
// ❌ WRONG
void render(TFT_eSprite *sprite) {
  SafeDraw::fillRect(ctx, 10, 10, 50, 50, TFT_RED);  // ctx undefined!
}

// ✅ CORRECT
void render(TFT_eSprite *sprite) {
  HudLayer::RenderContext ctx(sprite, true, 0, 0, 
                              sprite ? sprite->width() : 480,
                              sprite ? sprite->height() : 320);
  SafeDraw::fillRect(ctx, 10, 10, 50, 50, TFT_RED);
}
```

### Mistake 2: Using Wrong Variable Names
**Problem**: Function parameters don't match usage
```cpp
// ❌ WRONG
void drawWheel3D(int screenCX, int screenCY, ...) {
  SafeDraw::fillCircle(ctx, cx, cy, 5, color);  // cx/cy undefined!
  //                            ^^  ^^ wrong names
}

// ✅ CORRECT
void drawWheel3D(int screenCX, int screenCY, ...) {
  SafeDraw::fillCircle(ctx, screenCX, screenCY, 5, color);
}
```

### Mistake 3: SafeDraw in Non-Sprite Code
**Problem**: Using SafeDraw for direct TFT rendering
```cpp
// ❌ WRONG (in calibration/menu code)
void drawCalibrationScreen() {
  tft->fillScreen(TFT_BLACK);
  SafeDraw::drawString(ctx, "Calibrate", 240, 60, 4);  // ctx undefined!
}

// ✅ CORRECT
void drawCalibrationScreen() {
  tft->fillScreen(TFT_BLACK);
  tft->drawString("Calibrate", 240, 60, 4);  // Direct TFT is correct here
}
```

---

## 🔒 WHEN TO USE EACH PATTERN

| Scenario | Pattern | Example Files |
|----------|---------|---------------|
| **HUD Rendering** | SafeDraw | hud.cpp, gauges.cpp, icons.cpp |
| **Complex Shapes** | Manual Translation | wheels_display.cpp (rotated wheels) |
| **Text Properties** | getDrawTarget() | All HUD files (setTextColor, etc.) |
| **Menu/Calibration** | Direct TFT | menu_hidden.cpp, touch_calibration.cpp |
| **Fullscreen Sprite** | Direct sprite->* OK | hud.cpp drawCarBody() only |

---

## 📝 CHECKLIST FOR NEW CODE

Before committing new rendering code, verify:

- [ ] Using SafeDraw for all sprite drawing?
- [ ] RenderContext properly created and passed?
- [ ] No mixing of screen and sprite-local coordinates?
- [ ] Text properties use getDrawTarget(), drawing uses SafeDraw?
- [ ] Menu/calibration code uses direct TFT (not SafeDraw)?
- [ ] No undefined variables (ctx, cx/cy mismatch, etc.)?
- [ ] Indentation consistent with existing code?

---

## 🆘 TROUBLESHOOTING

### Compilation Error: "ctx undeclared"
**Solution**: Create RenderContext before using SafeDraw
```cpp
HudLayer::RenderContext ctx(sprite, true, 0, 0, width, height);
```

### Compilation Error: "no matching function"
**Solution**: Check parameter types - SafeDraw expects RenderContext, not TFT_eSPI*
```cpp
// ❌ drawWheel3D(x, y, angle, drawTarget)
// ✅ drawWheel3D(x, y, angle, ctx)
```

### Runtime Crash: Stack canary / Buffer overflow
**Solution**: Check if screen coordinates are being written to sprite without translation
- Add SafeDraw:: prefix to all sprite draw calls
- Or use ctx.toLocalX/Y for manual translation

---

## 📚 REFERENCE DOCUMENTS

- **FINAL_ZERO_TOLERANCE_AUDIT_REPORT.md** - Complete technical audit
- **AUDIT_SECURITY_SUMMARY.md** - Executive summary
- **include/safe_draw.h** - SafeDraw API implementation
- **include/hud_layer.h** - RenderContext definition

---

**Questions?** Review the audit documents or check existing code patterns in hud.cpp, wheels_display.cpp, or gauges.cpp.

**Status**: ✅ Codebase is certified memory-safe. Follow these patterns to maintain safety.
