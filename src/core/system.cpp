#include "system.h"
#include "dfplayer.h"
#include "current.h"
#include "temperature.h"
#include "wheels.h"
#include "pedal.h"
#include "steering.h"
#include "relays.h"
#include "logger.h"
#include "storage.h"
#include "steering_motor.h"   // 🔒 v2.4.0: Para verificar motor dirección
#include "traction.h"         // 🔒 v2.4.0: Para verificar tracción

extern Storage::Config cfg;

static System::State currentState = System::OFF;

void System::init() {
    Logger::info("System init: entrando en PRECHECK");
    currentState = PRECHECK;
    
    // 🔒 v2.10.7: Enhanced diagnostic information
    Logger::infof("System init: Estado inicial OK");
    Logger::infof("System init: Free heap: %u bytes", ESP.getFreeHeap());
    
    #ifdef ARDUINO_ESP32S3_DEV
    Logger::info("System init: Platform ESP32-S3 detected");
    #endif
}

System::Health System::selfTest() {
    Health h{true,true,true,true,true};

    // Pedal (crítico)
    if(!Pedal::initOK()) {
        System::logError(100);
        Logger::errorf("SelfTest: pedal no responde");
        h.ok = false;
    }

    // Dirección (encoder)
    if(cfg.steeringEnabled) {
        if(!Steering::initOK()) {
            System::logError(200);
            Logger::errorf("SelfTest: encoder dirección no responde");
            h.steeringOK = false;
            h.ok = false;
        }
        
        // 🔒 v2.4.0: Verificar motor dirección también
        // NOTA CRÍTICA: El motor de dirección NO es crítico para arranque inicial porque:
        // 1. Puede inicializarse tardíamente una vez que I2C esté estable
        // 2. El vehículo está PARADO durante selfTest (marcha P obligatoria)
        // 3. El sistema de relés cortará potencia si hay fallo grave
        // Sin embargo, se marca h.steeringOK = false para indicar problema parcial
        if(!SteeringMotor::initOK()) {
            System::logError(250);
            Logger::errorf("SelfTest: motor dirección no responde (no crítico en arranque)");
            h.steeringOK = false;
            // h.ok permanece true - vehículo puede arrancar pero con precaución
        }
    }

    // Corriente
    if(cfg.currentSensorsEnabled) {
        if(!Sensors::currentInitOK()) {
            System::logError(300);
            Logger::errorf("SelfTest: INA226 no responde");
            h.currentOK = false;
            h.ok = false;
        }
    }

    // Temperatura
    if(cfg.tempSensorsEnabled) {
        if(!Sensors::temperatureInitOK()) {
            System::logError(400);
            Logger::errorf("SelfTest: DS18B20 no responde");
            h.tempsOK = false;
            h.ok = false;
        }
    }

    // Ruedas
    if(cfg.wheelSensorsEnabled) {
        if(!Sensors::wheelsInitOK()) {
            System::logError(500);
            Logger::errorf("SelfTest: sensores de rueda no responden");
            h.wheelsOK = false;
            h.ok = false;
        }
    }

    // Relés (crítico)
    if(!Relays::initOK()) {
        System::logError(600);
        Logger::errorf("SelfTest: relés no responden");
        h.ok = false;
    }
    
    // 🔒 v2.4.0: Tracción (no crítico pero loggear)
    if(cfg.tractionEnabled) {
        if(!Traction::initOK()) {
            Logger::warn("SelfTest: módulo tracción no inicializado");
            // No marcar como fallo crítico
        }
    }

    // DFPlayer (no crítico)
    if(!Audio::initOK()) {
        Logger::warn("SelfTest: DFPlayer no inicializado");
    }

    return h;
}

void System::update() {
    switch(currentState) {
        case PRECHECK: {
            auto h = selfTest();
            if(h.ok) {
                Logger::info("SelfTest OK → READY");
                currentState = READY;
            } else {
                Logger::errorf("SelfTest FAIL → ERROR");
                currentState = ERROR;
            }
        } break;

        case READY:
            Logger::info("System READY → RUN");
            currentState = RUN;
            break;

        case RUN:
            // Aquí se puede añadir lógica de watchdog o monitorización
            break;

        case ERROR:
            Relays::disablePower();
            break;

        case OFF:
        default:
            break;
    }
}

System::State System::getState() {
    return currentState;
}

// --- API de diagnóstico persistente ---
void System::logError(uint16_t code) {
    for(int i=0; i<cfg.errorCount; i++) {
        if(cfg.errors[i].code == code) return;
    }
    if(cfg.errorCount < Storage::Config::MAX_ERRORS) {
        cfg.errors[cfg.errorCount++] = {code, millis()};
    } else {
        for(int i=1; i<Storage::Config::MAX_ERRORS; i++)
            cfg.errors[i-1] = cfg.errors[i];
        cfg.errors[Storage::Config::MAX_ERRORS-1] = {code, millis()};
    }
    Storage::save(cfg);
}

const Storage::ErrorLog* System::getErrors() {
    return cfg.errors;
}

int System::getErrorCount() {
    return cfg.errorCount;
}

void System::clearErrors() {
    cfg.errorCount = 0;
    for(int i=0; i<Storage::Config::MAX_ERRORS; i++) {
        cfg.errors[i] = {0,0};
    }
    Storage::save(cfg);
}

bool System::hasError() {
    return currentState == ERROR || cfg.errorCount > 0;
}