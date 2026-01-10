#include "render_engine.h"
#include "logger.h"
#include <Arduino.h>

// Static member initialization
TFT_eSPI *RenderEngine::tft = nullptr;
TFT_eSprite *RenderEngine::sprites[2] = {nullptr, nullptr};
bool RenderEngine::initialized = false;

// Dirty rectangle tracking
int RenderEngine::dirtyX[2] = {0, 0};
int RenderEngine::dirtyY[2] = {0, 0};
int RenderEngine::dirtyW[2] = {0, 0};
int RenderEngine::dirtyH[2] = {0, 0};
bool RenderEngine::isDirty[2] = {false, false};

void RenderEngine::init(TFT_eSPI *tftDisplay) {
  if (initialized) {
    Logger::warn("RenderEngine: Already initialized");
    return;
  }

  if (tftDisplay == nullptr) {
    Logger::error("RenderEngine: Invalid TFT display pointer");
    return;
  }

  tft = tftDisplay;
  initialized = true;

  Logger::info("RenderEngine: Initialized successfully");
}

bool RenderEngine::createSprite(SpriteID id, int width, int height) {
  if (!initialized) {
    Logger::error("RenderEngine: Not initialized, cannot create sprite");
    return false;
  }

  if (id < CAR_BODY || id > STEERING) {
    Logger::errorf("RenderEngine: Invalid sprite ID %d", id);
    return false;
  }

  // Delete existing sprite if any
  if (sprites[id] != nullptr) {
    delete sprites[id];
    sprites[id] = nullptr;
  }

  // Create new sprite
  sprites[id] = new TFT_eSprite(tft);
  if (sprites[id] == nullptr) {
    Logger::errorf("RenderEngine: Failed to allocate sprite %d", id);
    return false;
  }

  // Create sprite buffer with 16-bit color depth
  if (!sprites[id]->createSprite(width, height)) {
    Logger::errorf("RenderEngine: Failed to create sprite buffer %d (%dx%d)",
                   id, width, height);
    delete sprites[id];
    sprites[id] = nullptr;
    return false;
  }

  // Initialize sprite to black
  sprites[id]->fillSprite(TFT_BLACK);

  Logger::infof("RenderEngine: Created sprite %d (%dx%d)", id, width, height);
  return true;
}

TFT_eSprite *RenderEngine::getSprite(SpriteID id) {
  if (!initialized) {
    Logger::error("RenderEngine: Not initialized");
    return nullptr;
  }

  if (id < CAR_BODY || id > STEERING) {
    Logger::errorf("RenderEngine: Invalid sprite ID %d", id);
    return nullptr;
  }

  return sprites[id];
}

void RenderEngine::markDirtyRect(int x, int y, int w, int h) {
  if (!initialized) { return; }

  // Mark both sprites as dirty in the specified region
  // (since they're layered, both need to be redrawn)
  updateDirtyBounds(CAR_BODY, x, y, w, h);
  updateDirtyBounds(STEERING, x, y, w, h);
}

void RenderEngine::updateDirtyBounds(SpriteID id, int x, int y, int w, int h) {
  if (sprites[id] == nullptr) { return; }

  if (!isDirty[id]) {
    // First dirty region - set initial bounds
    dirtyX[id] = x;
    dirtyY[id] = y;
    dirtyW[id] = w;
    dirtyH[id] = h;
    isDirty[id] = true;
  } else {
    // Expand existing dirty bounds to include new region
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

void RenderEngine::render() {
  if (!initialized || tft == nullptr) { return; }

  // Render CAR_BODY sprite (bottom layer)
  if (isDirty[CAR_BODY] && sprites[CAR_BODY] != nullptr) {
    // Push only the dirty bounding box to display
    sprites[CAR_BODY]->pushSprite(0, 0);
    isDirty[CAR_BODY] = false;
  }

  // Render STEERING sprite (top layer)
  if (isDirty[STEERING] && sprites[STEERING] != nullptr) {
    // Push only the dirty bounding box to display
    // Use transparent color for the steering sprite so car body shows through
    sprites[STEERING]->pushSprite(0, 0);
    isDirty[STEERING] = false;
  }
}

void RenderEngine::clear() {
  if (!initialized) { return; }

  for (int i = 0; i < 2; i++) {
    if (sprites[i] != nullptr) { sprites[i]->fillSprite(TFT_BLACK); }
    isDirty[i] = false;
    dirtyX[i] = 0;
    dirtyY[i] = 0;
    dirtyW[i] = 0;
    dirtyH[i] = 0;
  }
}

bool RenderEngine::isInitialized() { return initialized; }
