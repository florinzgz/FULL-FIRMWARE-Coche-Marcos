#include "hud.h"
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>   // Librería táctil

#include "gauges.h"
#include "wheels_display.h"
#include "icons.h"
#include "menu_hidden.h"
#include "touch_map.h"         // 👈 añadido

#include "pedal.h"
#include "steering.h"
#include "buttons.h"
#include "shifter.h"
#include "current.h"
#include "temperature.h"
#include "wheels.h"
#include "traction.h"
#include "system.h"
#include "logger.h"
#include "settings.h"
#include "storage.h"
#include "dfplayer.h"
#include "alerts.h"
#include "pins.h"

static TFT_eSPI tft;
static XPT2046_Touchscreen touch(PIN_TOUCH_CS, PIN_TOUCH_IRQ);

// Layout 480x320 (rotación 1 → 480x320)
static const int X_SPEED = 120;
static const int Y_SPEED = 180;
static const int X_RPM   = 360;
static const int Y_RPM   = 180;

// Zonas ruedas
static const int X_FL = 120;
static const int Y_FL = 90;
static const int X_FR = 360;
static const int Y_FR = 90;
static const int X_RL = 120;
static const int Y_RL = 260;
static const int X_RR = 360;
static const int Y_RR = 260;

extern Storage::Config cfg;   // acceso a flags

void HUD::init() {
    // CRITICAL: Enable backlight and reset display BEFORE tft.init()
    pinMode(PIN_TFT_BL, OUTPUT);
    digitalWrite(PIN_TFT_BL, HIGH);  // Turn on backlight
    
    pinMode(PIN_TFT_RST, OUTPUT);
    digitalWrite(PIN_TFT_RST, LOW);
    delay(10);
    digitalWrite(PIN_TFT_RST, HIGH);
    delay(10);
    
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    Gauges::init(&tft);
    WheelsDisplay::init(&tft);
    Icons::init(&tft);
    MenuHidden::init(&tft);

    if (touch.begin()) {
        touch.setRotation(1);
        Logger::info("Touchscreen XPT2046 inicializado OK");
    } else {
        Logger::error("Touchscreen XPT2046 no detectado");
        System::logError(760); // código reservado: fallo táctil
    }

    Logger::info("HUD init OK - Display ILI9488 ready");
}

void HUD::showLogo() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Mercedes AMG", 240, 120, 4);
    tft.drawString("Sistema de arranque", 240, 170, 2);
}

void HUD::showReady() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("READY", 240, 40, 4);
}

void HUD::showError() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("ERROR", 240, 40, 4);
}

void HUD::update() {
    auto pedal = Pedal::get();
    auto steer = Steering::get();
    auto sh    = Shifter::get();
    auto sys   = System::getState();
    auto tr    = Traction::get();
    auto btns  = Buttons::get();

    // Velocidad global = media ruedas delanteras
    float vFL = cfg.wheelSensorsEnabled ? Sensors::getWheelSpeed(0) : 0.0f;
    float vFR = cfg.wheelSensorsEnabled ? Sensors::getWheelSpeed(1) : 0.0f;
    float speedKmh = (vFL + vFR) * 0.5f;
    if(speedKmh > MAX_SPEED_KMH) speedKmh = MAX_SPEED_KMH;

    // Tacómetro proporcional a velocidad (placeholder)
    float rpm = (speedKmh / MAX_SPEED_KMH) * MAX_RPM;
    if(rpm > MAX_RPM) rpm = MAX_RPM;

    // Render gauges (ya optimizados internamente)
    Gauges::drawSpeed(X_SPEED, Y_SPEED, speedKmh, MAX_SPEED_KMH, pedal.percent);
    Gauges::drawRPM(X_RPM, Y_RPM, rpm, MAX_RPM);

    // Ruedas (optimizado: solo redibuja si cambian ángulo/temp/esfuerzo)
    WheelsDisplay::drawWheel(X_FL, Y_FL,
        cfg.steeringEnabled ? steer.angleFL : 0.0f,
        cfg.tempSensorsEnabled ? tr.w[Traction::FL].tempC : 0.0f,
        cfg.currentSensorsEnabled ? tr.w[Traction::FL].effortPct : 0.0f
    );
    WheelsDisplay::drawWheel(X_FR, Y_FR,
        cfg.steeringEnabled ? steer.angleFR : 0.0f,
        cfg.tempSensorsEnabled ? tr.w[Traction::FR].tempC : 0.0f,
        cfg.currentSensorsEnabled ? tr.w[Traction::FR].effortPct : 0.0f
    );
    WheelsDisplay::drawWheel(X_RL, Y_RL,
        0.0f,
        cfg.tempSensorsEnabled ? tr.w[Traction::RL].tempC : 0.0f,
        cfg.currentSensorsEnabled ? tr.w[Traction::RL].effortPct : 0.0f
    );
    WheelsDisplay::drawWheel(X_RR, Y_RR,
        0.0f,
        cfg.tempSensorsEnabled ? tr.w[Traction::RR].tempC : 0.0f,
        cfg.currentSensorsEnabled ? tr.w[Traction::RR].effortPct : 0.0f
    );

    // Iconos y estados
    Icons::drawSystemState(sys);
    Icons::drawGear(sh.gear);
    Icons::drawFeatures(
        cfg.lightsEnabled && btns.lights,
        cfg.multimediaEnabled && btns.multimedia,
        tr.enabled4x4,
        pedal.percent > 0 && !btns.mode4x4
    );
    Icons::drawBattery(cfg.currentSensorsEnabled ? Sensors::getVoltage(0) : 0.0f);

    // Icono de advertencia si hay errores almacenados
    Icons::drawErrorWarning();

    // --- Lectura táctil usando touch_map ---
    bool batteryTouch = false;
    if (touch.touched()) {
        TS_Point p = touch.getPoint();
        int x = map(p.x, 200, 3900, 0, 480);
        int y = map(p.y, 200, 3900, 0, 320);

        TouchAction act = getTouchedZone(x, y);
        switch (act) {
            case TouchAction::Battery:
                batteryTouch = true;
                break;
            case TouchAction::Lights:
                Logger::info("Toque en icono luces");
                break;
            case TouchAction::Multimedia:
                Logger::info("Toque en icono multimedia");
                break;
            case TouchAction::Mode4x4:
                Logger::info("Toque en icono 4x4");
                break;
            case TouchAction::Warning:
                Logger::info("Toque en icono warning");
                break;
            default:
                break;
        }
    }

    // Menú oculto: botón físico o toque en batería
    MenuHidden::update(btns.batteryIcon || batteryTouch);
}