#pragma once
#include <Arduino.h>

namespace Traction {

    // Identificadores de ruedas
    enum Wheel { 
        FL = 0,   // Front Left (delantera izquierda)
        FR = 1,   // Front Right (delantera derecha)
        RL = 2,   // Rear Left (trasera izquierda)
        RR = 3    // Rear Right (trasera derecha)
    };

    // Estado de cada rueda
    struct WheelState {
        float demandPct;   // Pedido individual a la rueda (0–100 %)
        float outPWM;      // PWM aplicado (0–255)
        float effortPct;   // % de esfuerzo respecto a corriente máxima (INA226)
        float currentA;    // Corriente instantánea (A)
        float speedKmh;    // Velocidad de rueda (km/h)
        float tempC;       // Temperatura motor (°C, DS18B20)
    };

    // Estado global de tracción
    struct State {
        bool enabled4x4;   // true = 4x4, false = 4x2 (solo eje delantero)
        float demandPct;   // Demanda global desde pedal (0–100 %)
        WheelState w[4];   // Estado de cada rueda
    };

    // Inicialización de módulo de tracción
    void init();

    // Cambiar modo 4x4 / 4x2
    void setMode4x4(bool on);

    // Establecer demanda global (desde pedal)
    void setDemand(float percent);

    // Actualizar control de tracción:
    // - Lee sensores (corriente, temperatura, velocidad)
    // - Calcula reparto de par (Ackermann virtual)
    // - Aplica PWM a cada BTS7960
    void update();

    // Obtener estado actual
    const State& get();

    // Verificar inicialización
    bool initOK();
}