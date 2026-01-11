#include "hud_compositor.h"
#include "logger.h"
#include <cstring>

// Static member initialization
TFT_eSPI *HudCompositor::tft = nullptr;
TFT_eSprite *HudCompositor::layerSprites[LAYER_COUNT] = {nullptr};
HudLayer::LayerRenderer *HudCompositor::layerRenderers[LAYER_COUNT] = {nullptr};
bool HudCompositor::layerDirty[LAYER_COUNT] = {false};
bool HudCompositor::initialized = false;

// PHASE 7: Shadow mode static members
TFT_eSprite *HudCompositor::shadowSprite = nullptr;
bool HudCompositor::shadowEnabled = false;
uint32_t HudCompositor::shadowFrameCount = 0;
uint32_t HudCompositor::shadowMismatchCount = 0;
uint32_t HudCompositor::shadowLastMismatchBlocks = 0;

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
  if (idx < 0 || idx >= LAYER_COUNT) { return false; }

  // Clean up existing sprite if any
  if (layerSprites[idx]) {
    layerSprites[idx]->deleteSprite();
    delete layerSprites[idx];
    layerSprites[idx] = nullptr;
  }

  // Check PSRAM availability before allocation
  // Each sprite needs SCREEN_WIDTH * SCREEN_HEIGHT * 2 bytes (16-bit color)
  size_t spriteSize = SCREEN_WIDTH * SCREEN_HEIGHT * 2;
  if (ESP.getFreePsram() < spriteSize) {
    Logger::errorf("HudCompositor: Insufficient PSRAM for layer %d (need %u "
                   "bytes, have %u bytes)",
                   idx, spriteSize, ESP.getFreePsram());
    return false;
  }

  // Create new sprite
  layerSprites[idx] = new (std::nothrow) TFT_eSprite(tft);
  if (!layerSprites[idx]) {
    Logger::errorf("HudCompositor: Failed to allocate sprite for layer %d",
                   idx);
    return false;
  }

  // Create sprite buffer (16-bit color)
  void *spriteBuffer =
      layerSprites[idx]->createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);
  if (!spriteBuffer) {
    Logger::errorf("HudCompositor: Failed to create sprite buffer for layer %d",
                   idx);
    delete layerSprites[idx];
    layerSprites[idx] = nullptr;
    return false;
  }

  // Initialize sprite to transparent/black
  layerSprites[idx]->fillSprite(TFT_BLACK);

  Logger::infof(
      "HudCompositor: Created sprite for layer %d (PSRAM remaining: %u bytes)",
      idx, ESP.getFreePsram());
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
  if (idx < 0 || idx >= LAYER_COUNT) { return; }

  layerRenderers[idx] = nullptr;
}

void HudCompositor::markLayerDirty(HudLayer::Layer layer) {
  int idx = static_cast<int>(layer);
  if (idx < 0 || idx >= LAYER_COUNT) { return; }

  layerDirty[idx] = true;
}

void HudCompositor::markAllDirty() {
  for (int i = 0; i < LAYER_COUNT; i++) {
    layerDirty[i] = true;
  }
}

void HudCompositor::render() {
  if (!initialized) { return; }

  // Determine which layers to render
  bool fullscreenActive = false;
  if (layerRenderers[static_cast<int>(HudLayer::Layer::FULLSCREEN)]) {
    fullscreenActive =
        layerRenderers[static_cast<int>(HudLayer::Layer::FULLSCREEN)]
            ->isActive();
  }

  // PHASE 7: Dual render pass for shadow mode
  // First pass: Render to main sprites
  for (int i = 0; i < LAYER_COUNT; i++) {
    HudLayer::Layer layer = static_cast<HudLayer::Layer>(i);

    // Skip if no renderer registered
    if (!layerRenderers[i]) { continue; }

    // Skip if renderer not active
    if (!layerRenderers[i]->isActive()) { continue; }

    // If fullscreen is active, skip all other layers
    if (fullscreenActive && layer != HudLayer::Layer::FULLSCREEN) { continue; }

    // Create render context for main sprite
    HudLayer::RenderContext ctx(layerSprites[i], layerDirty[i]);

    // Call renderer
    layerRenderers[i]->render(ctx);

    // Clear dirty flag after rendering
    layerDirty[i] = false;
  }

  // PHASE 7: Second pass for shadow mode validation
  // Shadow sprite validates BASE layer only (primary HUD content)
  if (shadowEnabled && shadowSprite) {
    int baseIdx = static_cast<int>(HudLayer::Layer::BASE);
    
    // Only validate BASE layer if it's active
    if (layerRenderers[baseIdx] && layerRenderers[baseIdx]->isActive()) {
      // Clear shadow sprite before rendering
      shadowSprite->fillSprite(TFT_BLACK);

      // Create render context for shadow sprite
      HudLayer::RenderContext ctxShadow(shadowSprite, true);

      // Render BASE layer to shadow sprite
      layerRenderers[baseIdx]->render(ctxShadow);

      // Compare BASE layer sprite with shadow sprite
      compareShadowSprites();
    }
  }

  // Composite layers to TFT
  compositeLayers();
}

