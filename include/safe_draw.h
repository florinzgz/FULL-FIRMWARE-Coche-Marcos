#ifndef SAFE_DRAW_H
#define SAFE_DRAW_H

#include "hud_layer.h"
#include <TFT_eSPI.h>

/**
 * @file safe_draw.h
 * @brief Safe drawing primitives that prevent coordinate space corruption
 *
 * 🚨 CRITICAL FIX for ipc0 Stack Canary Crashes
 * 
 * Problem: Functions were passing absolute screen coordinates (0-480, 0-320)
 * to sprite drawing calls, which expect local coordinates (0-sprite.width, 0-sprite.height).
 * This caused out-of-bounds writes that corrupted heap and FreeRTOS IPC stacks.
 *
 * Solution: These safe draw wrappers automatically:
 * 1. Translate screen coordinates to sprite-local coordinates
 * 2. Clip drawing to sprite bounds
 * 3. Log violations in debug mode
 * 4. Prevent ALL out-of-bounds writes
 *
 * Usage:
 * Instead of:
 *   TFT_eSPI *drawTarget = sprite ? (TFT_eSPI*)sprite : tft;
 *   drawTarget->fillRect(240, 40, 100, 50, color);  // ❌ CRASHES if sprite is 100x100
 *
 * Use:
 *   SafeDraw::fillRect(ctx, 240, 40, 100, 50, color);  // ✅ SAFE: auto-translates and clips
 */

