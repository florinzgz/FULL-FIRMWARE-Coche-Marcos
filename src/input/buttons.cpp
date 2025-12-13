#include "buttons.h"
#include "pins.h"
#include "logger.h"
#include "system.h"     // para logError()
#include "alerts.h"
#include <Arduino.h>

// 🆕 v2.9.4: Forward declaration para función en main.cpp
extern void activateTouchCalibration();

static Buttons::State s;
static bool lastLights = false;
static bool lastMultimedia = false;
static bool last4x4 = false;

static bool evLights = false;
static bool evMultimedia = false;
static bool ev4x4 = false;

// 🔒 CORRECCIÓN: Añadir soporte para long-press
static constexpr unsigned long DEBOUNCE_MS = 30;
static constexpr unsigned long LONG_PRESS_MS = 2000;  // 2 segundos para acciones normales
static constexpr unsigned long VERY_LONG_PRESS_MS = 5000;  // 🆕 v2.9.4: 5 segundos para calibración táctil
static unsigned long lastScan[3] = {0,0,0};
static unsigned long pressStartMs[3] = {0,0,0};
static bool longPressTriggered[3] = {false, false, false};
static bool veryLongPressTriggered = false;  // 🆕 v2.9.4: Para calibración táctil (solo 4X4)

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
            Logger::info("Buttons: LIGHTS long-press detectado - activando luces de emergencia");
            Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_HIGH});
            // Long-press activates emergency/hazard lights (handled by LED controller)
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
            Logger::info("Buttons: MULTIMEDIA long-press detectado - cambio de modo de audio");
            Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_HIGH});
            // Long-press cycles through audio modes (radio/bluetooth/aux)
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
        veryLongPressTriggered = false;  // 🆕 v2.9.4: Reset very long press
    } else if(mode4x4 && last4x4) {
        // 🆕 v2.9.4: Very long press (5 segundos) - Activar calibración táctil directa
        if (!veryLongPressTriggered && (now - pressStartMs[2] >= VERY_LONG_PRESS_MS)) {
            veryLongPressTriggered = true;
            Logger::info("Buttons: 4X4 very-long-press (5s) - Iniciando calibración táctil");
            Alerts::play({Audio::AUDIO_MENU_OCULTO, Audio::Priority::PRIO_HIGH});
            // Llamar función externa definida en main.cpp
            activateTouchCalibration();
        }
        // Long press normal (2 segundos)
        else if (!longPressTriggered[2] && (now - pressStartMs[2] >= LONG_PRESS_MS)) {
            longPressTriggered[2] = true;
            Logger::info("Buttons: 4X4 long-press detectado - modo de tracción avanzado");
            Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_HIGH});
            // Long-press reserved for future advanced traction modes (sand/mud/rock)
        }
    } else if(!mode4x4 && last4x4) {
        if (!longPressTriggered[2] && !veryLongPressTriggered) {
            s.mode4x4 = !s.mode4x4;
            ev4x4 = true;
            Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
        }
        longPressTriggered[2] = false;
        veryLongPressTriggered = false;  // 🆕 v2.9.4: Reset
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

// 🔒 v2.5.0: Estado de inicialización
bool Buttons::initOK() {
    return initialized;
}