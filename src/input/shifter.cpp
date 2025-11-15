#include "shifter.h"
#include "pins.h"
#include "logger.h"
#include "dfplayer.h"
#include "alerts.h"

static Shifter::State s = {Shifter::P, false};

// Shifter conectado vía HY-M158 optoacopladores (señales 12V aisladas)
// Lee entrada digital con pull-up (LOW = activo)
static bool readPin(uint8_t pin) { return digitalRead(pin) == 0; }

static void announce(Shifter::Gear g) {
    // Use generic AUDIO_MODULO_OK for now - specific gear tracks need to be defined
    (void)g; // Suppress unused warning
    Alerts::play(Audio::AUDIO_MODULO_OK);
    // TODO: Add specific gear audio tracks to alerts.h:
    // AUDIO_MARCHA_P, AUDIO_MARCHA_D2, AUDIO_MARCHA_D1, AUDIO_MARCHA_N, AUDIO_MARCHA_R
}

void Shifter::init() {
    // Pines shifter conectados vía HY-M158 optoacopladores (12V → 3.3V)
    pinMode(PIN_SHIFTER_P, INPUT_PULLUP);   // P (Park)
    pinMode(PIN_SHIFTER_D2, INPUT_PULLUP);  // D2 (Drive 2)
    pinMode(PIN_SHIFTER_D1, INPUT_PULLUP);  // D1 (Drive 1)
    pinMode(PIN_SHIFTER_N, INPUT_PULLUP);   // N (Neutral)
    pinMode(PIN_SHIFTER_R, INPUT_PULLUP);   // R (Reverse)
    Logger::info("Shifter init (via HY-M158)");
}

void Shifter::update() {
    Shifter::Gear newGear = s.gear;

    // Lee cada posición del shifter (prioridad P > D2 > D1 > N > R)
    if(readPin(PIN_SHIFTER_P))       newGear = Shifter::P;
    else if(readPin(PIN_SHIFTER_D2)) newGear = Shifter::D2;
    else if(readPin(PIN_SHIFTER_D1)) newGear = Shifter::D1;
    else if(readPin(PIN_SHIFTER_N))  newGear = Shifter::N;
    else if(readPin(PIN_SHIFTER_R))  newGear = Shifter::R;

    if(newGear != s.gear) {
        s.gear = newGear;
        s.changed = true;
        announce(newGear);
        Logger::info("Gear changed");
    } else {
        s.changed = false;
    }
}

Shifter::State Shifter::get() { return s; }
void Shifter::setGear(Shifter::Gear g) { s.gear = g; s.changed = true; announce(g); }