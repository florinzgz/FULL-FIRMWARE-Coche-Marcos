#include "menu_hidden.h"
#include "storage.h"
#include "logger.h"
#include "alerts.h"
#include "pedal.h"
#include "steering.h"
#include "buttons.h"
#include "settings.h"
#include "system.h"
#include "error_codes.h"  // 🆕 v2.9.5: Descripciones de códigos de error
#include <TFT_eSPI.h>
// 🔒 v2.8.8: Eliminada librería XPT2046_Touchscreen separada
// Ahora usamos el touch integrado de TFT_eSPI
#include "pins.h"
#include "touch_map.h"  // 🔒 v2.8.3: Constantes centralizadas de calibración táctil
#include "touch_calibration.h"  // 🔒 v2.9.0: Touch calibration routine

static TFT_eSPI *tft = nullptr;
// 🔒 v2.8.8: Ya no necesitamos puntero separado al touch

// 🔒 CRITICAL: Helper function to ensure displayBrightness is never corrupted before saving
// Stack overflow or memory corruption could set brightness to 0, causing black screen
// This function validates brightness before every save to prevent permanent corruption
// Note: Only validates displayBrightness. Other fields are assumed correct from calibration functions.
static void safeSaveConfig() {
    // Validate displayBrightness before saving (valid range: 1-255)
    // Using same validation logic as main.cpp and hud_manager.cpp
    if (cfg.displayBrightness < 1 || cfg.displayBrightness > 255) {
        Logger::warnf("MenuHidden: displayBrightness corrupted (%d), restoring to default (%d)", 
                      cfg.displayBrightness, DISPLAY_BRIGHTNESS_DEFAULT);
        cfg.displayBrightness = DISPLAY_BRIGHTNESS_DEFAULT;
    }
    // Perform actual save to EEPROM with validated brightness value
    Storage::save(cfg);
}

static bool menuActive = false;
static uint16_t codeBuffer = 0;
static const uint16_t accessCode = 8989;

// Numeric keypad state for code entry
static bool numpadActive = false;
static uint32_t lastKeypadTouch = 0;
static const uint32_t KEYPAD_DEBOUNCE_MS = 300;  // Debounce for keypad buttons
static uint32_t wrongCodeDisplayStart = 0;  // For non-blocking error display

// Module configuration state
static bool modulesConfigFirstCall = true;
static bool regenAdjustFirstCall = true;  // 🔒 v2.10.0: Track first draw for regen adjust
static bool calibrationFirstCall = true;  // 🔒 v2.10.0: Track first draw for pedal/encoder calibration

static int selectedOption = 1;   // opción seleccionada (1..8)

// Cache para evitar redibujos innecesarios
static int lastSelectedOption = -1;
static uint16_t lastCodeBuffer = 0;
static bool lastMenuActive = false;

// Estado de calibración interactiva
enum class CalibrationState {
    NONE,
    PEDAL_MIN,
    PEDAL_MAX,
    PEDAL_DONE,
    ENCODER_CENTER,
    ENCODER_DONE,
    TOUCH_CALIBRATION,  // 🔒 v2.9.0: Touch screen calibration
    REGEN_ADJUST,       // ✅ v2.7.0: Ajuste interactivo de regen
    MODULES_CONFIG,     // Configuración de módulos ON/OFF
    CLEAR_ERRORS_CONFIRM // ✅ v2.7.0: Confirmación borrado errores
};
static CalibrationState calibState = CalibrationState::NONE;
static int pedalCalibMin = 0;
static int pedalCalibMax = 4095;
static uint32_t calibStartMs = 0;
static const uint32_t CALIB_TIMEOUT_MS = 30000;  // 30 segundos timeout
static int regenAdjustValue = 0;  // ✅ v2.7.0: Valor temporal para ajuste regen

// Constantes de debounce y timing (touch calibration centralizada en touch_map.h)
static const uint32_t DEBOUNCE_TIMEOUT_MS = 500;  // Timeout para debounce
static const uint32_t DEBOUNCE_SHORT_MS = 200;    // ✅ v2.7.0: Debounce corto para ajustes rápidos
static const uint32_t FEEDBACK_DISPLAY_MS = 1500; // ✅ v2.7.0: Tiempo de visualización de feedback

// Zonas táctiles del menú (9 opciones)
static const int MENU_X1 = 60;
static const int MENU_Y1 = 80;
static const int MENU_WIDTH = 360;
static const int MENU_ITEM_HEIGHT = 20;
static const int NUM_MENU_ITEMS = 9;

// Opciones del menú (evitar duplicación - DRY)
static const char* const MENU_ITEMS[NUM_MENU_ITEMS] = {
    "1) Calibrar pedal",
    "2) Calibrar encoder",
    "3) Calibrar touch",      // 🔒 v2.9.0: Nueva opción
    "4) Ajuste regen (%)",
    "5) Modulos/Sensores",
    "6) Guardar y salir",
    "7) Restaurar fabrica",
    "8) Ver errores",
    "9) Borrar errores"
};

// 🔒 v2.8.8: Helper para debounce con timeout (usando touch integrado TFT_eSPI)
static void waitTouchRelease(uint32_t maxWaitMs = DEBOUNCE_TIMEOUT_MS) {
    if (tft == nullptr) return;
    uint32_t startMs = millis();
    uint16_t tx, ty;
    while (tft->getTouch(&tx, &ty) && (millis() - startMs < maxWaitMs)) {
        yield();
    }
}

// -----------------------
// Funciones auxiliares de calibración real
// -----------------------

static void drawCalibrationScreen(const char* title, const char* instruction, const char* value) {
    if (tft == nullptr) return;  // Guard contra puntero nulo
    
    // 🔒 v2.10.0: Full screen clear on first call to prevent gauge ghosting
    if (calibrationFirstCall) {
        tft->fillScreen(TFT_BLACK);
        calibrationFirstCall = false;
    } else {
        tft->fillRect(60, 40, 360, 240, TFT_BLACK);
    }
    
    tft->drawRect(60, 40, 360, 240, TFT_CYAN);
    
    tft->setTextDatum(TC_DATUM);
    tft->setTextColor(TFT_CYAN, TFT_BLACK);
    tft->drawString(title, 240, 50, 4);
    
    tft->setTextDatum(MC_DATUM);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->drawString(instruction, 240, 130, 2);
    
    tft->setTextColor(TFT_GREEN, TFT_BLACK);
    tft->drawString(value, 240, 180, 4);
    
    // Barra de progreso de tiempo
    uint32_t elapsed = millis() - calibStartMs;
    int progress = constrain(elapsed * 200 / CALIB_TIMEOUT_MS, 0, 200);
    tft->fillRect(140, 220, progress, 10, TFT_DARKGREY);
    tft->drawRect(140, 220, 200, 10, TFT_WHITE);
    
    tft->setTextDatum(MC_DATUM);
    tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    tft->drawString("Toca pantalla para confirmar", 240, 260, 2);
}

