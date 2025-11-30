/**
 * @file menu_led_control.cpp
 * @brief LED control menu implementation for hidden menu
 */

#include "menu_led_control.h"
#include "led_controller.h"
#include "config_storage.h"
#include "eeprom_persistence.h"
#include "logger.h"
#include "alerts.h"
#include <TFT_eSPI.h>

// Forward declaration of TFT instance (shared with HUD)
extern TFT_eSPI tft;

// Static member definitions
bool MenuLEDControl::visible = false;
bool MenuLEDControl::needsRedraw = true;
uint8_t MenuLEDControl::selectedPattern = 0;
bool MenuLEDControl::draggingBrightness = false;
bool MenuLEDControl::draggingSpeed = false;
bool MenuLEDControl::pickingColor = false;
uint8_t MenuLEDControl::colorPickerH = 0;
uint8_t MenuLEDControl::colorPickerS = 255;

// Colors
static const uint16_t COLOR_BG = TFT_BLACK;
static const uint16_t COLOR_TEXT = TFT_WHITE;
static const uint16_t COLOR_ACCENT = TFT_CYAN;
static const uint16_t COLOR_SELECTED = TFT_GREEN;
static const uint16_t COLOR_SLIDER_BG = TFT_DARKGREY;
static const uint16_t COLOR_SLIDER_FG = TFT_BLUE;

// Layout constants
static const int HEADER_Y = 10;
static const int PATTERN_Y = 50;
static const int PATTERN_BTN_W = 55;
static const int PATTERN_BTN_H = 40;
static const int BRIGHTNESS_Y = 110;
static const int SPEED_Y = 160;
static const int COLOR_Y = 210;
static const int PREVIEW_Y = 260;
static const int SLIDER_X = 120;
static const int SLIDER_W = 300;
static const int SLIDER_H = 25;

// Pattern names
static const char* PATTERN_NAMES[] = {
    "Off",
    "Solid",
    "Breath",
    "Rainbow",
    "Chase",
    "Twinkle",
    "Fire",
    "Wave"
};
static const int NUM_PATTERNS = 8;

void MenuLEDControl::init() {
    loadSettings();
    visible = false;
    needsRedraw = true;
    Logger::info("MenuLEDControl: initialized");
}

void MenuLEDControl::update() {
    if (!visible) return;
    
    // Periodic redraw of preview
    static uint32_t lastPreviewUpdate = 0;
    if (millis() - lastPreviewUpdate > 100) {
        lastPreviewUpdate = millis();
        drawPreview();
    }
}

void MenuLEDControl::draw() {
    if (!visible || !needsRedraw) return;
    needsRedraw = false;
    
    tft.fillScreen(COLOR_BG);
    
    // Header
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.drawString("LED CONTROL", 240, HEADER_Y, 4);
    
    drawPatternSelector();
    drawBrightnessSlider();
    drawSpeedSlider();
    drawColorPicker();
    drawPreview();
    drawControls();
}

void MenuLEDControl::handleTouch(int16_t x, int16_t y) {
    if (!visible) return;
    
    // Reset dragging states on each touch
    if (y < BRIGHTNESS_Y - 10 || y > SPEED_Y + SLIDER_H + 10) {
        draggingBrightness = false;
        draggingSpeed = false;
    }
    
    // Check pattern selection
    handlePatternSelect(x, y);
    
    // Check brightness slider
    handleBrightnessSlider(x, y);
    
    // Check speed slider
    handleSpeedSlider(x, y);
    
    // Check color picker
    handleColorPicker(x, y);
    
    // Check control buttons
    // Save button (bottom left)
    if (x >= 20 && x <= 150 && y >= 280 && y <= 315) {
        saveSettings();
        Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_HIGH});
        needsRedraw = true;
    }
    
    // Back button (bottom right)
    if (x >= 330 && x <= 460 && y >= 280 && y <= 315) {
        hide();
    }
}

void MenuLEDControl::show() {
    visible = true;
    needsRedraw = true;
    loadSettings();
    draw();
    Logger::info("MenuLEDControl: shown");
}

void MenuLEDControl::hide() {
    visible = false;
    Logger::info("MenuLEDControl: hidden");
}

bool MenuLEDControl::isVisible() {
    return visible;
}

