/**
 * @file menu_power_config.cpp
 * @brief Power management configuration menu for relay timing and test
 */

#include "menu_power_config.h"
#include "relays.h"
#include "config_storage.h"
#include "logger.h"
#include "alerts.h"
#include <TFT_eSPI.h>

// Forward declaration of TFT instance (shared with HUD)
extern TFT_eSPI tft;

// Static member definitions
bool MenuPowerConfig::needsRedraw = true;
uint16_t MenuPowerConfig::powerHoldDelay = 5000;
uint16_t MenuPowerConfig::aux12VDelay = 100;
uint16_t MenuPowerConfig::traction24VDelay = 500;
uint8_t MenuPowerConfig::activeTest = 0;
unsigned long MenuPowerConfig::testStartTime = 0;

// Colors
static const uint16_t COLOR_BG = TFT_BLACK;
static const uint16_t COLOR_TEXT = TFT_WHITE;
static const uint16_t COLOR_ACCENT = TFT_CYAN;
static const uint16_t COLOR_ACTIVE = TFT_GREEN;
static const uint16_t COLOR_WARNING = TFT_YELLOW;
static const uint16_t COLOR_DANGER = TFT_RED;
static const uint16_t COLOR_INACTIVE = TFT_DARKGREY;

// Layout constants
static const uint16_t HEADER_Y = 10;
static const uint16_t SLIDER_Y_START = 55;
static const uint16_t SLIDER_SPACING = 55;
static const uint16_t TEST_BTN_Y = 195;
static const uint16_t ACTION_BTN_Y = 265;

void MenuPowerConfig::init() {
    // Load current configuration
    auto& config = ConfigStorage::getCurrentConfig();
    powerHoldDelay = config.power_hold_delay;
    aux12VDelay = config.aux_delay;
    traction24VDelay = config.traction_delay;
    
    activeTest = 0;
    testStartTime = 0;
    needsRedraw = true;
    
    Logger::info("MenuPowerConfig: initialized");
}

void MenuPowerConfig::update() {
    // Check if test should auto-stop after 3 seconds
    if (activeTest != 0 && (millis() - testStartTime > 3000)) {
        stopAllTests();
        needsRedraw = true;
    }
}

void MenuPowerConfig::draw() {
    if (!needsRedraw) return;
    needsRedraw = false;
    
    // 🔒 CRITICAL FIX: Validate TFT is available before drawing
    // In exceptional cases (e.g., during boot), tft may not be initialized
    // This prevents crashes when draw() is called prematurely
    if (!tft.getReady()) {
        Serial.println("[ERROR] MenuPowerConfig::draw() called but TFT not ready");
        needsRedraw = true;  // Try again next frame
        return;
    }
    
    tft.fillScreen(COLOR_BG);
    
    // Header
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.drawString("POWER CONFIGURATION", 240, HEADER_Y, 4);
    
    tft.drawLine(20, 42, 460, 42, TFT_DARKGREY);
    
    // Draw sliders
    drawSlider(SLIDER_X, SLIDER_Y_START, "Power Hold:", powerHoldDelay, 100, 10000);
    drawSlider(SLIDER_X, SLIDER_Y_START + SLIDER_SPACING, "12V Aux:", aux12VDelay, 10, 2000);
    drawSlider(SLIDER_X, SLIDER_Y_START + 2 * SLIDER_SPACING, "24V Traction:", traction24VDelay, 100, 5000);
    
    drawTestButtons();
    drawActionButtons();
    drawCurrentValues();
}

void MenuPowerConfig::handleTouch(uint16_t x, uint16_t y) {
    // Check sliders
    if (handleSliderTouch(x, y, SLIDER_Y_START, powerHoldDelay, 100, 10000)) {
        needsRedraw = true;
        return;
    }
    if (handleSliderTouch(x, y, SLIDER_Y_START + SLIDER_SPACING, aux12VDelay, 10, 2000)) {
        needsRedraw = true;
        return;
    }
    if (handleSliderTouch(x, y, SLIDER_Y_START + 2 * SLIDER_SPACING, traction24VDelay, 100, 5000)) {
        needsRedraw = true;
        return;
    }
    
    // Check test buttons - buttons are drawn at X = 100 + i * 90 (100, 190, 280, 370)
    if (y >= TEST_BTN_Y && y <= TEST_BTN_Y + 45) {
        for (int i = 0; i < 4; i++) {
            int btnX = 100 + i * 90;
            int btnW = (i == 3) ? 100 : 80;
            if (x >= btnX && x <= btnX + btnW) {
                if (i == 3) {
                    stopAllTests();
                    needsRedraw = true;
                } else {
                    handleTestButton(i + 1);
                }
                break;
            }
        }
    }
    
    // Check action buttons
    if (y >= ACTION_BTN_Y && y <= ACTION_BTN_Y + BUTTON_HEIGHT) {
        if (x >= 20 && x <= 140) {
            saveConfiguration();
        } else if (x >= 170 && x <= 290) {
            resetToDefaults();
        } else if (x >= 340 && x <= 460) {
            // Back - handled by parent menu
            stopAllTests();
        }
    }
}

