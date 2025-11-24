#include "shifter.h"
#include "pins.h"
#include "logger.h"
#include "dfplayer.h"
#include "alerts.h"

static Shifter::State s = {Shifter::P, false};

// 🔒 CORRECCIÓN CRÍTICA: Debounce para prevenir lecturas erróneas por rebotes
static constexpr uint32_t DEBOUNCE_MS = 50;
static uint32_t lastChangeMs = 0;
static uint8_t stableReadings = 0;
static Shifter::Gear pendingGear = Shifter::P;

// Shifter conectado vía HY-M158 optoacopladores (señales 12V aisladas)
// NOTA: Con pull-up (interno o GPIO), idle = HIGH (1), activo cuando optoacoplador tira a LOW (0)
// Si se cambia a MCP23017 con pull-up interno, verificar que la lógica de polaridad sea consistente
// Verificar hardware: si optoacoplador invierte la señal, ajustar lógica aquí
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
    Shifter::Gear detectedGear = s.gear;

    // 🔒 CORRECCIÓN CRÍTICA: Lee cada posición con debounce
    // Lee cada posición del shifter (prioridad P > D2 > D1 > N > R)
    if(readPin(PIN_SHIFTER_P))       detectedGear = Shifter::P;
    else if(readPin(PIN_SHIFTER_D2)) detectedGear = Shifter::D2;
    else if(readPin(PIN_SHIFTER_D1)) detectedGear = Shifter::D1;
    else if(readPin(PIN_SHIFTER_N))  detectedGear = Shifter::N;
    else if(readPin(PIN_SHIFTER_R))  detectedGear = Shifter::R;

    uint32_t now = millis();
    
    // Implementar debounce: requiere lecturas estables durante DEBOUNCE_MS
    if (detectedGear != pendingGear) {
        // Nueva lectura detectada
        pendingGear = detectedGear;
        lastChangeMs = now;
        stableReadings = 1;
        s.changed = false;
    } else if (detectedGear != s.gear) {
        // La lectura coincide con pending pero aún no es el gear actual
        if (now - lastChangeMs >= DEBOUNCE_MS) {
            // Debounce completado, aceptar cambio
            s.gear = detectedGear;
            s.changed = true;
            announce(detectedGear);
            Logger::infof("Shifter: Cambio de marcha a %d", (int)detectedGear);
            stableReadings = 0;
        } else {
            stableReadings++;
            s.changed = false;
        }
    } else {
        // Lectura estable igual al gear actual
        s.changed = false;
    }
}

Shifter::State Shifter::get() { return s; }
void Shifter::setGear(Shifter::Gear g) { s.gear = g; s.changed = true; announce(g); }