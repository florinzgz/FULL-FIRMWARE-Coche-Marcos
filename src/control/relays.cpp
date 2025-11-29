#include "relays.h"
#include "logger.h"
#include "system.h"
#include "pins.h"
#include "sensors.h"  // Para verificar condiciones críticas
#include <Arduino.h>

// Flag de inicialización y estado
static bool initialized = false;
static Relays::State state = {false, false, false, false, false};

// Timeouts / seguridad
static unsigned long lastStateChangeMs = 0;
static const unsigned long RELAY_DEBOUNCE_MS = 50;  // Debounce para cambios de estado

// Emergency deferred logging flag (ISR-safe)
static volatile bool emergencyRequested = false;

// Critical section spinlock for ISR-safe flag handling (ESP32)
#if defined(ESP32) || defined(ESP_PLATFORM)
static portMUX_TYPE emergencyMux = portMUX_INITIALIZER_UNLOCKED;
#endif

// Constantes configurables
static const int   BATTERY_CHANNEL             = 4;
static const float BATTERY_OVERCURRENT_LIMIT_A = 120.0f;
static const float BATTERY_VOLTAGE_MIN_V       = 20.0f;
static const float BATTERY_VOLTAGE_MAX_V       = 30.0f;
static const float MOTOR_OVERTEMP_LIMIT_C      = 80.0f;

// Secuencia no bloqueante
enum SequenceState {
    SEQ_IDLE = 0,
    SEQ_EN_ENABLE_MAIN,
    SEQ_EN_ENABLE_TRAC,
    SEQ_EN_ENABLE_DIR,
    SEQ_EN_DONE,
    SEQ_DIS_DISABLE_DIR,
    SEQ_DIS_DISABLE_TRAC,
    SEQ_DIS_DISABLE_MAIN,
    SEQ_DIS_DONE
};

static SequenceState seqState = SEQ_IDLE;
static unsigned long seqStepStartMs = 0;
static const unsigned long ENABLE_STEP_DELAY_MS   = 50;
static const unsigned long DISABLE_STEP_DELAY_MS  = 20;
static const unsigned long SEQUENCE_TIMEOUT_MS    = 5000;

void Relays::init() {
    Logger::info("Relays init - Configurando GPIOs");

    pinMode(PIN_RELAY_MAIN,  OUTPUT);
    pinMode(PIN_RELAY_TRAC,  OUTPUT);
    pinMode(PIN_RELAY_DIR,   OUTPUT);
    pinMode(PIN_RELAY_SPARE, OUTPUT);
#ifdef PIN_RELAY_MEDIA
    pinMode(PIN_RELAY_MEDIA, OUTPUT);
#endif

    digitalWrite(PIN_RELAY_MAIN,  LOW);
    digitalWrite(PIN_RELAY_TRAC,  LOW);
    digitalWrite(PIN_RELAY_DIR,   LOW);
    digitalWrite(PIN_RELAY_SPARE, LOW);
#ifdef PIN_RELAY_MEDIA
    digitalWrite(PIN_RELAY_MEDIA, LOW);
#endif

    state = {false, false, false, false, false};
    lastStateChangeMs = millis();
    seqState = SEQ_IDLE;
    initialized = true;

    Logger::info("Relays init OK - All relays OFF (safe state)");
}

void Relays::enablePower() {
    if(!initialized) {
        Logger::warn("Relays enablePower() llamado sin init");
        return;
    }
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

    if (state.mainOn && state.tractionOn && state.steeringOn) {
        Logger::info("Relays: ya habilitados");
        lastStateChangeMs = now;
        return;
    }
    if (seqState != SEQ_IDLE) {
        Logger::warn("Relays: secuencia en curso, no se inicia nueva enable");
        return;
    }

    seqState = SEQ_EN_ENABLE_MAIN;
    seqStepStartMs = millis();
    Logger::info("Relays: iniciada secuencia no bloqueante de enablePower");
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

    if (!state.mainOn && !state.tractionOn && !state.steeringOn) {
        Logger::info("Relays: ya deshabilitados");
        lastStateChangeMs = now;
        return;
    }
    if (seqState != SEQ_IDLE) {
        Logger::warn("Relays: secuencia en curso, no se inicia nueva disable");
        return;
    }

    seqState = SEQ_DIS_DISABLE_DIR;
    seqStepStartMs = millis();
    Logger::info("Relays: iniciada secuencia no bloqueante de disablePower");
}

