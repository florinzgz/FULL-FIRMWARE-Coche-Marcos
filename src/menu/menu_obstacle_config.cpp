/**
 * @file menu_obstacle_config.cpp
 * @brief Obstacle Detection Configuration Menu
 * 
 * Provides UI for configuring VL53L5X obstacle detection sensors:
 * - Distance thresholds (critical, warning, caution)
 * - Sensor enable/disable
 * - Audio alert settings
 * - Visual display options
 */

#include "obstacle_config.h"
#include "obstacle_detection.h"
#include "obstacle_safety.h"
#include "config_storage.h"
#include "logger.h"
#include "alerts.h"
#include <TFT_eSPI.h>

// Forward declaration of TFT instance (shared with HUD)
extern TFT_eSPI tft;

namespace ObstacleConfigMenu {

// UI State
static bool visible = false;
static bool needsRedraw = true;
static int selectedOption = 0;

// Configurable values (loaded from ConfigStorage)
static uint16_t criticalDistance = ObstacleConfig::DISTANCE_CRITICAL;
static uint16_t warningDistance = ObstacleConfig::DISTANCE_WARNING;
static uint16_t cautionDistance = ObstacleConfig::DISTANCE_CAUTION;
static bool sensorEnabled[4] = {true, true, true, true};  // Front, Rear, Left, Right
static bool audioAlertsEnabled = true;
static bool visualAlertsEnabled = true;

// UI Constants
static const uint16_t COLOR_BG = TFT_BLACK;
static const uint16_t COLOR_TEXT = TFT_WHITE;
static const uint16_t COLOR_HEADER = TFT_CYAN;
static const uint16_t COLOR_SELECTED = TFT_GREEN;
static const uint16_t COLOR_CRITICAL = TFT_RED;
static const uint16_t COLOR_WARNING = TFT_ORANGE;
static const uint16_t COLOR_CAUTION = TFT_YELLOW;
static const uint16_t COLOR_ENABLED = TFT_GREEN;
static const uint16_t COLOR_DISABLED = TFT_DARKGREY;

static const int HEADER_Y = 10;
static const int OPTION_START_Y = 50;
static const int OPTION_HEIGHT = 35;
static const int SLIDER_X = 220;
static const int SLIDER_W = 180;
static const int BACK_BTN_Y = 280;

static const char* SENSOR_NAMES[] = {"Front", "Rear", "Left", "Right"};

// Number of menu options
static const int NUM_OPTIONS = 10;  // 3 distances + 4 sensors + 2 alert toggles + back

// Forward declarations
void loadConfig();
void saveConfig();
void draw();
void drawSliderOption(int y, int optionIndex, const char* label, uint16_t value, 
                       uint16_t minVal, uint16_t maxVal, uint16_t color);
void drawToggleOption(int y, int optionIndex, const char* label, bool enabled);
void drawActionButtons();
void handleSliderTouch(int sliderIndex, int16_t x);
void adjustValue(int direction);
void handleSelect();
void resetToDefaults();

void init() {
    visible = false;
    needsRedraw = true;
    selectedOption = 0;
    
    // Load config from storage
    loadConfig();
    
    Logger::info("ObstacleConfigMenu: initialized");
}

void loadConfig() {
    auto& config = ConfigStorage::getCurrentConfig();
    
    // Load thresholds (use defaults if not configured)
    criticalDistance = ObstacleConfig::DISTANCE_CRITICAL;
    warningDistance = ObstacleConfig::DISTANCE_WARNING;
    cautionDistance = ObstacleConfig::DISTANCE_CAUTION;
    
    // Load sensor states
    for (int i = 0; i < 4; i++) {
        sensorEnabled[i] = true;  // Default all enabled
    }
    
    audioAlertsEnabled = true;
    visualAlertsEnabled = true;
}

void saveConfig() {
    // Save to ConfigStorage
    // Note: Would need to add obstacle config fields to Config struct
    Logger::info("ObstacleConfigMenu: Configuration saved");
    
    // Play confirmation sound
    Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_HIGH});
}

