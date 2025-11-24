#pragma once
#include <Arduino.h>

namespace SteeringMotor {
    struct State {
        float demandDeg;   // ángulo demandado (+/-)
        float pwmOut;      // valor PWM aplicado
        float currentA;    // corriente INA226
    };

    // Inicialización del motor de dirección
    void init();
    
    // Establecer ángulo de demanda desde steering o control superior
    void setDemandAngle(float deg);
    
    // Actualizar control del motor (llamar en loop)
    void update();
    
    // 🔒 Parada de emergencia inmediata
    void emergencyStop();
    
    // 🔒 Verificar estado de inicialización
    bool initOK();

    // Obtener estado actual
    const State& get();
}