void HudCompositor::compositeLayers() {
  if (!tft) { return; }

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
    //
    // KNOWN LIMITATION: TFT_eSprite doesn't support true alpha blending.
    // We push each layer opaque, which works because:
    // 1. BASE layer is rendered directly to TFT (not via compositor yet)
    // 2. Overlay layers (STATUS, DIAGNOSTICS) clear their backgrounds to BLACK
    // 3. BLACK pixels in overlays don't cover important BASE content
    //
    // For true transparency, future enhancement could:
    // - Use pushSprite(x, y, transColor) with color key
    // - Implement custom pixel-by-pixel compositing
    // - Use DMA2D hardware acceleration on supported chips

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
        // Push sprite opaque (see KNOWN LIMITATION above)
        layerSprites[i]->pushSprite(0, 0);
      }
    }
  }
}

void HudCompositor::clear() {
  for (int i = 0; i < LAYER_COUNT; i++) {
    if (layerSprites[i]) { layerSprites[i]->fillSprite(TFT_BLACK); }
  }
  markAllDirty();
}

bool HudCompositor::isInitialized() { return initialized; }

TFT_eSprite *HudCompositor::getLayerSprite(HudLayer::Layer layer) {
  int idx = static_cast<int>(layer);
  if (idx < 0 || idx >= LAYER_COUNT) { return nullptr; }
  return layerSprites[idx];
}

// ============================================================================
// PHASE 7: Shadow Mode Implementation
// ============================================================================

bool HudCompositor::createShadowSprite() {
  // Clean up existing shadow sprite if any
  if (shadowSprite) {
    shadowSprite->deleteSprite();
    delete shadowSprite;
    shadowSprite = nullptr;
  }

  // Check PSRAM availability before allocation
  size_t spriteSize = SCREEN_WIDTH * SCREEN_HEIGHT * 2;
  if (ESP.getFreePsram() < spriteSize) {
    Logger::errorf(
        "HudCompositor: Insufficient PSRAM for shadow sprite (need %u bytes, "
        "have %u bytes)",
        spriteSize, ESP.getFreePsram());
    return false;
  }

  // Create new shadow sprite
  shadowSprite = new (std::nothrow) TFT_eSprite(tft);
  if (!shadowSprite) {
    Logger::error("HudCompositor: Failed to allocate shadow sprite");
    return false;
  }

  // Create sprite buffer (16-bit color)
  void *spriteBuffer = shadowSprite->createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);
  if (!spriteBuffer) {
    Logger::error(
        "HudCompositor: Failed to create shadow sprite buffer");
    delete shadowSprite;
    shadowSprite = nullptr;
    return false;
  }

  // Initialize sprite to transparent/black
  shadowSprite->fillSprite(TFT_BLACK);

  Logger::infof(
      "HudCompositor: Shadow sprite created (PSRAM remaining: %u bytes)",
      ESP.getFreePsram());
  return true;
}

