#include "buttons.h"
#include "pins.h"
#include "logger.h"
#include "system.h"     // para logError()
#include "alerts.h"
#include <Arduino.h>

// 🆕 v2.9.4: Forward declaration para función en main.cpp
extern void activateTouchCalibration();

enum Btn { BTN_LIGHTS = 0, BTN_MEDIA = 1, BTN_4X4 = 2, BTN_COUNT = 3 };

static Buttons::State s;
static bool lastState[BTN_COUNT]       = {false, false, false};
static bool stableState[BTN_COUNT]     = {false, false, false}; // ÚLTIMO ESTADO ESTABLE
static unsigned long lastSignal[BTN_COUNT]        = {0,0,0};
static unsigned long pressStartMs[BTN_COUNT]      = {0,0,0};
static bool longPressTriggered[BTN_COUNT]         = {false, false, false};
static bool veryLongPressTriggered                = false; // solo BTN_4X4

static bool ev[BTN_COUNT] = {false,false,false};

static bool initialized = false;

// Logger antispam
static bool invalidPinLogged[BTN_COUNT] = {false, false, false};

// Parámetros
static constexpr unsigned long DEBOUNCE_MS       = 30;
static constexpr unsigned long LONG_PRESS_MS     = 2000;
static constexpr unsigned long VERY_LONG_PRESS_MS= 5000;

static bool readPin(uint8_t pin, int idx) {
    if(pin == 0xFF) {
        if (!invalidPinLogged[idx]) {
            Logger::error("Buttons: pin inválido");
            System::logError(740);
            invalidPinLogged[idx] = true;
        }
        return false;
    }
    bool nowState = (digitalRead(pin) == 0); // LOW==pressed
    unsigned long now = millis();

    // Debounce: solo establezco stableState si el valor nuevo se mantiene el tiempo mínimo
    if (nowState != stableState[idx]) {
        // Flanco²: inicio ventana de debounce
        lastSignal[idx] = now;
    }
    // Si la duración del valor nuevo supera DEBOUNCE_MS, ya es estable
    if (now - lastSignal[idx] >= DEBOUNCE_MS) {
        stableState[idx] = nowState;
    }
    // Devuelvo el último estable, no el inmediato
    return stableState[idx];
}

void Buttons::init() {
    pinMode(PIN_BTN_LIGHTS,    INPUT_PULLUP);
    pinMode(PIN_BTN_MEDIA,     INPUT_PULLUP);
    pinMode(PIN_BTN_4X4,       INPUT_PULLUP);

    s = {false, false, false, false};
    for(int i=0;i<BTN_COUNT;i++) {
        lastState[i] = stableState[i] = ev[i] = false;
        invalidPinLogged[i] = false;
        longPressTriggered[i] = false;
        lastSignal[i] = 0;
        pressStartMs[i] = 0;
    }
    veryLongPressTriggered = false;
    initialized = true;
    Logger::info("Buttons init OK");
}

