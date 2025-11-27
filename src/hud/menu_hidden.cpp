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
#include <XPT2046_Touchscreen.h>
#include "pins.h"

static TFT_eSPI *tft = nullptr;
static XPT2046_Touchscreen *touch = nullptr;

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
    ENCODER_DONE
};
static CalibrationState calibState = CalibrationState::NONE;
static int pedalCalibMin = 0;
static int pedalCalibMax = 4095;
static uint32_t calibStartMs = 0;
static const uint32_t CALIB_TIMEOUT_MS = 30000;  // 30 segundos timeout

// Constantes de calibración táctil
static const int TOUCH_MIN_RAW = 200;   // Valor mínimo raw del touch
static const int TOUCH_MAX_RAW = 3900;  // Valor máximo raw del touch
static const uint32_t DEBOUNCE_TIMEOUT_MS = 500;  // Timeout para debounce

// Zonas táctiles del menú (8 opciones)
static const int MENU_X1 = 60;
static const int MENU_Y1 = 80;
static const int MENU_WIDTH = 360;
static const int MENU_ITEM_HEIGHT = 20;
static const int NUM_MENU_ITEMS = 8;

// Opciones del menú (evitar duplicación - DRY)
static const char* const MENU_ITEMS[NUM_MENU_ITEMS] = {
    "1) Calibrar pedal",
    "2) Calibrar encoder",
    "3) Ajuste regen (%)",
    "4) Modulos ON/OFF",
    "5) Guardar y salir",
    "6) Restaurar fabrica",
    "7) Ver errores",
    "8) Borrar errores"
};

// Helper para debounce con timeout
static void waitTouchRelease(uint32_t maxWaitMs = DEBOUNCE_TIMEOUT_MS) {
    if (touch == nullptr) return;
    uint32_t startMs = millis();
    while (touch->touched() && (millis() - startMs < maxWaitMs)) {
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
        if (pedalCalibMax > pedalCalibMin + 500) {
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
            while (millis() - waitStart < 1500) { yield(); }
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
        while (millis() - waitStart < 1500) { yield(); }
        
        calibState = CalibrationState::NONE;
    }
    
    // Timeout
    if (millis() - calibStartMs > CALIB_TIMEOUT_MS) {
        Logger::warn("Calibración encoder timeout");
        calibState = CalibrationState::NONE;
        Alerts::play({Audio::AUDIO_ENCODER_ERROR, Audio::Priority::PRIO_NORMAL});
    }
}

static void applyRegenAdjust(int value) {
    cfg.regenPercent = constrain(value, 0, 100);
    Storage::save(cfg);
    Logger::infof("Ajuste de regen guardado: %d%%", cfg.regenPercent);
    Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
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
    
    // Esperar toque para volver (máximo 5 segundos para evitar bloqueo)
    uint32_t waitStart = millis();
    while (millis() - waitStart < 5000) {
        if (touch != nullptr && touch->touched()) {
            waitTouchRelease();  // Debounce antes de salir
            break;
        }
        yield();
    }
}

static void clearErrorsMenu() {
    System::clearErrors();
    Logger::info("Errores persistentes borrados.");
    Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_HIGH});
    
    tft->fillRect(60, 40, 360, 240, TFT_BLACK);
    tft->drawRect(60, 40, 360, 240, TFT_GREEN);
    tft->setTextDatum(MC_DATUM);
    tft->setTextColor(TFT_GREEN, TFT_BLACK);
    tft->drawString("ERRORES BORRADOS", 240, 140, 4);
    
    uint32_t waitStart = millis();
    while (millis() - waitStart < 1500) { yield(); }
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

void MenuHidden::initTouch(XPT2046_Touchscreen *touchScreen) {
    touch = touchScreen;
    Logger::info("MenuHidden touch init OK");
}

void MenuHidden::update(bool batteryIconPressed) {
    // Si hay calibración en proceso, manejarla
    if (calibState != CalibrationState::NONE) {
        bool touched = false;
        if (touch != nullptr && touch->touched()) {
            touched = true;
            // Debounce con timeout
            waitTouchRelease();
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
    if (touch != nullptr && touch->touched()) {
        TS_Point p = touch->getPoint();
        
        // Mapear coordenadas táctiles a pantalla 480x320
        int touchX = map(p.x, TOUCH_MIN_RAW, TOUCH_MAX_RAW, 0, 480);
        int touchY = map(p.y, TOUCH_MIN_RAW, TOUCH_MAX_RAW, 0, 320);
        
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
                case 3: applyRegenAdjust(REGEN_DEFAULT); break;
                case 4: applyModules(true, true, true); break;
                case 5: saveAndExit(); break;
                case 6: restoreFactory(); break;
                case 7: showErrors(); break;
                case 8: clearErrorsMenu(); break;
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