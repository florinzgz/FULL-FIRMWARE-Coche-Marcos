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
  BASE = 0,       // Base HUD (gauges, speed, RPM)
  STATUS = 1,     // Status overlays (limp indicator)
  DIAGNOSTICS = 2, // Diagnostic overlays (limp diagnostics)
  OVERLAY = 3,    // Modal overlays (menu, keypad)
  FULLSCREEN = 4  // Full screen modes (calibrations)
};

/**
 * @brief Rendering context for a layer
 *
 * Each layer receives a RenderContext that provides:
 * - sprite: The sprite to draw into
 * - dirty: Whether the layer needs redrawing
 *
 * Modules should:
 * 1. Check if dirty or if content changed
 * 2. Draw into sprite using sprite->* methods
 * 3. Never access TFT directly
 */
struct RenderContext {
  TFT_eSprite *sprite; // Sprite to render into
  bool dirty;          // Layer needs full redraw

  RenderContext() : sprite(nullptr), dirty(true) {}
  RenderContext(TFT_eSprite *spr, bool d) : sprite(spr), dirty(d) {}

  /**
   * @brief Check if sprite is valid for rendering
   */
  bool isValid() const { return sprite != nullptr; }
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
