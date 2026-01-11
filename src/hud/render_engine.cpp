#include "render_engine.h"
#include "logger.h"
#include <Arduino.h>

// ====== STATIC STORAGE ======
TFT_eSPI *RenderEngine::tft = nullptr;
#ifdef RENDER_SHADOW_MODE
TFT_eSprite *RenderEngine::sprites[3] = {nullptr, nullptr, nullptr};
#else
TFT_eSprite *RenderEngine::sprites[2] = {nullptr, nullptr};
#endif
bool RenderEngine::initialized = false;

#ifdef RENDER_SHADOW_MODE
int RenderEngine::dirtyX[3];
int RenderEngine::dirtyY[3];
int RenderEngine::dirtyW[3];
int RenderEngine::dirtyH[3];
bool RenderEngine::isDirty[3];

// Shadow rendering statistics (Phase 2 & 4)
uint32_t RenderEngine::shadowComparisonCount = 0;
uint32_t RenderEngine::shadowMismatchCount = 0;
uint32_t RenderEngine::shadowLastMismatch = 0;
uint32_t RenderEngine::shadowMaxMismatch = 0;   // Phase 4
uint64_t RenderEngine::shadowTotalMismatch = 0; // Phase 4

// Safety protection statistics (Phase 5)
uint32_t RenderEngine::shadowClampedRects = 0;  // Dirty rects clamped to bounds
uint32_t RenderEngine::shadowRejectedRects = 0; // Invalid dirty rects rejected
uint32_t RenderEngine::shadowNullSprites = 0; // Null sprite accesses prevented
uint32_t RenderEngine::shadowDMABlocks = 0;   // Invalid DMA transfers blocked

// Shadow ignore regions (Phase 3.6)
static RenderEngine::ShadowMask shadowMasks[8];
static uint8_t shadowMaskCount = 0;
#else
int RenderEngine::dirtyX[2];
int RenderEngine::dirtyY[2];
int RenderEngine::dirtyW[2];
int RenderEngine::dirtyH[2];
bool RenderEngine::isDirty[2];
#endif

// ====== INIT ======
void RenderEngine::init(TFT_eSPI *tftDisplay) {
  if (initialized) return;
  tft = tftDisplay;
  initialized = true;

#ifdef RENDER_SHADOW_MODE
  Logger::info("RenderEngine: Initialized (SHADOW MODE ENABLED)");
#else
  Logger::info("RenderEngine: Initialized");
#endif
}

// ====== CREATE SPRITE ======
bool RenderEngine::createSprite(SpriteID id, int w, int h) {
  if (!initialized) return false;

#ifdef RENDER_SHADOW_MODE
  // Validate sprite ID for shadow mode (0-2)
  if (id < 0 || id > 2) {
    Logger::errorf("RenderEngine: Invalid sprite ID %d (shadow mode)", id);
    return false;
  }
#else
  // Validate sprite ID for normal mode (0-1)
  if (id < 0 || id > 1) {
    Logger::errorf("RenderEngine: Invalid sprite ID %d", id);
    return false;
  }
#endif

  if (sprites[id]) {
    sprites[id]->deleteSprite();
    delete sprites[id];
  }

  sprites[id] = new TFT_eSprite(tft);

  // Force PSRAM + DMA
  sprites[id]->setColorDepth(16);
  sprites[id]->setSwapBytes(true);

  if (!sprites[id]->createSprite(w, h)) {
    Logger::errorf("RenderEngine: PSRAM sprite %d allocation failed", id);
    delete sprites[id];
    sprites[id] = nullptr;
    return false;
  }

  sprites[id]->fillSprite(TFT_BLACK);

  // Note: TFT_eSprite does not support setTransparentColor() method.
  // Transparency for steering layers must be handled differently if needed.

  isDirty[id] = true;
  dirtyX[id] = 0;
  dirtyY[id] = 0;
  dirtyW[id] = w;
  dirtyH[id] = h;

#ifdef RENDER_SHADOW_MODE
  if (id == STEERING_SHADOW) {
    Logger::infof("RenderEngine: Shadow sprite ready (%dx%d) - VALIDATION ONLY",
                  w, h);
  } else {
    Logger::infof("RenderEngine: Sprite %d ready (%dx%d)", id, w, h);
  }
#else
  Logger::infof("RenderEngine: Sprite %d ready (%dx%d)", id, w, h);
#endif
  return true;
}

