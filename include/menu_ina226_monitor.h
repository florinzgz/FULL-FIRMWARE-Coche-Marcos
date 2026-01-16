#ifndef MENU_INA226_MONITOR_H
#define MENU_INA226_MONITOR_H

#include "current.h"
#include <Arduino.h>
#include <TFT_eSPI.h>

// Forward declaration
namespace HudLayer {
struct RenderContext;
}

/**
 * INA226 Real-time Monitor Menu
 *
 * Displays live current and voltage readings from all 6 INA226 sensors:
 * - Motor FL/FR/RL/RR (4 traction motors)
 * - Battery (main power monitoring)
 * - Steering Motor (RS390 12V)
 *
 * Features:
 * - 10Hz update rate for smooth real-time display
 * - Color-coded current values (blue→yellow→red based on load)
 * - Status indicators (green=OK, yellow=warning, red=overcurrent, grey=offline)
 * - Live statistics: total current, peak, average, battery voltage
 * - Reset stats functionality
 * - Compatible with I2C Recovery system (auto-skip offline sensors)
 *
 * UI Layout:
 * - 6 sensor cards in 2x3 grid
 * - Statistics panel at bottom
 * - Reset Stats and Back buttons
 */

class MenuINA226Monitor {
public:
  static void init(TFT_eSPI *tft);
  static void draw();
  static void handleTouch(uint16_t x, uint16_t y);
  static void update(); // Call from loop() for live updates

private:
  static TFT_eSPI *_tft;
  static unsigned long _lastUpdate;
  static const uint16_t UPDATE_INTERVAL = 100; // 10Hz

  // Statistics tracking
  static float _peakCurrent;
  static float _totalCurrentSum;
  static uint32_t _sampleCount;

  // Sensor data cache
  struct SensorData {
    float current;
    float voltage;
    bool online;
    uint8_t state; // 0=OK, 1=BACKOFF, 2=OFFLINE
  };

  static SensorData _sensors[6];

  // UI positions
  static const uint16_t CARD_WIDTH = 155;
  static const uint16_t CARD_HEIGHT = 100;
  static const uint16_t CARD_MARGIN = 5;
  static const uint16_t STATS_Y = 215;
  static const uint16_t BTN_Y = 285;

  // Helper methods
  static void drawSensorCard(uint8_t index, uint16_t x, uint16_t y,
                             const HudLayer::RenderContext &ctx);
  static void drawStatistics();
  static void drawButtons();
  static void updateSensorData();
  static uint16_t getCurrentColor(float current);
  static const char *getSensorName(uint8_t index);
  static void resetStatistics();
};

#endif // MENU_INA226_MONITOR_H
