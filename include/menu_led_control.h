#ifndef MENU_LED_CONTROL_H
#define MENU_LED_CONTROL_H

#include <Arduino.h>

// LED Control Menu for Hidden Menu
class MenuLEDControl {
public:
  static void init();
  static void update();
  static void draw();
  static void handleTouch(int16_t x, int16_t y);
  static void show();
  static void hide();
  static bool isVisible();

private:
  static bool visible;
  static bool needsRedraw;
  static uint8_t selectedPattern;
  static bool draggingBrightness;
  static bool draggingSpeed;
  static bool pickingColor;
  static uint8_t colorPickerH;
  static uint8_t colorPickerS;

  // UI element bounds
  struct Button {
    int16_t x, y, w, h;
    const char *label;
  };

  static void drawPatternSelector();
  static void drawBrightnessSlider();
  static void drawSpeedSlider();
  static void drawColorPicker();
  static void drawPreview();
  static void drawControls();

  static void handlePatternSelect(int16_t x, int16_t y);
  static void handleBrightnessSlider(int16_t x, int16_t y);
  static void handleSpeedSlider(int16_t x, int16_t y);
  static void handleColorPicker(int16_t x, int16_t y);

  static void saveSettings();
  static void loadSettings();
};

#endif // MENU_LED_CONTROL_H
