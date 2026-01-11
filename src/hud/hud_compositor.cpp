#include "hud_compositor.h"
#include "logger.h"
#include <cstring>

// Static member initialization
TFT_eSPI *HudCompositor::tft = nullptr;
TFT_eSprite *HudCompositor::layerSprites[LAYER_COUNT] = {nullptr};
HudLayer::LayerRenderer *HudCompositor::layerRenderers[LAYER_COUNT] = {nullptr};
bool HudCompositor::layerDirty[LAYER_COUNT] = {false};
bool HudCompositor::initialized = false;

bool HudCompositor::init(TFT_eSPI *tftDisplay) {
  if (initialized) {
    Logger::warn("HudCompositor: Already initialized");
    return true;
  }

  if (!tftDisplay) {
    Logger::error("HudCompositor: NULL TFT display");
    return false;
  }

  tft = tftDisplay;

  // Create sprites for all layers
  bool allCreated = true;
  for (int i = 0; i < LAYER_COUNT; i++) {
    if (!createLayerSprite(static_cast<HudLayer::Layer>(i))) {
      Logger::errorf("HudCompositor: Failed to create sprite for layer %d", i);
      allCreated = false;
    }
  }

  if (!allCreated) {
    Logger::error("HudCompositor: Failed to create all layer sprites");
    // Clean up any sprites that were created
    for (int i = 0; i < LAYER_COUNT; i++) {
      if (layerSprites[i]) {
        layerSprites[i]->deleteSprite();
        delete layerSprites[i];
        layerSprites[i] = nullptr;
      }
    }
    return false;
  }

  // Mark all layers dirty for initial draw
  markAllDirty();

  initialized = true;
  Logger::info("HudCompositor: Initialized successfully");
  return true;
}

bool HudCompositor::createLayerSprite(HudLayer::Layer layer) {
  int idx = static_cast<int>(layer);
  if (idx < 0 || idx >= LAYER_COUNT) {
    return false;
  }

  // Clean up existing sprite if any
  if (layerSprites[idx]) {
    layerSprites[idx]->deleteSprite();
    delete layerSprites[idx];
    layerSprites[idx] = nullptr;
  }

  // Create new sprite
  layerSprites[idx] = new (std::nothrow) TFT_eSprite(tft);
  if (!layerSprites[idx]) {
    Logger::errorf("HudCompositor: Failed to allocate sprite for layer %d", idx);
    return false;
  }

  // Create sprite buffer (16-bit color)
  void *spriteBuffer =
      layerSprites[idx]->createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);
  if (!spriteBuffer) {
    Logger::errorf(
        "HudCompositor: Failed to create sprite buffer for layer %d", idx);
    delete layerSprites[idx];
    layerSprites[idx] = nullptr;
    return false;
  }

  // Initialize sprite to transparent/black
  layerSprites[idx]->fillSprite(TFT_BLACK);

  return true;
}

void HudCompositor::registerLayer(HudLayer::Layer layer,
                                  HudLayer::LayerRenderer *renderer) {
  int idx = static_cast<int>(layer);
  if (idx < 0 || idx >= LAYER_COUNT) {
    Logger::errorf("HudCompositor: Invalid layer index %d", idx);
    return;
  }

  layerRenderers[idx] = renderer;
  markLayerDirty(layer);
  Logger::infof("HudCompositor: Registered renderer for layer %d", idx);
}

void HudCompositor::unregisterLayer(HudLayer::Layer layer) {
  int idx = static_cast<int>(layer);
  if (idx < 0 || idx >= LAYER_COUNT) {
    return;
  }

  layerRenderers[idx] = nullptr;
}

void HudCompositor::markLayerDirty(HudLayer::Layer layer) {
  int idx = static_cast<int>(layer);
  if (idx < 0 || idx >= LAYER_COUNT) {
    return;
  }

  layerDirty[idx] = true;
}

void HudCompositor::markAllDirty() {
  for (int i = 0; i < LAYER_COUNT; i++) {
    layerDirty[i] = true;
  }
}

void HudCompositor::render() {
  if (!initialized) {
    return;
  }

  // Determine which layers to render
  bool fullscreenActive = false;
  if (layerRenderers[static_cast<int>(HudLayer::Layer::FULLSCREEN)]) {
    fullscreenActive =
        layerRenderers[static_cast<int>(HudLayer::Layer::FULLSCREEN)]
            ->isActive();
  }

  // Render active layers
  for (int i = 0; i < LAYER_COUNT; i++) {
    HudLayer::Layer layer = static_cast<HudLayer::Layer>(i);

    // Skip if no renderer registered
    if (!layerRenderers[i]) {
      continue;
    }

    // Skip if renderer not active
    if (!layerRenderers[i]->isActive()) {
      continue;
    }

    // If fullscreen is active, skip all other layers
    if (fullscreenActive && layer != HudLayer::Layer::FULLSCREEN) {
      continue;
    }

    // Create render context
    HudLayer::RenderContext ctx(layerSprites[i], layerDirty[i]);

    // Call renderer
    layerRenderers[i]->render(ctx);

    // Clear dirty flag after rendering
    layerDirty[i] = false;
  }

  // Composite layers to TFT
  compositeLayers();
}

void HudCompositor::compositeLayers() {
  if (!tft) {
    return;
  }

  // Check if fullscreen is active
  bool fullscreenActive = false;
  int fullscreenIdx = static_cast<int>(HudLayer::Layer::FULLSCREEN);
  if (layerRenderers[fullscreenIdx] &&
      layerRenderers[fullscreenIdx]->isActive()) {
    fullscreenActive = true;
  }

  if (fullscreenActive) {
    // Only push fullscreen layer
    if (layerSprites[fullscreenIdx]) {
      layerSprites[fullscreenIdx]->pushSprite(0, 0);
    }
  } else {
    // Composite BASE → STATUS → DIAGNOSTICS → OVERLAY
    // We'll composite onto BASE sprite and push that
    
    // Push BASE layer first (it's the background)
    int baseIdx = static_cast<int>(HudLayer::Layer::BASE);
    if (layerSprites[baseIdx] && layerRenderers[baseIdx] &&
        layerRenderers[baseIdx]->isActive()) {
      layerSprites[baseIdx]->pushSprite(0, 0);
    }

    // Push overlays on top
    for (int i = 1; i < LAYER_COUNT; i++) {
      if (i == fullscreenIdx) {
        continue; // Skip fullscreen when compositing normal layers
      }

      if (layerSprites[i] && layerRenderers[i] &&
          layerRenderers[i]->isActive()) {
        // Push with transparency (we'll use black as transparent for now)
        // Note: TFT_eSprite doesn't support true alpha, so we push opaque
        layerSprites[i]->pushSprite(0, 0);
      }
    }
  }
}

void HudCompositor::clear() {
  for (int i = 0; i < LAYER_COUNT; i++) {
    if (layerSprites[i]) {
      layerSprites[i]->fillSprite(TFT_BLACK);
    }
  }
  markAllDirty();
}

bool HudCompositor::isInitialized() { return initialized; }

TFT_eSprite *HudCompositor::getLayerSprite(HudLayer::Layer layer) {
  int idx = static_cast<int>(layer);
  if (idx < 0 || idx >= LAYER_COUNT) {
    return nullptr;
  }
  return layerSprites[idx];
}
