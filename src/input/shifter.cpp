#include "shifter.h"
#include "pins.h"
#include "logger.h"
#include "dfplayer.h"
#include "alerts.h"
#include <Adafruit_MCP23X17.h>

static Shifter::State s = {Shifter::P, false};

// 🔒 v2.5.0: Flag de inicialización
static bool initialized = false;

// 🔒 CORRECCIÓN CRÍTICA: Debounce para prevenir lecturas erróneas por rebotes
static constexpr uint32_t DEBOUNCE_MS = 50;
static uint32_t lastChangeMs = 0;
static uint8_t stableReadings = 0;
static Shifter::Gear pendingGear = Shifter::P;

// ✅ v2.3.0: Todo el shifter ahora vía MCP23017 (GPIOB0-B4, pines 8-12 consecutivos)
static Adafruit_MCP23X17* mcpShifter = nullptr;
static bool mcpAvailable = false;

// Lee entrada del MCP23017 con pull-up interno (LOW = activo)
// Los optoacopladores HY-M158 invierten: activo = LED encendido = transistor conduce = LOW
static bool readMcpPin(uint8_t pin) {
    if (!mcpAvailable || mcpShifter == nullptr) return false;
    return mcpShifter->digitalRead(pin) == 0;
}

static void announce(Shifter::Gear g) {
    // Use generic AUDIO_MODULO_OK for now - specific gear tracks need to be defined
    (void)g; // Suppress unused warning
    Alerts::play(Audio::AUDIO_MODULO_OK);
    // TODO: Add specific gear audio tracks to alerts.h:
    // AUDIO_MARCHA_P, AUDIO_MARCHA_D2, AUDIO_MARCHA_D1, AUDIO_MARCHA_N, AUDIO_MARCHA_R
}

void Shifter::init() {
    // ✅ v2.3.0: Todo el shifter ahora vía MCP23017 (GPIOB0-B4)
    // Pines consecutivos 8-12 para las 5 posiciones: P, R, N, D1, D2
    
    // 🔒 CRITICAL FIX: Prevent memory leak on repeated init
    // Delete existing object if init() is called multiple times
    if (mcpShifter != nullptr) {
        delete mcpShifter;
        mcpShifter = nullptr;
        mcpAvailable = false;
    }
    
    mcpShifter = new Adafruit_MCP23X17();
    
    // 🔒 CRITICAL FIX: Check if allocation succeeded
    if (mcpShifter == nullptr) {
        mcpAvailable = false;
        Logger::error(700, "Shifter: Failed to allocate MCP23017 object!");
        initialized = true;
        return;
    }
    
    if (mcpShifter->begin_I2C(I2C_ADDR_MCP23017)) {
        // Configurar todos los pines del shifter como entrada con pull-up
        mcpShifter->pinMode(MCP_PIN_SHIFTER_P,  INPUT_PULLUP);  // GPIOB0: Park
        mcpShifter->pinMode(MCP_PIN_SHIFTER_R,  INPUT_PULLUP);  // GPIOB1: Reverse
        mcpShifter->pinMode(MCP_PIN_SHIFTER_N,  INPUT_PULLUP);  // GPIOB2: Neutral
        mcpShifter->pinMode(MCP_PIN_SHIFTER_D1, INPUT_PULLUP);  // GPIOB3: Drive 1
        mcpShifter->pinMode(MCP_PIN_SHIFTER_D2, INPUT_PULLUP);  // GPIOB4: Drive 2
        
        mcpAvailable = true;
        Logger::info("Shifter: MCP23017 GPIOB0-B4 configurado (pines 8-12)");
    } else {
        mcpAvailable = false;
        Logger::error(700, "Shifter: MCP23017 no disponible!");
        // Liberar memoria si init falla
        delete mcpShifter;
        mcpShifter = nullptr;
    }
    
    initialized = true;  // 🔒 v2.5.0: Marcar como inicializado
    Logger::info("Shifter init completado (via MCP23017)");
}

void Shifter::update() {
    // Si MCP23017 no está disponible, mantener última posición conocida
    if (!mcpAvailable) {
        s.changed = false;
        return;
    }
    
    Shifter::Gear detectedGear = s.gear;

    // 🔒 Lee cada posición del shifter vía MCP23017 (prioridad P > R > N > D1 > D2)
    // Orden de prioridad: Park es más importante que cualquier otra posición
    if(readMcpPin(MCP_PIN_SHIFTER_P))       detectedGear = Shifter::P;
    else if(readMcpPin(MCP_PIN_SHIFTER_R))  detectedGear = Shifter::R;
    else if(readMcpPin(MCP_PIN_SHIFTER_N))  detectedGear = Shifter::N;
    else if(readMcpPin(MCP_PIN_SHIFTER_D1)) detectedGear = Shifter::D1;
    else if(readMcpPin(MCP_PIN_SHIFTER_D2)) detectedGear = Shifter::D2;

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

// 🔒 v2.5.0: Estado de inicialización
bool Shifter::initOK() { return initialized; }