// ====== GET ======
TFT_eSprite *RenderEngine::getSprite(SpriteID id) {
  // Phase 5: Add nullptr safety guard
  TFT_eSprite *sprite = sprites[id];

#ifdef RENDER_SHADOW_MODE
  // Track null sprite accesses for safety metrics
  if (sprite == nullptr) {
    shadowNullSprites++;
    Logger::warnf("RenderEngine: getSprite(%d) returned nullptr", (int)id);
  }
#endif

  return sprite;
}

// ====== DIRTY TRACKING ======
void RenderEngine::markDirtyRect(int x, int y, int w, int h) {
  // Phase 5: Bounds clamping to prevent memory corruption
  // Sprite dimensions are 480×320
  const int SPRITE_WIDTH = 480;
  const int SPRITE_HEIGHT = 320;

  // Clamp rectangle to sprite bounds
  int originalX = x, originalY = y, originalW = w, originalH = h;

  // Clamp x and adjust width
  if (x < 0) {
    w += x; // Reduce width by the negative offset
    x = 0;
  }
  if (x >= SPRITE_WIDTH) {
    // Rectangle starts beyond sprite bounds - reject
#ifdef RENDER_SHADOW_MODE
    shadowRejectedRects++;
#endif
    return;
  }

  // Clamp y and adjust height
  if (y < 0) {
    h += y; // Reduce height by the negative offset
    y = 0;
  }
  if (y >= SPRITE_HEIGHT) {
    // Rectangle starts beyond sprite bounds - reject
#ifdef RENDER_SHADOW_MODE
    shadowRejectedRects++;
#endif
    return;
  }

  // Clamp width to not exceed right edge
  if (x + w > SPRITE_WIDTH) { w = SPRITE_WIDTH - x; }

  // Clamp height to not exceed bottom edge
  if (y + h > SPRITE_HEIGHT) { h = SPRITE_HEIGHT - y; }

  // Reject if invalid dimensions
  if (w <= 0 || h <= 0) {
#ifdef RENDER_SHADOW_MODE
    shadowRejectedRects++;
#endif
    return;
  }

#ifdef RENDER_SHADOW_MODE
  // Track if clamping occurred
  if (x != originalX || y != originalY || w != originalW || h != originalH) {
    shadowClampedRects++;
    Logger::warnf(
        "RenderEngine: Clamped dirty rect (%d,%d,%d,%d) -> (%d,%d,%d,%d)",
        originalX, originalY, originalW, originalH, x, y, w, h);
  }
#endif

  updateDirtyBounds(CAR_BODY, x, y, w, h);
  updateDirtyBounds(STEERING, x, y, w, h);
}

void RenderEngine::updateDirtyBounds(SpriteID id, int x, int y, int w, int h) {
  if (!isDirty[id]) {
    dirtyX[id] = x;
    dirtyY[id] = y;
    dirtyW[id] = w;
    dirtyH[id] = h;
    isDirty[id] = true;
  } else {
    int x1 = min(dirtyX[id], x);
    int y1 = min(dirtyY[id], y);
    int x2 = max(dirtyX[id] + dirtyW[id], x + w);
    int y2 = max(dirtyY[id] + dirtyH[id], y + h);
    dirtyX[id] = x1;
    dirtyY[id] = y1;
    dirtyW[id] = x2 - x1;
    dirtyH[id] = y2 - y1;
  }
}