static void startPedalCalibration() {
    calibState = CalibrationState::PEDAL_MIN;
    calibStartMs = millis();
    pedalCalibMin = 0;
    pedalCalibMax = 4095;
    calibrationFirstCall = true;  // 🔒 v2.10.0: Reset for full screen clear on first draw
    Logger::info("Iniciando calibración de pedal - fase MIN");
    Alerts::play({Audio::AUDIO_CAL_PEDAL, Audio::Priority::PRIO_HIGH});
}

static void startEncoderCalibration() {
    calibState = CalibrationState::ENCODER_CENTER;
    calibStartMs = millis();
    calibrationFirstCall = true;  // 🔒 v2.10.0: Reset for full screen clear on first draw
    Logger::info("Iniciando calibración de encoder - centrando");
    Alerts::play({Audio::AUDIO_CAL_ENCODER, Audio::Priority::PRIO_HIGH});
}

// 🔒 v2.9.0: Touch screen calibration
static void startTouchCalibration() {
    calibState = CalibrationState::TOUCH_CALIBRATION;
    calibStartMs = millis();
    Logger::info("Iniciando calibración de touch screen");
    Alerts::play({Audio::AUDIO_CAL_ENCODER, Audio::Priority::PRIO_HIGH});  // Reuse encoder sound
    
    // Initialize and start touch calibration
    TouchCalibration::init(tft);
    TouchCalibration::start();
}

static void updatePedalCalibration(bool touched) {
    auto pedal = Pedal::get();
    char valueStr[32];
    
    if (calibState == CalibrationState::PEDAL_MIN) {
        snprintf(valueStr, sizeof(valueStr), "ADC: %d", pedal.raw);
        drawCalibrationScreen("CALIBRAR PEDAL", "Suelta el pedal (mínimo)", valueStr);
        
        // Actualizar mínimo continuamente
        if (pedal.raw > 0) {
            pedalCalibMin = pedal.raw;
        }
        
        if (touched) {
            Logger::infof("Pedal MIN calibrado: %d", pedalCalibMin);
            calibState = CalibrationState::PEDAL_MAX;
            calibStartMs = millis();
            Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
        }
    }
    else if (calibState == CalibrationState::PEDAL_MAX) {
        snprintf(valueStr, sizeof(valueStr), "ADC: %d", pedal.raw);
        drawCalibrationScreen("CALIBRAR PEDAL", "Pisa el pedal al máximo", valueStr);
        
        // Actualizar máximo continuamente
        if (pedal.raw > pedalCalibMin + 100) {
            pedalCalibMax = pedal.raw;
        }
        
        if (touched) {
            Logger::infof("Pedal MAX calibrado: %d", pedalCalibMax);
            calibState = CalibrationState::PEDAL_DONE;
        }
    }
    else if (calibState == CalibrationState::PEDAL_DONE) {
        // Validar calibración
        if (pedalCalibMax >= pedalCalibMin + 500) {
            // Guardar calibración
            cfg.pedalMin = pedalCalibMin;
            cfg.pedalMax = pedalCalibMax;
            Pedal::setCalibration(pedalCalibMin, pedalCalibMax, cfg.pedalCurve);
            safeSaveConfig();
            
            Logger::infof("Calibración pedal guardada: %d - %d", pedalCalibMin, pedalCalibMax);
            Alerts::play({Audio::AUDIO_PEDAL_OK, Audio::Priority::PRIO_HIGH});
            
            tft->fillRect(60, 40, 360, 240, TFT_BLACK);
            tft->drawRect(60, 40, 360, 240, TFT_GREEN);
            tft->setTextDatum(MC_DATUM);
            tft->setTextColor(TFT_GREEN, TFT_BLACK);
            tft->drawString("CALIBRACION OK", 240, 140, 4);
            
            snprintf(valueStr, sizeof(valueStr), "MIN:%d MAX:%d", pedalCalibMin, pedalCalibMax);
            tft->drawString(valueStr, 240, 180, 2);
            
            // Esperar un momento
            uint32_t waitStart = millis();
            while (millis() - waitStart < FEEDBACK_DISPLAY_MS) { yield(); }
        } else {
            // Calibración inválida
            Logger::error("Calibración pedal fallida - rango insuficiente");
            Alerts::play({Audio::AUDIO_PEDAL_ERROR, Audio::Priority::PRIO_HIGH});
            
            tft->fillRect(60, 40, 360, 240, TFT_BLACK);
            tft->drawRect(60, 40, 360, 240, TFT_RED);
            tft->setTextDatum(MC_DATUM);
            tft->setTextColor(TFT_RED, TFT_BLACK);
            tft->drawString("CALIBRACION FALLIDA", 240, 140, 4);
            tft->setTextColor(TFT_WHITE, TFT_BLACK);
            tft->drawString("Rango insuficiente", 240, 180, 2);
            
            uint32_t waitStart = millis();
            while (millis() - waitStart < 2000) { yield(); }
        }
        
        // 🔒 v2.10.0: Clear screen when exiting pedal calibration
        if (tft != nullptr) {
            tft->fillScreen(TFT_BLACK);
        }
        
        calibState = CalibrationState::NONE;
    }
    
    // Timeout
    if (millis() - calibStartMs > CALIB_TIMEOUT_MS) {
        Logger::warn("Calibración pedal timeout");
        
        // 🔒 v2.10.0: Clear screen on timeout
        if (tft != nullptr) {
            tft->fillScreen(TFT_BLACK);
        }
        
        calibState = CalibrationState::NONE;
        Alerts::play({Audio::AUDIO_PEDAL_ERROR, Audio::Priority::PRIO_NORMAL});
    }
}

