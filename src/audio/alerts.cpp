#include "alerts.h"
#include "queue.h"
#include "logger.h"
#include "system.h"   // para logError()

using namespace Audio;

// 🔎 Flag de inicialización
static bool initialized = false;

void Alerts::init() {
    // Inicializaciones específicas de audio si hicieran falta
    Logger::info("Alerts init");
    initialized = true;
}

void Alerts::play(const Audio::Item &item) {
    if(!initialized) {
        Logger::warn("Alerts play(item) llamado sin init");
        System::logError(720); // código reservado: alerta sin init
        return;
    }

    if(item.track == 0) {
        Logger::warn("Alerts play(item): track inválido");
        System::logError(721); // código reservado: track inválido
        return;
    }

    // Empujar directamente a la cola de audio
    if(!AudioQueue::push(item.track, item.prio)) {
        Logger::errorf("Alerts: cola de audio llena");   // ← corregido
        System::logError(722); // código reservado: cola llena
        return;
    }

    Logger::infof("Alerts queued track %u prio %u",
                  (unsigned)item.track, (unsigned)item.prio);
}

void Alerts::play(Audio::Track t) {
    if(!initialized) {
        Logger::warn("Alerts play(Track) llamado sin init");
        System::logError(720);
        return;
    }

    uint16_t trackId = static_cast<uint16_t>(t);
    if(trackId == 0) {
        Logger::warn("Alerts play(Track): track inválido");
        System::logError(721);
        return;
    }

    Audio::Item it { trackId, Priority::PRIO_NORMAL };
    if(!AudioQueue::push(it.track, it.prio)) {
        Logger::errorf("Alerts: cola de audio llena");   // ← corregido
        System::logError(722);
        return;
    }

    Logger::infof("Alerts queued track %u (from Track) prio %u",
                  (unsigned)it.track, (unsigned)it.prio);
}

bool Alerts::initOK() {
    return initialized;
}