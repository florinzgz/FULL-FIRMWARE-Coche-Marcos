#include "render_engine.h"
#include "logger.h"

// Static member initialization
TFT_eSPI* RenderEngine::tft = nullptr;
TFT_eSprite* RenderEngine::carBodySprite = nullptr;
TFT_eSprite* RenderEngine::steeringSprite = nullptr;
bool RenderEngine::carBodyDirty = false;
bool RenderEngine::steeringDirty = false;
int RenderEngine::carBodyX = 0;
int RenderEngine::carBodyY = 0;
int RenderEngine::steeringX = 0;
int RenderEngine::steeringY = 0;

void RenderEngine::init(TFT_eSPI* tftInstance) {
  tft = tftInstance;
  Logger::info("RenderEngine: Initialized");
}

void RenderEngine::createSprite(SpriteID id, int x, int y, int w, int h) {
  if (!tft) {
    Logger::error("RenderEngine: TFT not initialized");
    return;
  }

  TFT_eSprite** spritePtr = nullptr;
  int* xPtr = nullptr;
  int* yPtr = nullptr;
  const char* name = nullptr;

  switch (id) {
    case SpriteID::CAR_BODY:
      spritePtr = &carBodySprite;
      xPtr = &carBodyX;
      yPtr = &carBodyY;
      name = "CAR_BODY";
      break;
    case SpriteID::STEERING:
      spritePtr = &steeringSprite;
      xPtr = &steeringX;
      yPtr = &steeringY;
      name = "STEERING";
      break;
  }

  if (*spritePtr) {
    Logger::warnf("RenderEngine: Sprite %s already exists, deleting", name);
    delete *spritePtr;
  }

  *spritePtr = new TFT_eSprite(tft);
  (*spritePtr)->setColorDepth(16);
  
  // Create sprite in PSRAM
  if (!(*spritePtr)->createSprite(w, h)) {
    Logger::errorf("RenderEngine: Failed to create sprite %s (%dx%d)", name, w, h);
    delete *spritePtr;
    *spritePtr = nullptr;
    return;
  }

  *xPtr = x;
  *yPtr = y;

  Logger::infof("RenderEngine: Created sprite %s at (%d,%d) size %dx%d in PSRAM", 
                name, x, y, w, h);
}

TFT_eSprite* RenderEngine::getSprite(SpriteID id) {
  switch (id) {
    case SpriteID::CAR_BODY:
      return carBodySprite;
    case SpriteID::STEERING:
      return steeringSprite;
    default:
      return nullptr;
  }
}

void RenderEngine::markDirty(SpriteID id) {
  switch (id) {
    case SpriteID::CAR_BODY:
      carBodyDirty = true;
      break;
    case SpriteID::STEERING:
      steeringDirty = true;
      break;
  }
}

void RenderEngine::render() {
  if (carBodyDirty && carBodySprite) {
    carBodySprite->pushSprite(carBodyX, carBodyY);
    carBodyDirty = false;
  }

  if (steeringDirty && steeringSprite) {
    steeringSprite->pushSprite(steeringX, steeringY);
    steeringDirty = false;
  }
}

void RenderEngine::cleanup() {
  if (carBodySprite) {
    carBodySprite->deleteSprite();
    delete carBodySprite;
    carBodySprite = nullptr;
  }
  if (steeringSprite) {
    steeringSprite->deleteSprite();
    delete steeringSprite;
    steeringSprite = nullptr;
  }
  Logger::info("RenderEngine: Cleaned up");
}