static void updateEncoderCalibration(bool touched) {
    auto steer = Steering::get();
    char valueStr[32];
    
    if (calibState == CalibrationState::ENCODER_CENTER) {
        snprintf(valueStr, sizeof(valueStr), "Ticks: %ld", steer.ticks);
        drawCalibrationScreen("CALIBRAR VOLANTE", "Centra el volante y confirma", valueStr);
        
        if (touched) {
            // Centrar el encoder
            Steering::center();
            cfg.steerZeroOffset = Steering::getZeroOffset();
            safeSaveConfig();
            
            Logger::infof("Encoder centrado. Offset: %ld", cfg.steerZeroOffset);
            calibState = CalibrationState::ENCODER_DONE;
        }
    }
    else if (calibState == CalibrationState::ENCODER_DONE) {
        Alerts::play({Audio::AUDIO_ENCODER_OK, Audio::Priority::PRIO_HIGH});
        
        tft->fillRect(60, 40, 360, 240, TFT_BLACK);
        tft->drawRect(60, 40, 360, 240, TFT_GREEN);
        tft->setTextDatum(MC_DATUM);
        tft->setTextColor(TFT_GREEN, TFT_BLACK);
        tft->drawString("VOLANTE CENTRADO", 240, 140, 4);
        
        snprintf(valueStr, sizeof(valueStr), "Offset: %ld", cfg.steerZeroOffset);
        tft->drawString(valueStr, 240, 180, 2);
        
        uint32_t waitStart = millis();
        while (millis() - waitStart < FEEDBACK_DISPLAY_MS) { yield(); }
        
        // 🔒 v2.10.0: Clear screen when exiting encoder calibration
        if (tft != nullptr) {
            tft->fillScreen(TFT_BLACK);
        }
        
        calibState = CalibrationState::NONE;
    }
    
    // Timeout
    if (millis() - calibStartMs > CALIB_TIMEOUT_MS) {
        Logger::warn("Calibración encoder timeout");
        
        // 🔒 v2.10.0: Clear screen on timeout
        if (tft != nullptr) {
            tft->fillScreen(TFT_BLACK);
        }
        
        calibState = CalibrationState::NONE;
        Alerts::play({Audio::AUDIO_ENCODER_ERROR, Audio::Priority::PRIO_NORMAL});
    }
}

// ✅ v2.7.0: Iniciar ajuste interactivo de regeneración
static void startRegenAdjust() {
    calibState = CalibrationState::REGEN_ADJUST;
    calibStartMs = millis();
    regenAdjustValue = cfg.regenPercent;  // Cargar valor actual
    regenAdjustFirstCall = true;  // 🔒 v2.10.0: Reset for full screen clear on first draw
    Logger::info("Iniciando ajuste de regen - usa touch para ajustar");
    Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
}

// ✅ v2.7.0: Dibujar pantalla de ajuste de regen
static void drawRegenAdjustScreen() {
    if (tft == nullptr) return;
    
    // 🔒 v2.10.0: Full screen clear on first call to prevent gauge ghosting
    // Subsequent redraws only clear the menu area to reduce flicker
    if (regenAdjustFirstCall) {
        tft->fillScreen(TFT_BLACK);
        regenAdjustFirstCall = false;
    } else {
        tft->fillRect(60, 40, 360, 240, TFT_BLACK);
    }
    
    tft->drawRect(60, 40, 360, 240, TFT_CYAN);
    
    tft->setTextDatum(TC_DATUM);
    tft->setTextColor(TFT_CYAN, TFT_BLACK);
    tft->drawString("AJUSTE REGENERACION", 240, 50, 4);
    
    // Valor actual grande
    char valueStr[16];
    snprintf(valueStr, sizeof(valueStr), "%d%%", regenAdjustValue);
    tft->setTextDatum(MC_DATUM);
    tft->setTextColor(TFT_GREEN, TFT_BLACK);
    tft->drawString(valueStr, 240, 120, 6);
    
    // Barra visual del valor
    int barWidth = (regenAdjustValue * 280) / 100;
    tft->fillRect(100, 170, 280, 20, TFT_DARKGREY);
    tft->fillRect(100, 170, barWidth, 20, TFT_GREEN);
    tft->drawRect(100, 170, 280, 20, TFT_WHITE);
    
    // Botones [-] y [+]
    tft->fillRect(80, 200, 60, 40, TFT_RED);
    tft->drawRect(80, 200, 60, 40, TFT_WHITE);
    tft->setTextColor(TFT_WHITE, TFT_RED);
    tft->drawString("-10", 110, 220, 2);
    
    tft->fillRect(340, 200, 60, 40, TFT_GREEN);
    tft->drawRect(340, 200, 60, 40, TFT_WHITE);
    tft->setTextColor(TFT_BLACK, TFT_GREEN);
    tft->drawString("+10", 370, 220, 2);
    
    // Botón Guardar
    tft->fillRect(180, 200, 120, 40, TFT_BLUE);
    tft->drawRect(180, 200, 120, 40, TFT_WHITE);
    tft->setTextColor(TFT_WHITE, TFT_BLUE);
    tft->drawString("GUARDAR", 240, 220, 2);
    
    // Instrucciones
    tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    tft->drawString("Toca [-] o [+] para ajustar", 240, 260, 2);
}

// ✅ v2.7.0: Actualizar ajuste de regen
static void updateRegenAdjust(int touchX, int touchY, bool touched) {
    if (!touched) {
        drawRegenAdjustScreen();
        return;
    }
    
    bool needsRedraw = false;
    
    // Detectar toque en botón [-] (disminuir 10%)
    if (touchX >= 80 && touchX <= 140 && touchY >= 200 && touchY <= 240) {
        regenAdjustValue = constrain(regenAdjustValue - 10, 0, 100);
        Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
        needsRedraw = true;
    }
    // Detectar toque en botón [+] (aumentar 10%)
    else if (touchX >= 340 && touchX <= 400 && touchY >= 200 && touchY <= 240) {
        regenAdjustValue = constrain(regenAdjustValue + 10, 0, 100);
        Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
        needsRedraw = true;
    }
    // Detectar toque en barra (ajuste directo)
    else if (touchX >= 100 && touchX <= 380 && touchY >= 170 && touchY <= 190) {
        regenAdjustValue = constrain(((touchX - 100) * 100) / 280, 0, 100);
        needsRedraw = true;
    }
    // Detectar toque en botón GUARDAR
    else if (touchX >= 180 && touchX <= 300 && touchY >= 200 && touchY <= 240) {
        cfg.regenPercent = regenAdjustValue;
        safeSaveConfig();
        Logger::infof("Ajuste de regen guardado: %d%%", cfg.regenPercent);
        Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_HIGH});
        
        // Mostrar confirmación
        tft->fillRect(60, 40, 360, 240, TFT_BLACK);
        tft->drawRect(60, 40, 360, 240, TFT_GREEN);
        tft->setTextDatum(MC_DATUM);
        tft->setTextColor(TFT_GREEN, TFT_BLACK);
        tft->drawString("REGEN GUARDADO", 240, 130, 4);
        
        char valueStr[16];
        snprintf(valueStr, sizeof(valueStr), "%d%%", regenAdjustValue);
        tft->drawString(valueStr, 240, 170, 4);
        
        uint32_t waitStart = millis();
        while (millis() - waitStart < FEEDBACK_DISPLAY_MS) { yield(); }
        
        // 🔒 v2.10.0: Clear screen when exiting regen adjust
        if (tft != nullptr) {
            tft->fillScreen(TFT_BLACK);
        }
        
        calibState = CalibrationState::NONE;
        return;
    }
    
    if (needsRedraw) {
        waitTouchRelease(DEBOUNCE_SHORT_MS);  // Debounce corto para permitir ajuste rápido
        drawRegenAdjustScreen();
    }
    
    // Timeout
    if (millis() - calibStartMs > CALIB_TIMEOUT_MS) {
        Logger::warn("Ajuste regen timeout - cancelado");
        calibState = CalibrationState::NONE;
    }
}