// ====== RENDER ======
void RenderEngine::render() {
  if (!initialized) return;

  // Phase 5: DMA transfer safety checks
  const int SPRITE_WIDTH = 480;
  const int SPRITE_HEIGHT = 320;

  // Bottom layer (car body)
  if (isDirty[CAR_BODY]) {
    // Phase 5: Validate DMA bounds before transfer
    bool safeToTransfer = true;

    // Check for nullptr
    if (sprites[CAR_BODY] == nullptr) {
      safeToTransfer = false;
#ifdef RENDER_SHADOW_MODE
      shadowDMABlocks++;
      Logger::error("RenderEngine: Blocked CAR_BODY DMA - sprite is nullptr");
#endif
    }

    // Check dirty rectangle bounds
    if (safeToTransfer) {
      if (dirtyX[CAR_BODY] < 0 || dirtyY[CAR_BODY] < 0 ||
          dirtyX[CAR_BODY] + dirtyW[CAR_BODY] > SPRITE_WIDTH ||
          dirtyY[CAR_BODY] + dirtyH[CAR_BODY] > SPRITE_HEIGHT ||
          dirtyW[CAR_BODY] <= 0 || dirtyH[CAR_BODY] <= 0) {
        safeToTransfer = false;
#ifdef RENDER_SHADOW_MODE
        shadowDMABlocks++;
        Logger::errorf(
            "RenderEngine: Blocked CAR_BODY DMA - invalid bounds (%d,%d,%d,%d)",
            dirtyX[CAR_BODY], dirtyY[CAR_BODY], dirtyW[CAR_BODY],
            dirtyH[CAR_BODY]);
#endif
      }
    }

    if (safeToTransfer) {
      sprites[CAR_BODY]->pushImageDMA(dirtyX[CAR_BODY], dirtyY[CAR_BODY],
                                      dirtyW[CAR_BODY], dirtyH[CAR_BODY],
                                      (uint16_t *)sprites[CAR_BODY]->getPointer());
    }

    isDirty[CAR_BODY] = false;
  }

  // Top layer (steering wheel – transparent)
  if (isDirty[STEERING]) {
    // Phase 5: Validate DMA bounds before transfer
    bool safeToTransfer = true;

    // Check for nullptr
    if (sprites[STEERING] == nullptr) {
      safeToTransfer = false;
#ifdef RENDER_SHADOW_MODE
      shadowDMABlocks++;
      Logger::error("RenderEngine: Blocked STEERING DMA - sprite is nullptr");
#endif
    }

    // Check dirty rectangle bounds
    if (safeToTransfer) {
      if (dirtyX[STEERING] < 0 || dirtyY[STEERING] < 0 ||
          dirtyX[STEERING] + dirtyW[STEERING] > SPRITE_WIDTH ||
          dirtyY[STEERING] + dirtyH[STEERING] > SPRITE_HEIGHT ||
          dirtyW[STEERING] <= 0 || dirtyH[STEERING] <= 0) {
        safeToTransfer = false;
#ifdef RENDER_SHADOW_MODE
        shadowDMABlocks++;
        Logger::errorf(
            "RenderEngine: Blocked STEERING DMA - invalid bounds (%d,%d,%d,%d)",
            dirtyX[STEERING], dirtyY[STEERING], dirtyW[STEERING],
            dirtyH[STEERING]);
#endif
      }
    }

    if (safeToTransfer) {
      sprites[STEERING]->pushImageDMA(dirtyX[STEERING], dirtyY[STEERING],
                                      dirtyW[STEERING], dirtyH[STEERING],
                                      (uint16_t *)sprites[STEERING]->getPointer());
    }

    isDirty[STEERING] = false;
  }
}

