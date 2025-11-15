#include "icons.h"
#include "system.h"   // para consultar errores persistentes
#include "logger.h"
#include <TFT_eSPI.h>
#include <Arduino.h>  // para constrain()
#include <math.h>     // para fabs()

static TFT_eSPI *tft = nullptr;
static bool initialized = false;

// Cache de último estado para evitar redibujos innecesarios
static System::State lastSysState = (System::State)-1;
static Shifter::Gear lastGear = (Shifter::Gear)-1;
static bool lastLights = false;
static bool lastMedia = false;
static bool lastMode4x4 = false;
static bool lastRegen = false;
static float lastBattery = -999.0f;
static int lastErrorCount = -1;

void Icons::init(TFT_eSPI *display) {
    tft = display;
    initialized = true;
    Logger::info("Icons init OK");
}

void Icons::drawSystemState(System::State st) {
    if(!initialized) return;
    if(st == lastSysState) return; // no cambio → no redibujar
    lastSysState = st;

    const char* txt = "OFF";
    uint16_t col = TFT_WHITE;
    switch(st){
        case System::PRECHECK: txt="PRE";   col=TFT_YELLOW; break;
        case System::READY:    txt="READY"; col=TFT_GREEN;  break;
        case System::RUN:      txt="RUN";   col=TFT_CYAN;   break;
        case System::ERROR:    txt="ERROR"; col=TFT_RED;    break;
        default: break;
    }
    tft->fillRect(5, 5, 80, 20, TFT_BLACK);
    tft->setTextDatum(TL_DATUM);
    tft->setTextColor(col, TFT_BLACK);
    tft->drawString(txt, 10, 10, 2);
}

void Icons::drawGear(Shifter::Gear g) {
    if(!initialized) return;
    if(g == lastGear) return;
    lastGear = g;

    const char* txt = "P";
    switch(g){
        case Shifter::D2: txt="D2"; break;
        case Shifter::D1: txt="D1"; break;
        case Shifter::N:  txt="N";  break;
        case Shifter::R:  txt="R";  break;
        default: txt="P"; break;
    }
    tft->fillRect(395, 5, 80, 20, TFT_BLACK);
    tft->setTextDatum(TL_DATUM);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->drawString(txt, 410, 10, 2);
}

void Icons::drawFeatures(bool lights, bool media, bool mode4x4, bool regenOn) {
    if(!initialized) return;
    if(lights==lastLights && media==lastMedia && mode4x4==lastMode4x4 && regenOn==lastRegen) return;

    // Luces
    if(lights != lastLights) {
        tft->fillRect(LIGHTS_X1, LIGHTS_Y1, LIGHTS_X2 - LIGHTS_X1, LIGHTS_Y2 - LIGHTS_Y1, TFT_BLACK);
        tft->setTextDatum(TL_DATUM);
        tft->setTextColor(lights ? TFT_YELLOW : TFT_DARKGREY, TFT_BLACK);
        tft->drawString("LUCES", LIGHTS_X1 + 5, LIGHTS_Y1 + 20, 2);
        lastLights = lights;
    }

    // Multimedia
    if(media != lastMedia) {
        tft->fillRect(MEDIA_X1, MEDIA_Y1, MEDIA_X2 - MEDIA_X1, MEDIA_Y2 - MEDIA_Y1, TFT_BLACK);
        tft->setTextColor(media ? TFT_GREEN : TFT_DARKGREY, TFT_BLACK);
        tft->drawString("MEDIA", MEDIA_X1 + 5, MEDIA_Y1 + 20, 2);
        lastMedia = media;
    }

    // 4x4 / 4x2
    if(mode4x4 != lastMode4x4) {
        tft->fillRect(MODE4X4_X1, MODE4X4_Y1, MODE4X4_X2 - MODE4X4_X1, MODE4X4_Y2 - MODE4X4_Y1, TFT_BLACK);
        tft->setTextColor(mode4x4 ? TFT_CYAN : TFT_ORANGE, TFT_BLACK);
        tft->drawString(mode4x4 ? "4x4" : "4x2", MODE4X4_X1 + 5, MODE4X4_Y1 + 10, 2);
        lastMode4x4 = mode4x4;
    }

    // Regenerativo
    if(regenOn != lastRegen) {
        tft->fillRect(310, 285, 100, 30, TFT_BLACK);
        tft->setTextColor(regenOn ? TFT_BLUE : TFT_DARKGREY, TFT_BLACK);
        tft->drawString("REGEN", 315, 290, 2);
        lastRegen = regenOn;
    }
}

void Icons::drawBattery(float volts) {
    if(!initialized) return;
    volts = constrain(volts, 0.0f, 99.9f);
    if(fabs(volts - lastBattery) < 0.1f) return; // no cambio significativo
    lastBattery = volts;

    tft->fillRect(BATTERY_X1, BATTERY_Y1, BATTERY_X2 - BATTERY_X1, BATTERY_Y2 - BATTERY_Y1, TFT_BLACK);
    tft->setTextDatum(TL_DATUM);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f V", volts);
    tft->drawString(buf, BATTERY_X1 + 5, BATTERY_Y1 + 10, 2);
}

void Icons::drawErrorWarning() {
    if(!initialized) return;
    int count = System::getErrorCount();
    if(count == lastErrorCount) return;
    lastErrorCount = count;

    if(count > 0) {
        int midX = (WARNING_X1 + WARNING_X2) / 2;
        int topY = WARNING_Y1 + 5;
        int botY = WARNING_Y2 - 5;
        tft->fillTriangle(WARNING_X1, botY, WARNING_X2, botY, midX, topY, TFT_YELLOW);
        tft->setTextColor(TFT_YELLOW, TFT_BLACK);
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", count);
        tft->drawString(buf, WARNING_X2 + 5, WARNING_Y1 + 15, 2);
    } else {
        tft->fillRect(WARNING_X1, WARNING_Y1, (WARNING_X2 - WARNING_X1) + 40, WARNING_Y2 - WARNING_Y1, TFT_BLACK);
    }
}