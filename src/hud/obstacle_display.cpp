// Obstacle Detection HUD Display
// v2.12.0: Updated for single TOFSense-M S sensor (front only)
#include "obstacle_display.h"
#include "logger.h"
#include "obstacle_config.h"
#include "obstacle_detection.h"
#include <TFT_eSPI.h>

extern TFT_eSPI tft;

namespace ObstacleDisplay {
static DisplayConfig config;
static uint32_t lastUpdateMs = 0;

void init() {
  Logger::info("ObstacleDisplay: Init (v2.12.0 - Single sensor)");
  config = DisplayConfig();
}

void update() {
  if (!config.enabled) return;
  uint32_t now = millis();
  if (now - lastUpdateMs < (1000 / config.updateRate)) return;
  lastUpdateMs = now;
  if (config.showBars) drawDistanceBars();
}

void setEnabled(bool enable) { config.enabled = enable; }

void drawProximityIndicators() {
  // Draw color-coded proximity indicator for single front sensor
  // v2.12.0: Only one sensor (SENSOR_FRONT)
  auto level =
      ObstacleDetection::getProximityLevel(ObstacleDetection::SENSOR_FRONT);
  bool healthy = ObstacleDetection::isHealthy(ObstacleDetection::SENSOR_FRONT);

  uint16_t color = TFT_GREEN;
  if (!healthy) {
    color = TFT_DARKGREY; // Sensor unhealthy/timeout
  } else if (level == ObstacleDetection::LEVEL_CRITICAL) {
    color = TFT_RED;
  } else if (level == ObstacleDetection::LEVEL_WARNING) {
    color = TFT_YELLOW;
  } else if (level == ObstacleDetection::LEVEL_CAUTION) {
    color = TFT_ORANGE;
  }

  // Draw front sensor indicator at top center
  int x = 240; // Center of screen (480px width)
  int y = 100; // Top area
  tft.fillCircle(x, y, 15, color);

  // Draw label
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("FRONT", x, y + 25, 2);
}

void drawDistanceBars() {
  // Draw distance bar for single front sensor
  // v2.13.0: TOFSense-M S 8x8 matrix mode (minimum of 64 pixels)
  uint16_t dist =
      ObstacleDetection::getMinDistance(ObstacleDetection::SENSOR_FRONT);
  bool healthy = ObstacleDetection::isHealthy(ObstacleDetection::SENSOR_FRONT);

  if (!healthy) {
    // Show sensor error state
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("SENSOR ERROR", 240, 300, 4);
    return;
  }

  if (dist >= ObstacleConfig::DISTANCE_INVALID) {
    // No obstacle detected
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("CLEAR", 240, 300, 4);
    return;
  }

  // Draw distance bar (0-4m range for TOFSense-M S 8x8 matrix)
  int barLen = constrain(map(dist, 0, 4000, 0, 200), 0, 200);
  int x = 140; // Centered
  int barY = 300;

  // Bar color based on proximity
  uint16_t barColor = TFT_GREEN;
  if (dist < ObstacleConfig::DISTANCE_CRITICAL)
    barColor = TFT_RED;
  else if (dist < ObstacleConfig::DISTANCE_WARNING)
    barColor = TFT_ORANGE;
  else if (dist < ObstacleConfig::DISTANCE_CAUTION)
    barColor = TFT_YELLOW;

  // Draw bar background
  tft.fillRect(x, barY, 200, 15, TFT_DARKGREY);
  // Draw filled portion
  tft.fillRect(x, barY, barLen, 15, barColor);
  // Draw border
  tft.drawRect(x, barY, 200, 15, TFT_WHITE);

  // Draw distance text
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  char distStr[32];
  if (dist < 1000) {
    snprintf(distStr, sizeof(distStr), "%dmm (8x8)", dist);
  } else {
    snprintf(distStr, sizeof(distStr), "%.2fm (8x8)", dist / 1000.0f);
  }
  tft.drawString(distStr, 240, barY - 20, 4);
}

const DisplayConfig &getConfig() { return config; }
void setConfig(const DisplayConfig &c) { config = c; }
} // namespace ObstacleDisplay
