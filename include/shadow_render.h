#ifndef SHADOW_RENDER_H
#define SHADOW_RENDER_H

/**
 * @brief Shadow Rendering Helpers (Phase 3: Mirror Validation)
 *
 * This header provides helper macros and functions for mirroring TFT drawing
 * operations to the STEERING_SHADOW sprite for validation purposes.
 *
 * CRITICAL: All code in this file is ONLY active when RENDER_SHADOW_MODE is
 * defined. Production builds (without the flag) have ZERO overhead.
 *
 * Usage Pattern:
 *   // Original code (unchanged):
 *   tft->drawLine(x1, y1, x2, y2, color);
 *
 *   // Add mirror (Phase 3):
 *   #ifdef RENDER_SHADOW_MODE
 *   SHADOW_MIRROR_drawLine(x1, y1, x2, y2, color);
 *   #endif
 *
 * The shadow sprite receives identical drawing commands for pixel-perfect
 * comparison.
 */

#ifdef RENDER_SHADOW_MODE

#include "render_engine.h"
#include <TFT_eSPI.h>

// Get the shadow sprite for drawing
inline TFT_eSprite *getShadowSprite() {
  return RenderEngine::getSprite(RenderEngine::STEERING_SHADOW);
}

// ============================================================================
// SHADOW MIRRORING MACROS
// ============================================================================
// These macros mirror common TFT_eSPI drawing operations to the shadow sprite.
// They maintain the exact same API as TFT_eSPI for easy mirroring.

// Basic drawing primitives
#define SHADOW_MIRROR_drawPixel(x, y, color)                                   \
  do {                                                                         \
    auto __s = getShadowSprite();                                              \
    if (__s) __s->drawPixel(x, y, color);                                      \
  } while (0)

#define SHADOW_MIRROR_drawLine(x1, y1, x2, y2, color)                          \
  do {                                                                         \
    auto __s = getShadowSprite();                                              \
    if (__s) __s->drawLine(x1, y1, x2, y2, color);                             \
  } while (0)

#define SHADOW_MIRROR_drawFastHLine(x, y, w, color)                            \
  do {                                                                         \
    auto __s = getShadowSprite();                                              \
    if (__s) __s->drawFastHLine(x, y, w, color);                               \
  } while (0)

#define SHADOW_MIRROR_drawFastVLine(x, y, h, color)                            \
  do {                                                                         \
    auto __s = getShadowSprite();                                              \
    if (__s) __s->drawFastVLine(x, y, h, color);                               \
  } while (0)

// Rectangles
#define SHADOW_MIRROR_drawRect(x, y, w, h, color)                              \
  do {                                                                         \
    auto __s = getShadowSprite();                                              \
    if (__s) __s->drawRect(x, y, w, h, color);                                 \
  } while (0)

#define SHADOW_MIRROR_fillRect(x, y, w, h, color)                              \
  do {                                                                         \
    auto __s = getShadowSprite();                                              \
    if (__s) __s->fillRect(x, y, w, h, color);                                 \
  } while (0)

#define SHADOW_MIRROR_drawRoundRect(x, y, w, h, r, color)                      \
  do {                                                                         \
    auto __s = getShadowSprite();                                              \
    if (__s) __s->drawRoundRect(x, y, w, h, r, color);                         \
  } while (0)

#define SHADOW_MIRROR_fillRoundRect(x, y, w, h, r, color)                      \
  do {                                                                         \
    auto __s = getShadowSprite();                                              \
    if (__s) __s->fillRoundRect(x, y, w, h, r, color);                         \
  } while (0)

// Circles
#define SHADOW_MIRROR_drawCircle(x, y, r, color)                               \
  do {                                                                         \
    auto __s = getShadowSprite();                                              \
    if (__s) __s->drawCircle(x, y, r, color);                                  \
  } while (0)

#define SHADOW_MIRROR_fillCircle(x, y, r, color)                               \
  do {                                                                         \
    auto __s = getShadowSprite();                                              \
    if (__s) __s->fillCircle(x, y, r, color);                                  \
  } while (0)

// Triangles
#define SHADOW_MIRROR_drawTriangle(x1, y1, x2, y2, x3, y3, color)              \
  do {                                                                         \
    auto __s = getShadowSprite();                                              \
    if (__s) __s->drawTriangle(x1, y1, x2, y2, x3, y3, color);                 \
  } while (0)

#define SHADOW_MIRROR_fillTriangle(x1, y1, x2, y2, x3, y3, color)              \
  do {                                                                         \
    auto __s = getShadowSprite();                                              \
    if (__s) __s->fillTriangle(x1, y1, x2, y2, x3, y3, color);                 \
  } while (0)

