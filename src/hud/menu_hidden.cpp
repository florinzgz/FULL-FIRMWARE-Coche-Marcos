#include "menu_hidden.h"
#include "storage.h"
#include "logger.h"
#include "alerts.h"
#include "pedal.h"
#include "steering.h"
#include "buttons.h"
#include "settings.h"
#include "system.h"
#include <TFT_eSPI.h>
// 🔒 v2.8.8: Eliminada librería XPT2046_Touchscreen separada
// Ahora usamos el touch integrado de TFT_eSPI
#include "pins.h"
#include "touch_map.h"  // 🔒 v2.8.3: Constantes centralizadas de calibración táctil
#include "touch_calibration.h"  // 🔒 v2.9.0: Touch calibration routine

static TFT_eSPI *tft = nullptr;
// 🔒 v2.8.8: Ya no necesitamos puntero separado al touch

static bool menuActive = false;
static uint16_t codeBuffer = 0;
static const uint16_t accessCode = 8989;

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
    "5) Modulos ON/OFF",
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
    
    tft->fillRect(60, 40, 360, 240, TFT_BLACK);
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
    Logger::info("Iniciando calibración de pedal - fase MIN");
    Alerts::play({Audio::AUDIO_CAL_PEDAL, Audio::Priority::PRIO_HIGH});
}

static void startEncoderCalibration() {
    calibState = CalibrationState::ENCODER_CENTER;
    calibStartMs = millis();
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
            Storage::save(cfg);
            
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
        
        calibState = CalibrationState::NONE;
    }
    
    // Timeout
    if (millis() - calibStartMs > CALIB_TIMEOUT_MS) {
        Logger::warn("Calibración pedal timeout");
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
            Storage::save(cfg);
            
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
        
        calibState = CalibrationState::NONE;
    }
    
    // Timeout
    if (millis() - calibStartMs > CALIB_TIMEOUT_MS) {
        Logger::warn("Calibración encoder timeout");
        calibState = CalibrationState::NONE;
        Alerts::play({Audio::AUDIO_ENCODER_ERROR, Audio::Priority::PRIO_NORMAL});
    }
}

// ✅ v2.7.0: Iniciar ajuste interactivo de regeneración
static void startRegenAdjust() {
    calibState = CalibrationState::REGEN_ADJUST;
    calibStartMs = millis();
    regenAdjustValue = cfg.regenPercent;  // Cargar valor actual
    Logger::info("Iniciando ajuste de regen - usa touch para ajustar");
    Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
}

// ✅ v2.7.0: Dibujar pantalla de ajuste de regen
static void drawRegenAdjustScreen() {
    if (tft == nullptr) return;
    
    tft->fillRect(60, 40, 360, 240, TFT_BLACK);
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
        Storage::save(cfg);
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

static void applyModules(bool lights, bool media, bool traction) {
    cfg.lightsEnabled = lights;
    cfg.multimediaEnabled = media;
    cfg.tractionEnabled = traction;
    Storage::save(cfg);
    Logger::infof("Módulos guardados: lights=%d media=%d traction=%d", lights, media, traction);
    Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
}

static void saveAndExit() {
    Storage::save(cfg);
    Logger::info("Configuración guardada. Saliendo de menú oculto.");
    Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_HIGH});
    menuActive = false;
    codeBuffer = 0;
    lastMenuActive = false;
}

static void restoreFactory() {
    Storage::defaults(cfg);
    Logger::info("Configuración restaurada a valores de fábrica.");
    Storage::save(cfg);
    Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_HIGH});
}

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
        char line[64];
        int y = 80;
        int displayed = 0;
        for (int i = 0; i < count && displayed < 8; i++) {
            if (errors[i].code != 0) {
                snprintf(line, sizeof(line), "Error %d: Codigo %d", i+1, errors[i].code);
                tft->drawString(line, 80, y, 2);
                y += 18;
                displayed++;
            }
        }
        
        snprintf(line, sizeof(line), "Total: %d errores", count);
        tft->setTextColor(TFT_YELLOW, TFT_BLACK);
        tft->drawString(line, 80, y + 10, 2);
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
// Dibujo del menú
// -----------------------
static void drawMenuFull() {
    tft->fillRect(60, 40, 360, 240, TFT_BLACK);
    tft->drawRect(60, 40, 360, 240, TFT_WHITE);
    tft->setTextDatum(TL_DATUM);

    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->drawString("MENU OCULTO", 80, 50, 2);

    for(int i=0; i<NUM_MENU_ITEMS; i++) {
        uint16_t col = (i+1 == selectedOption) ? TFT_YELLOW : TFT_WHITE;
        tft->setTextColor(col, TFT_BLACK);
        tft->drawString(MENU_ITEMS[i], 80, 80 + i*20, 2);
    }

    // Código
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->drawString("Code:", 80, 240, 2);
    tft->drawString(String(codeBuffer), 140, 240, 2);
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
    // Limpiar zona del código
    tft->fillRect(140, 230, 100, 20, TFT_BLACK);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->drawString(String(codeBuffer), 140, 240, 2);
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
        if(batteryIconPressed) {
            codeBuffer = (codeBuffer * 10) + 8;
            if(codeBuffer > 9999) codeBuffer = 0;

            if(codeBuffer == accessCode) {
                menuActive = true;
                Alerts::play({Audio::AUDIO_MENU_OCULTO, Audio::Priority::PRIO_HIGH});
                drawMenuFull();
                lastMenuActive = true;
                lastSelectedOption = selectedOption;
                lastCodeBuffer = codeBuffer;
            } else if(codeBuffer != lastCodeBuffer) {
                updateCodeDisplay();
                lastCodeBuffer = codeBuffer;
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
                case 5: applyModules(true, true, true); break;
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
    return menuActive;
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