void MenuLEDControl::drawPatternSelector() {
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString("Pattern:", 20, PATTERN_Y, 2);
    
    // Draw pattern buttons in 2 rows
    for (int i = 0; i < NUM_PATTERNS; i++) {
        int row = i / 4;
        int col = i % 4;
        int btnX = 100 + col * (PATTERN_BTN_W + 10);
        int btnY = PATTERN_Y + row * (PATTERN_BTN_H + 5);
        
        uint16_t btnColor = (i == selectedPattern) ? COLOR_SELECTED : TFT_DARKGREY;
        tft.fillRoundRect(btnX, btnY, PATTERN_BTN_W, PATTERN_BTN_H, 5, btnColor);
        tft.drawRoundRect(btnX, btnY, PATTERN_BTN_W, PATTERN_BTN_H, 5, COLOR_TEXT);
        
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor((i == selectedPattern) ? COLOR_BG : COLOR_TEXT, btnColor);
        tft.drawString(PATTERN_NAMES[i], btnX + PATTERN_BTN_W/2, btnY + PATTERN_BTN_H/2, 1);
    }
}

void MenuLEDControl::drawBrightnessSlider() {
    auto& cfg = LEDController::getConfig();
    
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString("Brightness:", 20, BRIGHTNESS_Y + 5, 2);
    
    // Draw slider background
    tft.fillRoundRect(SLIDER_X, BRIGHTNESS_Y, SLIDER_W, SLIDER_H, 5, COLOR_SLIDER_BG);
    
    // Draw filled portion
    int fillW = (cfg.brightness * SLIDER_W) / 255;
    tft.fillRoundRect(SLIDER_X, BRIGHTNESS_Y, fillW, SLIDER_H, 5, COLOR_SLIDER_FG);
    
    // Draw border
    tft.drawRoundRect(SLIDER_X, BRIGHTNESS_Y, SLIDER_W, SLIDER_H, 5, COLOR_TEXT);
    
    // Draw value
    char valueStr[8];
    snprintf(valueStr, sizeof(valueStr), "%d%%", (cfg.brightness * 100) / 255);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString(valueStr, SLIDER_X + SLIDER_W + 30, BRIGHTNESS_Y + SLIDER_H/2, 2);
}

void MenuLEDControl::drawSpeedSlider() {
    auto& cfg = LEDController::getConfig();
    uint8_t speed = 128;  // Default mid speed
    
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString("Speed:", 20, SPEED_Y + 5, 2);
    
    // Draw slider background
    tft.fillRoundRect(SLIDER_X, SPEED_Y, SLIDER_W, SLIDER_H, 5, COLOR_SLIDER_BG);
    
    // Draw filled portion
    int fillW = (speed * SLIDER_W) / 255;
    tft.fillRoundRect(SLIDER_X, SPEED_Y, fillW, SLIDER_H, 5, TFT_GREEN);
    
    // Draw border
    tft.drawRoundRect(SLIDER_X, SPEED_Y, SLIDER_W, SLIDER_H, 5, COLOR_TEXT);
    
    // Draw value
    char valueStr[8];
    snprintf(valueStr, sizeof(valueStr), "%d%%", (speed * 100) / 255);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString(valueStr, SLIDER_X + SLIDER_W + 30, SPEED_Y + SLIDER_H/2, 2);
}

void MenuLEDControl::drawColorPicker() {
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString("Color:", 20, COLOR_Y + 5, 2);
    
    // Draw color spectrum bar (hue 0-255)
    int barX = SLIDER_X;
    int barW = SLIDER_W;
    int barH = SLIDER_H;
    
    // Draw rainbow gradient
    for (int i = 0; i < barW; i++) {
        uint8_t h = (i * 255) / barW;
        uint16_t color = tft.color565(
            h < 85 ? 255 - h * 3 : (h < 170 ? 0 : (h - 170) * 3),
            h < 85 ? h * 3 : (h < 170 ? 255 - (h - 85) * 3 : 0),
            h < 85 ? 0 : (h < 170 ? (h - 85) * 3 : 255 - (h - 170) * 3)
        );
        tft.drawFastVLine(barX + i, COLOR_Y, barH, color);
    }
    
    // Draw border
    tft.drawRect(barX, COLOR_Y, barW, barH, COLOR_TEXT);
    
    // Draw selection marker
    int markerX = barX + (colorPickerH * barW) / 255;
    tft.drawTriangle(markerX - 5, COLOR_Y + barH + 2, 
                     markerX + 5, COLOR_Y + barH + 2,
                     markerX, COLOR_Y + barH + 8, COLOR_TEXT);
    
    // Draw current color preview
    uint16_t currentColor = tft.color565(
        colorPickerH < 85 ? 255 - colorPickerH * 3 : (colorPickerH < 170 ? 0 : (colorPickerH - 170) * 3),
        colorPickerH < 85 ? colorPickerH * 3 : (colorPickerH < 170 ? 255 - (colorPickerH - 85) * 3 : 0),
        colorPickerH < 85 ? 0 : (colorPickerH < 170 ? (colorPickerH - 85) * 3 : 255 - (colorPickerH - 170) * 3)
    );
    tft.fillRect(barX + barW + 10, COLOR_Y, 30, barH, currentColor);
    tft.drawRect(barX + barW + 10, COLOR_Y, 30, barH, COLOR_TEXT);
}

