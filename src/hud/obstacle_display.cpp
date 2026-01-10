// Obstacle Detection HUD Display
// v2.12.0: Updated for single TOFSense-M S sensor (front only)
// v2.20.0: RenderEngine sprite layer integration (STEERING layer)
#include "obstacle_display.h"
#include "logger.h"
#include "obstacle_config.h"
#include "obstacle_detection.h"
#include "render_engine.h"
#include <TFT_eSPI.h>

namespace ObstacleDisplay {

static DisplayConfig config;
static uint32_t lastUpdateMs = 0;

// Bounding box for obstacle HUD area
static constexpr int HUD_X = 0;
static constexpr int HUD_Y = 60;
static constexpr int HUD_W = 480;
static constexpr int HUD_H = 260;

static TFT_eSprite *getSprite() {
  return RenderEngine::getSprite(RenderEngine::STEERING);
}

void init() {
  Logger::info("ObstacleDisplay: Init (v2.12.0 - Single sensor)");
  config = DisplayConfig();
}

void setEnabled(bool enable) { config.enabled = enable; }

void update() {
  if (!config.enabled) return;

  uint32_t now = millis();
  if (now - lastUpdateMs < (1000 / config.updateRate)) return;
  lastUpdateMs = now;

  auto sprite = getSprite();
  if (!sprite) return;

  if (config.showBars) {
    drawDistanceBars();
    RenderEngine::markDirtyRect(HUD_X, HUD_Y, HUD_W, HUD_H);
  }
}

void drawProximityIndicators() {
  auto sprite = getSprite();
  if (!sprite) return;

  auto level =
      ObstacleDetection::getProximityLevel(ObstacleDetection::SENSOR_FRONT);
  bool healthy = ObstacleDetection::isHealthy(ObstacleDetection::SENSOR_FRONT);

  uint16_t color = TFT_GREEN;
  if (!healthy) {
    color = TFT_DARKGREY;
  } else if (level == ObstacleDetection::LEVEL_CRITICAL) {
    color = TFT_RED;
  } else if (level == ObstacleDetection::LEVEL_WARNING) {
    color = TFT_YELLOW;
  } else if (level == ObstacleDetection::LEVEL_CAUTION) {
    color = TFT_ORANGE;
  }

  int x = 240;
  int y = 100;

  sprite->fillCircle(x, y, 15, color);
  sprite->setTextDatum(TC_DATUM);
  sprite->setTextColor(TFT_WHITE, TFT_BLACK);
  sprite->drawString("FRONT", x, y + 25, 2);

  RenderEngine::markDirtyRect(x - 20, y - 20, 40, 60);
}

void drawDistanceBars() {
  auto sprite = getSprite();
  if (!sprite) return;

  uint16_t dist =
      ObstacleDetection::getMinDistance(ObstacleDetection::SENSOR_FRONT);
  bool healthy = ObstacleDetection::isHealthy(ObstacleDetection::SENSOR_FRONT);

  const int barX = 140;
  const int barY = 300;
  const int barW = 200;
  const int barH = 15;

  // Clear text+bar area first
  sprite->fillRect(0, barY - 40, 480, 60, TFT_BLACK);

  if (!healthy) {
    sprite->setTextDatum(MC_DATUM);
    sprite->setTextColor(TFT_RED, TFT_BLACK);
    sprite->drawString("SENSOR ERROR", 240, barY + 5, 4);
    RenderEngine::markDirtyRect(0, barY - 40, 480, 60);
    return;
  }

  if (dist >= ObstacleConfig::DISTANCE_INVALID) {
    sprite->setTextDatum(MC_DATUM);
    sprite->setTextColor(TFT_GREEN, TFT_BLACK);
    sprite->drawString("CLEAR", 240, barY + 5, 4);
    RenderEngine::markDirtyRect(0, barY - 40, 480, 60);
    return;
  }

  int barLen = constrain(map(dist, 0, 4000, 0, barW), 0, barW);

  uint16_t barColor = TFT_GREEN;
  if (dist < ObstacleConfig::DISTANCE_CRITICAL)
    barColor = TFT_RED;
  else if (dist < ObstacleConfig::DISTANCE_WARNING)
    barColor = TFT_ORANGE;
  else if (dist < ObstacleConfig::DISTANCE_CAUTION)
    barColor = TFT_YELLOW;

  sprite->fillRect(barX, barY, barW, barH, TFT_DARKGREY);
  sprite->fillRect(barX, barY, barLen, barH, barColor);
  sprite->drawRect(barX, barY, barW, barH, TFT_WHITE);

  char distStr[32];
  if (dist < 1000) {
    snprintf(distStr, sizeof(distStr), "%dmm (8x8)", dist);
  } else {
    snprintf(distStr, sizeof(distStr), "%.2fm (8x8)", dist / 1000.0f);
  }

  sprite->setTextDatum(TC_DATUM);
  sprite->setTextColor(TFT_WHITE, TFT_BLACK);
  sprite->drawString(distStr, 240, barY - 20, 4);

  RenderEngine::markDirtyRect(0, barY - 40, 480, 60);
}

const DisplayConfig &getConfig() { return config; }
void setConfig(const DisplayConfig &c) { config = c; }

} // namespace ObstacleDisplay