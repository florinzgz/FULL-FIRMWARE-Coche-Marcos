#pragma once

#include <TFT_eSPI.h>

// Minimal rendering engine for sprite-based HUD elements
// Only for static/semi-static elements (car body, steering wheel)
// Everything else uses direct draw with dirty rect tracking
namespace RenderEngine {

// Sprite identifiers
enum SpriteID : uint8_t {
  SPRITE_CAR_BODY = 0,      // Static car visualization
  SPRITE_STEERING = 1,       // Steering wheel overlay
  SPRITE_COUNT = 2
};

// Dirty rectangle for direct-draw elements
struct DirtyRect {
  int16_t x, y, w, h;
  bool dirty;
};

// Initialize rendering engine
void init(TFT_eSPI *display);

// Create sprite with PSRAM allocation
bool createSprite(SpriteID id, int16_t x, int16_t y, int16_t w, int16_t h);

// Get sprite for drawing
TFT_eSprite *getSprite(SpriteID id);

// Mark sprite as dirty (needs push to screen)
void markSpriteDirty(SpriteID id);

// Mark screen region as dirty (for direct draws)
void markDirtyRect(int16_t x, int16_t y, int16_t w, int16_t h);

// Render all dirty sprites using DMA
void render();

// Get sprite name for debugging
const char *getSpriteName(SpriteID id);

// Cleanup
void cleanup();

} // namespace RenderEngine
