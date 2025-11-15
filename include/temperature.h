#pragma once
namespace Sensors {
    // Número de sensores de temperatura: 4 motores + 1 ambiente
    constexpr int NUM_TEMPS = 5;

    // Inicialización y actualización
    void initTemperature();
    void updateTemperature();

    // 🔎 Nuevo: estado de inicialización global de sensores de temperatura
    bool temperatureInitOK();

    // Lectura de temperatura en °C
    // Índices:
    // 0 = Motor FL
    // 1 = Motor FR
    // 2 = Motor RL
    // 3 = Motor RR
    // 4 = Ambiente
    float getTemperature(int index);

    // Estado del sensor (true = OK, false = fallo)
    bool isTemperatureSensorOk(int index);
}