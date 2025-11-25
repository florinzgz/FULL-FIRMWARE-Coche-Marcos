#include "relays.h"
#include "logger.h"
#include "system.h"
#include "pins.h"
#include "sensors.h"  // Para verificar condiciones críticas
#include <Arduino.h>

// Flag de inicialización y estado
static bool initialized = false;
static Relays::State state = {false, false, false, false, false};

// 🔒 Timeouts de seguridad
static unsigned long lastStateChangeMs = 0;
static const unsigned long RELAY_DEBOUNCE_MS = 50;  // Debounce para cambios de estado

void Relays::init() {
    // 🔒 CORRECCIÓN 5.1: Implementación real de hardware con GPIOs
    Logger::info("Relays init - Configurando GPIOs");
    
    // Configurar pines como salidas
    pinMode(PIN_RELAY_MAIN, OUTPUT);
    pinMode(PIN_RELAY_TRAC, OUTPUT);
    pinMode(PIN_RELAY_DIR, OUTPUT);
    pinMode(PIN_RELAY_SPARE, OUTPUT);
    
    // Estado inicial: todos OFF (seguridad)
    digitalWrite(PIN_RELAY_MAIN, LOW);
    digitalWrite(PIN_RELAY_TRAC, LOW);
    digitalWrite(PIN_RELAY_DIR, LOW);
    digitalWrite(PIN_RELAY_SPARE, LOW);
    
    state = {false, false, false, false, false};
    lastStateChangeMs = millis();
    initialized = true;
    
    Logger::info("Relays init OK - All relays OFF (safe state)");
}

void Relays::enablePower() {
    if(!initialized) {
        Logger::warn("Relays enablePower() llamado sin init");
        return;
    }
    
    // 🔒 CORRECCIÓN 5.5: Verificar que no haya errores del sistema
    if (System::hasError()) {
        Logger::errorf("Relays: no se puede activar con errores del sistema activos");
        System::logError(601);
        return;
    }
    
    unsigned long now = millis();
    if (now - lastStateChangeMs < RELAY_DEBOUNCE_MS) {
        Logger::warn("Relays: debounce - cambio demasiado rápido");
        return;
    }
    
    // 🔒 Activar relés en secuencia segura con delays
    // 1. Main power first
    digitalWrite(PIN_RELAY_MAIN, HIGH);
    state.mainOn = true;
    Logger::info("Relay MAIN: ON");
    delay(50);  // 50ms para estabilización
    
    // 2. Traction power
    digitalWrite(PIN_RELAY_TRAC, HIGH);
    state.tractionOn = true;
    Logger::info("Relay TRAC: ON");
    delay(50);
    
    // 3. Steering power
    digitalWrite(PIN_RELAY_DIR, HIGH);
    state.steeringOn = true;
    Logger::info("Relay DIR: ON");
    
    lastStateChangeMs = now;
    Logger::info("Relays power enabled - Secuencia completada");
}

void Relays::disablePower() {
    if(!initialized) {
        Logger::warn("Relays disablePower() llamado sin init");
        return;
    }
    
    unsigned long now = millis();
    if (now - lastStateChangeMs < RELAY_DEBOUNCE_MS) {
        Logger::warn("Relays: debounce - cambio demasiado rápido");
        return;
    }
    
    // 🔒 Desactivar en orden inverso (seguridad)
    digitalWrite(PIN_RELAY_DIR, LOW);
    state.steeringOn = false;
    Logger::warn("Relay DIR: OFF");
    delay(20);
    
    digitalWrite(PIN_RELAY_TRAC, LOW);
    state.tractionOn = false;
    Logger::warn("Relay TRAC: OFF");
    delay(20);
    
    digitalWrite(PIN_RELAY_MAIN, LOW);
    state.mainOn = false;
    Logger::warn("Relay MAIN: OFF");
    
    lastStateChangeMs = now;
    Logger::warn("Relays power disabled - Secuencia completada");
}

