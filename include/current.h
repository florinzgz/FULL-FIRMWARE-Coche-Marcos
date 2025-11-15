#pragma once
namespace Sensors {
    constexpr int NUM_CURRENTS = 6;   // 1 batería + 4 ruedas + dirección

    void initCurrent();
    void updateCurrent();

    float getCurrent(int channel);        // Corriente en amperios
    float getVoltage(int channel);        // Voltaje en voltios
    float getPower(int channel);          // Potencia en vatios
    float getShuntVoltage(int channel);   // Voltaje en shunt (mV)

    bool isCurrentSensorOk(int channel);  // Estado del sensor

    // 🔎 Nuevo: estado de inicialización global de sensores de corriente
    bool currentInitOK();
}