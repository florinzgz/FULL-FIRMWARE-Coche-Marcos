/**
 * @file led_control_menu.cpp
 * @brief Complete LED WS2812B control interface for hidden menu
 */

#include "led_control_menu.h"
#include "alerts.h"
#include "config_storage.h"
#include "eeprom_persistence.h"
#include "led_controller.h"
#include "logger.h"
#include <TFT_eSPI.h>

// Forward declaration of TFT instance (shared with HUD)
extern TFT_eSPI *tft;

// Colors
static const uint16_t COLOR_BG = TFT_BLACK;
static const uint16_t COLOR_TEXT = TFT_WHITE;
static const uint16_t COLOR_ACCENT = TFT_CYAN;
static const uint16_t COLOR_SELECTED = TFT_GREEN;
static const uint16_t COLOR_SLIDER_BG = TFT_DARKGREY;

// Layout constants
static const int HEADER_Y = 5;
static const int PATTERN_Y = 40;
static const int PATTERN_BTN_W = 50;
static const int PATTERN_BTN_H = 35;
static const int BRIGHTNESS_Y = 130;
static const int SPEED_Y = 180;
static const int COLOR_Y = 230;
static const int SLIDER_X = 100;
static const int SLIDER_W = 280;
static const int SLIDER_H = 30;
static const int BACK_BTN_X = 400;
static const int BACK_BTN_Y = 275;
static const int BACK_BTN_W = 70;
static const int BACK_BTN_H = 35;

// Pattern names - consistent with menu_led_control.cpp
static const char *PATTERN_NAMES[] = {"Off",   "Solid",   "Breath", "Rainbow",
                                      "Chase", "Twinkle", "Fire",   "Wave"};
static const int NUM_PATTERNS = 8;

// Preset colors
static const uint32_t PRESET_COLORS[] = {
    0xFF0000, // Red
    0x00FF00, // Green
    0x0000FF, // Blue
    0xFFFF00, // Yellow
    0xFF00FF, // Magenta
    0x00FFFF, // Cyan
    0xFFFFFF, // White
    0xFF8000  // Orange
};
static const int NUM_PRESET_COLORS = 8;

LEDControlMenu::LEDControlMenu()
    : currentPattern(0), brightness(128), speed(128), customColor(0xFF0000),
      isDraggingBrightness(false), isDraggingSpeed(false),
      isDraggingColor(false) {}

void LEDControlMenu::init() {
  // Load saved settings from ConfigStorage
  auto &config = ConfigStorage::getCurrentConfig();
  currentPattern = config.led_pattern;
  brightness = config.led_brightness;
  speed = config.led_speed;
  customColor = config.led_color;

  // Apply settings to LED controller
  applySettings();

  Logger::info("LEDControlMenu: initialized");
  drawUI();
}

bool LEDControlMenu::handleTouch(uint16_t x, uint16_t y) {
  bool needsRedraw = false;

  // Check Back button first
  if (x >= BACK_BTN_X && x <= BACK_BTN_X + BACK_BTN_W && y >= BACK_BTN_Y &&
      y <= BACK_BTN_Y + BACK_BTN_H) {
    saveSettings();
    return false; // Exit menu
  }

  // Check SAVE button (10, BACK_BTN_Y, 80, BACK_BTN_H)
  if (x >= 10 && x <= 10 + 80 && y >= BACK_BTN_Y &&
      y <= BACK_BTN_Y + BACK_BTN_H) {
    saveSettings();
    Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_HIGH});
    needsRedraw = true;
  }

  // Check pattern buttons
  if (handlePatternButtonTouch(x, y)) { needsRedraw = true; }

  // Check brightness slider
  if (handleBrightnessTouch(x, y)) { needsRedraw = true; }

  // Check speed slider
  if (handleSpeedTouch(x, y)) { needsRedraw = true; }

  // Check color picker
  if (handleColorPickerTouch(x, y)) { needsRedraw = true; }

  if (needsRedraw) {
    applySettings();
    drawUI();
  }

  return true; // Stay in menu
}

void LEDControlMenu::drawUI() {
  tft->fillScreen(COLOR_BG);

  // Header
  tft->setTextDatum(TC_DATUM);
  tft->setTextColor(COLOR_ACCENT, COLOR_BG);
  tft->drawString("LED CONTROL", 200, HEADER_Y, 4);

  tft->drawLine(10, 35, 390, 35, TFT_DARKGREY);

  drawPatternButtons();
  drawBrightnessSlider();
  drawSpeedSlider();
  drawColorPicker();
  drawBackButton();
}

