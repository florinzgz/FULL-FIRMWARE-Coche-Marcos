#include "sensors.h"
#include "logger.h"

namespace Sensors {

    static bool ok = false;

    void init() {
        // Aquí inicializarías tus sensores reales
        ok = true;
        Logger::info("Sensors init OK");
    }

    void update() {
        // Aquí refrescarías lecturas reales
    }

    bool initOK() {
        return ok;
    }
}