uint16_t HudCompositor::computeBlockChecksum(TFT_eSprite *sprite, int blockX,
                                              int blockY) {
  if (!sprite) return 0;

  uint16_t checksum = 0;
  int startX = blockX * SHADOW_BLOCK_SIZE;
  int startY = blockY * SHADOW_BLOCK_SIZE;
  int endX = startX + SHADOW_BLOCK_SIZE;
  int endY = startY + SHADOW_BLOCK_SIZE;

  // Clamp to sprite bounds
  if (endX > SCREEN_WIDTH) endX = SCREEN_WIDTH;
  if (endY > SCREEN_HEIGHT) endY = SCREEN_HEIGHT;

  // XOR-based checksum (fast and good enough for corruption detection)
  for (int y = startY; y < endY; y++) {
    for (int x = startX; x < endX; x++) {
      uint16_t pixel = sprite->readPixel(x, y);
      checksum ^= pixel;
      // Add position influence to detect shifted content
      checksum ^= static_cast<uint16_t>((x + y) & 0xFFFF);
    }
  }

  return checksum;
}

void HudCompositor::compareShadowSprites() {
  if (!shadowEnabled || !shadowSprite) return;

  shadowFrameCount++;

  // Compare BASE layer sprite with shadow sprite
  // Note: Both sprites contain identical BASE layer content
  // - layerSprites[BASE] is the main BASE layer sprite (never modified by compositor)
  // - shadowSprite is the validation sprite (rendered separately)
  // The compositeLayers() function pushes sprites to TFT but doesn't modify them
  int baseIdx = static_cast<int>(HudLayer::Layer::BASE);
  TFT_eSprite *mainSprite = layerSprites[baseIdx];

  if (!mainSprite) {
    Logger::warn("HudCompositor: No BASE sprite available for shadow comparison");
    return;
  }

  uint32_t mismatchBlocks = 0;
  bool firstMismatch = true;
  int firstMismatchX = -1;
  int firstMismatchY = -1;

  // Compare sprites in 16x16 blocks
  for (int by = 0; by < SHADOW_BLOCKS_Y; by++) {
    for (int bx = 0; bx < SHADOW_BLOCKS_X; bx++) {
      uint16_t checksumMain = computeBlockChecksum(mainSprite, bx, by);
      uint16_t checksumShadow = computeBlockChecksum(shadowSprite, bx, by);

      if (checksumMain != checksumShadow) {
        mismatchBlocks++;
        if (firstMismatch) {
          firstMismatchX = bx;
          firstMismatchY = by;
          firstMismatch = false;
        }
      }
    }
  }

  shadowLastMismatchBlocks = mismatchBlocks;

  // Log mismatches
  if (mismatchBlocks > 0) {
    shadowMismatchCount++;
    Logger::errorf(
        "HUD SHADOW MISMATCH: Frame %u, %u/%u blocks differ, first at "
        "block(%d,%d) px(%d,%d)",
        shadowFrameCount, mismatchBlocks, SHADOW_BLOCKS_X * SHADOW_BLOCKS_Y,
        firstMismatchX, firstMismatchY, firstMismatchX * SHADOW_BLOCK_SIZE,
        firstMismatchY * SHADOW_BLOCK_SIZE);
    
    // Note: Visual indicator removed to avoid interfering with BASE layer
    // Corruption is visible in hidden menu statistics (red color when M > 0)
  }
}

void HudCompositor::setShadowMode(bool enabled) {
  if (enabled && !shadowEnabled) {
    // Enable shadow mode
    if (createShadowSprite()) {
      shadowEnabled = true;
      shadowFrameCount = 0;
      shadowMismatchCount = 0;
      shadowLastMismatchBlocks = 0;
      Logger::info("HudCompositor: Shadow mode ENABLED");
    } else {
      Logger::error("HudCompositor: Failed to enable shadow mode");
      shadowEnabled = false;
    }
  } else if (!enabled && shadowEnabled) {
    // Disable shadow mode
    shadowEnabled = false;
    if (shadowSprite) {
      shadowSprite->deleteSprite();
      delete shadowSprite;
      shadowSprite = nullptr;
    }
    Logger::info("HudCompositor: Shadow mode DISABLED");
  }
}

bool HudCompositor::isShadowModeEnabled() { return shadowEnabled; }

void HudCompositor::getShadowStats(uint32_t &outTotalComparisons,
                                   uint32_t &outMismatchCount,
                                   uint32_t &outLastMismatchBlocks) {
  outTotalComparisons = shadowFrameCount;
  outMismatchCount = shadowMismatchCount;
  outLastMismatchBlocks = shadowLastMismatchBlocks;
}