// v2.14.0: Simplified - lights and media removed
static void applyModules(bool traction) {
    cfg.tractionEnabled = traction;
    safeSaveConfig();
    Logger::infof("Módulos guardados: traction=%d", traction);
    Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
}

// Module configuration screen functions
// Track module states locally until save
// v2.14.0: Removed lightsEnabled and multimediaEnabled
struct ModuleToggleState {
    bool tractionEnabled;
    bool currentSensorsEnabled;
    bool tempSensorsEnabled;
    bool wheelSensorsEnabled;
};
static ModuleToggleState tempModulesState;

static void syncModulesStateFromConfig() {
    // v2.14.0: Simplified
    tempModulesState.tractionEnabled = cfg.tractionEnabled;
    tempModulesState.currentSensorsEnabled = cfg.currentSensorsEnabled;
    tempModulesState.tempSensorsEnabled = cfg.tempSensorsEnabled;
    tempModulesState.wheelSensorsEnabled = cfg.wheelSensorsEnabled;
}

static void drawModulesConfigScreen() {
    if (tft == nullptr) return;
    
    // 🔒 v2.10.0: Full screen clear on first call to prevent gauge ghosting
    // Subsequent redraws only clear the menu area to reduce flicker
    if (modulesConfigFirstCall) {
        tft->fillScreen(TFT_BLACK);
        modulesConfigFirstCall = false;
    } else {
        tft->fillRect(60, 40, 360, 240, TFT_BLACK);
    }
    
    tft->drawRect(60, 40, 360, 240, TFT_CYAN);
    
    tft->setTextDatum(TC_DATUM);
    tft->setTextColor(TFT_CYAN, TFT_BLACK);
    tft->drawString("CONFIG. MÓDULOS Y SENSORES", 240, 50, 4);

    // Layout
    const int btnW = 140;
    const int btnH = 30;
    const int col1X = 70;
    const int col2X = 250;
    const int startY = 90;
    const int rowSpacing = 40;

    auto drawToggle = [](int x, int y, int w, int h, const char* label, bool enabled) {
        uint16_t color = enabled ? TFT_GREEN : TFT_RED;
        tft->fillRect(x, y, w, h, color);
        tft->drawRect(x, y, w, h, TFT_WHITE);
        tft->setTextDatum(MC_DATUM);
        tft->setTextColor(TFT_WHITE, color);
        tft->drawString(label, x + w / 2, y + h / 2, 2);
    };

    tft->setTextDatum(TL_DATUM);
    tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    tft->drawString("Módulos:", col1X, startY - 20, 2);
    tft->drawString("Sensores:", col2X, startY - 20, 2);

    char label[32];
    // Helper: formats label text into the provided buffer (side effect only)
    auto makeLabel = [](char* dest, size_t size, const char* name, bool enabled) {
        snprintf(dest, size, "%s: %s", name, enabled ? "ON" : "OFF");
    };

    // v2.14.0: LUCES and MEDIA removed - cleaner interface
    
    makeLabel(label, sizeof(label), "TRACCIÓN", tempModulesState.tractionEnabled);
    drawToggle(col1X, startY, btnW, btnH, label, tempModulesState.tractionEnabled);

    makeLabel(label, sizeof(label), "INA226", tempModulesState.currentSensorsEnabled);
    drawToggle(col1X, startY + rowSpacing, btnW, btnH, label, tempModulesState.currentSensorsEnabled);

    makeLabel(label, sizeof(label), "TEMP", tempModulesState.tempSensorsEnabled);
    drawToggle(col1X, startY + 2 * rowSpacing, btnW, btnH, label, tempModulesState.tempSensorsEnabled);

    makeLabel(label, sizeof(label), "RUEDAS", tempModulesState.wheelSensorsEnabled);
    drawToggle(col2X, startY, btnW, btnH, label, tempModulesState.wheelSensorsEnabled);
    
    // Save button
    tft->fillRect(140, 230, 200, 40, TFT_BLUE);
    tft->drawRect(140, 230, 200, 40, TFT_WHITE);
    tft->setTextDatum(MC_DATUM);
    tft->setTextColor(TFT_WHITE, TFT_BLUE);
    tft->drawString("GUARDAR Y VOLVER", 240, 250, 2);
    
    // Instructions
    tft->setTextDatum(BC_DATUM);
    tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    tft->drawString("Toca para activar/desactivar módulos y sensores", 240, 300, 1);
}

static void updateModulesConfig(int touchX, int touchY, bool touched) {
    // Initialize screen on first call
    if (modulesConfigFirstCall) {
        drawModulesConfigScreen();
        modulesConfigFirstCall = false;
        return;
    }
    
    if (!touched) {
        return;  // Don't redraw unless there's a touch
    }
    
    const int btnW = 140;
    const int btnH = 30;
    const int col1X = 70;
    const int col2X = 250;
    const int startY = 90;
    const int rowSpacing = 40;
    bool stateChanged = false;
    
    auto inButton = [](int touchXCoord, int touchYCoord, int buttonX, int buttonY, int buttonWidth, int buttonHeight) {
        return touchXCoord >= buttonX && touchXCoord <= buttonX + buttonWidth &&
               touchYCoord >= buttonY && touchYCoord <= buttonY + buttonHeight;
    };

    // Check which button was touched
    // v2.14.0: Updated button layout (removed LUCES and MEDIA)
    // New layout: TRACCIÓN, INA226, TEMP in col1; RUEDAS in col2
    if (inButton(touchX, touchY, col1X, startY, btnW, btnH)) {
        tempModulesState.tractionEnabled = !tempModulesState.tractionEnabled;
        Logger::infof("Tracción: %s", tempModulesState.tractionEnabled ? "ON" : "OFF");
        Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
        stateChanged = true;
    } else if (inButton(touchX, touchY, col1X, startY + rowSpacing, btnW, btnH)) {
        tempModulesState.currentSensorsEnabled = !tempModulesState.currentSensorsEnabled;
        Logger::infof("Sensores INA226: %s", tempModulesState.currentSensorsEnabled ? "ON" : "OFF");
        Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
        stateChanged = true;
    } else if (inButton(touchX, touchY, col1X, startY + 2 * rowSpacing, btnW, btnH)) {
        tempModulesState.tempSensorsEnabled = !tempModulesState.tempSensorsEnabled;
        Logger::infof("Sensores temperatura: %s", tempModulesState.tempSensorsEnabled ? "ON" : "OFF");
        Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
        stateChanged = true;
    } else if (inButton(touchX, touchY, col2X, startY, btnW, btnH)) {
        tempModulesState.wheelSensorsEnabled = !tempModulesState.wheelSensorsEnabled;
        Logger::infof("Sensores rueda: %s", tempModulesState.wheelSensorsEnabled ? "ON" : "OFF");
        Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
        stateChanged = true;
    }
    
    // Save button
    if (touchX >= 140 && touchX <= 340 && touchY >= 230 && touchY <= 270) {
        // Apply temp values to config and save
        // v2.14.0: lightsEnabled and multimediaEnabled removed
        cfg.tractionEnabled = tempModulesState.tractionEnabled;
        cfg.currentSensorsEnabled = tempModulesState.currentSensorsEnabled;
        cfg.tempSensorsEnabled = tempModulesState.tempSensorsEnabled;
        cfg.wheelSensorsEnabled = tempModulesState.wheelSensorsEnabled;
        safeSaveConfig();
        Logger::info("Configuración de módulos/sensores guardada");
        Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_HIGH});
        
        // 🔒 v2.10.0: Clear screen when exiting modules config
        if (tft != nullptr) {
            tft->fillScreen(TFT_BLACK);
        }
        
        calibState = CalibrationState::NONE;
        modulesConfigFirstCall = true;  // Reset for next time
        return;  // Exit config
    }
    
    // Only redraw if state actually changed
    if (stateChanged) {
        drawModulesConfigScreen();
    }
}

