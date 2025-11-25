#pragma once
#include <Arduino.h>

namespace Shifter {
    enum Gear { P, D2, D1, N, R };

    struct State {
        Gear gear;
        bool changed;  // flanco de cambio
    };

    void init();
    void update();

    State get();
    void setGear(Gear g); // para forzar desde menú o pruebas
    
    // 🔒 v2.5.0: Estado de inicialización
    bool initOK();
}