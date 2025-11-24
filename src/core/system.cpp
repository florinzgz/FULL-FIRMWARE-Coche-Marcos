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

extern Storage::Config cfg;

static System::State currentState = System::OFF;

void System::init() {
    Logger::info("System init: entrando en PRECHECK");
    currentState = PRECHECK;
}

System::Health System::selfTest() {
    Health h{true,true,true,true,true};

    // Pedal (crítico)
    if(!Pedal::initOK()) {
        System::logError(100);
        Logger::errorf("SelfTest: pedal no responde");
        h.ok = false;
    }

    // Dirección
    if(cfg.steeringEnabled) {
        if(!Steering::initOK()) {
            System::logError(200);
            Logger::errorf("SelfTest: dirección no responde");
            h.steeringOK = false;
            h.ok = false;
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