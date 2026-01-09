/**
 * @file menu_obstacle_config.cpp
 * @brief Obstacle Detection Configuration Menu
 * 
 * v2.12.0: Actualizado para TOFSense-M S (sensor único frontal)
 * Configuración del sensor TOFSense-M S:
 * - Umbrales de distancia (critical, warning, caution)
 * - Activación/desactivación del sensor frontal
 * - Opciones de alerta sonora y visual
 * - Persistencia real en EEPROM
 */

#include "obstacle_config.h"
#include "obstacle_detection.h"
#include "obstacle_safety.h"
#include "config_storage.h"
#include "logger.h"
#include "alerts.h"
#include <TFT_eSPI.h>

extern TFT_eSPI tft;

namespace ObstacleConfigMenu {

// v2.12.0: Solo un sensor de obstáculos (Front)
static const int kNumObstacles = 1;
static const char* SENSOR_NAMES[kNumObstacles] = {"Front"};

// UI State
static bool visible = false;
static bool needsRedraw = true;
static int selectedOption = 0;

// Config (cargados desde ConfigStorage)
static uint16_t criticalDistance = ObstacleConfig::DISTANCE_CRITICAL;
static uint16_t warningDistance  = ObstacleConfig::DISTANCE_WARNING;
static uint16_t cautionDistance  = ObstacleConfig::DISTANCE_CAUTION;
static bool sensorEnabled[kNumObstacles]   = {true};    // Solo Front
static bool audioAlertsEnabled  = true;
static bool visualAlertsEnabled = true;

// UI Constants
static const uint16_t COLOR_BG        = TFT_BLACK;
static const uint16_t COLOR_TEXT      = TFT_WHITE;
static const uint16_t COLOR_HEADER    = TFT_CYAN;
static const uint16_t COLOR_SELECTED  = TFT_GREEN;
static const uint16_t COLOR_CRITICAL  = TFT_RED;
static const uint16_t COLOR_WARNING   = TFT_ORANGE;
static const uint16_t COLOR_CAUTION   = TFT_YELLOW;
static const uint16_t COLOR_ENABLED   = TFT_GREEN;
static const uint16_t COLOR_DISABLED  = TFT_DARKGREY;

static const int HEADER_Y      = 10;
static const int OPTION_START_Y= 50;
static const int OPTION_HEIGHT = 35;
static const int SLIDER_X      = 220;
static const int SLIDER_W      = 180;
static const int BACK_BTN_Y    = 280;

// Número de opciones en el menú (3 distancias + 1 sensor + 2 toggles + botón Save)
static const int NUM_OPTIONS = 6;

// Forward declarations
void loadConfig();
void saveConfig();
void draw();
void drawSliderOption(int y, int optionIndex, const char* label, uint16_t value, uint16_t minVal, uint16_t maxVal, uint16_t color);
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
    loadConfig();
    Logger::info("ObstacleConfigMenu: initialized (v2.12.0 - Single sensor)");
}

void loadConfig() {
    // 🔒 v2.13.1: Load obstacle config from ConfigStorage
    // Note: System has 1 front sensor (TOFSense-M S). Array supports future expansion.
    ConfigStorage::init();
    auto& cfg = ConfigStorage::getCurrentConfig();
    
    criticalDistance = cfg.obstacle_critical_distance;
    warningDistance  = cfg.obstacle_warning_distance;
    cautionDistance  = cfg.obstacle_caution_distance;
    
    // Load front sensor config (index 0)
    // Future: If multiple sensors added, loop through kNumObstacles
    sensorEnabled[0] = cfg.obstacle_sensor_enabled;
    
    audioAlertsEnabled  = cfg.obstacle_audio_alerts;
    visualAlertsEnabled = cfg.obstacle_visual_alerts;
    
    Logger::info("ObstacleConfigMenu: Configuration loaded from persistent storage");
}

void saveConfig() {
    // 🔒 v2.13.1: Save obstacle config to persistent storage
    // Note: System has 1 front sensor (TOFSense-M S). Array supports future expansion.
    ConfigStorage::init();
    auto& cfg = ConfigStorage::getCurrentConfig();
    
    cfg.obstacle_critical_distance = criticalDistance;
    cfg.obstacle_warning_distance  = warningDistance;
    cfg.obstacle_caution_distance  = cautionDistance;
    
    // Save front sensor config (index 0)
    // Future: If multiple sensors added, loop through kNumObstacles
    cfg.obstacle_sensor_enabled = sensorEnabled[0];
    
    cfg.obstacle_audio_alerts   = audioAlertsEnabled;
    cfg.obstacle_visual_alerts  = visualAlertsEnabled;
    
    if (ConfigStorage::save(cfg)) {
        Logger::info("ObstacleConfigMenu: Configuration saved to persistent storage");
        Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_HIGH});
    } else {
        Logger::error("ObstacleConfigMenu: Failed to save configuration");
        Alerts::play({Audio::AUDIO_ERROR, Audio::Priority::PRIO_HIGH});
    }
}