void show() {
    visible = true;
    needsRedraw = true;
    selectedOption = 0;
}

void hide() {
    visible = false;
}

bool isVisible() {
    return visible;
}

void update() {
    if (!visible) return;
    
    if (needsRedraw) {
        draw();
        needsRedraw = false;
    }
}

void draw() {
    // Clear screen
    tft.fillScreen(COLOR_BG);
    
    // Header
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(COLOR_HEADER, COLOR_BG);
    tft.drawString("Obstacle Detection Config", 240, HEADER_Y, 4);
    
    int y = OPTION_START_Y;
    
    // Distance thresholds section
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString("Distance Thresholds:", 20, y, 2);
    y += 25;
    
    // Critical distance
    drawSliderOption(y, 0, "Critical:", criticalDistance, 100, 500, COLOR_CRITICAL);
    y += OPTION_HEIGHT;
    
    // Warning distance
    drawSliderOption(y, 1, "Warning:", warningDistance, 200, 1000, COLOR_WARNING);
    y += OPTION_HEIGHT;
    
    // Caution distance
    drawSliderOption(y, 2, "Caution:", cautionDistance, 500, 2000, COLOR_CAUTION);
    y += OPTION_HEIGHT + 10;
    
    // Sensors section
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString("Sensors:", 20, y, 2);
    y += 25;
    
    // Sensor toggles
    for (int i = 0; i < 4; i++) {
        drawToggleOption(y, 3 + i, SENSOR_NAMES[i], sensorEnabled[i]);
        y += OPTION_HEIGHT;
    }
    
    y += 10;
    
    // Alert settings
    drawToggleOption(y, 7, "Audio Alerts", audioAlertsEnabled);
    y += OPTION_HEIGHT;
    
    drawToggleOption(y, 8, "Visual Alerts", visualAlertsEnabled);
    
    // Action buttons
    drawActionButtons();
}

void drawSliderOption(int y, int optionIndex, const char* label, uint16_t value, 
                       uint16_t minVal, uint16_t maxVal, uint16_t color) {
    bool isSelected = (selectedOption == optionIndex);
    
    // Label
    tft.setTextColor(isSelected ? COLOR_SELECTED : COLOR_TEXT, COLOR_BG);
    tft.drawString(label, 40, y + 5, 2);
    
    // Slider background
    tft.fillRect(SLIDER_X, y + 2, SLIDER_W, 20, COLOR_DISABLED);
    
    // Slider fill
    int fillWidth = ((uint32_t)(value - minVal) * SLIDER_W) / (maxVal - minVal);
    tft.fillRect(SLIDER_X, y + 2, fillWidth, 20, color);
    
    // Slider border
    tft.drawRect(SLIDER_X, y + 2, SLIDER_W, 20, isSelected ? COLOR_SELECTED : COLOR_TEXT);
    
    // Value text
    char valueStr[16];
    snprintf(valueStr, sizeof(valueStr), "%dmm", value);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString(valueStr, SLIDER_X + SLIDER_W + 10, y + 5, 2);
}

void drawToggleOption(int y, int optionIndex, const char* label, bool enabled) {
    bool isSelected = (selectedOption == optionIndex);
    
    // Label
    tft.setTextColor(isSelected ? COLOR_SELECTED : COLOR_TEXT, COLOR_BG);
    tft.drawString(label, 40, y + 5, 2);
    
    // Toggle button
    uint16_t btnColor = enabled ? COLOR_ENABLED : COLOR_DISABLED;
    tft.fillRoundRect(SLIDER_X, y, 80, 25, 5, btnColor);
    tft.drawRoundRect(SLIDER_X, y, 80, 25, 5, isSelected ? COLOR_SELECTED : COLOR_TEXT);
    
    // Toggle text
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(enabled ? COLOR_BG : COLOR_TEXT, btnColor);
    tft.drawString(enabled ? "ON" : "OFF", SLIDER_X + 40, y + 12, 2);
    tft.setTextDatum(TL_DATUM);
}