// 🔒 v2.4.1: Parada de emergencia inmediata
void Relays::emergencyStop() {
    // CRÍTICO: Sin debounce, sin delays, sin verificaciones
    // Esta función debe desactivar TODOS los relés lo más rápido posible
    // Se llama desde ISR del watchdog o en caso de emergencia crítica
    
    // Desactivar todos los relés inmediatamente
    digitalWrite(PIN_RELAY_DIR, LOW);
    digitalWrite(PIN_RELAY_TRAC, LOW);
    digitalWrite(PIN_RELAY_MAIN, LOW);
    digitalWrite(PIN_RELAY_SPARE, LOW);
    
    // Actualizar estado
    state.mainOn = false;
    state.tractionOn = false;
    state.steeringOn = false;
    state.lightsOn = false;
    state.mediaOn = false;
    
    // 🔒 v2.4.1: Log solo si no estamos en contexto ISR
    // Usar detección de contexto compatible con ESP32 Arduino framework
    #if defined(ESP32) || defined(ESP_PLATFORM)
        // ESP32: xPortInIsrContext() es parte de FreeRTOS incluido en el framework
        if (!xPortInIsrContext()) {
            Logger::error("EMERGENCY STOP - Todos los relés desactivados");
            System::logError(699); // código: parada de emergencia
        }
    #else
        // Para otras plataformas, siempre intentar logging
        Logger::error("EMERGENCY STOP - Todos los relés desactivados");
        System::logError(699);
    #endif
}

void Relays::setLights(bool on) {
    if(!initialized) {
        Logger::warn("Relays setLights() llamado sin init");
        return;
    }
    
    // 🔒 Control real del hardware
    digitalWrite(PIN_RELAY_SPARE, on ? HIGH : LOW);
    state.lightsOn = on;
    Logger::infof("Relays lights %s (GPIO %d)", on ? "ON" : "OFF", PIN_RELAY_SPARE);
}

void Relays::setMedia(bool on) {
    if(!initialized) {
        Logger::warn("Relays setMedia() llamado sin init");
        return;
    }
    
    // 🔒 Media usa el mismo relé SPARE que lights
    // Si necesita pin separado, modificar pins.h
    state.mediaOn = on;
    Logger::infof("Relays media %s (controlado por software)", on ? "ON" : "OFF");
}

void Relays::update() {
    if(!initialized) return;

    // 🔒 CORRECCIÓN 5.4: Lógica real de detección de errores críticos
    bool system_error = false;
    
    // 🔒 v2.4.1: Histéresis para evitar ciclos rápidos on/off
    // NOTA: Estas variables estáticas son seguras porque update() solo se llama
    // desde el loop principal en main.cpp, nunca desde ISRs o múltiples tareas.
    static uint32_t lastErrorMs = 0;
    static uint8_t consecutiveErrors = 0;
    uint32_t now = millis();
    
    // Check 1: Overcurrent en batería (canal 4 = batería)
    float batteryCurrent = Sensors::getCurrent(4);
    if (batteryCurrent > 120.0f && batteryCurrent < 999.0f) { // 120A = 120% del máximo (100A)
        Logger::errorf("Relays: OVERCURRENT batería %.1fA (límite 100A)", batteryCurrent);
        System::logError(602); // código: overcurrent batería
        system_error = true;
    }
    
    // Check 2: Overtemperature en cualquier motor
    for (int i = 0; i < 4; i++) {
        float temp = Sensors::getTemperature(i);
        if (temp > 80.0f && temp < 150.0f) { // 80°C límite crítico
            Logger::errorf("Relays: OVERTEMP motor %d: %.1f°C (límite 80°C)", i, temp);
            System::logError(603 + i); // códigos 603-606
            system_error = true;
        }
    }
    
    // Check 3: Batería muy baja (crítico para sistema 24V)
    float batteryVoltage = Sensors::getVoltage(4);
    if (batteryVoltage > 0.0f && batteryVoltage < 20.0f) { // <20V crítico para 24V nominal
        Logger::errorf("Relays: BATERÍA BAJA %.1fV (crítico <20V)", batteryVoltage);
        System::logError(607); // código: batería baja crítica
        system_error = true;
    }
    
    // Check 4: Batería muy alta (sobrecarga)
    if (batteryVoltage > 30.0f && batteryVoltage < 999.0f) { // >30V peligroso para 24V
        Logger::errorf("Relays: BATERÍA ALTA %.1fV (peligro >30V)", batteryVoltage);
        System::logError(608); // código: batería sobrecargada
        system_error = true;
    }
    
    // 🔒 v2.4.1: Lógica de histéresis para evitar ciclos rápidos
    if(system_error) {
        consecutiveErrors++;
        lastErrorMs = now;
        
        // Solo desactivar si hay 3+ errores consecutivos (para evitar falsos positivos)
        if (consecutiveErrors >= 3) {
            Logger::errorf("Relays: ERROR CRÍTICO (%d consecutivos) - Desactivando todos los relés", consecutiveErrors);
            System::logError(600); // código: fallo crítico relés
            disablePower();
        }
    } else {
        // Resetear contador de errores después de 1 segundo sin errores
        if (now - lastErrorMs > 1000) {
            consecutiveErrors = 0;
        }
    }
}

const Relays::State& Relays::get() {
    return state;
}

bool Relays::initOK() {
    return initialized;
}