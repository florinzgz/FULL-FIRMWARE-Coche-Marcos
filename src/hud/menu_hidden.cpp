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

static TFT_eSPI *tft = nullptr;

static bool menuActive = false;
static uint16_t codeBuffer = 0;
static const uint16_t accessCode = 8989;

static int selectedOption = 1;   // opción seleccionada (1..8)

// Cache para evitar redibujos innecesarios
static int lastSelectedOption = -1;
static uint16_t lastCodeBuffer = 0;
static bool lastMenuActive = false;

// -----------------------
// Funciones auxiliares (stubs)
// -----------------------
static void applyCalibrationPedal() {
    Logger::info("Calibración de pedal ejecutada");
}
static void applyCalibrationEncoder() {
    Logger::info("Calibración de encoder ejecutada");
}
static void applyRegenAdjust(int value) {
    Logger::infof("Ajuste de regen a %d%%", value);
}
static void applyModules(bool lights, bool media, bool traction) {
    Logger::infof("Módulos ajustados: lights=%d media=%d traction=%d", lights, media, traction);
}
static void saveAndExit() {
    Storage::save(cfg);
    Logger::info("Configuración guardada. Saliendo de menú oculto.");
    menuActive = false;
    codeBuffer = 0;
    lastMenuActive = false;
}
static void restoreFactory() {
    Storage::defaults(cfg);
    Logger::info("Configuración restaurada a valores de fábrica.");
    Storage::save(cfg);
}
static void showErrors() {
    int count = System::getErrorCount();
    Logger::infof("Errores persistentes: %d", count);
}
static void clearErrorsMenu() {
    System::clearErrors();
    Logger::info("Errores persistentes borrados.");
}

// -----------------------
// Dibujo del menú
// -----------------------
static void drawMenuFull() {
    tft->fillRect(60, 40, 360, 240, TFT_BLACK);
    tft->drawRect(60, 40, 360, 240, TFT_WHITE);
    tft->setTextDatum(TL_DATUM);

    const char* items[8] = {
        "1) Calibrar pedal",
        "2) Calibrar encoder",
        "3) Ajuste regen (%)",
        "4) Modulos ON/OFF",
        "5) Guardar y salir",
        "6) Restaurar fabrica",
        "7) Ver errores",
        "8) Borrar errores"
    };

    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->drawString("MENU OCULTO", 80, 50, 2);

    for(int i=0; i<8; i++) {
        uint16_t col = (i+1 == selectedOption) ? TFT_YELLOW : TFT_WHITE;
        tft->setTextColor(col, TFT_BLACK);
        tft->drawString(items[i], 80, 80 + i*20, 2);
    }

    // Código
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->drawString("Code:", 80, 240, 2);
    tft->drawString(String(codeBuffer), 140, 240, 2);
}

static void updateOptionHighlight() {
    const char* items[8] = {
        "1) Calibrar pedal",
        "2) Calibrar encoder",
        "3) Ajuste regen (%)",
        "4) Modulos ON/OFF",
        "5) Guardar y salir",
        "6) Restaurar fabrica",
        "7) Ver errores",
        "8) Borrar errores"
    };

    // Redibujar solo la línea anterior y la nueva
    if(lastSelectedOption != -1 && lastSelectedOption != selectedOption) {
        tft->setTextColor(TFT_WHITE, TFT_BLACK);
        tft->drawString(items[lastSelectedOption-1], 80, 80 + (lastSelectedOption-1)*20, 2);
    }
    tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    tft->drawString(items[selectedOption-1], 80, 80 + (selectedOption-1)*20, 2);
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

    auto btns = Buttons::get();

    // TODO: Implement touch-based menu navigation
    // For now, menu options can only be changed via touch screen
    /*
    if(btns.up) {
        if(selectedOption > 1) selectedOption--;
        else selectedOption = 8;
    }
    if(btns.down) {
        if(selectedOption < 8) selectedOption++;
        else selectedOption = 1;
    }
    if(selectedOption != lastSelectedOption) {
        updateOptionHighlight();
        lastSelectedOption = selectedOption;
    }

    if(btns.ok) {
        switch(selectedOption) {
            case 1: applyCalibrationPedal(); break;
            case 2: applyCalibrationEncoder(); break;
            case 3: applyRegenAdjust(REGEN_DEFAULT); break;
            case 4: applyModules(true, true, true); break;
            case 5: saveAndExit(); break;
            case 6: restoreFactory(); break;
            case 7: showErrors(); break;
            case 8: clearErrorsMenu(); break;
        }
        drawMenuFull(); // refresco completo tras acción
        lastSelectedOption = selectedOption;
        lastCodeBuffer = codeBuffer;
    }
    */
}