// Interactive module configuration screen
static void startModulesConfig() {
    calibState = CalibrationState::MODULES_CONFIG;
    modulesConfigFirstCall = true;
    // Initialize temp values from current config
    syncModulesStateFromConfig();
    Logger::info("Iniciando configuración de módulos");
    Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
    drawModulesConfigScreen();
}

static void saveAndExit() {
    safeSaveConfig();
    Logger::info("Configuración guardada. Saliendo de menú oculto.");
    Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_HIGH});
    
    // 🔒 v2.10.0: Clear screen when exiting menu to ensure clean redraw of HUD
    // This prevents menu remnants from staying on screen
    if (tft != nullptr) {
        tft->fillScreen(TFT_BLACK);
    }
    
    menuActive = false;
    codeBuffer = 0;
    lastMenuActive = false;
}

static void restoreFactory() {
    Storage::defaults(cfg);
    Logger::info("Configuración restaurada a valores de fábrica.");
    safeSaveConfig();
    Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_HIGH});
}

// 🆕 v2.9.5: Mejorada visualización de errores con descripciones
// Constantes para visualización de errores
static const int MAX_DISPLAYED_ERRORS = 7;  // Máximo de errores mostrados a la vez
static const int ERROR_LINE_LENGTH_THRESHOLD = 40;  // Umbral para usar fuente pequeña

static void showErrors() {
    int count = System::getErrorCount();
    const Storage::ErrorLog* errors = System::getErrors();
    
    tft->fillRect(60, 40, 360, 240, TFT_BLACK);
    tft->drawRect(60, 40, 360, 240, TFT_ORANGE);
    tft->setTextDatum(TC_DATUM);
    tft->setTextColor(TFT_ORANGE, TFT_BLACK);
    tft->drawString("ERRORES PERSISTENTES", 240, 50, 2);
    
    tft->setTextDatum(TL_DATUM);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    
    if (count == 0) {
        tft->setTextDatum(MC_DATUM);
        tft->setTextColor(TFT_GREEN, TFT_BLACK);
        tft->drawString("Sin errores", 240, 150, 4);
    } else {
        char line[80];  // Increased buffer size to safely accommodate error code + description
        int y = 80;
        int displayed = 0;
        for (int i = 0; i < count && displayed < MAX_DISPLAYED_ERRORS; i++) {
            if (errors[i].code != 0) {
                // 🆕 v2.9.5: Mostrar código Y descripción
                const char* desc = ErrorCodes::getErrorDescription(errors[i].code);
                snprintf(line, sizeof(line), "%d: %s", errors[i].code, desc);
                
                // Usar fuente más pequeña si la descripción es larga
                if (strlen(line) > ERROR_LINE_LENGTH_THRESHOLD) {
                    tft->drawString(line, 70, y, 1);  // Fuente 1 (más pequeña)
                    y += 15;
                } else {
                    tft->drawString(line, 70, y, 2);  // Fuente 2 (normal)
                    y += 18;
                }
                displayed++;
            }
        }
        
        // Mostrar total
        snprintf(line, sizeof(line), "Total: %d errores", count);
        tft->setTextColor(TFT_YELLOW, TFT_BLACK);
        tft->drawString(line, 70, y + 5, 2);
        
        // Mensaje de ayuda si hay más errores de los que se pueden mostrar
        if (count > MAX_DISPLAYED_ERRORS) {
            tft->setTextColor(TFT_CYAN, TFT_BLACK);
            tft->setTextDatum(MC_DATUM);
            snprintf(line, sizeof(line), "(Mostrando %d de %d)", displayed, count);
            tft->drawString(line, 240, y + 25, 1);
        }
    }
    
    tft->setTextDatum(MC_DATUM);
    tft->setTextColor(TFT_CYAN, TFT_BLACK);
    tft->drawString("Toca para volver", 240, 260, 2);
    
    Logger::infof("Mostrando %d errores persistentes", count);
    
    // 🔒 v2.8.8: Esperar toque para volver usando touch integrado TFT_eSPI
    uint32_t waitStart = millis();
    uint16_t tx, ty;
    while (millis() - waitStart < 5000) {
        if (tft != nullptr && tft->getTouch(&tx, &ty)) {
            waitTouchRelease();  // Debounce antes de salir
            break;
        }
        yield();
    }
}

// ✅ v2.7.0: Iniciar confirmación para borrar errores
static void startClearErrorsConfirm() {
    int count = System::getErrorCount();
    if (count == 0) {
        // No hay errores que borrar
        tft->fillRect(60, 40, 360, 240, TFT_BLACK);
        tft->drawRect(60, 40, 360, 240, TFT_GREEN);
        tft->setTextDatum(MC_DATUM);
        tft->setTextColor(TFT_GREEN, TFT_BLACK);
        tft->drawString("SIN ERRORES", 240, 130, 4);
        tft->setTextColor(TFT_WHITE, TFT_BLACK);
        tft->drawString("No hay errores que borrar", 240, 170, 2);
        
        uint32_t waitStart = millis();
        while (millis() - waitStart < FEEDBACK_DISPLAY_MS) { yield(); }
        return;
    }
    
    calibState = CalibrationState::CLEAR_ERRORS_CONFIRM;
    calibStartMs = millis();
    Logger::info("Solicitando confirmación para borrar errores");
}

