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

// 🆕 Leer posiciones P y D2 desde MCP23017 GPIOB (movidos de GPIO 47 y 48)
// MCP_PIN_SHIFTER_P = 9 (GPIOB1), MCP_PIN_SHIFTER_D2 = 10 (GPIOB2)
static volatile uint8_t cachedGPIOB = 0xFF;  // Cache del registro GPIOB
static volatile bool gpiobReadSuccess = false;

static bool readMCP23017_GPIOB() {
    // Lee el registro GPIOB completo (solo 1 lectura I2C por ciclo de update)
    Wire.beginTransmission(I2C_ADDR_MCP23017);
    Wire.write(0x13);  // GPIOB register
    uint8_t error = Wire.endTransmission();
    if (error != 0) {
        static bool errorLogged = false;
        if (!errorLogged) {
            Logger::warnf("Shifter: MCP23017 I2C error %d, P/D2 positions unavailable", error);
            errorLogged = true;
        }
        gpiobReadSuccess = false;
        return false;
    }
    Wire.requestFrom((uint8_t)I2C_ADDR_MCP23017, (uint8_t)1);
    if (Wire.available()) {
        cachedGPIOB = Wire.read();
        gpiobReadSuccess = true;
        return true;
    }
    gpiobReadSuccess = false;
    return false;
}

// Lee posición P desde MCP23017 GPIOB1
static bool readShifterP_MCP23017() {
    if (!gpiobReadSuccess) return false;
    // MCP_PIN_SHIFTER_P = 9 significa GPIOB1, bit 1 del registro GPIOB
    // Lógica invertida: LOW = activo (igual que los otros pines del shifter)
    return (cachedGPIOB & (1 << (MCP_PIN_SHIFTER_P - 8))) == 0;
}

// Lee posición D2 desde MCP23017 GPIOB2
static bool readShifterD2_MCP23017() {
    if (!gpiobReadSuccess) return false;
    // MCP_PIN_SHIFTER_D2 = 10 significa GPIOB2, bit 2 del registro GPIOB
    // Lógica invertida: LOW = activo (igual que los otros pines del shifter)
    return (cachedGPIOB & (1 << (MCP_PIN_SHIFTER_D2 - 8))) == 0;
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
    // Solo D1, N, R están en GPIO directo ahora
    pinMode(PIN_SHIFTER_D1, INPUT_PULLUP);  // D1 (Drive 1) - GPIO 7
    pinMode(PIN_SHIFTER_N, INPUT_PULLUP);   // N (Neutral) - GPIO 18
    pinMode(PIN_SHIFTER_R, INPUT_PULLUP);   // R (Reverse) - GPIO 19 (INPUT crítico)
    
    // Máscara de bits para GPIOB1 y GPIOB2 (P y D2)
    constexpr uint8_t SHIFTER_PD2_MASK = (1 << 1) | (1 << 2);  // GPIOB1 | GPIOB2
    
    // 🆕 Configurar MCP23017 GPIOB1 y GPIOB2 como entradas con pull-up para P y D2
    // IODIRB register = 0x01, bits 1 y 2 = inputs
    Wire.beginTransmission(I2C_ADDR_MCP23017);
    Wire.write(0x01);  // IODIRB register (I/O direction)
    Wire.write(SHIFTER_PD2_MASK);  // Bits 1,2 = input (P y D2), resto outputs
    Wire.endTransmission();
    
    // GPPUB register = 0x0D, enable pull-ups on GPIOB1 y GPIOB2
    Wire.beginTransmission(I2C_ADDR_MCP23017);
    Wire.write(0x0D);  // GPPUB register (pull-up)
    Wire.write(SHIFTER_PD2_MASK);  // Enable pull-up on GPIOB1 y GPIOB2
    Wire.endTransmission();
    
    Logger::info("Shifter init (D1/N/R via GPIO, P/D2 via MCP23017 GPIOB)");
}

void Shifter::update() {
    Shifter::Gear detectedGear = s.gear;

    // 🆕 Leer registro GPIOB del MCP23017 una sola vez por ciclo
    if (!readMCP23017_GPIOB()) {
        // I2C read failed - P/D2 positions unavailable, fallback to GPIO-only positions
        // Log only on first failure (handled inside readMCP23017_GPIOB)
    }

    // 🔒 CORRECCIÓN CRÍTICA: Lee cada posición con debounce
    // Lee cada posición del shifter (prioridad P > D2 > D1 > N > R)
    // 🆕 P y D2 ahora se leen desde MCP23017 GPIOB
    if(readShifterP_MCP23017())       detectedGear = Shifter::P;
    else if(readShifterD2_MCP23017()) detectedGear = Shifter::D2;
    else if(readPin(PIN_SHIFTER_D1))  detectedGear = Shifter::D1;
    else if(readPin(PIN_SHIFTER_N))   detectedGear = Shifter::N;
    else if(readPin(PIN_SHIFTER_R))   detectedGear = Shifter::R;  // GPIO 19 directo

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