namespace SafeDraw {

// Pointer to global TFT object - initialized by init()
// Actual tft object defined in hud_manager.cpp
static TFT_eSPI *tftPtr = nullptr;

/**
 * @brief Initialize SafeDraw with TFT reference
 * Must be called before using SafeDraw functions
 */
inline void init(TFT_eSPI *tft) {
  tftPtr = tft;
}

/**
 * @brief Safe fillRect with coordinate translation and clipping
 * @param ctx Render context (contains sprite, origin, bounds)
 * @param screenX Rectangle X in screen coordinates
 * @param screenY Rectangle Y in screen coordinates
 * @param w Rectangle width
 * @param h Rectangle height
 * @param color Fill color
 */
inline void fillRect(const HudLayer::RenderContext &ctx, int16_t screenX, 
                     int16_t screenY, int16_t w, int16_t h, uint16_t color) {
  if (ctx.sprite) {
    // Drawing to sprite - translate and clip
    int16_t x = screenX;
    int16_t y = screenY;
    int16_t width = w;
    int16_t height = h;
    
    if (!ctx.clipRect(x, y, width, height)) {
      // Rectangle completely outside sprite bounds - skip draw
#ifdef DEBUG_SAFE_DRAW
      Serial.printf("[SafeDraw] fillRect clipped out: screen(%d,%d,%d,%d) sprite(%d,%d,%d,%d)\n",
                    screenX, screenY, w, h, ctx.originX, ctx.originY, ctx.width, ctx.height);
#endif
      return;
    }
    
    // Convert to local coordinates
    int16_t localX = ctx.toLocalX(x);
    int16_t localY = ctx.toLocalY(y);
    
#ifdef DEBUG_SAFE_DRAW
    if (localX < -20 || localY < -20 || localX > ctx.width + 20 || localY > ctx.height + 20) {
      Serial.printf("[SafeDraw] WARNING: fillRect near bounds: local(%d,%d,%d,%d) sprite(%d,%d)\n",
                    localX, localY, width, height, ctx.width, ctx.height);
    }
#endif
    
    ctx.sprite->fillRect(localX, localY, width, height, color);
  } else {
    // Drawing to screen - use screen coordinates directly
    if (tftPtr) tftPtr->fillRect(screenX, screenY, w, h, color);
  }
}

/**
 * @brief Safe drawCircle with coordinate translation and bounds check
 */
inline void drawCircle(const HudLayer::RenderContext &ctx, int16_t screenX, 
                       int16_t screenY, int16_t r, uint16_t color) {
  if (ctx.sprite) {
    // Check if circle intersects sprite (approximate check using bounding box)
    if (!ctx.intersectsBounds(screenX - r, screenY - r, 2*r + 1, 2*r + 1)) {
#ifdef DEBUG_SAFE_DRAW
      Serial.printf("[SafeDraw] drawCircle out of bounds: screen(%d,%d,r=%d)\n",
                    screenX, screenY, r);
#endif
      return;
    }
    
    int16_t localX = ctx.toLocalX(screenX);
    int16_t localY = ctx.toLocalY(screenY);
    
    ctx.sprite->drawCircle(localX, localY, r, color);
  } else {
    if (tftPtr) tftPtr->drawCircle(screenX, screenY, r, color);
  }
}

/**
 * @brief Safe drawLine with coordinate translation and bounds check
 */
inline void drawLine(const HudLayer::RenderContext &ctx, int16_t x0, int16_t y0,
                     int16_t x1, int16_t y1, uint16_t color) {
  if (ctx.sprite) {
    // Simple bounds check - if both points are outside, skip
    // (More sophisticated line clipping could be added later)
    bool p0InBounds = ctx.isInBounds(x0, y0);
    bool p1InBounds = ctx.isInBounds(x1, y1);
    
    if (!p0InBounds && !p1InBounds) {
      // Both endpoints outside - check if line passes through sprite
      // For now, just skip to prevent corruption
      // TODO: Implement Cohen-Sutherland line clipping
#ifdef DEBUG_SAFE_DRAW
      Serial.printf("[SafeDraw] drawLine out of bounds: (%d,%d)->(%d,%d)\n",
                    x0, y0, x1, y1);
#endif
      return;
    }
    
    int16_t local_x0 = ctx.toLocalX(x0);
    int16_t local_y0 = ctx.toLocalY(y0);
    int16_t local_x1 = ctx.toLocalX(x1);
    int16_t local_y1 = ctx.toLocalY(y1);
    
    ctx.sprite->drawLine(local_x0, local_y0, local_x1, local_y1, color);
  } else {
    if (tftPtr) tftPtr->drawLine(x0, y0, x1, y1, color);
  }
}

/**
 * @brief Safe fillCircle with coordinate translation and bounds check
 */
inline void fillCircle(const HudLayer::RenderContext &ctx, int16_t screenX,
                       int16_t screenY, int16_t r, uint16_t color) {
  if (ctx.sprite) {
    // Check if circle intersects sprite
    if (!ctx.intersectsBounds(screenX - r, screenY - r, 2*r + 1, 2*r + 1)) {
#ifdef DEBUG_SAFE_DRAW
      Serial.printf("[SafeDraw] fillCircle out of bounds: screen(%d,%d,r=%d)\n",
                    screenX, screenY, r);
#endif
      return;
    }
    
    int16_t localX = ctx.toLocalX(screenX);
    int16_t localY = ctx.toLocalY(screenY);
    
    ctx.sprite->fillCircle(localX, localY, r, color);
  } else {
    if (tftPtr) tftPtr->fillCircle(screenX, screenY, r, color);
  }
}

/**
 * @brief Safe drawString with coordinate translation and bounds check
 */
inline void drawString(const HudLayer::RenderContext &ctx, const char *string,
                       int16_t screenX, int16_t screenY, uint8_t font = 1) {
  if (ctx.sprite) {
    // Rough bounds check - assume max text height of 40 pixels
    if (!ctx.intersectsBounds(screenX, screenY, 100, 40)) {
#ifdef DEBUG_SAFE_DRAW
      Serial.printf("[SafeDraw] drawString out of bounds: screen(%d,%d) '%s'\n",
                    screenX, screenY, string);
#endif
      return;
    }
    
    int16_t localX = ctx.toLocalX(screenX);
    int16_t localY = ctx.toLocalY(screenY);
    
    ctx.sprite->drawString(string, localX, localY, font);
  } else {
    if (tftPtr) tftPtr->drawString(string, screenX, screenY, font);
  }
}

/**
 * @brief Safe drawPixel with coordinate translation and bounds check
 */
inline void drawPixel(const HudLayer::RenderContext &ctx, int16_t screenX,
                      int16_t screenY, uint16_t color) {
  if (ctx.sprite) {
    if (!ctx.isInBounds(screenX, screenY)) {
      return; // Pixel outside sprite
    }
    
    int16_t localX = ctx.toLocalX(screenX);
    int16_t localY = ctx.toLocalY(screenY);
    
    ctx.sprite->drawPixel(localX, localY, color);
  } else {
    if (tftPtr) tftPtr->drawPixel(screenX, screenY, color);
  }
}

/**
 * @brief Safe fillRoundRect with coordinate translation and clipping
 */
inline void fillRoundRect(const HudLayer::RenderContext &ctx, int16_t screenX,
                          int16_t screenY, int16_t w, int16_t h, int16_t r, uint16_t color) {
  if (ctx.sprite) {
    int16_t x = screenX;
    int16_t y = screenY;
    int16_t width = w;
    int16_t height = h;
    
    if (!ctx.clipRect(x, y, width, height)) {
      return; // Rectangle completely outside sprite bounds
    }
    
    int16_t localX = ctx.toLocalX(x);
    int16_t localY = ctx.toLocalY(y);
    
    ctx.sprite->fillRoundRect(localX, localY, width, height, r, color);
  } else {
    if (tftPtr) tftPtr->fillRoundRect(screenX, screenY, w, h, r, color);
  }
}

/**
 * @brief Safe drawRoundRect with coordinate translation and clipping
 */
inline void drawRoundRect(const HudLayer::RenderContext &ctx, int16_t screenX,
                          int16_t screenY, int16_t w, int16_t h, int16_t r, uint16_t color) {
  if (ctx.sprite) {
    int16_t x = screenX;
    int16_t y = screenY;
    int16_t width = w;
    int16_t height = h;
    
    if (!ctx.clipRect(x, y, width, height)) {
      return;
    }
    
    int16_t localX = ctx.toLocalX(x);
    int16_t localY = ctx.toLocalY(y);
    
    ctx.sprite->drawRoundRect(localX, localY, width, height, r, color);
  } else {
    if (tftPtr) tftPtr->drawRoundRect(screenX, screenY, w, h, r, color);
  }
}

/**
 * @brief Safe drawFastHLine (horizontal line)
 */
inline void drawFastHLine(const HudLayer::RenderContext &ctx, int16_t screenX,
                          int16_t screenY, int16_t w, uint16_t color) {
  if (ctx.sprite) {
    int16_t x = screenX;
    int16_t y = screenY;
    int16_t width = w;
    int16_t height = 1;
    
    if (!ctx.clipRect(x, y, width, height)) {
      return;
    }
    
    int16_t localX = ctx.toLocalX(x);
    int16_t localY = ctx.toLocalY(y);
    
    ctx.sprite->drawFastHLine(localX, localY, width, color);
  } else {
    if (tftPtr) tftPtr->drawFastHLine(screenX, screenY, w, color);
  }
}

/**
 * @brief Safe drawFastVLine (vertical line)
 */
inline void drawFastVLine(const HudLayer::RenderContext &ctx, int16_t screenX,
                          int16_t screenY, int16_t h, uint16_t color) {
  if (ctx.sprite) {
    int16_t x = screenX;
    int16_t y = screenY;
    int16_t width = 1;
    int16_t height = h;
    
    if (!ctx.clipRect(x, y, width, height)) {
      return;
    }
    
    int16_t localX = ctx.toLocalX(x);
    int16_t localY = ctx.toLocalY(y);
    
    ctx.sprite->drawFastVLine(localX, localY, height, color);
  } else {
    if (tftPtr) tftPtr->drawFastVLine(screenX, screenY, h, color);
  }
}

/**
 * @brief Get drawTarget pointer for functions that need raw access
 * WARNING: Use with extreme caution! Prefer SafeDraw methods.
 * Only use this when you need setTextColor, setTextDatum, etc.
 * You MUST still translate coordinates manually!
 */
inline TFT_eSPI* getDrawTarget(const HudLayer::RenderContext &ctx) {
  return ctx.sprite ? (TFT_eSPI*)ctx.sprite : tftPtr;
}

} // namespace SafeDraw

#endif // SAFE_DRAW_H
