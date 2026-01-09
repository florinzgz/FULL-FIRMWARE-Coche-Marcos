#ifndef MENU_POWER_CONFIG_H
#define MENU_POWER_CONFIG_H

#include "config_storage.h"
#include <Arduino.h>

/**
 * Power Management Configuration Menu
 *
 * Features:
 * - Adjust relay timings (Power Hold, 12V Aux, 24V Traction)
 * - Test individual relays (safe testing mode)
 * - View/modify startup sequence delays
 * - Emergency shutdown sequence test
 * - Save configuration to EEPROM
 *
 * UI Components:
 * - 3 sliders for relay delays (100-5000ms)
 * - 4 test buttons (Power Hold, 12V Aux, 24V Traction, All Off)
 * - Current values display
 * - Save/Reset/Back buttons
 */

class MenuPowerConfig {
public:
  static void init();
  static void update();
  static void draw();
  static void handleTouch(uint16_t x, uint16_t y);

private:
  // UI State
  static bool needsRedraw;
  static uint16_t powerHoldDelay;
  static uint16_t aux12VDelay;
  static uint16_t traction24VDelay;
  static uint8_t activeTest; // 0=none, 1=hold, 2=aux, 3=traction
  static unsigned long testStartTime;

  // UI Elements
  static constexpr uint16_t SLIDER_X = 40;
  static constexpr uint16_t SLIDER_WIDTH = 240;
  static constexpr uint16_t SLIDER_HEIGHT = 20;
  static constexpr uint16_t LABEL_X = 10;

  static constexpr uint16_t BUTTON_WIDTH = 70;
  static constexpr uint16_t BUTTON_HEIGHT = 40;

  // Draw functions
  static void drawSlider(uint16_t x, uint16_t y, const char *label,
                         uint16_t value, uint16_t min, uint16_t max);
  static void drawTestButtons();
  static void drawActionButtons();
  static void drawCurrentValues();

  // Input handlers
  static bool handleSliderTouch(uint16_t x, uint16_t y, uint16_t sliderY,
                                uint16_t &value, uint16_t min, uint16_t max);
  static void handleTestButton(uint8_t testId);
  static void stopAllTests();

  // Actions
  static void saveConfiguration();
  static void resetToDefaults();
  static void testRelay(uint8_t relayId);

  // Helper functions
  static uint16_t mapTouchToValue(uint16_t touchX, uint16_t min, uint16_t max);
};

#endif // MENU_POWER_CONFIG_H
