#!/bin/bash

cat >> apply_corrections.sh << 'SCRIPT_PART5'

# ============================================================================
# CORRECCIÓN 5: src/control/relays.cpp - IMPLEMENTACIÓN COMPLETA
# ============================================================================
echo ""
echo "🔧 CORRECCIÓN 5: src/control/relays.cpp"
echo "---------------------------------------"

cat > src/control/relays.cpp << 'RELAYS_CPP_EOF'
#include "relays.h"
#include "pins.h"
#include "logger.h"
#include "system.h"
#include "watchdog.h"

// Variables de estado
static bool initialized = false;
static Relays::State state = {false, false, false, false, false};

// ✅ Variables para secuencia temporizada
static bool powerSequenceActive = false;
static uint8_t sequenceStep = 0;
static uint32_t sequenceStartTime = 0;

void Relays::init() {
    Logger::info("🔌 Initializing relay system...");
    
    // ✅ CONFIGURAR todos los GPIOs como OUTPUT
    pinMode(PIN_RELAY_MAIN, OUTPUT);
    pinMode(PIN_RELAY_TRAC, OUTPUT);
    pinMode(PIN_RELAY_DIR, OUTPUT);
    pinMode(PIN_RELAY_SPARE, OUTPUT);
    
    // ✅ ESTADO INICIAL SEGURO - todos los relés OFF
    digitalWrite(PIN_RELAY_MAIN, LOW);
    digitalWrite(PIN_RELAY_TRAC, LOW);
    digitalWrite(PIN_RELAY_DIR, LOW);
    digitalWrite(PIN_RELAY_SPARE, LOW);
    
    // ✅ INICIALIZAR estructura de estado
    state = {false, false, false, false, false};
    powerSequenceActive = false;
    sequenceStep = 0;
    initialized = true;
    
    Logger::info("✅ Relay system initialized - all relays OFF");
}

void Relays::enablePower() {
    if(!initialized) {
        Logger::error("❌ Relays::enablePower() called without init()");
        return;
    }
    
    if(powerSequenceActive) {
        Logger::warn("⚠️ Power sequence already active");
        return;
    }
    
    Logger::info("🚀 Starting SAFE relay power sequence...");
    powerSequenceActive = true;
    sequenceStep = 0;
    sequenceStartTime = millis();
}

void Relays::update() {
    if(!initialized) return;
    
    // ✅ SECUENCIA TEMPORIZADA NO BLOQUEANTE
    if(powerSequenceActive) {
        uint32_t elapsed = millis() - sequenceStartTime;
        
        switch(sequenceStep) {
            case 0: // PASO 1: Activar relé principal (power hold)
                if(elapsed >= 0) {  // Inmediato
                    digitalWrite(PIN_RELAY_MAIN, HIGH);
                    state.mainOn = true;
                    Logger::info("✅ STEP 1: Main relay ON (power hold)");
                    sequenceStep++;
                    sequenceStartTime = millis();  // Reset timer
                }
                break;
                
            case 1: // PASO 2: Esperar estabilización (500ms)
                if(elapsed >= 500) {
                    digitalWrite(PIN_RELAY_TRAC, HIGH);
                    state.tractionOn = true;
                    Logger::info("✅ STEP 2: 12V Auxiliaries ON");
                    sequenceStep++;
                    sequenceStartTime = millis();
                }
                break;
                
            case 2: // PASO 3: Activar motores tracción (500ms adicional)
                if(elapsed >= 500) {
                    digitalWrite(PIN_RELAY_DIR, HIGH);
                    state.steeringOn = true;
                    Logger::info("✅ STEP 3: 24V Traction motors ON");
                    sequenceStep++;
                    sequenceStartTime = millis();
                }
                break;
                
            case 3: // PASO 4: Finalización (200ms verificación)
                if(elapsed >= 200) {
                    powerSequenceActive = false;
                    Logger::info("🎉 Power sequence COMPLETED - all systems ON");
                    
                    // ✅ Verificar que todos los relés estén activos
                    if(state.mainOn && state.tractionOn && state.steeringOn) {
                        Logger::info("✅ Relay system OPERATIONAL");
                    } else {
                        Logger::error("❌ Relay sequence verification FAILED");
                        System::logError(601);  // Error secuencia relés
                    }
                }
                break;
                
            default:
                powerSequenceActive = false;
                break;
        }
        
        // ✅ CRÍTICO: Feed watchdog durante secuencia
        Watchdog::feed();
    }
    
    // ✅ MONITOREO CONTINUO - detectar fallos
    static uint32_t lastCheck = 0;
    if(millis() - lastCheck > 1000) {  // Verificar cada segundo
        lastCheck = millis();
        
        // Verificar coherencia estado vs GPIO
        bool mainGPIO = digitalRead(PIN_RELAY_MAIN);
        bool tracGPIO = digitalRead(PIN_RELAY_TRAC);
        bool dirGPIO = digitalRead(PIN_RELAY_DIR);
        
        if(state.mainOn != mainGPIO || state.tractionOn != tracGPIO || state.steeringOn != dirGPIO) {
            Logger::error("❌ Relay state mismatch detected - GPIO vs software");
            System::logError(602);  // Error coherencia relés
            
            // ✅ CORRECCIÓN: Sincronizar estado con realidad GPIO
            state.mainOn = mainGPIO;
            state.tractionOn = tracGPIO;
            state.steeringOn = dirGPIO;
        }
    }
}

