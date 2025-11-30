/**
 * @file menu_sensor_config.cpp
 * @brief Sensor enable/disable configuration menu for emergency bypass
 */

#include "menu_sensor_config.h"
#include "config_storage.h"
#include "logger.h"
#include "alerts.h"
#include <TFT_eSPI.h>

// Forward declaration of TFT instance (shared with HUD)
extern TFT_eSPI tft;

// Static member definitions
bool MenuSensorConfig::sensorFL = true;
bool MenuSensorConfig::sensorFR = true;
bool MenuSensorConfig::sensorRL = true;
bool MenuSensorConfig::sensorRR = true;
bool MenuSensorConfig::sensorINA226 = true;
bool MenuSensorConfig::needsRedraw = true;
uint32_t MenuSensorConfig::lastSaveTime = 0;
uint32_t MenuSensorConfig::lastResetTime = 0;

// Button definitions
const MenuSensorConfig::Button MenuSensorConfig::btnSensorFL = {340, 60, 100, 35, "FL"};
const MenuSensorConfig::Button MenuSensorConfig::btnSensorFR = {340, 100, 100, 35, "FR"};
const MenuSensorConfig::Button MenuSensorConfig::btnSensorRL = {340, 140, 100, 35, "RL"};
const MenuSensorConfig::Button MenuSensorConfig::btnSensorRR = {340, 180, 100, 35, "RR"};
const MenuSensorConfig::Button MenuSensorConfig::btnINA226 = {340, 220, 100, 35, "INA"};
const MenuSensorConfig::Button MenuSensorConfig::btnSave = {20, 270, 120, 40, "SAVE"};
const MenuSensorConfig::Button MenuSensorConfig::btnReset = {170, 270, 120, 40, "RESET"};
const MenuSensorConfig::Button MenuSensorConfig::btnBack = {340, 270, 120, 40, "BACK"};

// Colors
static const uint16_t COLOR_BG = TFT_BLACK;
static const uint16_t COLOR_TEXT = TFT_WHITE;
static const uint16_t COLOR_ACCENT = TFT_CYAN;
static const uint16_t COLOR_ENABLED = TFT_GREEN;
static const uint16_t COLOR_DISABLED = TFT_RED;
static const uint16_t COLOR_WARNING = TFT_YELLOW;
static const uint16_t COLOR_INACTIVE = TFT_DARKGREY;

void MenuSensorConfig::init() {
    loadConfig();
    needsRedraw = true;
    Logger::info("MenuSensorConfig: initialized");
}

void MenuSensorConfig::update() {
    // Clear save/reset confirmation after 2 seconds
    if (lastSaveTime > 0 && (millis() - lastSaveTime > 2000)) {
        lastSaveTime = 0;
        needsRedraw = true;
    }
    if (lastResetTime > 0 && (millis() - lastResetTime > 2000)) {
        lastResetTime = 0;
        needsRedraw = true;
    }
}

void MenuSensorConfig::draw() {
    if (!needsRedraw) return;
    needsRedraw = false;
    
    tft.fillScreen(COLOR_BG);
    
    // Header
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.drawString("SENSOR CONFIGURATION", 240, 10, 4);
    
    tft.drawLine(20, 42, 460, 42, TFT_DARKGREY);
    
    // Section: Wheel Sensors
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString("Wheel Sensors:", 20, 55, 2);
    
    // Draw sensor toggle buttons with labels
    drawToggleWithLabel(btnSensorFL, "Front Left (FL)", sensorFL);
    drawToggleWithLabel(btnSensorFR, "Front Right (FR)", sensorFR);
    drawToggleWithLabel(btnSensorRL, "Rear Left (RL)", sensorRL);
    drawToggleWithLabel(btnSensorRR, "Rear Right (RR)", sensorRR);
    
    // Section: Current Sensors
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString("Current Sensors:", 20, 215, 2);
    
    drawToggleWithLabel(btnINA226, "INA226 Monitors", sensorINA226);
    
    // Draw status bar
    drawStatusBar();
    
    // Draw action buttons
    drawActionButtons();
}

