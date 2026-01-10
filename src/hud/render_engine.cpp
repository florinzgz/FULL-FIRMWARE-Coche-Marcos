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
uint32_t RenderEngine::shadowMaxMismatch = 0;        // Phase 4
uint64_t RenderEngine::shadowTotalMismatch = 0;      // Phase 4
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

  // Steering layers must be transparent (both real and shadow)
#ifdef RENDER_SHADOW_MODE
  if (id == STEERING || id == STEERING_SHADOW) {
    sprites[id]->setTransparentColor(TFT_BLACK);
  }
#else
  if (id == STEERING) {
    sprites[id]->setTransparentColor(TFT_BLACK);
  }
#endif

  isDirty[id] = true;
  dirtyX[id] = 0;
  dirtyY[id] = 0;
  dirtyW[id] = w;
  dirtyH[id] = h;

#ifdef RENDER_SHADOW_MODE
  if (id == STEERING_SHADOW) {
    Logger::infof("RenderEngine: Shadow sprite ready (%dx%d) - VALIDATION ONLY", w, h);
  } else {
    Logger::infof("RenderEngine: Sprite %d ready (%dx%d)", id, w, h);
  }
#else
  Logger::infof("RenderEngine: Sprite %d ready (%dx%d)", id, w, h);
#endif
  return true;
}

// ====== GET ======
TFT_eSprite *RenderEngine::getSprite(SpriteID id) { return sprites[id]; }

// ====== DIRTY TRACKING ======
void RenderEngine::markDirtyRect(int x, int y, int w, int h) {
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

  // Bottom layer (car body)
  if (isDirty[CAR_BODY]) {
    sprites[CAR_BODY]->pushImageDMA(dirtyX[CAR_BODY], dirtyY[CAR_BODY],
                                    dirtyW[CAR_BODY], dirtyH[CAR_BODY],
                                    (uint16_t *)sprites[CAR_BODY]->getPointer(
                                        dirtyX[CAR_BODY], dirtyY[CAR_BODY]),
                                    480);
    isDirty[CAR_BODY] = false;
  }

  // Top layer (steering wheel – transparent)
  if (isDirty[STEERING]) {
    sprites[STEERING]->pushImageDMA(dirtyX[STEERING], dirtyY[STEERING],
                                    dirtyW[STEERING], dirtyH[STEERING],
                                    (uint16_t *)sprites[STEERING]->getPointer(
                                        dirtyX[STEERING], dirtyY[STEERING]),
                                    480);
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

  // Get sprite dimensions
  int width = 480;
  int height = 320;

  // Compare pixel by pixel
  // Note: For performance, we could limit comparison to dirty regions only
  // For now, we compare the full sprite for maximum validation
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      uint16_t pixelReal = sprites[STEERING]->readPixel(x, y);
      uint16_t pixelShadow = sprites[STEERING_SHADOW]->readPixel(x, y);
      
      if (pixelReal != pixelShadow) {
        mismatchCount++;
      }
    }
  }

  shadowLastMismatch = mismatchCount;
  
  // Phase 4: Update enhanced statistics
  shadowTotalMismatch += mismatchCount;
  if (mismatchCount > shadowMaxMismatch) {
    shadowMaxMismatch = mismatchCount;
  }

  if (mismatchCount > 0) {
    shadowMismatchCount++;
    
    // Log only if mismatch count is significant (>100 pixels)
    // to avoid spam from minor antialiasing differences
    if (mismatchCount > 100) {
      Logger::warnf("RenderEngine: Shadow mismatch detected: %u pixels differ", 
                    mismatchCount);
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
  const uint32_t totalPixels = 480 * 320;  // 153,600 pixels
  
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
#endif