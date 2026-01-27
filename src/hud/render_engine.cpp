#include "render_engine.h"
#include "boot_guard.h"
#include "logger.h"
#include <Arduino.h>
#include <new>

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

  uint32_t expectedSize = w * h * 2; // 16-bit color = 2 bytes per pixel
  
  // 🔒 v2.18.1: Allow operation without PSRAM for hardware compatibility
  // Some hardware batches have bootloop issues with OPI PSRAM enabled
  // System must be able to operate using internal SRAM only
  bool usePsram = psramFound();
  
  if (!usePsram) {
    Logger::warnf("RenderEngine: PSRAM not detected - using heap for sprite %d "
                   "(may impact performance)", id);
    Logger::warnf("  Sprite size: %dx%d = %u KB", w, h, expectedSize / 1024);
    
    // Check if we have enough heap
    uint32_t freeHeap = ESP.getFreeHeap();
    if (freeHeap < expectedSize + 50000) { // Keep 50KB margin
      Logger::errorf("RenderEngine: Insufficient heap for sprite %d (need %u KB, "
                     "have %u KB, margin 50KB)",
                     id, expectedSize / 1024, freeHeap / 1024);
      return false;
    }
    Logger::infof("  Using heap (free: %u KB, need: %u KB)", freeHeap / 1024,
                  expectedSize / 1024);
  } else {
    uint32_t freePsram = ESP.getFreePsram();
    if (freePsram < expectedSize) {
      Logger::errorf("RenderEngine: Insufficient PSRAM for sprite %d (need %u "
                     "bytes, have %u bytes)",
                     id, expectedSize, freePsram);
      return false;
    }
  }

  if (sprites[id]) {
    sprites[id]->deleteSprite();
    delete sprites[id];
  }

  sprites[id] = new (std::nothrow) TFT_eSprite(tft);
  if (!sprites[id]) {
    BootGuard::setResetMarker(BootGuard::RESET_MARKER_NULL_POINTER);
    Logger::errorf("RenderEngine: Sprite %d allocation failed (nullptr)", id);
    return false;
  }

  // 🔒 CRITICAL FIX: Force sprite buffers to PSRAM to prevent heap corruption
  // Large full-screen sprites (480×320×16bit = ~300KB each) should use PSRAM
  // to avoid heap fragmentation causing "Stack canary watchpoint triggered (ipc0)"
  // 🔒 v2.18.1: Only enable PSRAM if available (some hardware has bootloop with OPI)
  if (usePsram) {
    sprites[id]->setAttribute(PSRAM_ENABLE, 1);
  } else {
    // Without PSRAM, sprite will use heap - this may cause memory pressure
    Logger::warn("  Sprite will use heap (no PSRAM) - monitor memory usage");
  }

  sprites[id]->setColorDepth(16);
  sprites[id]->setSwapBytes(true);

  // 🔍 VERIFICATION: Log memory state before sprite allocation
  uint32_t heapBefore = ESP.getFreeHeap();
  uint32_t psramBefore = usePsram ? ESP.getFreePsram() : 0;
  Logger::infof("RenderEngine: Creating sprite %d (%dx%d, ~%u KB)", id, w, h,
                expectedSize / 1024);
  if (usePsram) {
    Logger::infof("  Before: Heap=%u KB, PSRAM=%u KB", heapBefore / 1024,
                  psramBefore / 1024);
  } else {
    Logger::infof("  Before: Heap=%u KB (no PSRAM)", heapBefore / 1024);
  }

  if (!sprites[id]->createSprite(w, h)) {
    Logger::errorf("RenderEngine: Sprite %d allocation failed", id);
    delete sprites[id];
    sprites[id] = nullptr;
    return false;
  }

  // 🔍 VERIFICATION: Log memory state after sprite allocation
  uint32_t heapAfter = ESP.getFreeHeap();
  uint32_t psramAfter = usePsram ? ESP.getFreePsram() : 0;
  int32_t heapDelta = (int32_t)heapBefore - (int32_t)heapAfter;
  int32_t psramDelta = usePsram ? ((int32_t)psramBefore - (int32_t)psramAfter) : 0;

  if (usePsram) {
    Logger::infof("  After:  Heap=%u KB, PSRAM=%u KB", heapAfter / 1024,
                  psramAfter / 1024);
    Logger::infof("  Delta:  Heap=%d KB, PSRAM=%d KB", heapDelta / 1024,
                  psramDelta / 1024);
  } else {
    Logger::infof("  After:  Heap=%u KB (no PSRAM)", heapAfter / 1024);
    Logger::infof("  Delta:  Heap=%d KB", heapDelta / 1024);
  }

  // 🔍 VERIFICATION: Check sprite attributes
  if (usePsram) {
    uint8_t psramAttr = sprites[id]->getAttribute(PSRAM_ENABLE);
    Logger::infof("  PSRAM_ENABLE attribute: %u", psramAttr);
  }

  // 🔍 VERIFICATION: Validate allocation location
  if (usePsram && psramDelta < (int32_t)(expectedSize * 0.9)) {
    Logger::errorf("  ⚠️  WARNING: Sprite %d may NOT be in PSRAM!", id);
    Logger::errorf("  Expected PSRAM delta ~%u KB, got %d KB",
                   expectedSize / 1024, psramDelta / 1024);
  } else if (usePsram) {
    Logger::infof("  ✅ Sprite %d confirmed in PSRAM (%d KB allocated)", id,
                  psramDelta / 1024);
  } else if (heapDelta >= (int32_t)(expectedSize * 0.9)) {
    Logger::infof("  ✅ Sprite %d allocated in heap (%d KB)", id,
                  heapDelta / 1024);
  }

  if (heapDelta > (int32_t)(expectedSize / 10)) {
    Logger::warnf("  ⚠️  Unexpected heap usage: %d KB (expected < %u KB)",
                  heapDelta / 1024, expectedSize / 1024 / 10);
  }

  sprites[id]->fillSprite(TFT_BLACK);

  isDirty[id] = true;
  dirtyX[id] = 0;
  dirtyY[id] = 0;
  dirtyW[id] = w;
  dirtyH[id] = h;

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