// ====== CLEAR ======
void RenderEngine::clear() {
#ifdef RENDER_SHADOW_MODE
  for (int i = 0; i < 3; i++) {
#else
  for (int i = 0; i < 2; i++) {
#endif
    if (sprites[i]) sprites[i]->fillSprite(TFT_BLACK);
    isDirty[i] = true;
    dirtyX[i] = 0;
    dirtyY[i] = 0;
    dirtyW[i] = 480;
    dirtyH[i] = 320;
  }
}

bool RenderEngine::isInitialized() { return initialized; }

#ifdef RENDER_SHADOW_MODE
// ====== SHADOW SPRITE COMPARISON ======
uint32_t RenderEngine::compareShadowSprites() {
  if (!initialized) return 0;
  if (!sprites[STEERING] || !sprites[STEERING_SHADOW]) {
    Logger::warn("RenderEngine: Cannot compare - sprites not allocated");
    return 0;
  }

  shadowComparisonCount++;
  uint32_t mismatchCount = 0;
  uint32_t ignoredPixels = 0; // Phase 3.6: Track skipped pixels

  // Get sprite dimensions
  int width = 480;
  int height = 320;

  // Compare pixel by pixel
  // Phase 3.6: Skip pixels in ignore regions
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      // Phase 3.6: Check if this pixel should be ignored
      if (isShadowIgnored(x, y)) {
        ignoredPixels++;
        continue;
      }

      uint16_t pixelReal = sprites[STEERING]->readPixel(x, y);
      uint16_t pixelShadow = sprites[STEERING_SHADOW]->readPixel(x, y);

      if (pixelReal != pixelShadow) { mismatchCount++; }
    }
  }

  shadowLastMismatch = mismatchCount;

  // Phase 4: Update enhanced statistics
  shadowTotalMismatch += mismatchCount;
  if (mismatchCount > shadowMaxMismatch) { shadowMaxMismatch = mismatchCount; }

  if (mismatchCount > 0) {
    shadowMismatchCount++;

    // Log only if mismatch count is significant (>100 pixels)
    // to avoid spam from minor antialiasing differences
    if (mismatchCount > 100) {
      Logger::warnf("RenderEngine: Shadow mismatch detected: %u pixels differ "
                    "(%u ignored)",
                    mismatchCount, ignoredPixels);
    }
  }

  return mismatchCount;
}

void RenderEngine::getShadowStats(uint32_t &outTotalComparisons,
                                  uint32_t &outTotalMismatches,
                                  uint32_t &outLastMismatchCount) {
  outTotalComparisons = shadowComparisonCount;
  outTotalMismatches = shadowMismatchCount;
  outLastMismatchCount = shadowLastMismatch;
}

// Phase 4: Get detailed comparison metrics
void RenderEngine::getShadowMetrics(float &outMatchPercentage,
                                    uint32_t &outMaxMismatch,
                                    float &outAvgMismatch) {
  const uint32_t totalPixels = 480 * 320; // 153,600 pixels

  // Calculate match percentage based on last comparison
  if (totalPixels > 0) {
    uint32_t matchingPixels = totalPixels - shadowLastMismatch;
    outMatchPercentage = (float)matchingPixels * 100.0f / (float)totalPixels;
  } else {
    outMatchPercentage = 0.0f;
  }

  outMaxMismatch = shadowMaxMismatch;

  // Calculate average mismatch
  if (shadowComparisonCount > 0) {
    outAvgMismatch = (float)shadowTotalMismatch / (float)shadowComparisonCount;
  } else {
    outAvgMismatch = 0.0f;
  }
}

// Phase 5: Get safety protection statistics
void RenderEngine::getSafetyStats(uint32_t &outClampedRects,
                                  uint32_t &outRejectedRects,
                                  uint32_t &outNullSprites,
                                  uint32_t &outDMABlocks) {
  outClampedRects = shadowClampedRects;
  outRejectedRects = shadowRejectedRects;
  outNullSprites = shadowNullSprites;
  outDMABlocks = shadowDMABlocks;
}

// Phase 3.6: Shadow ignore region management
void RenderEngine::clearShadowIgnoreRegions() { shadowMaskCount = 0; }

void RenderEngine::addShadowIgnoreRegion(uint16_t x, uint16_t y, uint16_t w,
                                         uint16_t h) {
  if (shadowMaskCount < 8) {
    shadowMasks[shadowMaskCount].x = x;
    shadowMasks[shadowMaskCount].y = y;
    shadowMasks[shadowMaskCount].w = w;
    shadowMasks[shadowMaskCount].h = h;
    shadowMaskCount++;
  }
}

bool RenderEngine::isShadowIgnored(uint16_t x, uint16_t y) {
  for (uint8_t i = 0; i < shadowMaskCount; i++) {
    const ShadowMask &m = shadowMasks[i];
    if (x >= m.x && x < m.x + m.w && y >= m.y && y < m.y + m.h) { return true; }
  }
  return false;
}
#endif