void show() {
    visible = true;
    needsRedraw = true;
    selectedOption = 0;
    loadConfig();
}

void hide() {
    saveConfig(); // Guarda siempre al salir
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
    tft.fillScreen(COLOR_BG);
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(COLOR_HEADER, COLOR_BG);
    tft.drawString("Obstacle Detection Config", 240, HEADER_Y, 4);
    
    // v2.13.0: TOFSense-M S 8x8 matrix subtitle
    tft.setTextColor(TFT_DARKGREY, COLOR_BG);
    tft.drawString("TOFSense-M S (8x8 Matrix, 4m Range)", 240, HEADER_Y + 25, 2);

    int y = OPTION_START_Y;

    // Umbrales de distancia
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString("Distance Thresholds:", 20, y, 2);
    y += 25;

    drawSliderOption(y, 0, "Critical:", criticalDistance, 100, 500, COLOR_CRITICAL);
    y += OPTION_HEIGHT;
    drawSliderOption(y, 1, "Warning:", warningDistance, 200, 1000, COLOR_WARNING);
    y += OPTION_HEIGHT;
    drawSliderOption(y, 2, "Caution:", cautionDistance, 500, 2000, COLOR_CAUTION);
    y += OPTION_HEIGHT + 10;

    // Sensor (solo Front)
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString("Sensor:", 20, y, 2);
    y += 25;
    drawToggleOption(y, 3, SENSOR_NAMES[0], sensorEnabled[0]);
    y += OPTION_HEIGHT + 10;

    // Alertas
    drawToggleOption(y, 4, "Audio Alerts", audioAlertsEnabled);
    y += OPTION_HEIGHT;
    drawToggleOption(y, 5, "Visual Alerts", visualAlertsEnabled);

    // Action buttons
    drawActionButtons();
}

void drawSliderOption(int y, int optionIndex, const char* label, uint16_t value, uint16_t minVal, uint16_t maxVal, uint16_t color) {
    bool isSelected = (selectedOption == optionIndex);
    tft.setTextColor(isSelected ? COLOR_SELECTED : COLOR_TEXT, COLOR_BG);
    tft.drawString(label, 40, y + 5, 2);
    tft.fillRect(SLIDER_X, y + 2, SLIDER_W, 20, COLOR_DISABLED);
    int fillWidth = ((uint32_t)(value - minVal) * SLIDER_W) / (maxVal - minVal);
    tft.fillRect(SLIDER_X, y + 2, fillWidth, 20, color);
    tft.drawRect(SLIDER_X, y + 2, SLIDER_W, 20, isSelected ? COLOR_SELECTED : COLOR_TEXT);
    char valueStr[16];
    snprintf(valueStr, sizeof(valueStr), "%dmm", value);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString(valueStr, SLIDER_X + SLIDER_W + 10, y + 5, 2);
}

void drawToggleOption(int y, int optionIndex, const char* label, bool enabled) {
    bool isSelected = (selectedOption == optionIndex);
    tft.setTextColor(isSelected ? COLOR_SELECTED : COLOR_TEXT, COLOR_BG);
    tft.drawString(label, 40, y + 5, 2);
    uint16_t btnColor = enabled ? COLOR_ENABLED : COLOR_DISABLED;
    tft.fillRoundRect(SLIDER_X, y, 80, 25, 5, btnColor);
    tft.drawRoundRect(SLIDER_X, y, 80, 25, 5, isSelected ? COLOR_SELECTED : COLOR_TEXT);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(enabled ? COLOR_BG : COLOR_TEXT, btnColor);
    tft.drawString(enabled ? "ON" : "OFF", SLIDER_X + 40, y + 12, 2);
    tft.setTextDatum(TL_DATUM);
}

