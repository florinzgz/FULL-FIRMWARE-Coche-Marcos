#include "shifter.h"
#include "pins.h"
#include "logger.h"
#include "dfplayer.h"
#include "alerts.h"
#include "mcp_shared.h"
#include "system.h"
#include <Adafruit_MCP23X17.h>

static Shifter::State s = {Shifter::P, false};

// 🔒 v2.5.0: Flag de inicialización
static bool initialized = false;

// 🔒 CORRECCIÓN CRÍTICA: Debounce para prevenir lecturas erróneas por rebotes
static constexpr uint32_t DEBOUNCE_MS = 50;
static uint32_t lastChangeMs = 0;
static uint8_t stableReadings = 0;
static Shifter::Gear pendingGear = Shifter::P;

// Lee entrada del MCP23017 con pull-up interno (LOW = activo)
// Los optoacopladores HY-M158 invierten: activo = LED encendido = transistor conduce = LOW
static bool readMcpPin(uint8_t pin) {
    if (!MCPShared::initialized) return false;
    return MCPShared::mcp.digitalRead(pin) == 0;
}

static void announce(Shifter::Gear g) {
    // 🔒 v2.10.3: Gear-specific audio feedback using existing audio library
    // Different audio tracks distinguish between gears for driver feedback
    switch(g) {
        case Shifter::Gear::P:
            Alerts::play(Audio::AUDIO_MODULO_OK); // Confirmation beep for park
            break;
        case Shifter::Gear::R:
            // Use dedicated reverse gear audio - "Marcha atrás activada"
            Alerts::play(Audio::AUDIO_MARCHA_R);
            break;
        case Shifter::Gear::N:
            Alerts::play(Audio::AUDIO_MODULO_OK); // Neutral confirmation
            break;
        case Shifter::Gear::D1:
        case Shifter::Gear::D2:
            Alerts::play(Audio::AUDIO_MODULO_OK); // Drive mode confirmation
            break;
    }
}

void Shifter::init() {
    // MCP23017 is initialized by MCPShared::init() in ControlManager
    // Configure GPIOB pins for shifter inputs
    
    if (MCPShared::initialized) {
        // Configurar GPIOB0-B4 como INPUT con PULLUP para shifter
        MCPShared::mcp.pinMode(MCP_PIN_SHIFTER_P, INPUT_PULLUP);
        MCPShared::mcp.pinMode(MCP_PIN_SHIFTER_R, INPUT_PULLUP);
        MCPShared::mcp.pinMode(MCP_PIN_SHIFTER_N, INPUT_PULLUP);
        MCPShared::mcp.pinMode(MCP_PIN_SHIFTER_D1, INPUT_PULLUP);
        MCPShared::mcp.pinMode(MCP_PIN_SHIFTER_D2, INPUT_PULLUP);
        
        Logger::info("Shifter: MCP23017 GPIOB configured");
        initialized = true;
    } else {
        Logger::error("Shifter: MCP23017 not available");
        System::logError(700);
        initialized = false;
    }
    
    s = {Gear::P, false};
}

void Shifter::update() {
    // Si MCP23017 no está disponible, mantener última posición conocida
    if (!MCPShared::initialized) {
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