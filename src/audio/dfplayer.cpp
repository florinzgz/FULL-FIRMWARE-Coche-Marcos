#include "dfplayer.h"
#include "logger.h"
#include "queue.h"
#include "system.h"   // para logError()

using namespace Audio;

// Flag de inicialización
static bool initialized = false;

void Audio::DFPlayer::init() {
    // Reemplaza con inicialización real del driver DFPlayer
    bool ok = true; // placeholder
    if (!ok) {
        Logger::errorf("DFPlayer init failed");   // ← corregido
        System::logError(700); // código reservado: fallo init
        initialized = false;
        return;
    }
    Logger::info("DFPlayer initialized");
    initialized = true;
}

void Audio::DFPlayer::play(uint16_t track) {
    if(!initialized) {
        Logger::warn("DFPlayer play() llamado sin init");
        System::logError(701);
        return;
    }

    bool ready = true; // placeholder
    if (!ready) {
        Logger::warnf("DFPlayer not ready, cannot play track %u", (unsigned)track);
        System::logError(701);
        return;
    }

    // Reemplaza por la llamada real al driver DFPlayer
    Logger::infof("DFPlayer play track %u", (unsigned)track);
}

void Audio::DFPlayer::update() {
    if(!initialized) return;

    int error_code = 0; // placeholder
    if (error_code != 0) {
        Logger::errorf("DFPlayer error code %d", error_code);
        System::logError(702 + error_code);
    }

    // Despachar desde la cola si quieres:
    // Item it;
    // if (AudioQueue::pop(it)) play(it.track);
}

bool Audio::DFPlayer::initOK() {
    return initialized;
}