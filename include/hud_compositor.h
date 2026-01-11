#pragma once

#include "hud_layer.h"
#include <TFT_eSPI.h>

/**
 * @file hud_compositor.h
 * @brief HUD Compositor for Phase 5 layered rendering
 *
 * PHASE 5 — HUD RENDER PIPELINE & LAYERED COMPOSITOR
 *
 * The compositor is the ONLY code allowed to touch the physical TFT display.
 * All HUD modules must render into their layer sprites via RenderContext.
 *
 * ARCHITECTURE:
 * - Owns one sprite per layer (BASE, STATUS, DIAGNOSTICS, OVERLAY, FULLSCREEN)
 * - Manages layer renderer registration
 * - Composites layers in deterministic order
 * - Pushes only the final composite to TFT
 *
 * OVERLAY RULES:
 * - If FULLSCREEN active: Only FULLSCREEN is drawn
 * - Otherwise: BASE + STATUS + DIAGNOSTICS + OVERLAY are composited
 *
 * WHY THIS MATTERS:
 * - No more ghosting or flicker
 * - No more fillScreen hacks
 * - Menus can open/close cleanly
 * - HUD and diagnostics never overwrite each other
 * - System becomes deterministic and debuggable
 */
class HudCompositor {
public:
  /**
   * @brief Initialize the compositor
   * @param tftDisplay Pointer to TFT_eSPI display instance
   * @return true if initialization succeeded, false otherwise
   */
  static bool init(TFT_eSPI *tftDisplay);

  /**
   * @brief Register a layer renderer
   * @param layer Layer to register renderer for
   * @param renderer Pointer to renderer implementation
   *
   * Note: Compositor does NOT take ownership of renderer.
   * Caller must ensure renderer lifetime exceeds compositor usage.
   */
  static void registerLayer(HudLayer::Layer layer,
                            HudLayer::LayerRenderer *renderer);

  /**
   * @brief Unregister a layer renderer
   * @param layer Layer to unregister
   */
  static void unregisterLayer(HudLayer::Layer layer);

  /**
   * @brief Mark a layer as dirty (needs redraw)
   * @param layer Layer to mark dirty
   */
  static void markLayerDirty(HudLayer::Layer layer);

  /**
   * @brief Mark all layers as dirty
   */
  static void markAllDirty();

  /**
   * @brief Composite and render all active layers to TFT
   *
   * Pipeline:
   * 1. Clear dirty layers
   * 2. Call render() on active layer renderers
   * 3. Composite layers in order
   * 4. Push final composite to TFT
   *
   * If FULLSCREEN layer is active, only FULLSCREEN is rendered.
   * Otherwise, BASE + STATUS + DIAGNOSTICS + OVERLAY are composited.
   */
  static void render();

  /**
   * @brief Clear all layer sprites
   */
  static void clear();

  /**
   * @brief Check if compositor is initialized
   * @return true if initialized, false otherwise
   */
  static bool isInitialized();

  /**
   * @brief Get the sprite for a specific layer
   * @param layer Layer to get sprite for
   * @return Pointer to sprite, or nullptr if not available
   *
   * Note: This is provided for advanced use cases.
   * Normal rendering should use the RenderContext passed to render().
   */
  static TFT_eSprite *getLayerSprite(HudLayer::Layer layer);

private:
  static constexpr int LAYER_COUNT = 5;
  static constexpr int SCREEN_WIDTH = 480;
  static constexpr int SCREEN_HEIGHT = 320;

  static TFT_eSPI *tft;
  static TFT_eSprite *layerSprites[LAYER_COUNT];
  static HudLayer::LayerRenderer *layerRenderers[LAYER_COUNT];
  static bool layerDirty[LAYER_COUNT];
  static bool initialized;

  // Helper to create a layer sprite
  static bool createLayerSprite(HudLayer::Layer layer);

  // Helper to composite layers to TFT
  static void compositeLayers();
};
