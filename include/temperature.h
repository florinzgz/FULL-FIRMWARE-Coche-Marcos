#pragma once
#include <stdint.h>

namespace Sensors {
// Número de sensores de temperatura: 4 motores + 1 ambiente
constexpr int NUM_TEMPS = 5;

// 🔒 Constantes de configuración (evitar números mágicos)
constexpr float EMA_FILTER_ALPHA =
    0.2f; // Factor de suavizado (0.0 = sin filtro, 1.0 = sin suavizado)
constexpr float TEMP_MIN_CELSIUS = -40.0f; // Rango mínimo válido
constexpr float TEMP_MAX_CELSIUS = 150.0f; // Rango máximo válido
constexpr uint32_t UPDATE_INTERVAL_MS =
    1000; // Frecuencia de actualización (1 Hz)
constexpr float TEMP_CRITICAL_CELSIUS =
    85.0f; // Temperatura crítica para motores

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

// 🔒 MEJORA OPCIONAL: Diagnóstico avanzado
struct TemperatureStatus {
  uint8_t sensorsDetected;   // Sensores detectados en bus OneWire
  uint8_t sensorsWorking;    // Sensores funcionando correctamente
  bool criticalTempDetected; // Algún sensor superó temperatura crítica
  float maxTemp;             // Temperatura máxima registrada
  uint32_t lastUpdateMs;     // Timestamp de última actualización
};

TemperatureStatus getTemperatureStatus();
} // namespace Sensors