void MenuSensorConfig::handleTouch(int16_t x, int16_t y) {
    // Check sensor toggle buttons
    if (isTouchInButton(x, y, btnSensorFL)) {
        sensorFL = !sensorFL;
        needsRedraw = true;
        Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
        Logger::infof("Sensor FL %s", sensorFL ? "enabled" : "disabled");
        return;
    }
    
    if (isTouchInButton(x, y, btnSensorFR)) {
        sensorFR = !sensorFR;
        needsRedraw = true;
        Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
        Logger::infof("Sensor FR %s", sensorFR ? "enabled" : "disabled");
        return;
    }
    
    if (isTouchInButton(x, y, btnSensorRL)) {
        sensorRL = !sensorRL;
        needsRedraw = true;
        Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
        Logger::infof("Sensor RL %s", sensorRL ? "enabled" : "disabled");
        return;
    }
    
    if (isTouchInButton(x, y, btnSensorRR)) {
        sensorRR = !sensorRR;
        needsRedraw = true;
        Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
        Logger::infof("Sensor RR %s", sensorRR ? "enabled" : "disabled");
        return;
    }
    
    if (isTouchInButton(x, y, btnINA226)) {
        sensorINA226 = !sensorINA226;
        needsRedraw = true;
        Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
        Logger::infof("INA226 sensors %s", sensorINA226 ? "enabled" : "disabled");
        return;
    }
    
    // Check action buttons
    if (isTouchInButton(x, y, btnSave)) {
        saveConfig();
        lastSaveTime = millis();
        Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_HIGH});
        needsRedraw = true;
        return;
    }
    
    if (isTouchInButton(x, y, btnReset)) {
        resetToDefaults();
        lastResetTime = millis();
        Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
        needsRedraw = true;
        return;
    }
    
    // Back button is handled by parent menu
}

void MenuSensorConfig::loadConfig() {
    auto& config = ConfigStorage::getCurrentConfig();
    sensorFL = config.sensorFL_enabled;
    sensorFR = config.sensorFR_enabled;
    sensorRL = config.sensorRL_enabled;
    sensorRR = config.sensorRR_enabled;
    sensorINA226 = config.ina226_enabled;
    
    Logger::info("Sensor config loaded");
}

void MenuSensorConfig::saveConfig() {
    auto& config = ConfigStorage::getCurrentConfig();
    config.sensorFL_enabled = sensorFL;
    config.sensorFR_enabled = sensorFR;
    config.sensorRL_enabled = sensorRL;
    config.sensorRR_enabled = sensorRR;
    config.ina226_enabled = sensorINA226;
    
    ConfigStorage::save(config);
    Logger::info("Sensor config saved");
}

void MenuSensorConfig::resetToDefaults() {
    sensorFL = ConfigStorage::DEFAULT_CONFIG.sensorFL_enabled;
    sensorFR = ConfigStorage::DEFAULT_CONFIG.sensorFR_enabled;
    sensorRL = ConfigStorage::DEFAULT_CONFIG.sensorRL_enabled;
    sensorRR = ConfigStorage::DEFAULT_CONFIG.sensorRR_enabled;
    sensorINA226 = ConfigStorage::DEFAULT_CONFIG.ina226_enabled;
    
    Logger::info("Sensor config reset to defaults");
}

void MenuSensorConfig::drawToggleButton(int16_t x, int16_t y, int16_t w, int16_t h,
                                          const char* /* label - unused, shows ON/OFF */, bool enabled) {
    uint16_t bgColor = enabled ? COLOR_ENABLED : COLOR_DISABLED;
    uint16_t textColor = enabled ? COLOR_BG : COLOR_TEXT;
    
    tft.fillRoundRect(x, y, w, h, 5, bgColor);
    tft.drawRoundRect(x, y, w, h, 5, COLOR_TEXT);
    
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(textColor, bgColor);
    tft.drawString(enabled ? "ON" : "OFF", x + w/2, y + h/2, 2);
}