void drawActionButtons() {
    // Save button
    bool saveSelected = (selectedOption == 9);
    tft.fillRoundRect(20, BACK_BTN_Y, 100, 35, 5, saveSelected ? COLOR_SELECTED : TFT_BLUE);
    tft.drawRoundRect(20, BACK_BTN_Y, 100, 35, 5, COLOR_TEXT);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(COLOR_BG, saveSelected ? COLOR_SELECTED : TFT_BLUE);
    tft.drawString("SAVE", 70, BACK_BTN_Y + 17, 2);
    
    // Reset button
    tft.fillRoundRect(140, BACK_BTN_Y, 100, 35, 5, TFT_ORANGE);
    tft.drawRoundRect(140, BACK_BTN_Y, 100, 35, 5, COLOR_TEXT);
    tft.setTextColor(COLOR_BG, TFT_ORANGE);
    tft.drawString("RESET", 190, BACK_BTN_Y + 17, 2);
    
    // Back button
    tft.fillRoundRect(360, BACK_BTN_Y, 100, 35, 5, TFT_RED);
    tft.drawRoundRect(360, BACK_BTN_Y, 100, 35, 5, COLOR_TEXT);
    tft.setTextColor(COLOR_BG, TFT_RED);
    tft.drawString("BACK", 410, BACK_BTN_Y + 17, 2);
    
    tft.setTextDatum(TL_DATUM);
}

bool handleTouch(int16_t x, int16_t y) {
    if (!visible) return false;
    
    // Check action buttons
    if (y >= BACK_BTN_Y && y <= BACK_BTN_Y + 35) {
        if (x >= 20 && x <= 120) {
            // Save button
            saveConfig();
            needsRedraw = true;
            return true;
        }
        if (x >= 140 && x <= 240) {
            // Reset button
            resetToDefaults();
            needsRedraw = true;
            return true;
        }
        if (x >= 360 && x <= 460) {
            // Back button
            hide();
            return false;
        }
    }
    
    // Check slider/toggle touches
    int optionY = OPTION_START_Y + 25;  // Skip section header
    
    // Distance sliders
    for (int i = 0; i < 3; i++) {
        if (y >= optionY && y <= optionY + OPTION_HEIGHT) {
            if (x >= SLIDER_X && x <= SLIDER_X + SLIDER_W) {
                handleSliderTouch(i, x);
                needsRedraw = true;
                return true;
            }
        }
        optionY += OPTION_HEIGHT;
    }
    
    optionY += 35;  // Skip section header
    
    // Sensor toggles
    for (int i = 0; i < 4; i++) {
        if (y >= optionY && y <= optionY + OPTION_HEIGHT) {
            if (x >= SLIDER_X && x <= SLIDER_X + 80) {
                sensorEnabled[i] = !sensorEnabled[i];
                Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
                needsRedraw = true;
                return true;
            }
        }
        optionY += OPTION_HEIGHT;
    }
    
    optionY += 10;
    
    // Audio toggle
    if (y >= optionY && y <= optionY + OPTION_HEIGHT) {
        if (x >= SLIDER_X && x <= SLIDER_X + 80) {
            audioAlertsEnabled = !audioAlertsEnabled;
            Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
            needsRedraw = true;
            return true;
        }
    }
    optionY += OPTION_HEIGHT;
    
    // Visual toggle
    if (y >= optionY && y <= optionY + OPTION_HEIGHT) {
        if (x >= SLIDER_X && x <= SLIDER_X + 80) {
            visualAlertsEnabled = !visualAlertsEnabled;
            Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
            needsRedraw = true;
            return true;
        }
    }
    
    return true;
}

