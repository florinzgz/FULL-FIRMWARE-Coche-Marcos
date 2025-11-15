#pragma once
#include "current.h"
#include "wheels.h"
#include "temperature.h"

// API global de sensores
namespace Sensors {
    // Inicializa todos los sensores
    void init();

    // Actualiza lecturas de todos los sensores
    void update();

    // Devuelve true si la inicialización fue correcta
    bool initOK();

    // Devuelve velocidad de rueda en km/h (idx = 0..3)
    float getWheelSpeed(int idx);

    // Devuelve voltaje de batería (V)
    float getVoltage(int idx);
}