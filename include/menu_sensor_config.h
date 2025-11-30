#ifndef MENU_SENSOR_CONFIG_H
#define MENU_SENSOR_CONFIG_H

#include <Arduino.h>
#include "config_storage.h"

/**
 * Sensor Enable/Disable Configuration Menu
 * 
 * Allows user to enable/disable individual sensors via UI
 * Useful for emergency bypass of faulty sensors
 * 
 * Features:
 * - Toggle switches for each sensor (FL/FR/RL/RR wheels + INA226)
 * - Visual status indicators (green=enabled, red=disabled)
 * - Emergency mode: continue operation with disabled sensors
 * - Save/Reset/Back buttons with visual feedback
 * - EEPROM persistence via ConfigStorage
 * 
 * UI Layout:
 * ┌─────────────────────────────────────┐
 * │   Sensor Configuration              │
 * ├─────────────────────────────────────┤
 * │ Wheel Sensors:                      │
 * │  [●] Front Left  (FL)  [Toggle]     │
 * │  [●] Front Right (FR)  [Toggle]     │
 * │  [●] Rear Left   (RL)  [Toggle]     │
 * │  [●] Rear Right  (RR)  [Toggle]     │
 * │                                     │
 * │ Current Sensors:                    │
 * │  [●] INA226 Monitors   [Toggle]     │
 * │                                     │
 * │ Status: 5/5 sensors enabled         │
 * │                                     │
 * │ [Save] [Reset] [Back]               │
 * └─────────────────────────────────────┘
 */

class MenuSensorConfig {
public:
    static void init();
    static void update();
    static void draw();
    static void handleTouch(int16_t x, int16_t y);
    
private:
    // Sensor toggle states (loaded from ConfigStorage)
    static bool sensorFL;
    static bool sensorFR;
    static bool sensorRL;
    static bool sensorRR;
    static bool sensorINA226;
    
    // UI state
    static bool needsRedraw;
    static uint32_t lastSaveTime;
    static uint32_t lastResetTime;
    
    // Button definitions
    struct Button {
        int16_t x, y, w, h;
        const char* label;
    };
    
    static const Button btnSensorFL;
    static const Button btnSensorFR;
    static const Button btnSensorRL;
    static const Button btnSensorRR;
    static const Button btnINA226;
    static const Button btnSave;
    static const Button btnReset;
    static const Button btnBack;
    
    // Helper functions
    static void loadConfig();
    static void saveConfig();
    static void resetToDefaults();
    static void drawToggleButton(int16_t x, int16_t y, int16_t w, int16_t h, 
                                   const char* label, bool enabled);
    static void drawToggleWithLabel(const Button& btn, const char* fullLabel, bool enabled);
    static void drawStatusBar();
    static void drawActionButtons();
    static int getEnabledCount();
    static bool isTouchInButton(int16_t tx, int16_t ty, const Button& btn);
};

#endif // MENU_SENSOR_CONFIG_H