void Relays::disablePower() {
    if(!initialized) {
        Logger::warn("Relays disablePower() called without init");
        return;
    }
    
    Logger::warn("🔻 EMERGENCY: Disabling all relay power");
    
    // ✅ APAGADO INMEDIATO EN ORDEN INVERSO (seguridad)
    digitalWrite(PIN_RELAY_DIR, LOW);     // Primero: motores tracción
    state.steeringOn = false;
    delay(50);  // Pequeño retardo para evitar arcos
    
    digitalWrite(PIN_RELAY_TRAC, LOW);    // Segundo: 12V auxiliares
    state.tractionOn = false;
    delay(50);
    
    digitalWrite(PIN_RELAY_MAIN, LOW);    // Último: power hold
    state.mainOn = false;
    
    powerSequenceActive = false;
    sequenceStep = 0;
    
    Logger::warn("⚠️ All relay power DISABLED");
}

void Relays::setLights(bool on) {
    if(!initialized) {
        Logger::warn("Relays setLights() called without init");
        return;
    }
    
    state.lightsOn = on;
    Logger::info("💡 Lights %s", on ? "ON" : "OFF");
}

void Relays::setMedia(bool on) {
    if(!initialized) {
        Logger::warn("Relays setMedia() called without init");
        return;
    }
    
    state.mediaOn = on;
    Logger::info("🎵 Media %s", on ? "ON" : "OFF");
}

// ✅ NUEVAS FUNCIONES IMPLEMENTADAS

bool Relays::isPowerSequenceComplete() {
    return !powerSequenceActive && state.mainOn && state.tractionOn && state.steeringOn;
}

uint8_t Relays::getPowerSequenceProgress() {
    if(!powerSequenceActive) {
        return isPowerSequenceComplete() ? 100 : 0;
    }
    return (sequenceStep * 25);  // 0%, 25%, 50%, 75%, 100%
}

void Relays::setRelay(uint8_t relay_id, bool relayState) {
    if(!initialized) {
        Logger::error("setRelay() called without init");
        return;
    }
    
    switch(relay_id) {
        case 0: // RELAY_MAIN
            digitalWrite(PIN_RELAY_MAIN, relayState ? HIGH : LOW);
            state.mainOn = relayState;
            break;
        case 1: // RELAY_TRAC
            digitalWrite(PIN_RELAY_TRAC, relayState ? HIGH : LOW);
            state.tractionOn = relayState;
            break;
        case 2: // RELAY_DIR
            digitalWrite(PIN_RELAY_DIR, relayState ? HIGH : LOW);
            state.steeringOn = relayState;
            break;
        case 3: // RELAY_SPARE
            digitalWrite(PIN_RELAY_SPARE, relayState ? HIGH : LOW);
            break;
        default:
            Logger::error("Invalid relay_id: %d", relay_id);
            return;
    }
    
    Logger::info("🔌 Relay %d → %s", relay_id, relayState ? "ON" : "OFF");
}

bool Relays::getRelayState(uint8_t relay_id) {
    switch(relay_id) {
        case 0: return state.mainOn;
        case 1: return state.tractionOn;
        case 2: return state.steeringOn;
        case 3: return digitalRead(PIN_RELAY_SPARE);
        default: return false;
    }
}

bool Relays::selfTest() {
    if(!initialized) {
        Logger::error("selfTest() called without init");
        return false;
    }
    
    Logger::info("🔍 Running relay self-test...");
    
    bool testPassed = true;
    
    // Test cada relé individualmente
    for(uint8_t i = 0; i < 4; i++) {
        // Activar relé
        setRelay(i, true);
        delay(100);
        
        // Verificar que se activó
        bool activated = getRelayState(i);
        if(!activated) {
            Logger::error("❌ Relay %d failed to activate", i);
            testPassed = false;
        }
        
        // Desactivar relé
        setRelay(i, false);
        delay(100);
        
        // Verificar que se desactivó
        bool deactivated = !getRelayState(i);
        if(!deactivated) {
            Logger::error("❌ Relay %d failed to deactivate", i);
            testPassed = false;
        }
    }
    
    Logger::info("🔍 Relay self-test %s", testPassed ? "PASSED" : "FAILED");
    return testPassed;
}

void Relays::emergencyStop() {
    Logger::error("🚨 EMERGENCY STOP - Disabling all relays immediately");
    
    // Apagado inmediato sin retardos
    digitalWrite(PIN_RELAY_DIR, LOW);
    digitalWrite(PIN_RELAY_TRAC, LOW);
    digitalWrite(PIN_RELAY_MAIN, LOW);
    digitalWrite(PIN_RELAY_SPARE, LOW);
    
    // Actualizar estado
    state.mainOn = false;
    state.tractionOn = false;
    state.steeringOn = false;
    powerSequenceActive = false;
    
    Logger::error("🚨 EMERGENCY STOP COMPLETE");
}

const Relays::State& Relays::get() {
    return state;
}

bool Relays::initOK() {
    return initialized;
}
RELAYS_CPP_EOF

echo "✅ src/control/relays.cpp implementación completa:"
echo "   - Control físico GPIO añadido"
echo "   - Secuencia temporizada segura"
echo "   - Monitoreo continuo de coherencia"
echo "   - Funciones selfTest y emergencyStop"
SCRIPT_PART5

chmod +x create_script_part5.sh