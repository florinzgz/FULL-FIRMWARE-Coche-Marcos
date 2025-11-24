// Obstacle Detection HUD Display
#include "obstacle_display.h"
#include "obstacle_detection.h"
#include "logger.h"
#include <TFT_eSPI.h>

extern TFT_eSPI tft;

namespace ObstacleDisplay {
static DisplayConfig config;
static uint32_t lastUpdateMs = 0;

void init() {
    Logger::info("ObstacleDisplay: Init");
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
    // Draw color-coded proximity indicators for each sensor
    for (uint8_t i = 0; i < 4; i++) {
        auto level = ObstacleDetection::getProximityLevel(i);
        uint16_t color = TFT_GREEN;
        if (level == ObstacleDetection::LEVEL_CRITICAL) color = TFT_RED;
        else if (level == ObstacleDetection::LEVEL_WARNING) color = TFT_YELLOW;
        else if (level == ObstacleDetection::LEVEL_CAUTION) color = TFT_ORANGE;
        
        int x = 240 + (i == 3 ? 60 : i == 1 ? -60 : 0);
        int y = 160 + (i == 0 ? -60 : i == 2 ? 60 : 0);
        tft.fillCircle(x, y, 10, color);
    }
}

void drawDistanceBars() {
    // Draw distance bars at bottom of screen
    for (uint8_t i = 0; i < 4; i++) {
        uint16_t dist = ObstacleDetection::getMinDistance(i);
        if (dist == 0xFFFF) continue;
        
        int barLen = constrain(map(dist, 0, 2000, 0, 100), 0, 100);
        int x = 40 + (i * 110);
        tft.fillRect(x, 300, barLen, 10, TFT_CYAN);
        tft.setCursor(x, 285);
        tft.printf("%dcm", dist / 10);
    }
}

const DisplayConfig& getConfig() { return config; }
void setConfig(const DisplayConfig& c) { config = c; }
}