// Text (requires text properties to be set separately)
#define SHADOW_MIRROR_drawString(str, x, y, font)                              \
  do {                                                                         \
    auto __s = getShadowSprite();                                              \
    if (__s) __s->drawString(str, x, y, font);                                 \
  } while (0)

#define SHADOW_MIRROR_drawChar(c, x, y, font)                                  \
  do {                                                                         \
    auto __s = getShadowSprite();                                              \
    if (__s) __s->drawChar(c, x, y, font);                                     \
  } while (0)

// Text properties (must be called before drawString)
#define SHADOW_MIRROR_setTextDatum(datum)                                      \
  do {                                                                         \
    auto __s = getShadowSprite();                                              \
    if (__s) __s->setTextDatum(datum);                                         \
  } while (0)

#define SHADOW_MIRROR_setTextColor(fg, bg)                                     \
  do {                                                                         \
    auto __s = getShadowSprite();                                              \
    if (__s) __s->setTextColor(fg, bg);                                        \
  } while (0)

#define SHADOW_MIRROR_setTextSize(size)                                        \
  do {                                                                         \
    auto __s = getShadowSprite();                                              \
    if (__s) __s->setTextSize(size);                                           \
  } while (0)

// Advanced drawing
#define SHADOW_MIRROR_drawArc(x, y, r_start, r_end, start_angle, end_angle,    \
                              fg, bg, smooth)                                  \
  do {                                                                         \
    auto __s = getShadowSprite();                                              \
    if (__s)                                                                   \
      __s->drawArc(x, y, r_start, r_end, start_angle, end_angle, fg, bg,       \
                   smooth);                                                    \
  } while (0)

// ============================================================================
// FUNCTION-BASED HELPERS (for complex cases where macros don't work well)
// ============================================================================

/**
 * @brief Mirror a series of drawing operations to shadow sprite
 *
 * Usage:
 *   if (auto shadow = beginShadowMirror()) {
 *     shadow->drawLine(...);
 *     shadow->fillCircle(...);
 *     // ... more drawing
 *   }
 */
inline TFT_eSprite *beginShadowMirror() { return getShadowSprite(); }

/**
 * @brief Check if shadow mirroring is active
 * @return true if RENDER_SHADOW_MODE is defined and shadow sprite exists
 */
inline bool isShadowMirrorActive() { return (getShadowSprite() != nullptr); }

#else // !RENDER_SHADOW_MODE

// When RENDER_SHADOW_MODE is not defined, all macros become no-ops
// This ensures ZERO overhead in production builds

#define SHADOW_MIRROR_drawPixel(x, y, color) ((void)0)
#define SHADOW_MIRROR_drawLine(x1, y1, x2, y2, color) ((void)0)
#define SHADOW_MIRROR_drawFastHLine(x, y, w, color) ((void)0)
#define SHADOW_MIRROR_drawFastVLine(x, y, h, color) ((void)0)
#define SHADOW_MIRROR_drawRect(x, y, w, h, color) ((void)0)
#define SHADOW_MIRROR_fillRect(x, y, w, h, color) ((void)0)
#define SHADOW_MIRROR_drawRoundRect(x, y, w, h, r, color) ((void)0)
#define SHADOW_MIRROR_fillRoundRect(x, y, w, h, r, color) ((void)0)
#define SHADOW_MIRROR_drawCircle(x, y, r, color) ((void)0)
#define SHADOW_MIRROR_fillCircle(x, y, r, color) ((void)0)
#define SHADOW_MIRROR_drawTriangle(x1, y1, x2, y2, x3, y3, color) ((void)0)
#define SHADOW_MIRROR_fillTriangle(x1, y1, x2, y2, x3, y3, color) ((void)0)
#define SHADOW_MIRROR_drawString(str, x, y, font) ((void)0)
#define SHADOW_MIRROR_drawChar(c, x, y, font) ((void)0)
#define SHADOW_MIRROR_setTextDatum(datum) ((void)0)
#define SHADOW_MIRROR_setTextColor(fg, bg) ((void)0)
#define SHADOW_MIRROR_setTextSize(size) ((void)0)
#define SHADOW_MIRROR_drawArc(x, y, r_start, r_end, start_angle, end_angle,    \
                              fg, bg, smooth)                                  \
  ((void)0)

inline void *beginShadowMirror() { return nullptr; }
inline bool isShadowMirrorActive() { return false; }

#endif // RENDER_SHADOW_MODE

#endif // SHADOW_RENDER_H