void LEDControlMenu::drawPatternButtons() {
  tft->setTextDatum(TL_DATUM);
  tft->setTextColor(COLOR_TEXT, COLOR_BG);
  tft->drawString("Pattern:", 10, PATTERN_Y + 20, 2);

  // Draw 8 pattern buttons in 2 rows of 4
  for (int i = 0; i < NUM_PATTERNS; i++) {
    int row = i / 4;
    int col = i % 4;
    int btnX = 90 + col * (PATTERN_BTN_W + 8);
    int btnY = PATTERN_Y + row * (PATTERN_BTN_H + 5);

    uint16_t btnColor = (i == currentPattern) ? COLOR_SELECTED : TFT_DARKGREY;
    uint16_t textColor = (i == currentPattern) ? COLOR_BG : COLOR_TEXT;

    tft->fillRoundRect(btnX, btnY, PATTERN_BTN_W, PATTERN_BTN_H, 4, btnColor);
    tft->drawRoundRect(btnX, btnY, PATTERN_BTN_W, PATTERN_BTN_H, 4, COLOR_TEXT);

    tft->setTextDatum(MC_DATUM);
    tft->setTextColor(textColor, btnColor);
    tft->drawString(PATTERN_NAMES[i], btnX + PATTERN_BTN_W / 2,
                   btnY + PATTERN_BTN_H / 2, 1);
  }
}

void LEDControlMenu::drawBrightnessSlider() {
  tft->setTextDatum(TL_DATUM);
  tft->setTextColor(COLOR_TEXT, COLOR_BG);
  tft->drawString("Bright:", 10, BRIGHTNESS_Y + 8, 2);

  // Background
  tft->fillRoundRect(SLIDER_X, BRIGHTNESS_Y, SLIDER_W, SLIDER_H, 5,
                    COLOR_SLIDER_BG);

  // Filled portion
  int fillW = (brightness * SLIDER_W) / 255;
  tft->fillRoundRect(SLIDER_X, BRIGHTNESS_Y, fillW, SLIDER_H, 5, TFT_YELLOW);

  // Border
  tft->drawRoundRect(SLIDER_X, BRIGHTNESS_Y, SLIDER_W, SLIDER_H, 5, COLOR_TEXT);

  // Value text
  char valueStr[8];
  snprintf(valueStr, sizeof(valueStr), "%d%%", (brightness * 100) / 255);
  tft->setTextDatum(MC_DATUM);
  tft->setTextColor(COLOR_BG, TFT_YELLOW);
  tft->drawString(valueStr, SLIDER_X + SLIDER_W / 2, BRIGHTNESS_Y + SLIDER_H / 2,
                 2);
}

void LEDControlMenu::drawSpeedSlider() {
  tft->setTextDatum(TL_DATUM);
  tft->setTextColor(COLOR_TEXT, COLOR_BG);
  tft->drawString("Speed:", 10, SPEED_Y + 8, 2);

  // Background
  tft->fillRoundRect(SLIDER_X, SPEED_Y, SLIDER_W, SLIDER_H, 5, COLOR_SLIDER_BG);

  // Filled portion
  int fillW = (speed * SLIDER_W) / 255;
  tft->fillRoundRect(SLIDER_X, SPEED_Y, fillW, SLIDER_H, 5, TFT_BLUE);

  // Border
  tft->drawRoundRect(SLIDER_X, SPEED_Y, SLIDER_W, SLIDER_H, 5, COLOR_TEXT);

  // Value text
  char valueStr[8];
  snprintf(valueStr, sizeof(valueStr), "%d%%", (speed * 100) / 255);
  tft->setTextDatum(MC_DATUM);
  tft->setTextColor(COLOR_TEXT, TFT_BLUE);
  tft->drawString(valueStr, SLIDER_X + SLIDER_W / 2, SPEED_Y + SLIDER_H / 2, 2);
}

void LEDControlMenu::drawColorPicker() {
  tft->setTextDatum(TL_DATUM);
  tft->setTextColor(COLOR_TEXT, COLOR_BG);
  tft->drawString("Color:", 10, COLOR_Y + 10, 2);

  // Draw preset color buttons
  for (int i = 0; i < NUM_PRESET_COLORS; i++) {
    int btnX = SLIDER_X + i * 35;
    int btnY = COLOR_Y;

    uint16_t color565 = rgb888to565(PRESET_COLORS[i]);
    bool isSelected = (customColor == PRESET_COLORS[i]);

    drawColorButton(btnX, btnY, PRESET_COLORS[i]);

    if (isSelected) {
      tft->drawRect(btnX - 2, btnY - 2, 34, 34, COLOR_SELECTED);
      tft->drawRect(btnX - 1, btnY - 1, 32, 32, COLOR_SELECTED);
    }
  }

  // Current color preview
  tft->fillRect(SLIDER_X + 290, COLOR_Y, 40, 30, rgb888to565(customColor));
  tft->drawRect(SLIDER_X + 290, COLOR_Y, 40, 30, COLOR_TEXT);
}

