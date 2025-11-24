#include "shifter.h"
#include "pins.h"
#include "logger.h"
#include "dfplayer.h"
#include "alerts.h"
#include <Wire.h>

static Shifter::State s = {Shifter::P, false};

// 🔒 CORRECCIÓN CRÍTICA: Debounce para prevenir lecturas erróneas por rebotes
static constexpr uint32_t DEBOUNCE_MS = 50;
static uint32_t lastChangeMs = 0;
static uint8_t stableReadings = 0;
static Shifter::Gear pendingGear = Shifter::P;

// Shifter conectado vía HY-M158 optoacopladores (señales 12V aisladas)
// Lee entrada digital con pull-up (LOW = activo)
static bool readPin(uint8_t pin) { return digitalRead(pin) == 0; }

// 🔒 CORRECCIÓN: Leer posición R desde MCP23017 GPIOB0 (evita conflicto con LED_REAR en GPIO 19)
static bool readShifterR_MCP23017() {
    // MCP23017 GPIOB register = 0x13, bit 0 = GPIOB0
    Wire.beginTransmission(I2C_ADDR_MCP23017);
    Wire.write(0x13);  // GPIOB register
    uint8_t error = Wire.endTransmission();
    if (error != 0) {
        // Log I2C error only once to avoid log spam
        static bool errorLogged = false;
        if (!errorLogged) {
            Logger::warnf("Shifter: MCP23017 I2C error %d, R position unavailable", error);
            errorLogged = true;
        }
        return false;  // Error I2C, asumir no activo
    }
    Wire.requestFrom((uint8_t)I2C_ADDR_MCP23017, (uint8_t)1);
    if (Wire.available()) {
        uint8_t gpiob = Wire.read();
        // MCP_PIN_SHIFTER_R = 8 significa GPIOB0, bit 0 del registro GPIOB
        // Lógica invertida: LOW = activo (igual que los otros pines del shifter)
        return (gpiob & (1 << (MCP_PIN_SHIFTER_R - 8))) == 0;
    }
    return false;
}

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
    // 🔒 NOTA: PIN_SHIFTER_R ahora se lee desde MCP23017 GPIOB0 (evita conflicto con LED_REAR GPIO 19)
    // Configurar MCP23017 GPIOB0 como entrada con pull-up
    Wire.beginTransmission(I2C_ADDR_MCP23017);
    Wire.write(0x01);  // IODIRB register (I/O direction)
    Wire.write(0x01);  // Bit 0 = input (1), resto outputs (0)
    Wire.endTransmission();
    Wire.beginTransmission(I2C_ADDR_MCP23017);
    Wire.write(0x0D);  // GPPUB register (pull-up)
    Wire.write(0x01);  // Enable pull-up on GPIOB0
    Wire.endTransmission();
    Logger::info("Shifter init (via HY-M158 + MCP23017 for R)");
}

void Shifter::update() {
    Shifter::Gear detectedGear = s.gear;

    // 🔒 CORRECCIÓN CRÍTICA: Lee cada posición con debounce
    // Lee cada posición del shifter (prioridad P > D2 > D1 > N > R)
    if(readPin(PIN_SHIFTER_P))       detectedGear = Shifter::P;
    else if(readPin(PIN_SHIFTER_D2)) detectedGear = Shifter::D2;
    else if(readPin(PIN_SHIFTER_D1)) detectedGear = Shifter::D1;
    else if(readPin(PIN_SHIFTER_N))  detectedGear = Shifter::N;
    // 🔒 NOTA: R se lee desde MCP23017 en lugar de GPIO directo
    else if(readShifterR_MCP23017()) detectedGear = Shifter::R;

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