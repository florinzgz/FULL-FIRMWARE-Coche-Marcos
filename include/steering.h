#pragma once
#include <Arduino.h>

namespace Steering {
    struct State {
        long ticks;        // contador incremental
        float angleDeg;    // ángulo global en grados (columna/volante)
        float angleFL;     // ángulo rueda delantera izquierda
        float angleFR;     // ángulo rueda delantera derecha
        bool centered;     // punto 0 fijado
        bool valid;        // encoder activo
    };

    void init();
    void update();

    // Nuevo: estado de inicialización
    bool initOK();

    // Centrado por sensor inductivo (Z)
    void center();

    // Configuración del encoder
    void setTicksPerTurn(long tpt);   // resolución por vuelta del encoder
    long getTicksPerTurn();

    // Offset 0°
    void setZeroOffset(long offset);
    long getZeroOffset();

    const State& get();
}