#include "dfplayer.h"
#include "logger.h"
#include "queue.h"
#include "system.h"   // para logError()
#include "pins.h"     // para PIN_DFPLAYER_TX y PIN_DFPLAYER_RX
#include <DFRobotDFPlayerMini.h>
#include <HardwareSerial.h>

using namespace Audio;

// DFPlayer instance and Serial for communication
static DFRobotDFPlayerMini dfPlayer;
static HardwareSerial DFSerial(0);  // UART0 for DFPlayer

// Flag de inicialización
static bool initialized = false;

void Audio::DFPlayer::init() {
    // Initialize Serial port for DFPlayer on UART0 pins
    // Note: This uses the same hardware UART as USB Serial (pins 43/44)
    // In production, Serial debugging may need to be disabled
    DFSerial.begin(9600, SERIAL_8N1, PIN_DFPLAYER_RX, PIN_DFPLAYER_TX);
    
    // Initialize DFPlayer with actual hardware check
    bool ok = dfPlayer.begin(DFSerial);  // Actual initialization result
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

    // Check for messages from DFPlayer
    if (dfPlayer.available()) {
        uint8_t type = dfPlayer.readType();  // Read message type
        int value = dfPlayer.read();         // Read message value
        
        // Log errors (types >= 0x01 are typically errors/notifications)
        // Common error types: Busy, Sleeping, SerialWrong, CheckSum, FileIndex, etc.
        if (type != 0 && type != 0x3D && type != 0x3F) {  // Ignore DFPlayerPlayFinished and normal feedback
            Logger::infof("DFPlayer message - Type: 0x%02X, Value: %d", type, value);
            
            // Log specific error codes if they indicate problems
            if (type == 0x01 || type == 0x02) {  // Error or status messages
                Logger::errorf("DFPlayer error - Type: 0x%02X, Value: %d", type, value);
                System::logError(702 + type);
            }
        }
    }

    // Despachar desde la cola si quieres:
    // Item it;
    // if (AudioQueue::pop(it)) play(it.track);
}

bool Audio::DFPlayer::initOK() {
    return initialized;
}