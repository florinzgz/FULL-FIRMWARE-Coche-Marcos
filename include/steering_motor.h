#pragma once
#include <Arduino.h>

namespace SteeringMotor {
    struct State {
        float demandDeg;   // ángulo demandado (+/-)
        float pwmOut;      // valor PWM aplicado
        float currentA;    // corriente INA226
    };

    void init();
    void setDemandAngle(float deg); // desde Steering::State.angleDeg o control
    void update();

    const State& get();
    
    // 🔒 v2.4.0: Estado de inicialización
    bool initOK();
}