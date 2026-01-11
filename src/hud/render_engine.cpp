#include "render_engine.h"
#include "logger.h"
#include <Arduino.h>

// ====== CONSTANTS ======
static constexpr int SPRITE_WIDTH = 480;
static constexpr int SPRITE_HEIGHT = 320;
static constexpr uint32_t SPRITE_TOTAL_PIXELS = SPRITE_WIDTH * SPRITE_HEIGHT;

#ifdef RENDER_SHADOW_MODE
static constexpr uint32_t SHADOW_MISMATCH_LOG_THRESHOLD = 100;
#endif

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

uint32_t RenderEngine::shadowComparisonCount = 0;
uint32_t RenderEngine::shadowMismatchCount = 0;
uint32_t RenderEngine::shadowLastMismatch = 0;
uint32_t RenderEngine::shadowMaxMismatch = 0;
uint64_t RenderEngine::shadowTotalMismatch = 0;

uint32_t RenderEngine::shadowClampedRects = 0;
uint32_t RenderEngine::shadowRejectedRects = 0;
uint32_t RenderEngine::shadowNullSprites = 0;
uint32_t RenderEngine::shadowDMABlocks = 0;

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
  if (id < 0 || id > 2) {
#else
  if (id < 0 || id > 1) {
#endif
    Logger::errorf("RenderEngine: Invalid sprite ID %d", id);
    return false;
  }

  if (sprites[id]) {
    sprites[id]->deleteSprite();
    delete sprites[id];
  }

  sprites[id] = new TFT_eSprite(tft);
  sprites[id]->setColorDepth(16);
  sprites[id]->setSwapBytes(true);

  if (!sprites[id]->createSprite(w, h)) {
    Logger::errorf("RenderEngine: Sprite %d allocation failed", id);
    delete sprites[id];
    sprites[id] = nullptr;
    return false;
  }

  sprites[id]->fillSprite(TFT_BLACK);

  isDirty[id] = true;
  dirtyX[id] = 0;
  dirtyY[id] = 0;
  dirtyW[id] = w;
  dirtyH[id] = h;

  Logger::infof("RenderEngine: Sprite %d ready (%dx%d)", id, w, h);
  return true;
}

// ====== GET ======
TFT_eSprite *RenderEngine::getSprite(SpriteID id) {
  TFT_eSprite *s = sprites[id];
#ifdef RENDER_SHADOW_MODE
  if (!s) {
    shadowNullSprites++;
    Logger::warnf("RenderEngine: getSprite(%d) returned nullptr", (int)id);
  }
#endif
  return s;
}

// ====== DIRTY ======
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

void RenderEngine::markDirtyRect(int x, int y, int w, int h) {
  updateDirtyBounds(CAR_BODY, x, y, w, h);
  updateDirtyBounds(STEERING, x, y, w, h);
}

// ====== RENDER ======
void RenderEngine::render() {
  if (!initialized) return;

  // Push dirty regions to display
  // Note: Sprites are full-screen (480x320), so source and target coordinates
  // are identical - we copy region (sx,sy,sw,sh) from sprite to (tx,ty) on
  // display
  if (isDirty[CAR_BODY] && sprites[CAR_BODY]) {
    sprites[CAR_BODY]->pushSprite(
        dirtyX[CAR_BODY], dirtyY[CAR_BODY],  // target position on display
        dirtyX[CAR_BODY], dirtyY[CAR_BODY],  // source position in sprite
        dirtyW[CAR_BODY], dirtyH[CAR_BODY]); // region size
    isDirty[CAR_BODY] = false;
  }

  if (isDirty[STEERING] && sprites[STEERING]) {
    sprites[STEERING]->pushSprite(
        dirtyX[STEERING], dirtyY[STEERING],  // target position on display
        dirtyX[STEERING], dirtyY[STEERING],  // source position in sprite
        dirtyW[STEERING], dirtyH[STEERING]); // region size
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
    dirtyW[i] = SPRITE_WIDTH;
    dirtyH[i] = SPRITE_HEIGHT;
  }
}

bool RenderEngine::isInitialized() { return initialized; }

#ifdef RENDER_SHADOW_MODE
uint32_t RenderEngine::compareShadowSprites() {
  if (!sprites[STEERING] || !sprites[STEERING_SHADOW]) return 0;

  shadowComparisonCount++;
  uint32_t mismatches = 0;

  for (int y = 0; y < SPRITE_HEIGHT; y++) {
    for (int x = 0; x < SPRITE_WIDTH; x++) {
      if (isShadowIgnored(x, y)) continue;
      if (sprites[STEERING]->readPixel(x, y) !=
          sprites[STEERING_SHADOW]->readPixel(x, y)) {
        mismatches++;
      }
    }
  }

  shadowLastMismatch = mismatches;
  shadowTotalMismatch += mismatches;
  shadowMaxMismatch = max(shadowMaxMismatch, mismatches);

  if (mismatches > SHADOW_MISMATCH_LOG_THRESHOLD) {
    Logger::warnf("Shadow mismatch: %u pixels", mismatches);
  }

  return mismatches;
}

void RenderEngine::clearShadowIgnoreRegions() { shadowMaskCount = 0; }

void RenderEngine::addShadowIgnoreRegion(uint16_t x, uint16_t y, uint16_t w,
                                         uint16_t h) {
  if (shadowMaskCount < 8) { shadowMasks[shadowMaskCount++] = {x, y, w, h}; }
}

bool RenderEngine::isShadowIgnored(uint16_t x, uint16_t y) {
  for (uint8_t i = 0; i < shadowMaskCount; i++) {
    const ShadowMask &m = shadowMasks[i];
    if (x >= m.x && x < m.x + m.w && y >= m.y && y < m.y + m.h) return true;
  }
  return false;
}
#endif