void MenuPowerConfig::drawSlider(uint16_t x, uint16_t y, const char* label, 
                                   uint16_t value, uint16_t min, uint16_t max) {
    // Draw label
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString(label, LABEL_X, y + 5, 2);
    
    // Draw slider background
    tft.fillRoundRect(x, y, SLIDER_WIDTH, SLIDER_HEIGHT, 5, COLOR_INACTIVE);
    
    // Calculate fill width
    uint16_t fillWidth = ((uint32_t)(value - min) * SLIDER_WIDTH) / (max - min);
    fillWidth = constrain(fillWidth, 0, SLIDER_WIDTH);
    
    // Draw filled portion with color based on value
    uint16_t fillColor = COLOR_ACTIVE;
    if (value > (max * 2 / 3)) fillColor = COLOR_WARNING;
    if (value > (max * 9 / 10)) fillColor = COLOR_DANGER;
    
    tft.fillRoundRect(x, y, fillWidth, SLIDER_HEIGHT, 5, fillColor);
    
    // Draw border
    tft.drawRoundRect(x, y, SLIDER_WIDTH, SLIDER_HEIGHT, 5, COLOR_TEXT);
    
    // Draw value
    char valueStr[16];
    snprintf(valueStr, sizeof(valueStr), "%d ms", value);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString(valueStr, x + SLIDER_WIDTH + 80, y + 5, 2);
}

void MenuPowerConfig::drawTestButtons() {
    const char* labels[] = {"PWR", "12V", "24V", "ALL OFF"};
    uint16_t colors[] = {
        (activeTest == 1) ? COLOR_ACTIVE : COLOR_INACTIVE,
        (activeTest == 2) ? COLOR_WARNING : COLOR_INACTIVE,
        (activeTest == 3) ? COLOR_DANGER : COLOR_INACTIVE,
        TFT_DARKGREY
    };
    
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString("Test Relays:", LABEL_X, TEST_BTN_Y + 10, 2);
    
    for (int i = 0; i < 4; i++) {
        uint16_t btnX = 100 + i * 90;
        uint16_t btnW = (i == 3) ? 100 : 80;
        
        tft.fillRoundRect(btnX, TEST_BTN_Y, btnW, 40, 5, colors[i]);
        tft.drawRoundRect(btnX, TEST_BTN_Y, btnW, 40, 5, COLOR_TEXT);
        
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor((activeTest == i+1 && i < 3) ? COLOR_BG : COLOR_TEXT, colors[i]);
        tft.drawString(labels[i], btnX + btnW/2, TEST_BTN_Y + 20, 2);
    }
    
    // Test status indicator
    if (activeTest > 0) {
        uint32_t elapsed = (millis() - testStartTime) / 1000;
        char statusStr[32];
        snprintf(statusStr, sizeof(statusStr), "Testing... %lus", (unsigned long)elapsed);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(COLOR_ACTIVE, COLOR_BG);
        tft.drawString(statusStr, 100, TEST_BTN_Y + 45, 1);
    }
}

void MenuPowerConfig::drawActionButtons() {
    // Save button
    tft.fillRoundRect(20, ACTION_BTN_Y, 120, BUTTON_HEIGHT, 5, COLOR_ACTIVE);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(COLOR_BG, COLOR_ACTIVE);
    tft.drawString("SAVE", 80, ACTION_BTN_Y + BUTTON_HEIGHT/2, 2);
    
    // Reset button
    tft.fillRoundRect(170, ACTION_BTN_Y, 120, BUTTON_HEIGHT, 5, COLOR_WARNING);
    tft.setTextColor(COLOR_BG, COLOR_WARNING);
    tft.drawString("RESET", 230, ACTION_BTN_Y + BUTTON_HEIGHT/2, 2);
    
    // Back button
    tft.fillRoundRect(340, ACTION_BTN_Y, 120, BUTTON_HEIGHT, 5, TFT_DARKGREY);
    tft.setTextColor(COLOR_TEXT, TFT_DARKGREY);
    tft.drawString("BACK", 400, ACTION_BTN_Y + BUTTON_HEIGHT/2, 2);
}