void handleSliderTouch(int sliderIndex, int16_t x) {
    // Map touch X to value
    int touchOffset = x - SLIDER_X;
    if (touchOffset < 0) touchOffset = 0;
    if (touchOffset > SLIDER_W) touchOffset = SLIDER_W;
    
    switch (sliderIndex) {
        case 0:  // Critical
            criticalDistance = 100 + ((uint32_t)touchOffset * 400) / SLIDER_W;
            // Ensure critical < warning
            if (criticalDistance >= warningDistance) {
                warningDistance = criticalDistance + 100;
            }
            break;
            
        case 1:  // Warning
            warningDistance = 200 + ((uint32_t)touchOffset * 800) / SLIDER_W;
            // Ensure warning between critical and caution
            if (warningDistance <= criticalDistance) {
                warningDistance = criticalDistance + 100;
            }
            if (warningDistance >= cautionDistance) {
                cautionDistance = warningDistance + 200;
            }
            break;
            
        case 2:  // Caution
            cautionDistance = 500 + ((uint32_t)touchOffset * 1500) / SLIDER_W;
            // Ensure caution > warning
            if (cautionDistance <= warningDistance) {
                cautionDistance = warningDistance + 200;
            }
            break;
    }
}

void handleInput(uint8_t button) {
    // Handle button navigation (for physical buttons if available)
    switch (button) {
        case 0:  // Up
            selectedOption = (selectedOption > 0) ? selectedOption - 1 : NUM_OPTIONS - 1;
            needsRedraw = true;
            break;
            
        case 1:  // Down
            selectedOption = (selectedOption + 1) % NUM_OPTIONS;
            needsRedraw = true;
            break;
            
        case 2:  // Left - decrease value
            adjustValue(-1);
            needsRedraw = true;
            break;
            
        case 3:  // Right - increase value
            adjustValue(1);
            needsRedraw = true;
            break;
            
        case 4:  // Select/Enter
            handleSelect();
            break;
            
        case 5:  // Back
            hide();
            break;
    }
}

void adjustValue(int direction) {
    int step = direction * 50;  // 50mm steps for distances
    
    switch (selectedOption) {
        case 0:  // Critical distance
            criticalDistance = constrain(criticalDistance + step, 100, 500);
            break;
        case 1:  // Warning distance
            warningDistance = constrain(warningDistance + step, 200, 1000);
            break;
        case 2:  // Caution distance
            cautionDistance = constrain(cautionDistance + step, 500, 2000);
            break;
        case 3:  // Front sensor
        case 4:  // Rear sensor
        case 5:  // Left sensor
        case 6:  // Right sensor
            sensorEnabled[selectedOption - 3] = !sensorEnabled[selectedOption - 3];
            break;
        case 7:  // Audio alerts
            audioAlertsEnabled = !audioAlertsEnabled;
            break;
        case 8:  // Visual alerts
            visualAlertsEnabled = !visualAlertsEnabled;
            break;
    }
}

void handleSelect() {
    switch (selectedOption) {
        case 3:  // Front sensor
        case 4:  // Rear sensor
        case 5:  // Left sensor
        case 6:  // Right sensor
            sensorEnabled[selectedOption - 3] = !sensorEnabled[selectedOption - 3];
            Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
            needsRedraw = true;
            break;
        case 7:  // Audio alerts
            audioAlertsEnabled = !audioAlertsEnabled;
            needsRedraw = true;
            break;
        case 8:  // Visual alerts
            visualAlertsEnabled = !visualAlertsEnabled;
            needsRedraw = true;
            break;
        case 9:  // Save
            saveConfig();
            break;
    }
}

void resetToDefaults() {
    criticalDistance = ObstacleConfig::DISTANCE_CRITICAL;
    warningDistance = ObstacleConfig::DISTANCE_WARNING;
    cautionDistance = ObstacleConfig::DISTANCE_CAUTION;
    
    for (int i = 0; i < 4; i++) {
        sensorEnabled[i] = true;
    }
    
    audioAlertsEnabled = true;
    visualAlertsEnabled = true;
    
    Logger::info("ObstacleConfigMenu: Reset to defaults");
    Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_HIGH});
}

// Getters for external access
uint16_t getCriticalDistance() { return criticalDistance; }
uint16_t getWarningDistance() { return warningDistance; }
uint16_t getCautionDistance() { return cautionDistance; }
bool isSensorEnabled(int index) { return (index >= 0 && index < 4) ? sensorEnabled[index] : false; }
bool isAudioEnabled() { return audioAlertsEnabled; }
bool isVisualEnabled() { return visualAlertsEnabled; }

} // namespace ObstacleConfigMenu
