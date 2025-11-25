#include "power_mgmt.h"
#include "pins.h"  // Include for relay pin definitions
#include "dfplayer.h"
#include "alerts.h"
#include "logger.h"

namespace PowerMgmt {

// -----------------------
// Usar definiciones de pines desde pins.h para evitar duplicación
// NOTA: Los relés se definen en pins.h líneas 36-39:
// PIN_RELAY_MAIN (GPIO2), PIN_RELAY_TRAC (GPIO4), PIN_RELAY_DIR (GPIO5), PIN_RELAY_SPARE (GPIO6)
// -----------------------
constexpr uint8_t PIN_RELAY_POWER_HOLD  = PIN_RELAY_MAIN;   // Relé 1: Power Hold Buck 5V
constexpr uint8_t PIN_RELAY_AUX_12V     = PIN_RELAY_TRAC;   // Relé 2: 12V Auxiliares (sensores/encoder)
constexpr uint8_t PIN_RELAY_MOTOR_24V   = PIN_RELAY_DIR;    // Relé 3: 24V Potencia (motores)
constexpr uint8_t PIN_RELAY_OPTIONAL    = PIN_RELAY_SPARE;  // Relé 4: Opcional/Seguridad

constexpr uint8_t PIN_KEY_DETECT        = 45;  // Detección llave (INPUT_PULLUP, conecta a GND cuando ON)

// -----------------------
// Configuración de tiempos
// -----------------------
constexpr uint32_t DEBOUNCE_TIME_MS     = 100;  // Debounce detección llave
constexpr uint32_t SHUTDOWN_DELAY_MS    = 5000; // 5 segundos power hold tras apagar llave
constexpr uint32_t AUDIO_WAIT_MS        = 2000; // Espera reproducción audio apagado

// -----------------------
// Variables de estado
// -----------------------
static PowerState currentState = PowerState::OFF;
static bool steeringCentered = false;
static bool wheelSensorsOK = false;
static bool initialized = false;  // 🔒 v2.5.0: Flag de inicialización

static uint32_t lastKeyChangeTime = 0;
static bool lastKeyState = false;
static bool keyStateStable = false;

static uint32_t shutdownStartTime = 0;
static uint32_t audioStartTime = 0;

// -----------------------
// Funciones internas
// -----------------------

/**
 * @brief Lee estado de llave con debounce
 * @return true si llave ON (GPIO LOW por pull-up a GND)
 */
static bool readKeyWithDebounce() {
    bool currentReading = (digitalRead(PIN_KEY_DETECT) == LOW);
    
    if (currentReading != lastKeyState) {
        lastKeyChangeTime = millis();
        lastKeyState = currentReading;
        keyStateStable = false;
    }
    
    if (!keyStateStable && (millis() - lastKeyChangeTime > DEBOUNCE_TIME_MS)) {
        keyStateStable = true;
    }
    
    return keyStateStable ? lastKeyState : !currentReading;
}

/**
 * @brief Activa un relé (HIGH = cerrado)
 */
static void activateRelay(uint8_t pin) {
    digitalWrite(pin, HIGH);
}

/**
 * @brief Desactiva un relé (LOW = abierto)
 */
static void deactivateRelay(uint8_t pin) {
    digitalWrite(pin, LOW);
}

// -----------------------
// Implementación API pública
// -----------------------

void init() {
    // Configurar GPIOs de relés como salidas (inicialmente desactivados)
    pinMode(PIN_RELAY_POWER_HOLD, OUTPUT);
    pinMode(PIN_RELAY_AUX_12V, OUTPUT);
    pinMode(PIN_RELAY_MOTOR_24V, OUTPUT);
    pinMode(PIN_RELAY_OPTIONAL, OUTPUT);
    
    deactivateRelay(PIN_RELAY_POWER_HOLD);
    deactivateRelay(PIN_RELAY_AUX_12V);
    deactivateRelay(PIN_RELAY_MOTOR_24V);
    deactivateRelay(PIN_RELAY_OPTIONAL);
    
    // Configurar detección de llave (INPUT_PULLUP)
    pinMode(PIN_KEY_DETECT, INPUT_PULLUP);
    lastKeyState = (digitalRead(PIN_KEY_DETECT) == LOW);
    keyStateStable = true;
    lastKeyChangeTime = millis();
    
    // CRÍTICO: Activar power hold inmediatamente para mantener buck activo
    activateRelay(PIN_RELAY_POWER_HOLD);
    
    // CRÍTICO: Activar 12V auxiliares ANTES de buscar centro de volante
    // El motor de dirección RS390 12V necesita alimentación para funcionar
    activateRelay(PIN_RELAY_AUX_12V);
    currentState = PowerState::POWER_HOLD;
    
    initialized = true;  // 🔒 v2.5.0: Marcar como inicializado
    Logger::info("PowerMgmt: Inicializado - Power Hold y 12V Auxiliares activos, esperando centrado volante");
}

void update() {
    bool keyOn = readKeyWithDebounce();
    
    // Detección de llave OFF → Iniciar apagado
    if (!keyOn && currentState >= PowerState::POWER_HOLD && currentState < PowerState::SHUTDOWN_START) {
        Logger::warn("PowerMgmt: Llave OFF detectada - Iniciando apagado");
        startShutdown();
        return;
    }
    
    // Máquina de estados
    switch (currentState) {
        case PowerState::OFF:
            // Sistema apagado (no debería llegar aquí si ESP32 está funcionando)
            break;
            
        case PowerState::POWER_HOLD:
            // Esperando notificación de volante centrado
            // Nota: 12V auxiliares ya está activo desde init() para permitir búsqueda de centro
            if (steeringCentered) {
                Logger::info("PowerMgmt: Volante centrado - Verificando sensores ruedas");
                currentState = PowerState::AUX_POWER;
            }
            break;
            
        case PowerState::AUX_POWER:
            // Esperando notificación de sensores ruedas OK
            if (wheelSensorsOK) {
                Logger::info("PowerMgmt: Sensores ruedas OK - Activando 24V motores");
                enableMotorPower();
                currentState = PowerState::FULL_POWER;
            }
            break;
            
        case PowerState::FULL_POWER:
            // Sistema operando normalmente
            break;
            
        case PowerState::SHUTDOWN_START:
            // Inicio secuencia apagado - Reproducir audio
            Logger::warn("PowerMgmt: Reproduciendo audio de apagado");
            Alerts::play(Audio::AUDIO_APAGADO);  // "Cerrando sistemas. Hasta pronto."
            audioStartTime = millis();
            currentState = PowerState::SHUTDOWN_AUDIO;
            break;
            
        case PowerState::SHUTDOWN_AUDIO:
            // Esperar reproducción audio (2 segundos)
            if (millis() - audioStartTime >= AUDIO_WAIT_MS) {
                Logger::warn("PowerMgmt: Cortando 24V motores");
                disableMotorPower();
                currentState = PowerState::SHUTDOWN_MOTORS;
            }
            break;
            
        case PowerState::SHUTDOWN_MOTORS:
            // Pequeña pausa tras cortar motores (100ms)
            if (millis() - audioStartTime >= (AUDIO_WAIT_MS + 100)) {
                Logger::warn("PowerMgmt: Cortando 12V auxiliares");
                disableAuxPower();
                shutdownStartTime = millis();
                currentState = PowerState::SHUTDOWN_FINAL;
            }
            break;
            
        case PowerState::SHUTDOWN_FINAL:
            // Esperar 5 segundos desde inicio apagado total
            if (millis() - shutdownStartTime >= SHUTDOWN_DELAY_MS) {
                Logger::warn("PowerMgmt: Liberando power hold - Sistema se apagará");
                deactivateRelay(PIN_RELAY_POWER_HOLD);
                currentState = PowerState::OFF;
                // Aquí el buck se apagará y ESP32 perderá alimentación
            }
            break;
            
        default:
            break;
    }
}

void enableAuxPower() {
    activateRelay(PIN_RELAY_AUX_12V);
    Logger::info("PowerMgmt: 12V Auxiliares activado");
}

void enableMotorPower() {
    activateRelay(PIN_RELAY_MOTOR_24V);
    Logger::info("PowerMgmt: 24V Motores activado");
}

void disableMotorPower() {
    deactivateRelay(PIN_RELAY_MOTOR_24V);
    Logger::info("PowerMgmt: 24V Motores desactivado");
}

void disableAuxPower() {
    deactivateRelay(PIN_RELAY_AUX_12V);
    Logger::info("PowerMgmt: 12V Auxiliares desactivado");
}

void startShutdown() {
    if (currentState < PowerState::SHUTDOWN_START) {
        shutdownStartTime = millis();
        currentState = PowerState::SHUTDOWN_START;
    }
}

bool isKeyOn() {
    return readKeyWithDebounce();
}

PowerState getState() {
    return currentState;
}

bool isFullPowerOn() {
    return currentState == PowerState::FULL_POWER;
}

void notifySteeringCentered() {
    steeringCentered = true;
    Logger::info("PowerMgmt: Notificación: Volante centrado");
}

void notifyWheelSensorsOK() {
    wheelSensorsOK = true;
    Logger::info("PowerMgmt: Notificación: Sensores ruedas OK");
}

// 🔒 v2.5.0: Estado de inicialización
bool initOK() {
    return initialized;
}

} // namespace PowerMgmt