void MenuLEDControl::drawPreview() {
    // Animated preview of current pattern
    static uint8_t animFrame = 0;
    animFrame++;
    
    int previewX = 160;
    int previewW = 160;
    int previewH = 20;
    
    tft.fillRect(previewX, PREVIEW_Y, previewW, previewH, COLOR_BG);
    
    // Fast sine approximation lookup table (0-255 -> 0-255 sine wave)
    // Pre-computed for performance: sin(x*2*PI/256) * 127 + 128
    static const uint8_t sinLut[] = {
        128,131,134,137,140,143,146,149,152,155,158,162,165,167,170,173,
        176,179,182,185,188,190,193,196,198,201,203,206,208,211,213,215,
        218,220,222,224,226,228,230,232,234,235,237,238,240,241,243,244,
        245,246,248,249,250,250,251,252,253,253,254,254,254,255,255,255,
        255,255,255,255,254,254,254,253,253,252,251,250,250,249,248,246,
        245,244,243,241,240,238,237,235,234,232,230,228,226,224,222,220,
        218,215,213,211,208,206,203,201,198,196,193,190,188,185,182,179,
        176,173,170,167,165,162,158,155,152,149,146,143,140,137,134,131,
        128,124,121,118,115,112,109,106,103,100,97,93,90,88,85,82,
        79,76,73,70,67,65,62,59,57,54,52,49,47,44,42,40,
        37,35,33,31,29,27,25,23,21,20,18,17,15,14,12,11,
        10,9,7,6,5,5,4,3,2,2,1,1,1,0,0,0,
        0,0,0,0,1,1,1,2,2,3,4,5,5,6,7,9,
        10,11,12,14,15,17,18,20,21,23,25,27,29,31,33,35,
        37,40,42,44,47,49,52,54,57,59,62,65,67,70,73,76,
        79,82,85,88,90,93,97,100,103,106,109,112,115,118,121,124
    };
    
    // Simple animation based on pattern
    for (int i = 0; i < 16; i++) {
        int ledX = previewX + i * 10;
        uint16_t ledColor;
        
        switch (selectedPattern) {
            case 0: // Off
                ledColor = TFT_DARKGREY;
                break;
            case 1: // Solid
                ledColor = TFT_RED;
                break;
            case 2: // Breath
                {
                    // Use lookup table for fast sine approximation
                    uint8_t lutIndex = (animFrame * 4 + i * 8) & 0xFF;
                    uint8_t brightness = sinLut[lutIndex];
                    ledColor = tft.color565(brightness, 0, 0);
                }
                break;
            case 3: // Rainbow
                {
                    uint8_t h = (animFrame + i * 16) % 256;
                    ledColor = tft.color565(h < 85 ? 255 - h * 3 : (h < 170 ? 0 : (h - 170) * 3),
                                            h < 85 ? h * 3 : (h < 170 ? 255 - (h - 85) * 3 : 0),
                                            h < 85 ? 0 : (h < 170 ? (h - 85) * 3 : 255 - (h - 170) * 3));
                }
                break;
            case 4: // Chase
                ledColor = ((animFrame / 4 + i) % 4 == 0) ? TFT_GREEN : TFT_DARKGREY;
                break;
            case 5: // Twinkle
                ledColor = (random(10) < 2) ? TFT_WHITE : TFT_DARKGREY;
                break;
            case 6: // Fire
                {
                    uint8_t r = 200 + random(55);
                    uint8_t g = random(100);
                    ledColor = tft.color565(r, g, 0);
                }
                break;
            case 7: // Wave
                {
                    // Use lookup table for fast sine approximation
                    uint8_t lutIndex = (animFrame * 6 - i * 12) & 0xFF;
                    uint8_t brightness = sinLut[lutIndex];
                    ledColor = tft.color565(0, 0, brightness);
                }
                break;
            default:
                ledColor = TFT_DARKGREY;
        }
        
        tft.fillCircle(ledX + 4, PREVIEW_Y + 10, 4, ledColor);
    }
}

