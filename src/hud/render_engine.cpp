#include "render_engine.h"
#include "logger.h"
#include <Arduino.h>

namespace RenderEngine {

// Sprite data
struct SpriteData {
  TFT_eSprite *sprite;
  int16_t x, y, w, h;
  bool dirty;
  bool created;
};

static TFT_eSPI *tft = nullptr;
static SpriteData sprites[SPRITE_COUNT];
static bool initialized = false;

void init(TFT_eSPI *display) {
  if (display == nullptr) {
    Logger::error("RenderEngine: null display");
    return;
  }

  tft = display;

  // Initialize sprite slots
  for (int i = 0; i < SPRITE_COUNT; i++) {
    sprites[i].sprite = nullptr;
    sprites[i].x = sprites[i].y = 0;
    sprites[i].w = sprites[i].h = 0;
    sprites[i].dirty = false;
    sprites[i].created = false;
  }

  initialized = true;
  Logger::info("RenderEngine: Initialized (minimal sprite mode)");

#ifdef BOARD_HAS_PSRAM
  Logger::infof("RenderEngine: PSRAM available: %d KB free",
                ESP.getFreePsram() / 1024);
#endif
}

bool createSprite(SpriteID id, int16_t x, int16_t y, int16_t w, int16_t h) {
  if (!initialized || id >= SPRITE_COUNT)
    return false;

  SpriteData &s = sprites[id];

  // Delete existing
  if (s.created && s.sprite) {
    delete s.sprite;
    s.sprite = nullptr;
    s.created = false;
  }

  // Create sprite
  s.sprite = new TFT_eSprite(tft);
  if (!s.sprite) {
    Logger::errorf("RenderEngine: Failed to allocate sprite %s",
                   getSpriteName(id));
    return false;
  }

  s.sprite->setColorDepth(16); // RGB565

  // Create buffer in PSRAM
  void *buffer = s.sprite->createSprite(w, h);
  if (!buffer) {
    Logger::errorf("RenderEngine: Failed sprite buffer %s (%dx%d)",
                   getSpriteName(id), w, h);
    delete s.sprite;
    s.sprite = nullptr;
    return false;
  }

  s.x = x;
  s.y = y;
  s.w = w;
  s.h = h;
  s.dirty = true;
  s.created = true;

  Logger::infof("RenderEngine: Created %s at (%d,%d) size %dx%d",
                getSpriteName(id), x, y, w, h);
  return true;
}

TFT_eSprite *getSprite(SpriteID id) {
  if (id >= SPRITE_COUNT || !sprites[id].created)
    return nullptr;
  return sprites[id].sprite;
}

void markSpriteDirty(SpriteID id) {
  if (id < SPRITE_COUNT && sprites[id].created) {
    sprites[id].dirty = true;
  }
}

void markDirtyRect(int16_t x, int16_t y, int16_t w, int16_t h) {
  // For direct-draw elements, this is informational only
  // The element itself must redraw when needed
  // This function is here for future optimization
}

void render() {
  if (!initialized)
    return;

  // Push dirty sprites using DMA
  for (int i = 0; i < SPRITE_COUNT; i++) {
    SpriteData &s = sprites[i];
    if (s.created && s.dirty && s.sprite) {
      s.sprite->pushSprite(s.x, s.y);
      s.dirty = false;
    }
  }
}

const char *getSpriteName(SpriteID id) {
  static const char *names[] = {"CAR_BODY", "STEERING"};
  return (id < SPRITE_COUNT) ? names[id] : "UNKNOWN";
}

void cleanup() {
  for (int i = 0; i < SPRITE_COUNT; i++) {
    if (sprites[i].created && sprites[i].sprite) {
      sprites[i].sprite->deleteSprite();
      delete sprites[i].sprite;
      sprites[i].sprite = nullptr;
      sprites[i].created = false;
    }
  }
  initialized = false;
  Logger::info("RenderEngine: Cleanup complete");
}

} // namespace RenderEngine