void Buttons::update() {
    if(!initialized) {
        Logger::warn("Buttons::update() llamado sin init");
        return;
    }

    unsigned long now = millis();
    bool state[BTN_COUNT] = {
        readPin(PIN_BTN_LIGHTS, BTN_LIGHTS),
        readPin(PIN_BTN_MEDIA,  BTN_MEDIA),
        readPin(PIN_BTN_4X4,    BTN_4X4)
    };

    // ----- BTN_LIGHTS -----
    if(state[BTN_LIGHTS] && !lastState[BTN_LIGHTS]) {
        pressStartMs[BTN_LIGHTS] = now;
        longPressTriggered[BTN_LIGHTS] = false;
    } else if (state[BTN_LIGHTS] && lastState[BTN_LIGHTS]) {
        if (!longPressTriggered[BTN_LIGHTS] && now - pressStartMs[BTN_LIGHTS] >= LONG_PRESS_MS) {
            longPressTriggered[BTN_LIGHTS] = true;
            Logger::info("Buttons: LIGHTS long-press detectado");
            Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_HIGH});
            // TODO: activar luces emergencia/hazard aquí
        }
    } else if(!state[BTN_LIGHTS] && lastState[BTN_LIGHTS]) {
        // botón soltado
        if (!longPressTriggered[BTN_LIGHTS]) {
            s.lights = !s.lights;
            ev[BTN_LIGHTS] = true;
            Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
        }
        longPressTriggered[BTN_LIGHTS] = false;
        pressStartMs[BTN_LIGHTS] = 0; // para evitar rebotes
    }

    // ----- BTN_MEDIA -----
    if(state[BTN_MEDIA] && !lastState[BTN_MEDIA]) {
        pressStartMs[BTN_MEDIA] = now;
        longPressTriggered[BTN_MEDIA] = false;
    } else if (state[BTN_MEDIA] && lastState[BTN_MEDIA]) {
        if (!longPressTriggered[BTN_MEDIA] && now - pressStartMs[BTN_MEDIA] >= LONG_PRESS_MS) {
            longPressTriggered[BTN_MEDIA] = true;
            Logger::info("Buttons: MULTIMEDIA long-press detectado - cambio de modo");
            Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_HIGH});
            // TODO: cambiar modo de audio aquí
        }
    } else if(!state[BTN_MEDIA] && lastState[BTN_MEDIA]) {
        if (!longPressTriggered[BTN_MEDIA]) {
            s.multimedia = !s.multimedia;
            ev[BTN_MEDIA] = true;
            Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
        }
        longPressTriggered[BTN_MEDIA] = false;
        pressStartMs[BTN_MEDIA] = 0;
    }

    // ----- BTN_4X4 -----
    if(state[BTN_4X4] && !lastState[BTN_4X4]) {
        pressStartMs[BTN_4X4] = now;
        longPressTriggered[BTN_4X4] = false;
        veryLongPressTriggered       = false;
    } else if (state[BTN_4X4] && lastState[BTN_4X4]) {
        if (!veryLongPressTriggered && now - pressStartMs[BTN_4X4] >= VERY_LONG_PRESS_MS) {
            veryLongPressTriggered = true;
            longPressTriggered[BTN_4X4] = true; // BLOQUEA cualquier toggle tras soltar
            Logger::info("Buttons: 4X4 very-long-press (5s) - calibración táctil");
            Alerts::play({Audio::AUDIO_MENU_OCULTO, Audio::Priority::PRIO_HIGH});
            activateTouchCalibration();
        } else if (!longPressTriggered[BTN_4X4] && now - pressStartMs[BTN_4X4] >= LONG_PRESS_MS) {
            longPressTriggered[BTN_4X4] = true;
            Logger::info("Buttons: 4X4 long-press detectado");
            Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_HIGH});
        }
    } else if(!state[BTN_4X4] && lastState[BTN_4X4]) {
        // BLOQUEA SI SE DISPARÓ VERY LONG PRESS
        if (!longPressTriggered[BTN_4X4] && !veryLongPressTriggered) {
            s.mode4x4 = !s.mode4x4;
            ev[BTN_4X4] = true;
            Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
        }
        longPressTriggered[BTN_4X4] = false;
        veryLongPressTriggered = false;
        pressStartMs[BTN_4X4] = 0;
    }

    // Actualiza lastState[]
    for(int i=0;i<BTN_COUNT;i++) {
        lastState[i] = state[i];
    }
}

const Buttons::State& Buttons::get() { return s; }
bool Buttons::toggledLights()        { bool e = ev[BTN_LIGHTS];   ev[BTN_LIGHTS]=false; return e; }
bool Buttons::toggledMultimedia()    { bool e = ev[BTN_MEDIA];    ev[BTN_MEDIA]=false; return e; }
bool Buttons::toggled4x4()           { bool e = ev[BTN_4X4];      ev[BTN_4X4]=false; return e; }
bool Buttons::initOK()               { return initialized; }