void drawActionButtons() {
    bool saveSelected = (selectedOption == NUM_OPTIONS);
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
    if (y >= BACK_BTN_Y && y <= BACK_BTN_Y + 35) {
        if (x >= 20 && x <= 120) { saveConfig(); needsRedraw = true; return true; }
        if (x >= 140 && x <= 240) { resetToDefaults(); needsRedraw = true; return true; }
        if (x >= 360 && x <= 460) { hide(); return false; }
    }
    int optionY = OPTION_START_Y + 25;
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
    optionY += 35; // sensores
    for (int i = 0; i < kNumObstacles; i++) {
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
    // Toggle audio
    if (y >= optionY && y <= optionY + OPTION_HEIGHT) {
        if (x >= SLIDER_X && x <= SLIDER_X + 80) {
            audioAlertsEnabled = !audioAlertsEnabled;
            Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
            needsRedraw = true;
            return true;
        }
    }
    optionY += OPTION_HEIGHT;
    // Toggle visual
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
    int touchOffset = x - SLIDER_X;
    if (touchOffset < 0) touchOffset = 0;
    if (touchOffset > SLIDER_W) touchOffset = SLIDER_W;
    switch (sliderIndex) {
        case 0:
            criticalDistance = 100 + ((uint32_t)touchOffset * 400) / SLIDER_W;
            if (criticalDistance >= warningDistance) warningDistance = criticalDistance + 100;
            break;
        case 1:
            warningDistance = 200 + ((uint32_t)touchOffset * 800) / SLIDER_W;
            if (warningDistance <= criticalDistance) warningDistance = criticalDistance + 100;
            if (warningDistance >= cautionDistance) cautionDistance = warningDistance + 200;
            break;
        case 2:
            cautionDistance = 500 + ((uint32_t)touchOffset * 1500) / SLIDER_W;
            if (cautionDistance <= warningDistance) cautionDistance = warningDistance + 200;
            break;
    }
}

void handleInput(uint8_t button) {
    switch (button) {
        case 0: selectedOption = (selectedOption > 0) ? selectedOption - 1 : NUM_OPTIONS; needsRedraw = true; break;
        case 1: selectedOption = (selectedOption + 1) % (NUM_OPTIONS + 1); needsRedraw = true; break;
        case 2: adjustValue(-1); needsRedraw = true; break;
        case 3: adjustValue(1); needsRedraw = true; break;
        case 4: handleSelect(); break;
        case 5: hide(); break;
    }
}

void adjustValue(int direction) {
    int step = direction * 50;
    switch (selectedOption) {
        case 0: criticalDistance = constrain(criticalDistance + step, 100, 500); break;
        case 1: warningDistance  = constrain(warningDistance  + step, 200, 1000); break;
        case 2: cautionDistance  = constrain(cautionDistance  + step, 500, 2000); break;
        case 3: sensorEnabled[0] = !sensorEnabled[0]; break;  // Solo Front sensor
        case 4: audioAlertsEnabled = !audioAlertsEnabled; break;
        case 5: visualAlertsEnabled = !visualAlertsEnabled; break;
    }
}

void handleSelect() {
    switch (selectedOption) {
        case 3:  // Solo Front sensor
            sensorEnabled[0] = !sensorEnabled[0];
            Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
            needsRedraw = true;
            break;
        case 4: audioAlertsEnabled = !audioAlertsEnabled; needsRedraw = true; break;
        case 5: visualAlertsEnabled = !visualAlertsEnabled; needsRedraw = true; break;
        case 6: saveConfig(); break;  // Save button is now option 6, not 7
    }
}

void resetToDefaults() {
    criticalDistance = ObstacleConfig::DISTANCE_CRITICAL;
    warningDistance  = ObstacleConfig::DISTANCE_WARNING;
    cautionDistance  = ObstacleConfig::DISTANCE_CAUTION;
    for (int i = 0; i < kNumObstacles; i++) {
        sensorEnabled[i] = true;
    }
    audioAlertsEnabled  = true;
    visualAlertsEnabled = true;
    Logger::info("ObstacleConfigMenu: Reset to defaults");
    Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_HIGH});
    saveConfig();
}

// Getters
uint16_t getCriticalDistance() { return criticalDistance; }
uint16_t getWarningDistance()  { return warningDistance; }
uint16_t getCautionDistance()  { return cautionDistance; }
bool isSensorEnabled(int index) { return (index >=0 && index < kNumObstacles) ? sensorEnabled[index] : false; }
bool isAudioEnabled()          { return audioAlertsEnabled; }
bool isVisualEnabled()         { return visualAlertsEnabled; }

} // namespace ObstacleConfigMenu