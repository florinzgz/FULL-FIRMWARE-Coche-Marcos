#include "buttons.h"
#include "pins.h"
#include "logger.h"
#include "system.h"     // para logError()
#include "alerts.h"
#include <Arduino.h>

// 🆕 v2.9.4: Forward declaration para función en main.cpp
extern void activateTouchCalibration();

// v2.14.0: Simplificado - solo botón de luces físico
// Multimedia y 4x4 ahora son solo touch screen
enum Btn { BTN_LIGHTS = 0, BTN_COUNT = 1 };

static Buttons::State s;
static bool lastState[BTN_COUNT]       = {false};
static bool stableState[BTN_COUNT]     = {false}; // ÚLTIMO ESTADO ESTABLE
static unsigned long lastSignal[BTN_COUNT]        = {0};
static unsigned long pressStartMs[BTN_COUNT]      = {0};
static bool longPressTriggered[BTN_COUNT]         = {false};

static bool ev[BTN_COUNT] = {false};

static bool initialized = false;

// Logger antispam
static bool invalidPinLogged[BTN_COUNT] = {false};

// Parámetros
static constexpr unsigned long DEBOUNCE_MS       = 30;
static constexpr unsigned long LONG_PRESS_MS     = 2000;

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
        // Flanco: inicio ventana de debounce
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
    // v2.14.0: BTN_MEDIA y BTN_4X4 eliminados

    s = {false};
    for(int i=0;i<BTN_COUNT;i++) {
        lastState[i] = stableState[i] = ev[i] = false;
        invalidPinLogged[i] = false;
        longPressTriggered[i] = false;
        lastSignal[i] = 0;
        pressStartMs[i] = 0;
    }
    // v2.14.0: Removed veryLongPressTriggered
    initialized = true;
    Logger::info("Buttons init OK (lights only)");
}

void Buttons::update() {
    if(!initialized) {
        Logger::warn("Buttons::update() llamado sin init");
        return;
    }

    unsigned long now = millis();
    bool state[BTN_COUNT] = {
        readPin(PIN_BTN_LIGHTS, BTN_LIGHTS)
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

    // v2.14.0: BTN_MEDIA and BTN_4X4 removed - now touch-only

    // Actualiza lastState[]
    for(int i=0;i<BTN_COUNT;i++) {
        lastState[i] = state[i];
    }
}

const Buttons::State& Buttons::get() { return s; }
bool Buttons::toggledLights()        { bool e = ev[BTN_LIGHTS];   ev[BTN_LIGHTS]=false; return e; }
// v2.14.0: toggledMultimedia() and toggled4x4() removed - touch-only
bool Buttons::initOK()               { return initialized; }