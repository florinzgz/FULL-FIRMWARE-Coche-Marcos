#ifndef OBSTACLE_DISPLAY_H
#define OBSTACLE_DISPLAY_H
#include <Arduino.h>
#include "obstacle_detection.h"

namespace ObstacleDisplay {
    struct DisplayConfig {
        bool enabled;
        bool showBars;
        bool showNumeric;
        uint8_t updateRate;  // Hz
        DisplayConfig() : enabled(true), showBars(true), showNumeric(true), updateRate(30) {}
    };
    
    void init();
    void update();
    void setEnabled(bool enable);
    void drawProximityIndicators();
    void drawDistanceBars();
    const DisplayConfig& getConfig();
    void setConfig(const DisplayConfig& config);
}
#endif
