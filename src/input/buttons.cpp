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

// 🔒 CORRECCIÓN: Añadir soporte para long-press
static constexpr unsigned long DEBOUNCE_MS = 30;
static constexpr unsigned long LONG_PRESS_MS = 2000;  // 2 segundos
static unsigned long lastScan[3] = {0,0,0};
static unsigned long pressStartMs[3] = {0,0,0};
static bool longPressTriggered[3] = {false, false, false};

static bool initialized = false;

static bool readPin(uint8_t pin, int idx) {
    if(pin == 0xFF) {
        Logger::error("Buttons: pin inválido");
        System::logError(740);
        return false;
    }
    bool reading = (digitalRead(pin) == 0);  // 0 = LOW, avoid conflict with Audio::Priority::PRIO_LOW
    unsigned long now = millis();
    
    // 🔒 CORRECCIÓN: Debounce mejorado
    if(now - lastScan[idx] < DEBOUNCE_MS) {
        return (idx==0?lastLights: idx==1?lastMultimedia: last4x4);
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

    unsigned long now = millis();
    
    bool lights      = readPin(PIN_BTN_LIGHTS, 0);
    bool multimedia  = readPin(PIN_BTN_MEDIA, 1);
    bool mode4x4     = readPin(PIN_BTN_4X4, 2);
    // batteryIcon button removed - no longer available

    // 🔒 CORRECCIÓN: Implementar long-press para cada botón
    // Botón LIGHTS
    if(lights && !lastLights) {
        // Botón presionado - iniciar timer
        pressStartMs[0] = now;
        longPressTriggered[0] = false;
    } else if(lights && lastLights) {
        // Botón mantenido - verificar long press
        if (!longPressTriggered[0] && (now - pressStartMs[0] >= LONG_PRESS_MS)) {
            longPressTriggered[0] = true;
            Logger::info("Buttons: LIGHTS long-press detectado");
            Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_HIGH});
            // TODO: Acción específica para long-press (ej: activar luces especiales)
        }
    } else if(!lights && lastLights) {
        // Botón liberado - toggle solo si no fue long-press
        if (!longPressTriggered[0]) {
            s.lights = !s.lights;
            evLights = true;
            Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
        }
        longPressTriggered[0] = false;
    }
    
    // Botón MULTIMEDIA
    if(multimedia && !lastMultimedia) {
        pressStartMs[1] = now;
        longPressTriggered[1] = false;
    } else if(multimedia && lastMultimedia) {
        if (!longPressTriggered[1] && (now - pressStartMs[1] >= LONG_PRESS_MS)) {
            longPressTriggered[1] = true;
            Logger::info("Buttons: MULTIMEDIA long-press detectado");
            Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_HIGH});
            // TODO: Acción específica para long-press
        }
    } else if(!multimedia && lastMultimedia) {
        if (!longPressTriggered[1]) {
            s.multimedia = !s.multimedia;
            evMultimedia = true;
            Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
        }
        longPressTriggered[1] = false;
    }
    
    // Botón 4X4
    if(mode4x4 && !last4x4) {
        pressStartMs[2] = now;
        longPressTriggered[2] = false;
    } else if(mode4x4 && last4x4) {
        if (!longPressTriggered[2] && (now - pressStartMs[2] >= LONG_PRESS_MS)) {
            longPressTriggered[2] = true;
            Logger::info("Buttons: 4X4 long-press detectado");
            Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_HIGH});
            // TODO: Acción específica para long-press
        }
    } else if(!mode4x4 && last4x4) {
        if (!longPressTriggered[2]) {
            s.mode4x4 = !s.mode4x4;
            ev4x4 = true;
            Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
        }
        longPressTriggered[2] = false;
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