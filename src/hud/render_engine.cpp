#include "render_engine.h"
#include "logger.h"
#include <Arduino.h>

// ====== STATIC STORAGE ======
TFT_eSPI *RenderEngine::tft = nullptr;
TFT_eSprite *RenderEngine::sprites[2] = {nullptr, nullptr};
bool RenderEngine::initialized = false;

int RenderEngine::dirtyX[2];
int RenderEngine::dirtyY[2];
int RenderEngine::dirtyW[2];
int RenderEngine::dirtyH[2];
bool RenderEngine::isDirty[2];

// ====== INIT ======
void RenderEngine::init(TFT_eSPI *tftDisplay) {
  if (initialized) return;
  tft = tftDisplay;
  initialized = true;

  Logger::info("RenderEngine: Initialized");
}

// ====== CREATE SPRITE ======
bool RenderEngine::createSprite(SpriteID id, int w, int h) {
  if (!initialized) return false;

  if (sprites[id]) {
    sprites[id]->deleteSprite();
    delete sprites[id];
  }

  sprites[id] = new TFT_eSprite(tft);

  // Force PSRAM + DMA
  sprites[id]->setColorDepth(16);
  sprites[id]->setSwapBytes(true);

  if (!sprites[id]->createSprite(w, h)) {
    Logger::error("RenderEngine: PSRAM sprite allocation failed");
    delete sprites[id];
    sprites[id] = nullptr;
    return false;
  }

  sprites[id]->fillSprite(TFT_BLACK);

  // Steering layer must be transparent
  if (id == STEERING) {
    sprites[id]->setTransparentColor(TFT_BLACK);
  }

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
  return sprites[id];
}

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
    sprites[CAR_BODY]->pushImageDMA(
      dirtyX[CAR_BODY], dirtyY[CAR_BODY],
      dirtyW[CAR_BODY], dirtyH[CAR_BODY],
      (uint16_t*)sprites[CAR_BODY]->getPointer(dirtyX[CAR_BODY], dirtyY[CAR_BODY]),
      480
    );
    isDirty[CAR_BODY] = false;
  }

  // Top layer (steering wheel – transparent)
  if (isDirty[STEERING]) {
    sprites[STEERING]->pushImageDMA(
      dirtyX[STEERING], dirtyY[STEERING],
      dirtyW[STEERING], dirtyH[STEERING],
      (uint16_t*)sprites[STEERING]->getPointer(dirtyX[STEERING], dirtyY[STEERING]),
      480
    );
    isDirty[STEERING] = false;
  }
}

// ====== CLEAR ======
void RenderEngine::clear() {
  for (int i = 0; i < 2; i++) {
    if (sprites[i]) sprites[i]->fillSprite(TFT_BLACK);
    isDirty[i] = true;
    dirtyX[i] = 0;
    dirtyY[i] = 0;
    dirtyW[i] = 480;
    dirtyH[i] = 320;
  }
}

bool RenderEngine::isInitialized() {
  return initialized;
}