#pragma once
#include <Arduino.h>

// ============================================================================
// telemetry.h - Sistema de Telemetría Avanzada v2.8.0
// ✅ Características de fiabilidad 100%
// - Checksum FNV-1a + Magic Number → detecta corrupción en datos persistentes
// - Guardado periódico automático con intervalo configurable
// - Métricas extendidas: SOC, temperatura batería, eficiencia regen
// - Exportación JSON → listo para SD, WiFi o app móvil
// ============================================================================

namespace Telemetry {

// -----------------------
// Datos de telemetría del vehículo
// -----------------------
struct VehicleData {
  uint32_t magic = 0xDEADBEEF; // Identificador para validar estructura
  uint32_t checksum = 0;       // Checksum para detectar corrupción

  // Distancias
  double totalDistanceKm = 0.0;
  double sessionDistanceKm = 0.0;
  double tripDistanceKm = 0.0;

  // Energía
  double energyConsumedKwh = 0.0;
  double sessionEnergyKwh = 0.0;
  double regenEnergyKwh = 0.0;
  double sessionRegenKwh = 0.0;

  // Tiempo de funcionamiento (segundos)
  uint64_t runtimeSeconds = 0;
  uint64_t sessionRuntimeSeconds = 0;

  // Velocidad
  float avgSpeedKmh = 0.0f;
  float maxSpeedKmh = 0.0f;
  float currentSpeedKmh = 0.0f;

  // Batería
  float minBatteryVoltage = 0.0f;
  float maxBatteryVoltage = 0.0f;
  float avgBatteryCurrent = 0.0f;
  float maxBatteryCurrent = 0.0f;
  float stateOfChargePercent = 0.0f;

  // Temperaturas
  float maxMotorTemp = 0.0f;
  float avgMotorTemp = 0.0f;
  float maxBatteryTemp = 0.0f;

  // Frenado regenerativo
  float regenEfficiencyPercent = 0.0f;
  uint32_t regenActivations = 0;

  // Timestamp
  uint32_t lastUpdateMs = 0;
};

// -----------------------
// Configuración del sistema de telemetría
// -----------------------
struct Config {
  bool enabled = true;
  uint16_t updateIntervalMs = 1000; // Intervalo de actualización
  bool persistToStorage = true;
  uint16_t persistIntervalSec = 60;  // Guardado cada 60s
  float batteryCapacityKwh = 0.576f; // 24V * 24Ah = 576Wh
};

// -----------------------
// API pública
// -----------------------
void init();
void update();

const VehicleData &getData();
const Config &getConfig();
void setConfig(const Config &c);

void resetSession();
void resetTrip();

void saveToStorage();
void loadFromStorage();

void setEnabled(bool enable);
bool initOK();

float getEstimatedRangeKm();
float getAvgConsumptionWhKm();

// Exportación JSON (para SD, WiFi, app móvil)
String exportToJson();

// -----------------------
// Funciones para actualizar métricas desde sensores
// -----------------------
void updateSpeed(float speedKmh);
void addDistance(float distanceKm);
void addEnergyConsumed(float energyKwh);
void addEnergyRegenerated(float energyKwh);
void updateBattery(float voltage, float current, float soc);
void updateTemperature(float motorTemp, float batteryTemp = 0.0f);
void updateRegenEfficiency(float efficiency);

} // namespace Telemetry
