#pragma once

#include <TFT_eSPI.h>

/**
 * @file hud_layer.h
 * @brief HUD Layer definitions for Phase 5 compositor architecture
 *
 * PHASE 5 — HUD RENDER PIPELINE & LAYERED COMPOSITOR
 *
 * This file defines the layer system that prevents flicker, ghosting,
 * and sprite corruption by ensuring deterministic rendering order.
 *
 * CORE PRINCIPLE:
 * - Each HUD component renders into its OWN sprite via RenderContext
 * - NO component may touch TFT directly
 * - Only HudCompositor pushes pixels to the display
 */

namespace HudLayer {

/**
 * @brief HUD rendering layers in draw order
 *
 * Layers are composited in this exact order:
 * - BASE: Speed, RPM, battery, temperatures, gauges
 * - STATUS: Limp indicator, warning icons
 * - DIAGNOSTICS: Limp diagnostics panel
 * - OVERLAY: Hidden menu, keypad, regen adjust
 * - FULLSCREEN: Touch calibration, pedal calibration, factory reset
 *
 * When FULLSCREEN is active, only FULLSCREEN is drawn.
 * Otherwise, layers are composited BASE → STATUS → DIAGNOSTICS → OVERLAY.
 */
enum class Layer {
  BASE = 0,        // Base HUD (gauges, speed, RPM)
  STATUS = 1,      // Status overlays (limp indicator)
  DIAGNOSTICS = 2, // Diagnostic overlays (limp diagnostics)
  OVERLAY = 3,     // Modal overlays (menu, keypad)
  FULLSCREEN = 4   // Full screen modes (calibrations)
};

/**
 * @brief Dirty rectangle for partial rendering (PHASE 8)
 *
 * Represents a rectangular region of the screen that needs to be re-rendered.
 * Coordinates are in screen space (0-479 horizontal, 0-319 vertical).
 */
struct DirtyRect {
  int16_t x, y, w, h;

  DirtyRect() : x(0), y(0), w(0), h(0) {}
  DirtyRect(int16_t x_, int16_t y_, int16_t w_, int16_t h_)
      : x(x_), y(y_), w(w_), h(h_) {}

  /**
   * @brief Check if rectangle is empty or invalid
   */
  bool isEmpty() const { return w <= 0 || h <= 0; }

  /**
   * @brief Check if this rectangle overlaps with another
   */
  bool overlaps(const DirtyRect &other) const {
    if (isEmpty() || other.isEmpty())
      return false;
    return !(x >= other.x + other.w || x + w <= other.x ||
             y >= other.y + other.h || y + h <= other.y);
  }

  /**
   * @brief Merge this rectangle with another (returns bounding box)
   */
  DirtyRect merge(const DirtyRect &other) const {
    if (isEmpty())
      return other;
    if (other.isEmpty())
      return *this;

    int16_t x1 = x < other.x ? x : other.x;
    int16_t y1 = y < other.y ? y : other.y;
    int16_t x2 = (x + w) > (other.x + other.w) ? (x + w) : (other.x + other.w);
    int16_t y2 = (y + h) > (other.y + other.h) ? (y + h) : (other.y + other.h);
    return DirtyRect(x1, y1, x2 - x1, y2 - y1);
  }
};

/**
 * @brief Rendering context for a layer
 *
 * Each layer receives a RenderContext that provides:
 * - sprite: The sprite to draw into
 * - dirty: Whether the layer needs redrawing
 * - dirtyRects: Array of dirty rectangles (PHASE 8)
 * - dirtyCount: Number of dirty rectangles
 *
 * Modules should:
 * 1. Check if dirty or if content changed
 * 2. Draw into sprite using sprite->* methods
 * 3. Call markDirty() to report what regions changed
 * 4. Never access TFT directly
 */
struct RenderContext {
  TFT_eSprite *sprite;  // Sprite to render into
  bool dirty;           // Layer needs full redraw
  DirtyRect *dirtyRects; // Array of dirty rectangles (PHASE 8)
  int *dirtyCount;       // Pointer to dirty rectangle count (PHASE 8)

  RenderContext() : sprite(nullptr), dirty(true), dirtyRects(nullptr), dirtyCount(nullptr) {}
  RenderContext(TFT_eSprite *spr, bool d) : sprite(spr), dirty(d), dirtyRects(nullptr), dirtyCount(nullptr) {}
  RenderContext(TFT_eSprite *spr, bool d, DirtyRect *rects, int *count)
      : sprite(spr), dirty(d), dirtyRects(rects), dirtyCount(count) {}

  /**
   * @brief Check if sprite is valid for rendering
   */
  bool isValid() const { return sprite != nullptr; }

  /**
   * @brief Mark a region as dirty (PHASE 8)
   * @param x X coordinate of dirty region
   * @param y Y coordinate of dirty region
   * @param w Width of dirty region
   * @param h Height of dirty region
   *
   * This informs the compositor that a specific region has changed and needs
   * to be pushed to the TFT. Dirty rectangles are automatically merged and
   * clipped by the compositor.
   */
  void markDirty(int16_t x, int16_t y, int16_t w, int16_t h);
};

/**
 * @brief Interface for layer renderers
 *
 * All HUD modules must implement this interface to participate
 * in the layered compositor system.
 *
 * Example implementation:
 * ```
 * class MyHudModule : public LayerRenderer {
 * public:
 *   void render(RenderContext& ctx) override {
 *     if (!ctx.isValid()) return;
 *     // Draw into ctx.sprite using sprite->drawString(), etc.
 *     // NEVER call tft->* directly
 *   }
 *
 *   bool isActive() const override {
 *     return myModuleEnabled;
 *   }
 * };
 * ```
 */
class LayerRenderer {
public:
  virtual ~LayerRenderer() = default;

  /**
   * @brief Render this layer's content into the sprite
   * @param ctx Render context with sprite and dirty flag
   */
  virtual void render(RenderContext &ctx) = 0;

  /**
   * @brief Check if this layer is currently active
   * @return true if layer should be rendered, false to skip
   */
  virtual bool isActive() const = 0;
};

} // namespace HudLayer