void MenuSensorConfig::drawToggleWithLabel(const Button& btn, const char* fullLabel, bool enabled) {
    // Draw label on the left
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(enabled ? COLOR_TEXT : COLOR_INACTIVE, COLOR_BG);
    tft.drawString(fullLabel, 40, btn.y + 10, 2);
    
    // Draw status indicator circle
    int circleX = 25;
    int circleY = btn.y + btn.h/2;
    tft.fillCircle(circleX, circleY, 8, enabled ? COLOR_ENABLED : COLOR_DISABLED);
    tft.drawCircle(circleX, circleY, 8, COLOR_TEXT);
    
    // Draw toggle button
    drawToggleButton(btn.x, btn.y, btn.w, btn.h, btn.label, enabled);
}

void MenuSensorConfig::drawStatusBar() {
    int enabledCount = getEnabledCount();
    int totalCount = 5;  // FL, FR, RL, RR, INA226
    
    // Status bar background
    int barY = 245;
    tft.fillRect(20, barY, 440, 20, COLOR_BG);
    
    // Draw status text
    char statusStr[48];
    snprintf(statusStr, sizeof(statusStr), "Status: %d/%d sensors enabled", enabledCount, totalCount);
    
    // Warning thresholds: green=all, yellow=<4, red=<2
    uint16_t statusColor = COLOR_ENABLED;
    if (enabledCount < 4) statusColor = COLOR_WARNING;  // Show warning earlier (3 or fewer)
    if (enabledCount < 2) statusColor = COLOR_DISABLED;
    
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(statusColor, COLOR_BG);
    tft.drawString(statusStr, 20, barY, 2);
    
    // Warning if too few sensors enabled
    if (enabledCount < 2) {
        tft.setTextColor(COLOR_DISABLED, COLOR_BG);
        tft.drawString("WARNING: Unsafe!", 280, barY, 2);
    }
}

void MenuSensorConfig::drawActionButtons() {
    // Save button
    uint16_t saveColor = (lastSaveTime > 0) ? TFT_BLUE : COLOR_ENABLED;
    const char* saveLabel = (lastSaveTime > 0) ? "SAVED!" : "SAVE";
    
    tft.fillRoundRect(btnSave.x, btnSave.y, btnSave.w, btnSave.h, 5, saveColor);
    tft.drawRoundRect(btnSave.x, btnSave.y, btnSave.w, btnSave.h, 5, COLOR_TEXT);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(COLOR_BG, saveColor);
    tft.drawString(saveLabel, btnSave.x + btnSave.w/2, btnSave.y + btnSave.h/2, 2);
    
    // Reset button
    uint16_t resetColor = (lastResetTime > 0) ? TFT_BLUE : COLOR_WARNING;
    const char* resetLabel = (lastResetTime > 0) ? "RESET!" : "RESET";
    
    tft.fillRoundRect(btnReset.x, btnReset.y, btnReset.w, btnReset.h, 5, resetColor);
    tft.drawRoundRect(btnReset.x, btnReset.y, btnReset.w, btnReset.h, 5, COLOR_TEXT);
    tft.setTextColor(COLOR_BG, resetColor);
    tft.drawString(resetLabel, btnReset.x + btnReset.w/2, btnReset.y + btnReset.h/2, 2);
    
    // Back button
    tft.fillRoundRect(btnBack.x, btnBack.y, btnBack.w, btnBack.h, 5, COLOR_INACTIVE);
    tft.drawRoundRect(btnBack.x, btnBack.y, btnBack.w, btnBack.h, 5, COLOR_TEXT);
    tft.setTextColor(COLOR_TEXT, COLOR_INACTIVE);
    tft.drawString("BACK", btnBack.x + btnBack.w/2, btnBack.y + btnBack.h/2, 2);
}

int MenuSensorConfig::getEnabledCount() {
    int count = 0;
    if (sensorFL) count++;
    if (sensorFR) count++;
    if (sensorRL) count++;
    if (sensorRR) count++;
    if (sensorINA226) count++;
    return count;
}

bool MenuSensorConfig::isTouchInButton(int16_t tx, int16_t ty, const Button& btn) {
    return (tx >= btn.x && tx <= btn.x + btn.w &&
            ty >= btn.y && ty <= btn.y + btn.h);
}