void MenuPowerConfig::drawCurrentValues() {
    // Current relay states
    auto relayState = Relays::get();
    
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    
    // Draw status indicators
    int statusY = TEST_BTN_Y + 50;
    
    // Power Hold status
    tft.fillCircle(110, statusY, 6, relayState.mainOn ? COLOR_ACTIVE : COLOR_DANGER);
    tft.drawString("Main", 120, statusY - 6, 1);
    
    // 12V Aux status
    tft.fillCircle(180, statusY, 6, relayState.steeringOn ? COLOR_ACTIVE : COLOR_INACTIVE);
    tft.drawString("Steer", 190, statusY - 6, 1);
    
    // 24V Traction status
    tft.fillCircle(250, statusY, 6, relayState.tractionOn ? COLOR_ACTIVE : COLOR_INACTIVE);
    tft.drawString("Traction", 260, statusY - 6, 1);
    
    // Lights status
    tft.fillCircle(330, statusY, 6, relayState.lightsOn ? COLOR_WARNING : COLOR_INACTIVE);
    tft.drawString("Lights", 340, statusY - 6, 1);
}

bool MenuPowerConfig::handleSliderTouch(uint16_t x, uint16_t y, uint16_t sliderY, 
                                          uint16_t& value, uint16_t min, uint16_t max) {
    if (y >= sliderY && y <= sliderY + SLIDER_HEIGHT &&
        x >= SLIDER_X && x <= SLIDER_X + SLIDER_WIDTH) {
        
        value = mapTouchToValue(x, min, max);
        return true;
    }
    return false;
}

void MenuPowerConfig::handleTestButton(uint8_t testId) {
    if (activeTest == testId) {
        // Toggle off
        stopAllTests();
    } else {
        // Start new test
        stopAllTests();
        activeTest = testId;
        testStartTime = millis();
        testRelay(testId);
        Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
    }
    needsRedraw = true;
}

void MenuPowerConfig::stopAllTests() {
    activeTest = 0;
    // Don't actually disable power in test mode - just stop the test indicator
    // In a real implementation, you might want to restore previous state
    Logger::info("Power config: all tests stopped");
}

void MenuPowerConfig::saveConfiguration() {
    auto& config = ConfigStorage::getCurrentConfig();
    config.power_hold_delay = powerHoldDelay;
    config.aux_delay = aux12VDelay;
    config.traction_delay = traction24VDelay;
    
    ConfigStorage::save(config);
    
    Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_HIGH});
    Logger::infof("Power config saved: hold=%d, aux=%d, trac=%d", 
                  powerHoldDelay, aux12VDelay, traction24VDelay);
    
    needsRedraw = true;
}

void MenuPowerConfig::resetToDefaults() {
    powerHoldDelay = ConfigStorage::DEFAULT_CONFIG.power_hold_delay;
    aux12VDelay = ConfigStorage::DEFAULT_CONFIG.aux_delay;
    traction24VDelay = ConfigStorage::DEFAULT_CONFIG.traction_delay;
    
    Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
    Logger::info("Power config reset to defaults");
    
    needsRedraw = true;
}

void MenuPowerConfig::testRelay(uint8_t relayId) {
    switch (relayId) {
        case 1:
            Logger::info("Testing Power Hold relay");
            // Relays::enablePower();  // Don't actually toggle in test
            break;
        case 2:
            Logger::info("Testing 12V Aux relay");
            break;
        case 3:
            Logger::info("Testing 24V Traction relay");
            break;
    }
}

uint16_t MenuPowerConfig::mapTouchToValue(uint16_t touchX, uint16_t min, uint16_t max) {
    // 🔒 CRITICAL FIX: Validate inputs to prevent division by zero and invalid ranges
    if (max <= min) {
        Serial.printf("[ERROR] MenuPowerConfig: Invalid range min=%d max=%d\n", min, max);
        return min;  // Safe fallback
    }
    
    if (touchX <= SLIDER_X) return min;
    if (touchX >= SLIDER_X + SLIDER_WIDTH) return max;
    
    uint32_t value = ((uint32_t)(touchX - SLIDER_X) * (max - min)) / SLIDER_WIDTH + min;
    
    // Round to nearest step size based on range for cleaner values
    // Use 100ms steps for ms values, 10 steps otherwise
    uint16_t range = max - min;
    uint16_t stepSize = (range >= 1000) ? 100 : ((range >= 100) ? 10 : 1);
    uint16_t halfStep = (stepSize + 1) / 2;  // Ensure at least 1 for proper rounding
    value = ((value + halfStep) / stepSize) * stepSize;
    
    return constrain(value, min, max);
}