void Relays::emergencyStop() {
    digitalWrite(PIN_RELAY_DIR,   LOW);
    digitalWrite(PIN_RELAY_TRAC,  LOW);
    digitalWrite(PIN_RELAY_MAIN,  LOW);
    digitalWrite(PIN_RELAY_SPARE, LOW);
#ifdef PIN_RELAY_MEDIA
    digitalWrite(PIN_RELAY_MEDIA, LOW);
#endif

    state.mainOn     = false;
    state.tractionOn = false;
    state.steeringOn = false;
    state.lightsOn   = false;
    state.mediaOn    = false;

    seqState = SEQ_IDLE;

    // Set flag atomically to avoid race conditions with update()
    // The critical section ensures the flag is not read by update()
    // between our check and write operations
#if defined(ESP32) || defined(ESP_PLATFORM)
    portENTER_CRITICAL_ISR(&emergencyMux);
    emergencyRequested = true;
    portEXIT_CRITICAL_ISR(&emergencyMux);
#else
    noInterrupts();
    emergencyRequested = true;
    interrupts();
#endif
}

void Relays::setLights(bool on) {
    if(!initialized) {
        Logger::warn("Relays setLights() llamado sin init");
        return;
    }
    digitalWrite(PIN_RELAY_SPARE, on ? HIGH : LOW);
    state.lightsOn = on;
    Logger::infof("Relays lights %s (GPIO %d)", on ? "ON" : "OFF", PIN_RELAY_SPARE);
}

void Relays::setMedia(bool on) {
    if(!initialized) {
        Logger::warn("Relays setMedia() llamado sin init");
        return;
    }
#ifdef PIN_RELAY_MEDIA
    digitalWrite(PIN_RELAY_MEDIA, on ? HIGH : LOW);
    state.mediaOn = on;
    Logger::infof("Relays media %s (GPIO %d)", on ? "ON" : "OFF", PIN_RELAY_MEDIA);
#else
    state.mediaOn = on;
    Logger::infof("Relays media %s (controlado por software)", on ? "ON" : "OFF");
#endif
}

