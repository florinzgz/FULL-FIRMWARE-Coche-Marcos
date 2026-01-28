#include "hud_compositor.h"
#include "boot_guard.h"
#include "logger.h"
#include <cstring>

// ============================================================================
// 🔒 v2.18.1: Memory management constants for PSRAM-less operation
// ============================================================================
namespace {
constexpr uint32_t HEAP_SAFETY_MARGIN_BYTES =
    50000; // 50KB margin for heap allocations
}

// Forward declaration for RenderContext::markDirty()
namespace HudLayer {
void RenderContext::markDirty(int16_t x, int16_t y, int16_t w, int16_t h) {
  // Only add dirty rect if we have the tracking arrays
  if (!dirtyRects || !dirtyCount) {
    return; // No dirty tracking available
  }

  // Call compositor to add dirty rect
  HudCompositor::addDirtyRect(x, y, w, h);
}
} // namespace HudLayer

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

// PHASE 8: Dirty rectangle tracking static members
HudLayer::DirtyRect HudCompositor::dirtyRects[MAX_DIRTY_RECTS];
int HudCompositor::dirtyRectCount = 0;

// PHASE 9: Render statistics static members
HudCompositor::RenderStats HudCompositor::renderStats = {};

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

  // 🔒 v2.18.1: Check memory availability (PSRAM or heap)
  // Each sprite needs SCREEN_WIDTH * SCREEN_HEIGHT * 2 bytes (16-bit color)
  size_t spriteSize = SCREEN_WIDTH * SCREEN_HEIGHT * 2;
  bool usePsram = psramFound();

  if (usePsram) {
    if (ESP.getFreePsram() < spriteSize) {
      Logger::errorf("HudCompositor: Insufficient PSRAM for layer %d (need %u "
                     "bytes, have %u bytes)",
                     idx, spriteSize, ESP.getFreePsram());
      return false;
    }
  } else {
    Logger::warnf(
        "HudCompositor: PSRAM not available - using heap for layer %d", idx);
    uint32_t freeHeap = ESP.getFreeHeap();
    if (freeHeap < spriteSize + HEAP_SAFETY_MARGIN_BYTES) {
      Logger::errorf(
          "HudCompositor: Insufficient heap for layer %d (need %u KB, "
          "have %u KB, margin %u KB)",
          idx, spriteSize / 1024, freeHeap / 1024,
          HEAP_SAFETY_MARGIN_BYTES / 1024);
      return false;
    }
  }

  // Create new sprite
  layerSprites[idx] = new (std::nothrow) TFT_eSprite(tft);
  if (!layerSprites[idx]) {
    BootGuard::setResetMarker(BootGuard::RESET_MARKER_NULL_POINTER);
    Logger::errorf("HudCompositor: Failed to allocate sprite for layer %d",
                   idx);
    return false;
  }

  // 🔒 CRITICAL: Force sprites to PSRAM to avoid heap pressure/bootloops
  // 🔒 v2.18.1: Only if PSRAM available (some hardware has bootloop with OPI)
  if (usePsram) {
    layerSprites[idx]->setAttribute(PSRAM_ENABLE, 1);
  } else {
    Logger::warn("  Layer sprite will use heap - monitor memory usage");
  }

  // Create sprite buffer (16-bit color)
  void *spriteBuffer =
      layerSprites[idx]->createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);
  if (!spriteBuffer) {
    // 🔒 N16R8 CRITICAL: PSRAM allocation failed - diagnostic output
    Serial.printf("[HudCompositor] PSRAM FAIL Layer %d - buffer is NULL\n", idx);
    Serial.printf("  PSRAM found: %s\n", psramFound() ? "YES" : "NO");
    Serial.printf("  Free PSRAM: %u bytes\n", ESP.getFreePsram());
    Serial.printf("  Free Heap: %u bytes\n", ESP.getFreeHeap());
    Logger::errorf("HudCompositor: Failed to create sprite buffer for layer %d",
                   idx);
    delete layerSprites[idx];
    layerSprites[idx] = nullptr;
    return false;
  }

  // Initialize sprite to transparent/black
  layerSprites[idx]->fillSprite(TFT_BLACK);

  if (usePsram) {
    Logger::infof("HudCompositor: Created sprite for layer %d (PSRAM "
                  "remaining: %u bytes)",
                  idx, ESP.getFreePsram());
  } else {
    Logger::infof(
        "HudCompositor: Created sprite for layer %d (Heap remaining: %u bytes)",
        idx, ESP.getFreeHeap());
  }
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

  // PHASE 9: Start frame timing
  uint32_t frameStartTime = millis();

  // PHASE 8: Clear dirty rectangles from previous frame
  clearDirtyRects();

  // Determine which layers to render
  bool fullscreenActive = false;
  if (layerRenderers[static_cast<int>(HudLayer::Layer::FULLSCREEN)]) {
    fullscreenActive =
        layerRenderers[static_cast<int>(HudLayer::Layer::FULLSCREEN)]
            ->isActive();
  }

  // PHASE 10: Selective full-screen dirty marking
  // Only mark full screen for non-BASE layers or first frame
  // BASE layer components will mark their own dirty regions
  static bool firstFrame = true;
  bool anyLayerDirty = false;
  bool baseLayerDirty = false;

  for (int i = 0; i < LAYER_COUNT; i++) {
    if (layerDirty[i] && layerRenderers[i] && layerRenderers[i]->isActive()) {
      anyLayerDirty = true;
      if (i == static_cast<int>(HudLayer::Layer::BASE)) {
        baseLayerDirty = true;
      }
      break;
    }
  }

  // Mark full screen dirty only for:
  // 1. First frame (to establish initial state)
  // 2. Non-BASE layers that are dirty (FULLSCREEN, OVERLAY, etc.)
  // BASE layer components will use granular dirty tracking
  if (firstFrame) {
    addDirtyRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    firstFrame = false;
  } else if (anyLayerDirty && !baseLayerDirty) {
    // Non-BASE layer is dirty, mark full screen
    addDirtyRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  }
  // If only BASE is dirty, components will mark their own regions

  // PHASE 8: Dual render pass for main and shadow
  // First pass: Render to main sprites with dirty rect tracking
  for (int i = 0; i < LAYER_COUNT; i++) {
    HudLayer::Layer layer = static_cast<HudLayer::Layer>(i);

    // Skip if no renderer registered
    if (!layerRenderers[i]) { continue; }

    // Skip if renderer not active
    if (!layerRenderers[i]->isActive()) { continue; }

    // If fullscreen is active, skip all other layers
    if (fullscreenActive && layer != HudLayer::Layer::FULLSCREEN) { continue; }

    // PHASE 8: Create render context with dirty rect tracking
    HudLayer::RenderContext ctx(layerSprites[i], layerDirty[i], dirtyRects,
                                &dirtyRectCount);

    // PHASE 8: Clear only dirty regions in sprite (optimization)
    if (dirtyRectCount > 0 && layerSprites[i]) {
      for (int r = 0; r < dirtyRectCount; r++) {
        const HudLayer::DirtyRect &rect = dirtyRects[r];
        if (!rect.isEmpty()) {
          layerSprites[i]->fillRect(rect.x, rect.y, rect.w, rect.h, TFT_BLACK);
        }
      }
    }

    // Call renderer
    layerRenderers[i]->render(ctx);

    // Clear dirty flag after rendering
    layerDirty[i] = false;
  }

  // PHASE 8: Second pass for shadow mode validation (only dirty rects)
  // Shadow sprite validates BASE layer only (primary HUD content)
  if (shadowEnabled && shadowSprite) {
    int baseIdx = static_cast<int>(HudLayer::Layer::BASE);

    // Only validate BASE layer if it's active
    if (layerRenderers[baseIdx] && layerRenderers[baseIdx]->isActive()) {
      // PHASE 8: Clear only dirty regions in shadow sprite
      if (dirtyRectCount > 0) {
        for (int r = 0; r < dirtyRectCount; r++) {
          const HudLayer::DirtyRect &rect = dirtyRects[r];
          if (!rect.isEmpty()) {
            shadowSprite->fillRect(rect.x, rect.y, rect.w, rect.h, TFT_BLACK);
          }
        }
      }

      // Create render context for shadow sprite
      HudLayer::RenderContext ctxShadow(shadowSprite, true, dirtyRects,
                                        &dirtyRectCount);

      // Render BASE layer to shadow sprite
      layerRenderers[baseIdx]->render(ctxShadow);

      // PHASE 8: Compare only dirty regions
      compareShadowSprites();
    }
  }

  // Composite layers to TFT (only dirty rectangles)
  compositeLayers();

  // PHASE 9: Update render statistics
  uint32_t frameEndTime = millis();
  uint32_t frameTime = frameEndTime - frameStartTime;

  // Update frame count
  renderStats.frameCount++;

  // Update frame time (exponential moving average, alpha = 0.1)
  renderStats.lastFrameTimeMs = frameTime;
  if (renderStats.avgFrameTimeMs == 0) {
    renderStats.avgFrameTimeMs = frameTime;
    // Calculate FPS from average frame time
    renderStats.fps = (frameTime > 0) ? (1000 / frameTime) : 0;
  } else {
    // Smoothed: avg = avg * 0.9 + new * 0.1
    // Using integer math: avg = (avg * 9 + new) / 10
    uint32_t newAvg = (renderStats.avgFrameTimeMs * 9 + frameTime) / 10;
    // Only recalculate FPS if average changed
    if (newAvg != renderStats.avgFrameTimeMs) {
      renderStats.avgFrameTimeMs = newAvg;
      renderStats.fps = (newAvg > 0) ? (1000 / newAvg) : 0;
    }
  }

  // Update dirty rect statistics
  renderStats.dirtyRectCount = dirtyRectCount;

  // Calculate dirty pixels
  uint32_t totalDirtyPixels = 0;
  for (int i = 0; i < dirtyRectCount; i++) {
    totalDirtyPixels += dirtyRects[i].w * dirtyRects[i].h;
  }
  renderStats.dirtyPixels = totalDirtyPixels;

  // Calculate bytes pushed (16-bit color = 2 bytes per pixel)
  renderStats.bytesPushed = totalDirtyPixels * 2;

  // Update shadow mode statistics
  renderStats.shadowEnabled = shadowEnabled;
  renderStats.shadowBlocksCompared = shadowFrameCount;
  renderStats.shadowMismatches = shadowMismatchCount;

  // Calculate PSRAM usage
  size_t psramUsed = 0;
  for (int i = 0; i < LAYER_COUNT; i++) {
    if (layerSprites[i]) {
      psramUsed += SCREEN_WIDTH * SCREEN_HEIGHT * 2; // 16-bit per pixel
    }
  }
  if (shadowSprite) { psramUsed += SCREEN_WIDTH * SCREEN_HEIGHT * 2; }
  renderStats.psramUsedBytes = psramUsed;
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

  // PHASE 8: Push only dirty rectangles to TFT
  if (dirtyRectCount == 0) {
    return; // Nothing to update
  }

  if (fullscreenActive) {
    // Only push fullscreen layer (dirty rects)
    if (layerSprites[fullscreenIdx]) {
      for (int r = 0; r < dirtyRectCount; r++) {
        const HudLayer::DirtyRect &rect = dirtyRects[r];
        if (!rect.isEmpty()) {
          // pushSprite(x, y, sx, sy, sw, sh)
          // x, y = destination on TFT
          // sx, sy = source position in sprite
          // sw, sh = width/height to copy
          layerSprites[fullscreenIdx]->pushSprite(rect.x, rect.y, rect.x,
                                                  rect.y, rect.w, rect.h);
        }
      }
    }
  } else {
    // PHASE 8: Composite BASE → STATUS → DIAGNOSTICS → OVERLAY (dirty rects
    // only)
    //
    // For each dirty rectangle:
    // 1. Push BASE layer first (background)
    // 2. Push overlay layers on top
    //
    // KNOWN LIMITATION: TFT_eSprite doesn't support true alpha blending.
    // We push each layer opaque, which works because:
    // 1. Overlay layers (STATUS, DIAGNOSTICS) clear their backgrounds to BLACK
    // 2. BLACK pixels in overlays don't cover important BASE content
    //
    // For true transparency, future enhancement could:
    // - Use pushSprite(x, y, transColor) with color key
    // - Implement custom pixel-by-pixel compositing
    // - Use DMA2D hardware acceleration on supported chips

    for (int r = 0; r < dirtyRectCount; r++) {
      const HudLayer::DirtyRect &rect = dirtyRects[r];
      if (rect.isEmpty()) continue;

      // Push BASE layer first (it's the background)
      int baseIdx = static_cast<int>(HudLayer::Layer::BASE);
      if (layerSprites[baseIdx] && layerRenderers[baseIdx] &&
          layerRenderers[baseIdx]->isActive()) {
        layerSprites[baseIdx]->pushSprite(rect.x, rect.y, rect.x, rect.y,
                                          rect.w, rect.h);
      }

      // Push overlays on top
      for (int i = 1; i < LAYER_COUNT; i++) {
        if (i == fullscreenIdx) {
          continue; // Skip fullscreen when compositing normal layers
        }

        if (layerSprites[i] && layerRenderers[i] &&
            layerRenderers[i]->isActive()) {
          // Push sprite opaque (see KNOWN LIMITATION above)
          layerSprites[i]->pushSprite(rect.x, rect.y, rect.x, rect.y, rect.w,
                                      rect.h);
        }
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

  // 🔒 v2.18.1: Check memory availability (PSRAM or heap)
  size_t spriteSize = SCREEN_WIDTH * SCREEN_HEIGHT * 2;
  bool usePsram = psramFound();

  if (usePsram) {
    if (ESP.getFreePsram() < spriteSize) {
      Logger::errorf(
          "HudCompositor: Insufficient PSRAM for shadow sprite (need %u bytes, "
          "have %u bytes)",
          spriteSize, ESP.getFreePsram());
      return false;
    }
  } else {
    Logger::warn(
        "HudCompositor: PSRAM not available - using heap for shadow sprite");
    uint32_t freeHeap = ESP.getFreeHeap();
    if (freeHeap < spriteSize + HEAP_SAFETY_MARGIN_BYTES) {
      Logger::errorf(
          "HudCompositor: Insufficient heap for shadow sprite (need %u KB, "
          "have %u KB, margin %u KB)",
          spriteSize / 1024, freeHeap / 1024, HEAP_SAFETY_MARGIN_BYTES / 1024);
      return false;
    }
  }

  // Create new shadow sprite
  shadowSprite = new (std::nothrow) TFT_eSprite(tft);
  if (!shadowSprite) {
    BootGuard::setResetMarker(BootGuard::RESET_MARKER_NULL_POINTER);
    Logger::error("HudCompositor: Failed to allocate shadow sprite");
    return false;
  }

  // 🔒 CRITICAL: Force shadow sprite to PSRAM to avoid heap pressure/bootloops
  // 🔒 v2.18.1: Only if PSRAM available
  if (usePsram) {
    shadowSprite->setAttribute(PSRAM_ENABLE, 1);
  } else {
    Logger::warn("  Shadow sprite will use heap - monitor memory usage");
  }

  // Create sprite buffer (16-bit color)
  void *spriteBuffer = shadowSprite->createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);
  if (!spriteBuffer) {
    // 🔒 N16R8 CRITICAL: PSRAM allocation failed - diagnostic output
    Serial.printf("[HudCompositor] PSRAM FAIL Shadow Sprite - buffer is NULL\n");
    Serial.printf("  PSRAM found: %s\n", psramFound() ? "YES" : "NO");
    Serial.printf("  Free PSRAM: %u bytes\n", ESP.getFreePsram());
    Serial.printf("  Free Heap: %u bytes\n", ESP.getFreeHeap());
    BootGuard::setResetMarker(BootGuard::RESET_MARKER_NULL_POINTER);
    Logger::error("HudCompositor: Failed to create shadow sprite buffer");
    delete shadowSprite;
    shadowSprite = nullptr;
    return false;
  }

  // Initialize sprite to transparent/black
  shadowSprite->fillSprite(TFT_BLACK);

  if (usePsram) {
    Logger::infof(
        "HudCompositor: Shadow sprite created (PSRAM remaining: %u bytes)",
        ESP.getFreePsram());
  } else {
    Logger::infof(
        "HudCompositor: Shadow sprite created (Heap remaining: %u bytes)",
        ESP.getFreeHeap());
  }
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
  // - layerSprites[BASE] is the main BASE layer sprite (never modified by
  // compositor)
  // - shadowSprite is the validation sprite (rendered separately)
  // The compositeLayers() function pushes sprites to TFT but doesn't modify
  // them
  int baseIdx = static_cast<int>(HudLayer::Layer::BASE);
  TFT_eSprite *mainSprite = layerSprites[baseIdx];

  if (!mainSprite) {
    Logger::warn(
        "HudCompositor: No BASE sprite available for shadow comparison");
    return;
  }

  uint32_t mismatchBlocks = 0;
  bool firstMismatch = true;
  int firstMismatchX = -1;
  int firstMismatchY = -1;

  // PHASE 8: Compare only blocks that intersect with dirty rectangles
  for (int by = 0; by < SHADOW_BLOCKS_Y; by++) {
    for (int bx = 0; bx < SHADOW_BLOCKS_X; bx++) {
      // Calculate block bounds in pixels
      int blockPixelX = bx * SHADOW_BLOCK_SIZE;
      int blockPixelY = by * SHADOW_BLOCK_SIZE;
      int blockPixelW = SHADOW_BLOCK_SIZE;
      int blockPixelH = SHADOW_BLOCK_SIZE;

      // PHASE 8: Check if this block intersects any dirty rectangle
      bool blockIsDirty = false;
      for (int r = 0; r < dirtyRectCount; r++) {
        const HudLayer::DirtyRect &rect = dirtyRects[r];
        // Check for intersection
        if (!(blockPixelX >= rect.x + rect.w ||
              blockPixelX + blockPixelW <= rect.x ||
              blockPixelY >= rect.y + rect.h ||
              blockPixelY + blockPixelH <= rect.y)) {
          blockIsDirty = true;
          break;
        }
      }

      // Only compare blocks that were rendered (dirty)
      if (!blockIsDirty) { continue; }

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
    Logger::errorf("HUD SHADOW MISMATCH: Frame %u, %u blocks differ, first at "
                   "block(%d,%d) px(%d,%d)",
                   shadowFrameCount, mismatchBlocks, firstMismatchX,
                   firstMismatchY, firstMismatchX * SHADOW_BLOCK_SIZE,
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

// ============================================================================
// PHASE 8: Dirty Rectangle Management
// ============================================================================

void HudCompositor::addDirtyRect(int16_t x, int16_t y, int16_t w, int16_t h) {
  // Create dirty rect
  HudLayer::DirtyRect rect(x, y, w, h);

  // Skip empty rectangles
  if (rect.isEmpty()) { return; }

  // Clip to screen bounds
  rect = clipRect(rect);
  if (rect.isEmpty()) {
    return; // Completely outside screen
  }

  // If we've reached the limit, merge all existing rects into a full-screen
  // rect
  if (dirtyRectCount >= MAX_DIRTY_RECTS) {
    dirtyRects[0] = HudLayer::DirtyRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    dirtyRectCount = 1;
    return;
  }

  // Check if new rect overlaps with any existing rect
  bool merged = false;
  for (int i = 0; i < dirtyRectCount; i++) {
    if (dirtyRects[i].overlaps(rect)) {
      // Merge with existing rect
      dirtyRects[i] = dirtyRects[i].merge(rect);
      merged = true;
      break;
    }
  }

  if (!merged) {
    // Add as new rect
    dirtyRects[dirtyRectCount++] = rect;
  }

  // After adding/merging, try to merge overlapping rects to keep count low
  mergeDirtyRects();
}

void HudCompositor::mergeDirtyRects() {
  // Simple merge pass: check all pairs and merge overlapping ones
  bool didMerge;
  do {
    didMerge = false;
    for (int i = 0; i < dirtyRectCount && !didMerge; i++) {
      for (int j = i + 1; j < dirtyRectCount; j++) {
        if (dirtyRects[i].overlaps(dirtyRects[j])) {
          // Merge j into i
          dirtyRects[i] = dirtyRects[i].merge(dirtyRects[j]);

          // Remove j by shifting remaining rects
          for (int k = j; k < dirtyRectCount - 1; k++) {
            dirtyRects[k] = dirtyRects[k + 1];
          }
          dirtyRectCount--;
          didMerge = true;
          break;
        }
      }
    }
  } while (didMerge && dirtyRectCount > 1);
}

HudLayer::DirtyRect HudCompositor::clipRect(const HudLayer::DirtyRect &rect) {
  int16_t x1 = rect.x;
  int16_t y1 = rect.y;
  int16_t x2 = rect.x + rect.w;
  int16_t y2 = rect.y + rect.h;

  // Clip to screen bounds
  if (x1 < 0) x1 = 0;
  if (y1 < 0) y1 = 0;
  if (x2 > SCREEN_WIDTH) x2 = SCREEN_WIDTH;
  if (y2 > SCREEN_HEIGHT) y2 = SCREEN_HEIGHT;

  // Check if completely clipped
  if (x1 >= x2 || y1 >= y2) { return HudLayer::DirtyRect(0, 0, 0, 0); }

  return HudLayer::DirtyRect(x1, y1, x2 - x1, y2 - y1);
}

void HudCompositor::clearDirtyRects() { dirtyRectCount = 0; }

// ============================================================================
// PHASE 9: Render Statistics
// ============================================================================

const HudCompositor::RenderStats &HudCompositor::getRenderStats() {
  return renderStats;
}
