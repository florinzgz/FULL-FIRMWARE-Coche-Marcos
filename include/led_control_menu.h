/**
 * @file led_control_menu.h
 * @brief Complete LED WS2812B control interface for hidden menu
 */

#ifndef LED_CONTROL_MENU_H
#define LED_CONTROL_MENU_H

#include <Arduino.h>

/**
 * @class LEDControlMenu
 * @brief Full-featured LED control UI with pattern selection, sliders, and color picker
 */
class LEDControlMenu {
public:
    /**
     * @brief Constructor
     */
    LEDControlMenu();
    
    /**
     * @brief Initialize menu and load saved settings
     */
    void init();
    
    /**
     * @brief Handle touch input
     * @param x Touch X coordinate
     * @param y Touch Y coordinate
     * @return true if staying in menu, false to exit
     */
    bool handleTouch(uint16_t x, uint16_t y);
    
private:
    // Current settings
    uint8_t currentPattern;
    uint8_t brightness;
    uint8_t speed;
    uint32_t customColor; // RGB888 format
    
    // Touch state
    bool isDraggingBrightness;
    bool isDraggingSpeed;
    bool isDraggingColor;
    
    // UI drawing functions
    void drawUI();
    void drawPatternButtons();
    void drawBrightnessSlider();
    void drawSpeedSlider();
    void drawColorPicker();
    void drawColorButton(int x, int y, uint32_t color);
    void drawBackButton();
    
    // Touch handlers
    bool handlePatternButtonTouch(uint16_t x, uint16_t y);
    bool handleBrightnessTouch(uint16_t x, uint16_t y);
    bool handleSpeedTouch(uint16_t x, uint16_t y);
    bool handleColorPickerTouch(uint16_t x, uint16_t y);
    
    // Settings management
    void applySettings();
    void saveSettings();
    
    // Utility
    uint16_t rgb888to565(uint32_t rgb888);
};

#endif // LED_CONTROL_MENU_H