void Relays::update() {
    if(!initialized) return;

    // Handle emergency flag atomically to avoid race condition with ISR
    bool handleEmergency = false;
#if defined(ESP32) || defined(ESP_PLATFORM)
    portENTER_CRITICAL(&emergencyMux);
    if (emergencyRequested) {
        emergencyRequested = false;
        handleEmergency = true;
    }
    portEXIT_CRITICAL(&emergencyMux);
#else
    noInterrupts();
    if (emergencyRequested) {
        emergencyRequested = false;
        handleEmergency = true;
    }
    interrupts();
#endif
    
    if (handleEmergency) {
        Logger::error("EMERGENCY STOP (deferred) - Todos los relés desactivados");
        System::logError(699);
        lastStateChangeMs = millis();
    }

    unsigned long now = millis();
    if (seqState != SEQ_IDLE && (now - seqStepStartMs > SEQUENCE_TIMEOUT_MS)) {
        Logger::errorf("Relays: secuencia timeout estado %d - forzando apagado", (int)seqState);
        System::logError(650);
        digitalWrite(PIN_RELAY_DIR,  LOW);
        digitalWrite(PIN_RELAY_TRAC, LOW);
        digitalWrite(PIN_RELAY_MAIN, LOW);
        state.mainOn = state.tractionOn = state.steeringOn = false;
        seqState = SEQ_IDLE;
        lastStateChangeMs = now;
    }

    switch (seqState) {
        case SEQ_IDLE: break;
        case SEQ_EN_ENABLE_MAIN:
            if (!state.mainOn) {
                digitalWrite(PIN_RELAY_MAIN, HIGH);
                state.mainOn = true;
                Logger::info("Relay MAIN: ON (non-blocking)");
            }
            if (now - seqStepStartMs >= ENABLE_STEP_DELAY_MS) {
                seqState = SEQ_EN_ENABLE_TRAC;
                seqStepStartMs = now;
            }
            break;
        case SEQ_EN_ENABLE_TRAC:
            if (!state.tractionOn) {
                digitalWrite(PIN_RELAY_TRAC, HIGH);
                state.tractionOn = true;
                Logger::info("Relay TRAC: ON (non-blocking)");
            }
            if (now - seqStepStartMs >= ENABLE_STEP_DELAY_MS) {
                seqState = SEQ_EN_ENABLE_DIR;
                seqStepStartMs = now;
            }
            break;
        case SEQ_EN_ENABLE_DIR:
            if (!state.steeringOn) {
                digitalWrite(PIN_RELAY_DIR, HIGH);
                state.steeringOn = true;
                Logger::info("Relay DIR: ON (non-blocking)");
            }
            seqState = SEQ_EN_DONE;
            seqStepStartMs = now;
            break;
        case SEQ_EN_DONE:
            lastStateChangeMs = now;
            Logger::info("Relays power enabled - Secuencia no bloqueante completada");
            seqState = SEQ_IDLE;
            break;
        case SEQ_DIS_DISABLE_DIR:
            if (state.steeringOn) {
                digitalWrite(PIN_RELAY_DIR, LOW);
                state.steeringOn = false;
                Logger::warn("Relay DIR: OFF (non-blocking)");
            }
            if (now - seqStepStartMs >= DISABLE_STEP_DELAY_MS) {
                seqState = SEQ_DIS_DISABLE_TRAC;
                seqStepStartMs = now;
            }
            break;
        case SEQ_DIS_DISABLE_TRAC:
            if (state.tractionOn) {
                digitalWrite(PIN_RELAY_TRAC, LOW);
                state.tractionOn = false;
                Logger::warn("Relay TRAC: OFF (non-blocking)");
            }
            if (now - seqStepStartMs >= DISABLE_STEP_DELAY_MS) {
                seqState = SEQ_DIS_DISABLE_MAIN;
                seqStepStartMs = now;
            }
            break;
        case SEQ_DIS_DISABLE_MAIN:
            if (state.mainOn) {
                digitalWrite(PIN_RELAY_MAIN, LOW);
                state.mainOn = false;
                Logger::warn("Relay MAIN: OFF (non-blocking)");
            }
            seqState = SEQ_DIS_DONE;
            seqStepStartMs = now;
            break;
        case SEQ_DIS_DONE:
            lastStateChangeMs = now;
            Logger::warn("Relays power disabled - Secuencia no bloqueante completada");
            seqState = SEQ_IDLE;
            break;
        default:
            seqState = SEQ_IDLE;
            break;
    }

    bool system_error = false;
    static uint32_t lastErrorMs = 0;
    static uint8_t consecutiveErrors = 0;

    float batteryCurrent = Sensors::getCurrent(BATTERY_CHANNEL);
    if (!isnan(batteryCurrent) && batteryCurrent > BATTERY_OVERCURRENT_LIMIT_A && batteryCurrent < 10000.0f) {
        Logger::errorf("Relays: OVERCURRENT batería %.1fA (límite %.1fA)", batteryCurrent, BATTERY_OVERCURRENT_LIMIT_A);
        System::logError(602);
        system_error = true;
    }

    for (int i = 0; i < 4; i++) {
        float temp = Sensors::getTemperature(i);
        if (!isnan(temp) && temp > MOTOR_OVERTEMP_LIMIT_C && temp < 1000.0f) {
            Logger::errorf("Relays: OVERTEMP motor %d: %.1f°C (límite %.1f°C)", i, temp, MOTOR_OVERTEMP_LIMIT_C);
            System::logError(603 + i);
            system_error = true;
        }
    }

    float batteryVoltage = Sensors::getVoltage(BATTERY_CHANNEL);
    if (!isnan(batteryVoltage) && batteryVoltage > 0.0f) {
        if (batteryVoltage < BATTERY_VOLTAGE_MIN_V) {
            Logger::errorf("Relays: BATERÍA BAJA %.1fV (crítico <%.1fV)", batteryVoltage, BATTERY_VOLTAGE_MIN_V);
            System::logError(607);
            system_error = true;
        } else if (batteryVoltage > BATTERY_VOLTAGE_MAX_V && batteryVoltage < 1000.0f) {
            Logger::errorf("Relays: BATERÍA ALTA %.1fV (peligro >%.1fV)", batteryVoltage, BATTERY_VOLTAGE_MAX_V);
            System::logError(608);
            system_error = true;
        }
    }

    if(system_error) {
        consecutiveErrors++;
        lastErrorMs = now;
        if (consecutiveErrors >= 3) {
            Logger::errorf("Relays: ERROR CRÍTICO (%d consecutivos) - Iniciando secuencia de apagado", consecutiveErrors);
            System::logError(600);
            if (seqState == SEQ_IDLE) {
                seqState = SEQ_DIS_DISABLE_DIR;
                seqStepStartMs = millis();
            }
        }
    } else {
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