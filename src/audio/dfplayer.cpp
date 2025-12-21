#include "dfplayer.h"
#include "logger.h"
#include "queue.h"
#include "system.h"   // para logError()
#include <DFRobotDFPlayerMini.h>

using namespace Audio;

// DFPlayer instance and Serial2 for communication
static DFRobotDFPlayerMini dfPlayer;

// Flag de inicialización
static bool initialized = false;

void Audio::DFPlayer::init() {
    // Initialize DFPlayer with actual hardware check
    bool ok = dfPlayer.begin(Serial2);  // Actual initialization result
    if (!ok) {
        Logger::errorf("DFPlayer init failed");
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

    // Validar rango de tracks (1-68)
    if(track == 0 || track > 68) {
        Logger::warnf("DFPlayer play(): track inválido (%u). Rango válido: 1-68", (unsigned)track);
        System::logError(721); // track inválido
        return;
    }

    bool ready = dfPlayer.available();  // Check actual status
    if (!ready) {
        Logger::warnf("DFPlayer not ready, cannot play track %u", (unsigned)track);
        System::logError(701);
        return;
    }

    // Play track using DFPlayer library
    dfPlayer.play(track);
    Logger::infof("DFPlayer play track %u", (unsigned)track);
}

void Audio::DFPlayer::update() {
    if(!initialized) return;

    // Check for errors from DFPlayer
    if (dfPlayer.available()) {
        uint8_t error_code = dfPlayer.readType();  // Read actual error
        if (error_code == DFPlayerError) {
            int errorDetail = dfPlayer.read();
            Logger::errorf("DFPlayer error: %d", errorDetail);
            System::logError(702 + errorDetail);
        }
    }

    // Despachar desde la cola si quieres:
    // Item it;
    // if (AudioQueue::pop(it)) play(it.track);
}

bool Audio::DFPlayer::initOK() {
    return initialized;
}