// ✅ v2.7.0: Dibujar pantalla de confirmación
static void drawClearErrorsConfirmScreen() {
    if (tft == nullptr) return;
    
    int count = System::getErrorCount();
    
    tft->fillRect(60, 40, 360, 240, TFT_BLACK);
    tft->drawRect(60, 40, 360, 240, TFT_RED);
    
    tft->setTextDatum(TC_DATUM);
    tft->setTextColor(TFT_RED, TFT_BLACK);
    tft->drawString("CONFIRMAR BORRADO", 240, 50, 4);
    
    tft->setTextDatum(MC_DATUM);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    
    char msgStr[48];
    snprintf(msgStr, sizeof(msgStr), "Borrar %d errores?", count);
    tft->drawString(msgStr, 240, 100, 2);
    tft->drawString("Esta accion es irreversible", 240, 125, 2);
    
    // Botón CANCELAR
    tft->fillRect(80, 180, 120, 50, TFT_DARKGREY);
    tft->drawRect(80, 180, 120, 50, TFT_WHITE);
    tft->setTextColor(TFT_WHITE, TFT_DARKGREY);
    tft->drawString("CANCELAR", 140, 205, 2);
    
    // Botón CONFIRMAR
    tft->fillRect(280, 180, 120, 50, TFT_RED);
    tft->drawRect(280, 180, 120, 50, TFT_WHITE);
    tft->setTextColor(TFT_WHITE, TFT_RED);
    tft->drawString("BORRAR", 340, 205, 2);
}

// ✅ v2.7.0: Actualizar confirmación de borrado
static void updateClearErrorsConfirm(int touchX, int touchY, bool touched) {
    if (!touched) {
        drawClearErrorsConfirmScreen();
        return;
    }
    
    // Detectar toque en botón CANCELAR
    if (touchX >= 80 && touchX <= 200 && touchY >= 180 && touchY <= 230) {
        Logger::info("Borrado de errores cancelado");
        Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
        waitTouchRelease();
        calibState = CalibrationState::NONE;
        return;
    }
    
    // Detectar toque en botón CONFIRMAR (BORRAR)
    if (touchX >= 280 && touchX <= 400 && touchY >= 180 && touchY <= 230) {
        System::clearErrors();
        Logger::info("Errores persistentes borrados (confirmado).");
        Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_HIGH});
        
        // Mostrar mensaje de éxito
        tft->fillRect(60, 40, 360, 240, TFT_BLACK);
        tft->drawRect(60, 40, 360, 240, TFT_GREEN);
        tft->setTextDatum(MC_DATUM);
        tft->setTextColor(TFT_GREEN, TFT_BLACK);
        tft->drawString("ERRORES BORRADOS", 240, 140, 4);
        
        uint32_t waitStart = millis();
        while (millis() - waitStart < FEEDBACK_DISPLAY_MS) { yield(); }
        
        calibState = CalibrationState::NONE;
        return;
    }
    
    // Timeout - cancelar automáticamente
    if (millis() - calibStartMs > CALIB_TIMEOUT_MS) {
        Logger::warn("Confirmación borrado errores timeout - cancelado");
        calibState = CalibrationState::NONE;
    }
}

// Función anterior mantenida por compatibilidad (ahora deprecated)
static void clearErrorsMenu() {
    // ✅ v2.7.0: Redirigir a confirmación interactiva
    startClearErrorsConfirm();
}

// Función para detectar toque en opciones del menú
static int getTouchedMenuOption(int touchX, int touchY) {
    // Verificar si está dentro del área del menú
    if (touchX < MENU_X1 || touchX > MENU_X1 + MENU_WIDTH) return -1;
    if (touchY < MENU_Y1 || touchY > MENU_Y1 + NUM_MENU_ITEMS * MENU_ITEM_HEIGHT) return -1;
    
    // Calcular qué opción fue tocada
    int option = (touchY - MENU_Y1) / MENU_ITEM_HEIGHT + 1;
    return (option >= 1 && option <= NUM_MENU_ITEMS) ? option : -1;
}

// -----------------------
// Numeric Keypad for Code Entry
// -----------------------

// Forward declarations
static void drawMenuFull();
static void updateCodeDisplay();

// Keypad layout constants
static const int KEYPAD_X = 100;
static const int KEYPAD_Y = 80;
static const int KEYPAD_BTN_WIDTH = 60;
static const int KEYPAD_BTN_HEIGHT = 50;
static const int KEYPAD_SPACING = 10;

// Keypad button structure
struct KeypadButton {
    int x, y;
    int value;  // -1 for backspace, -2 for enter
    const char* label;
};

// 3x4 keypad layout: 1-9, back, 0, enter
static const KeypadButton keypadButtons[12] = {
    {KEYPAD_X, KEYPAD_Y, 1, "1"},
    {KEYPAD_X + (KEYPAD_BTN_WIDTH + KEYPAD_SPACING), KEYPAD_Y, 2, "2"},
    {KEYPAD_X + 2 * (KEYPAD_BTN_WIDTH + KEYPAD_SPACING), KEYPAD_Y, 3, "3"},
    
    {KEYPAD_X, KEYPAD_Y + (KEYPAD_BTN_HEIGHT + KEYPAD_SPACING), 4, "4"},
    {KEYPAD_X + (KEYPAD_BTN_WIDTH + KEYPAD_SPACING), KEYPAD_Y + (KEYPAD_BTN_HEIGHT + KEYPAD_SPACING), 5, "5"},
    {KEYPAD_X + 2 * (KEYPAD_BTN_WIDTH + KEYPAD_SPACING), KEYPAD_Y + (KEYPAD_BTN_HEIGHT + KEYPAD_SPACING), 6, "6"},
    
    {KEYPAD_X, KEYPAD_Y + 2 * (KEYPAD_BTN_HEIGHT + KEYPAD_SPACING), 7, "7"},
    {KEYPAD_X + (KEYPAD_BTN_WIDTH + KEYPAD_SPACING), KEYPAD_Y + 2 * (KEYPAD_BTN_HEIGHT + KEYPAD_SPACING), 8, "8"},
    {KEYPAD_X + 2 * (KEYPAD_BTN_WIDTH + KEYPAD_SPACING), KEYPAD_Y + 2 * (KEYPAD_BTN_HEIGHT + KEYPAD_SPACING), 9, "9"},
    
    {KEYPAD_X, KEYPAD_Y + 3 * (KEYPAD_BTN_HEIGHT + KEYPAD_SPACING), -1, "<"},
    {KEYPAD_X + (KEYPAD_BTN_WIDTH + KEYPAD_SPACING), KEYPAD_Y + 3 * (KEYPAD_BTN_HEIGHT + KEYPAD_SPACING), 0, "0"},
    {KEYPAD_X + 2 * (KEYPAD_BTN_WIDTH + KEYPAD_SPACING), KEYPAD_Y + 3 * (KEYPAD_BTN_HEIGHT + KEYPAD_SPACING), -2, "OK"}
};

