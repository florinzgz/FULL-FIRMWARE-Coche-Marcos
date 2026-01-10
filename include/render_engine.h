#pragma once
#include <TFT_eSPI.h>

enum class SpriteID {
  CAR_BODY,
  STEERING
};

class RenderEngine {
public:
  static void init(TFT_eSPI* tftInstance);
  static void createSprite(SpriteID id, int x, int y, int w, int h);
  static TFT_eSprite* getSprite(SpriteID id);
  static void markDirty(SpriteID id);
  static void render();
  static void cleanup();

private:
  static TFT_eSPI* tft;
  static TFT_eSprite* carBodySprite;
  static TFT_eSprite* steeringSprite;
  static bool carBodyDirty;
  static bool steeringDirty;
  static int carBodyX, carBodyY;
  static int steeringX, steeringY;
};