void LEDControlMenu::drawColorButton(int x, int y, uint32_t color) {
  uint16_t color565 = rgb888to565(color);
  tft->fillRect(x, y, 30, 30, color565);
  tft->drawRect(x, y, 30, 30, COLOR_TEXT);
}

void LEDControlMenu::drawBackButton() {
  tft->fillRoundRect(BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H, 5,
                    TFT_DARKGREY);
  tft->drawRoundRect(BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H, 5,
                    COLOR_TEXT);

  tft->setTextDatum(MC_DATUM);
  tft->setTextColor(COLOR_TEXT, TFT_DARKGREY);
  tft->drawString("BACK", BACK_BTN_X + BACK_BTN_W / 2,
                 BACK_BTN_Y + BACK_BTN_H / 2, 2);

  // Save button
  tft->fillRoundRect(10, BACK_BTN_Y, 80, BACK_BTN_H, 5, COLOR_SELECTED);
  tft->drawRoundRect(10, BACK_BTN_Y, 80, BACK_BTN_H, 5, COLOR_TEXT);
  tft->setTextColor(COLOR_BG, COLOR_SELECTED);
  tft->drawString("SAVE", 50, BACK_BTN_Y + BACK_BTN_H / 2, 2);
}

bool LEDControlMenu::handlePatternButtonTouch(uint16_t x, uint16_t y) {
  if (y < PATTERN_Y || y > PATTERN_Y + 2 * (PATTERN_BTN_H + 5)) return false;

  for (int i = 0; i < NUM_PATTERNS; i++) {
    int row = i / 4;
    int col = i % 4;
    int btnX = 90 + col * (PATTERN_BTN_W + 8);
    int btnY = PATTERN_Y + row * (PATTERN_BTN_H + 5);

    if (x >= btnX && x <= btnX + PATTERN_BTN_W && y >= btnY &&
        y <= btnY + PATTERN_BTN_H) {
      if (currentPattern != i) {
        currentPattern = i;
        Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
        Logger::infof("LED pattern: %s", PATTERN_NAMES[i]);
        return true;
      }
      break;
    }
  }
  return false;
}

bool LEDControlMenu::handleBrightnessTouch(uint16_t x, uint16_t y) {
  if (y >= BRIGHTNESS_Y && y <= BRIGHTNESS_Y + SLIDER_H && x >= SLIDER_X &&
      x <= SLIDER_X + SLIDER_W) {

    brightness = ((uint32_t)(x - SLIDER_X) * 255) / SLIDER_W;
    brightness = constrain(brightness, 0, 255);
    LEDController::setBrightness(brightness);
    return true;
  }
  return false;
}

bool LEDControlMenu::handleSpeedTouch(uint16_t x, uint16_t y) {
  if (y >= SPEED_Y && y <= SPEED_Y + SLIDER_H && x >= SLIDER_X &&
      x <= SLIDER_X + SLIDER_W) {

    speed = ((uint32_t)(x - SLIDER_X) * 255) / SLIDER_W;
    speed = constrain(speed, 0, 255);
    return true;
  }
  return false;
}

bool LEDControlMenu::handleColorPickerTouch(uint16_t x, uint16_t y) {
  if (y < COLOR_Y || y > COLOR_Y + 30) return false;

  for (int i = 0; i < NUM_PRESET_COLORS; i++) {
    int btnX = SLIDER_X + i * 35;
    if (x >= btnX && x <= btnX + 30) {
      customColor = PRESET_COLORS[i];
      Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
      return true;
    }
  }
  return false;
}

void LEDControlMenu::applySettings() {
  LEDController::setBrightness(brightness);
  // Additional LED controller settings would be applied here
  // when the LED controller supports pattern/speed/color settings
}

void LEDControlMenu::saveSettings() {
  auto &config = ConfigStorage::getCurrentConfig();
  config.led_pattern = currentPattern;
  config.led_brightness = brightness;
  config.led_speed = speed;
  config.led_color = customColor;

  ConfigStorage::save(config);
  Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_HIGH});
  Logger::info("LED settings saved");
}

uint16_t LEDControlMenu::rgb888to565(uint32_t rgb888) {
  uint8_t r = (rgb888 >> 16) & 0xFF;
  uint8_t g = (rgb888 >> 8) & 0xFF;
  uint8_t b = rgb888 & 0xFF;

  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
