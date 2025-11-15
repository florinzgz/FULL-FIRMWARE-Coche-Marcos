#include "buttons.h"
#include "pins.h"
#include "logger.h"
#include "system.h"     // para logError()
#include "alerts.h"
#include <Arduino.h>

static Buttons::State s;
static bool lastLights = false;
static bool lastMultimedia = false;
static bool last4x4 = false;

static bool evLights = false;
static bool evMultimedia = false;
static bool ev4x4 = false;

static const unsigned long debounceMs = 30;
static unsigned long lastScan[4] = {0,0,0,0};

static bool initialized = false;

static bool readPin(uint8_t pin, int idx) {
    if(pin == 0xFF) {
        Logger::error("Buttons: pin inválido");
        System::logError(740);
        return false;
    }
    bool reading = (digitalRead(pin) == 0);  // 0 = LOW, avoid conflict with Audio::Priority::PRIO_LOW
    unsigned long now = millis();
    if(now - lastScan[idx] < debounceMs) {
        return (idx==0?lastLights: idx==1?lastMultimedia: idx==2?last4x4: s.batteryIcon);
    }
    lastScan[idx] = now;
    return reading;
}

void Buttons::init() {
    pinMode(PIN_BTN_LIGHTS,    INPUT_PULLUP);
    pinMode(PIN_BTN_MEDIA,     INPUT_PULLUP);
    pinMode(PIN_BTN_4X4,       INPUT_PULLUP);
    // PIN_BTN_BATTERY removed - no longer available

    s = {false, false, false, false};
    lastLights = lastMultimedia = last4x4 = false;
    evLights = evMultimedia = ev4x4 = false;

    initialized = true;
    Logger::info("Buttons init OK");
}

void Buttons::update() {
    if(!initialized) {
        Logger::warn("Buttons::update() llamado sin init");
        return;
    }

    bool lights      = readPin(PIN_BTN_LIGHTS, 0);
    bool multimedia  = readPin(PIN_BTN_MEDIA, 1);
    bool mode4x4     = readPin(PIN_BTN_4X4, 2);
    // batteryIcon button removed - no longer available

    if(lights && !lastLights) {
        s.lights = !s.lights;
        evLights = true;
        Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
    }
    if(multimedia && !lastMultimedia) {
        s.multimedia = !s.multimedia;
        evMultimedia = true;
        Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
    }
    if(mode4x4 && !last4x4) {
        s.mode4x4 = !s.mode4x4;
        ev4x4 = true;
        Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
    }

    lastLights = lights;
    lastMultimedia = multimedia;
    last4x4 = mode4x4;
}

const Buttons::State& Buttons::get() {
    return s;
}

bool Buttons::toggledLights() {
    bool e = evLights;
    evLights = false;
    return e;
}

bool Buttons::toggledMultimedia() {
    bool e = evMultimedia;
    evMultimedia = false;
    return e;
}

bool Buttons::toggled4x4() {
    bool e = ev4x4;
    ev4x4 = false;
    return e;
}