static void drawNumericKeypad() {
    if (tft == nullptr) return;
    
    // Clear screen
    tft->fillScreen(TFT_BLACK);
    
    // Title
    tft->setTextDatum(TC_DATUM);
    tft->setTextColor(TFT_CYAN, TFT_BLACK);
    tft->drawString("Código de acceso", 240, 20, 4);
    
    // Code display
    tft->setTextDatum(MC_DATUM);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    char codeStr[8];
    snprintf(codeStr, sizeof(codeStr), "%04d", codeBuffer);
    tft->drawString(codeStr, 240, 55, 4);
    
    // Draw keypad buttons
    for (int i = 0; i < 12; i++) {
        const KeypadButton& btn = keypadButtons[i];
        
        // Button background
        tft->fillRoundRect(btn.x, btn.y, KEYPAD_BTN_WIDTH, KEYPAD_BTN_HEIGHT, 5, TFT_NAVY);
        tft->drawRoundRect(btn.x, btn.y, KEYPAD_BTN_WIDTH, KEYPAD_BTN_HEIGHT, 5, TFT_WHITE);
        
        // Button label
        tft->setTextDatum(MC_DATUM);
        tft->setTextColor(TFT_WHITE, TFT_NAVY);
        tft->drawString(btn.label, btn.x + KEYPAD_BTN_WIDTH/2, btn.y + KEYPAD_BTN_HEIGHT/2, 4);
    }
    
    // Instructions
    tft->setTextDatum(BC_DATUM);
    tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    tft->drawString("Toca números para entrar 8989", 240, 310, 2);
}

static int getTouchedKeypadButton(int x, int y) {
    for (int i = 0; i < 12; i++) {
        const KeypadButton& btn = keypadButtons[i];
        if (x >= btn.x && x < btn.x + KEYPAD_BTN_WIDTH &&
            y >= btn.y && y < btn.y + KEYPAD_BTN_HEIGHT) {
            return i;  // Return button index
        }
    }
    return -1;  // No button touched
}

static void handleKeypadInput(int buttonIndex) {
    if (buttonIndex < 0 || buttonIndex >= 12) return;
    
    const KeypadButton& btn = keypadButtons[buttonIndex];
    
    if (btn.value == -1) {
        // Backspace
        codeBuffer = codeBuffer / 10;
        updateCodeDisplay();
        drawNumericKeypad();  // Redraw to show updated code
        Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
    } else if (btn.value == -2) {
        // Enter/OK - check code
        if (codeBuffer == accessCode) {
            menuActive = true;
            numpadActive = false;
            Alerts::play({Audio::AUDIO_MENU_OCULTO, Audio::Priority::PRIO_HIGH});
            drawMenuFull();
            lastMenuActive = true;
            lastSelectedOption = selectedOption;
            lastCodeBuffer = codeBuffer;
        } else {
            // Wrong code - show error with non-blocking delay
            tft->fillRect(100, 40, 280, 35, TFT_RED);
            tft->setTextDatum(MC_DATUM);
            tft->setTextColor(TFT_WHITE, TFT_RED);
            tft->drawString("CÓDIGO INCORRECTO", 240, 55, 2);
            Alerts::play({Audio::AUDIO_ERROR_GENERAL, Audio::Priority::PRIO_HIGH});
            // Set timestamp for non-blocking error display
            wrongCodeDisplayStart = millis();
            codeBuffer = 0;
            // Error message will be cleared in update loop after 1 second
        }
    } else {
        // Number button (0-9)
        if (codeBuffer > 999) {
            // Already 4 digits, ignore
            Alerts::play({Audio::AUDIO_ERROR_GENERAL, Audio::Priority::PRIO_NORMAL});
            return;
        }
        codeBuffer = (codeBuffer * 10) + btn.value;
        drawNumericKeypad();  // Redraw to show updated code
        Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
    }
}

// -----------------------
// Dibujo del menú
// -----------------------
static void drawMenuFull() {
    // 🔒 v2.10.0: Full screen clear to prevent gauge ghosting
    // The speed and RPM gauges at (70,175) and (410,175) with radius ~73px
    // extend beyond the menu rectangle (60,40,360,240), leaving visible artifacts
    // when the menu opens. Clear the entire screen first.
    tft->fillScreen(TFT_BLACK);
    
    // Draw menu rectangle border (background already black from fillScreen)
    tft->drawRect(60, 40, 360, 240, TFT_WHITE);
    tft->setTextDatum(TL_DATUM);

    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->drawString("MENU OCULTO", 80, 50, 2);

    for(int i=0; i<NUM_MENU_ITEMS; i++) {
        uint16_t col = (i+1 == selectedOption) ? TFT_YELLOW : TFT_WHITE;
        tft->setTextColor(col, TFT_BLACK);
        tft->drawString(MENU_ITEMS[i], 80, 80 + i*20, 2);
    }
    
    // No need to show code anymore - entered via keypad
}

static void updateOptionHighlight() {
    // Redibujar solo la línea anterior y la nueva
    if(lastSelectedOption != -1 && lastSelectedOption != selectedOption) {
        tft->setTextColor(TFT_WHITE, TFT_BLACK);
        tft->drawString(MENU_ITEMS[lastSelectedOption-1], 80, 80 + (lastSelectedOption-1)*20, 2);
    }
    tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    tft->drawString(MENU_ITEMS[selectedOption-1], 80, 80 + (selectedOption-1)*20, 2);
}

static void updateCodeDisplay() {
    // Update code display on numeric keypad
    if (!numpadActive) return;
    
    tft->fillRect(100, 40, 280, 35, TFT_BLACK);
    tft->setTextDatum(MC_DATUM);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    char codeStr[8];
    snprintf(codeStr, sizeof(codeStr), "%04d", codeBuffer);
    tft->drawString(codeStr, 240, 55, 4);
}

// -----------------------
// API pública
// -----------------------
void MenuHidden::init(TFT_eSPI *display) {
    tft = display;  // Store pointer - must be valid (non-null) display instance
    Storage::load(cfg);
    Logger::info("MenuHidden init OK");
}