void MenuLEDControl::drawControls() {
    // Save button
    tft.fillRoundRect(20, 280, 130, 35, 5, TFT_GREEN);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(COLOR_BG, TFT_GREEN);
    tft.drawString("SAVE", 85, 297, 2);
    
    // Back button
    tft.fillRoundRect(330, 280, 130, 35, 5, TFT_DARKGREY);
    tft.setTextColor(COLOR_TEXT, TFT_DARKGREY);
    tft.drawString("BACK", 395, 297, 2);
}

void MenuLEDControl::handlePatternSelect(int16_t x, int16_t y) {
    if (y < PATTERN_Y || y > PATTERN_Y + 2 * (PATTERN_BTN_H + 5)) return;
    
    for (int i = 0; i < NUM_PATTERNS; i++) {
        int row = i / 4;
        int col = i % 4;
        int btnX = 100 + col * (PATTERN_BTN_W + 10);
        int btnY = PATTERN_Y + row * (PATTERN_BTN_H + 5);
        
        if (x >= btnX && x <= btnX + PATTERN_BTN_W &&
            y >= btnY && y <= btnY + PATTERN_BTN_H) {
            if (selectedPattern != i) {
                selectedPattern = i;
                needsRedraw = true;
                
                // Apply pattern to LED controller (if mode mapping exists)
                // LEDController::setFrontMode((LEDController::FrontMode)selectedPattern);
                
                Logger::infof("LED pattern selected: %s", PATTERN_NAMES[i]);
            }
            break;
        }
    }
}

void MenuLEDControl::handleBrightnessSlider(int16_t x, int16_t y) {
    if (y >= BRIGHTNESS_Y && y <= BRIGHTNESS_Y + SLIDER_H &&
        x >= SLIDER_X && x <= SLIDER_X + SLIDER_W) {
        
        int newBrightness = ((x - SLIDER_X) * 255) / SLIDER_W;
        newBrightness = constrain(newBrightness, 0, 255);
        
        LEDController::setBrightness(newBrightness);
        needsRedraw = true;
    }
}

void MenuLEDControl::handleSpeedSlider(int16_t x, int16_t y) {
    if (y >= SPEED_Y && y <= SPEED_Y + SLIDER_H &&
        x >= SLIDER_X && x <= SLIDER_X + SLIDER_W) {
        
        // Speed would be stored and applied similarly
        needsRedraw = true;
    }
}

void MenuLEDControl::handleColorPicker(int16_t x, int16_t y) {
    if (y >= COLOR_Y && y <= COLOR_Y + SLIDER_H &&
        x >= SLIDER_X && x <= SLIDER_X + SLIDER_W) {
        
        colorPickerH = ((x - SLIDER_X) * 255) / SLIDER_W;
        colorPickerH = constrain(colorPickerH, 0, 255);
        needsRedraw = true;
    }
}

void MenuLEDControl::saveSettings() {
    auto& config = ConfigStorage::getCurrentConfig();
    auto& ledCfg = LEDController::getConfig();
    
    config.led_pattern = selectedPattern;
    config.led_brightness = ledCfg.brightness;
    // config.led_speed = speed;  // Would need to track this
    config.led_color = (colorPickerH << 16) | (colorPickerS << 8) | 255;
    
    ConfigStorage::save(config);
    Logger::info("LED settings saved");
}

void MenuLEDControl::loadSettings() {
    auto& config = ConfigStorage::getCurrentConfig();
    
    selectedPattern = config.led_pattern;
    LEDController::setBrightness(config.led_brightness);
    colorPickerH = (config.led_color >> 16) & 0xFF;
    colorPickerS = (config.led_color >> 8) & 0xFF;
    
    Logger::info("LED settings loaded");
}
