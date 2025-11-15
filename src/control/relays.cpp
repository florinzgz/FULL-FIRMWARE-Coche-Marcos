#include "relays.h"
#include "logger.h"
#include "system.h"   // para logError()

// Flag de inicialización y estado
static bool initialized = false;
static Relays::State state = {false, false, false, false, false};

void Relays::init() {
    Logger::info("Relays init");
    // inicialización de pines si procede
    state = {false, false, false, false, false};
    initialized = true;
}

void Relays::enablePower() {
    if(!initialized) {
        Logger::warn("Relays enablePower() llamado sin init");
        return;
    }
    state.mainOn = true;
    state.tractionOn = true;
    state.steeringOn = true;
    Logger::info("Relays power enabled");
}

void Relays::disablePower() {
    if(!initialized) {
        Logger::warn("Relays disablePower() llamado sin init");
        return;
    }
    state.mainOn = false;
    state.tractionOn = false;
    state.steeringOn = false;
    Logger::warn("Relays power disabled");
}

void Relays::setLights(bool on) {
    if(!initialized) {
        Logger::warn("Relays setLights() llamado sin init");
        return;
    }
    state.lightsOn = on;
    Logger::infof("Relays lights %s", on ? "ON" : "OFF");
}

void Relays::setMedia(bool on) {
    if(!initialized) {
        Logger::warn("Relays setMedia() llamado sin init");
        return;
    }
    state.mediaOn = on;
    Logger::infof("Relays media %s", on ? "ON" : "OFF");
}

void Relays::update() {
    if(!initialized) return;

    bool system_error = false; // sustituir por tu lógica real
    if(system_error) {
        Logger::errorf("Relays forced OFF due to system ERROR");  // ← corregido
        System::logError(600); // código reservado: fallo crítico relés
        disablePower();
    }
    // Nota: no spameamos "update OK" en cada ciclo, solo errores/transiciones
}

const Relays::State& Relays::get() {
    return state;
}

bool Relays::initOK() {
    return initialized;
}