void MenuHidden::update(bool batteryIconPressed) {
    // Si hay calibración en proceso, manejarla
    if (calibState != CalibrationState::NONE) {
        bool touched = false;
        int touchX = 0, touchY = 0;
        
        // 🔒 v2.8.8: Usar touch integrado de TFT_eSPI
        uint16_t tx, ty;
        if (tft != nullptr && tft->getTouch(&tx, &ty)) {
            touchX = (int)tx;
            touchY = (int)ty;
            touched = true;
            // Debounce con timeout (no para regen que necesita detección continua)
            if (calibState != CalibrationState::REGEN_ADJUST) {
                waitTouchRelease();
            }
        }
        
        if (calibState == CalibrationState::PEDAL_MIN || 
            calibState == CalibrationState::PEDAL_MAX ||
            calibState == CalibrationState::PEDAL_DONE) {
            updatePedalCalibration(touched);
        }
        else if (calibState == CalibrationState::ENCODER_CENTER ||
                 calibState == CalibrationState::ENCODER_DONE) {
            updateEncoderCalibration(touched);
        }
        // 🔒 v2.9.0: Handle touch calibration
        else if (calibState == CalibrationState::TOUCH_CALIBRATION) {
            // Update touch calibration routine
            if (!TouchCalibration::update()) {
                // Calibration finished (complete or failed)
                auto result = TouchCalibration::getResult();
                if (result.success) {
                    Logger::info("Touch calibration completed successfully");
                    Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_HIGH});
                } else {
                    Logger::warn("Touch calibration failed or cancelled");
                    Alerts::play({Audio::AUDIO_ERROR_GENERAL, Audio::Priority::PRIO_HIGH});
                }
                calibState = CalibrationState::NONE;
            }
        }
        // ✅ v2.7.0: Manejar ajuste interactivo de regen
        else if (calibState == CalibrationState::REGEN_ADJUST) {
            updateRegenAdjust(touchX, touchY, touched);
            if (touched) waitTouchRelease(DEBOUNCE_SHORT_MS);  // Debounce corto después de procesar
        }
        // Manejar configuración de módulos ON/OFF
        else if (calibState == CalibrationState::MODULES_CONFIG) {
            updateModulesConfig(touchX, touchY, touched);
            if (touched) waitTouchRelease(DEBOUNCE_SHORT_MS);  // Debounce corto después de procesar
        }
        // ✅ v2.7.0: Manejar confirmación de borrado de errores
        else if (calibState == CalibrationState::CLEAR_ERRORS_CONFIRM) {
            updateClearErrorsConfirm(touchX, touchY, touched);
        }
        
        // Si terminó la calibración, redibujar menú
        if (calibState == CalibrationState::NONE && menuActive) {
            drawMenuFull();
        }
        return;
    }
    
    if(!menuActive) {
        // Show keypad on first battery icon press
        if(batteryIconPressed && !numpadActive) {
            numpadActive = true;
            codeBuffer = 0;
            drawNumericKeypad();
            Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
            lastCodeBuffer = codeBuffer;
            return;
        }
        
        // Handle keypad interaction when active
        if(numpadActive) {
            uint16_t tx, ty;
            uint32_t now = millis();
            
            // Check for touch with debounce
            if (tft != nullptr && tft->getTouch(&tx, &ty)) {
                if (now - lastKeypadTouch > KEYPAD_DEBOUNCE_MS) {
                    int buttonIndex = getTouchedKeypadButton((int)tx, (int)ty);
                    if (buttonIndex >= 0) {
                        handleKeypadInput(buttonIndex);
                        lastKeypadTouch = now;
                        waitTouchRelease(DEBOUNCE_SHORT_MS);
                    }
                }
            }
            
            // Clear wrong code error message after 1 second (non-blocking)
            if (wrongCodeDisplayStart > 0 && (now - wrongCodeDisplayStart > 1000)) {
                wrongCodeDisplayStart = 0;
                drawNumericKeypad();  // Redraw keypad to clear error
            }
            
            // Exit keypad if battery pressed again (cancel)
            if (batteryIconPressed && (now - lastKeypadTouch > KEYPAD_DEBOUNCE_MS)) {
                numpadActive = false;
                codeBuffer = 0;
                lastCodeBuffer = 0;
                lastKeypadTouch = now;
                waitTouchRelease(DEBOUNCE_SHORT_MS);
                // Don't clear screen - let normal HUD update redraw
                // This prevents flickering from full screen clear
            }
        }
        
        return;
    }

    // ====== NAVEGACIÓN TÁCTIL DEL MENÚ ======
    // 🔒 v2.8.8: Usar touch integrado de TFT_eSPI
    uint16_t tx, ty;
    if (tft != nullptr && tft->getTouch(&tx, &ty)) {
        int touchX = (int)tx;
        int touchY = (int)ty;
        
        // Detectar opción tocada
        int touchedOption = getTouchedMenuOption(touchX, touchY);
        
        if (touchedOption > 0) {
            // Actualizar selección visual
            selectedOption = touchedOption;
            if (selectedOption != lastSelectedOption) {
                updateOptionHighlight();
                lastSelectedOption = selectedOption;
            }
            
            // Debounce con timeout - esperar a que suelte
            waitTouchRelease();
            
            // Ejecutar acción (sin sonido extra, cada acción tiene su sonido)
            
            switch(selectedOption) {
                case 1: startPedalCalibration(); break;
                case 2: startEncoderCalibration(); break;
                case 3: startTouchCalibration(); break;  // 🔒 v2.9.0: Touch calibration
                case 4: startRegenAdjust(); break;  // ✅ v2.7.0: Ajuste interactivo
                case 5: startModulesConfig(); break;  // Configuración interactiva de módulos
                case 6: saveAndExit(); break;
                case 7: restoreFactory(); break;
                case 8: showErrors(); break;
                case 9: startClearErrorsConfirm(); break;  // ✅ v2.7.0: Confirmación
            }
            
            // Redibujar menú después de acción (excepto si se cerró)
            if (menuActive && calibState == CalibrationState::NONE) {
                drawMenuFull();
            }
            lastSelectedOption = selectedOption;
            lastCodeBuffer = codeBuffer;
        }
    }
}

bool MenuHidden::isActive() {
    return menuActive || numpadActive;  // Include numpad in active state
}

void MenuHidden::activateDirectly() {
    // Activate hidden menu directly without code entry (for demo mode)
    if (!menuActive && tft != nullptr) {
        menuActive = true;
        codeBuffer = accessCode;  // Set code to access code to match state
        Alerts::play({Audio::AUDIO_MENU_OCULTO, Audio::Priority::PRIO_HIGH});
        drawMenuFull();
        lastMenuActive = true;
        lastSelectedOption = selectedOption;
        lastCodeBuffer = codeBuffer;
        Logger::info("Menu oculto activado directamente (modo demo)");
    }
}

void MenuHidden::startTouchCalibrationDirectly() {
    // 🆕 v2.9.4: Direct touch calibration activation via physical button
    // Allows calibrating touch when it doesn't work (no menu needed)
    if (tft == nullptr) {
        Logger::error("Cannot start touch calibration: TFT not initialized");
        return;
    }
    
    // Cancel any active calibration first
    if (calibState != CalibrationState::NONE) {
        Logger::warn("Cancelling previous calibration to start touch calibration");
        calibState = CalibrationState::NONE;
    }
    
    // Close menu if active
    if (menuActive) {
        menuActive = false;
        lastMenuActive = false;
    }
    
    Logger::info("Starting direct touch calibration (activated by physical button)");
    startTouchCalibration();
}
