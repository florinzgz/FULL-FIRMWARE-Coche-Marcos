#pragma once

namespace SteeringModel {
    struct Ackermann {
        float innerDeg;   // ángulo rueda interior (°)
        float outerDeg;   // ángulo rueda exterior (°)
    };

    // Configuración de la geometría
    void setGeometry(float wheelbase_m, float track_m, float maxInnerDeg);

    // Cálculo de Ackermann a partir de un ángulo global
    Ackermann compute(float wheelAngleDeg);
}