#include "telemetry.h"
#include "storage.h"
#include "logger.h"
#include <Preferences.h>
#include <cmath>

// ============================================================================
// telemetry.cpp - Sistema de Telemetría Avanzada v2.8.0
// Implementación con persistencia en NVS (Preferences)
// ============================================================================

namespace Telemetry {

// -----------------------
// Constantes
// -----------------------
static const float MIN_DISTANCE_THRESHOLD = 0.001f;  // km mínimo para cálculos

// -----------------------
// Variables internas
// -----------------------
static VehicleData data;
static Config cfg;
static bool initialized = false;
static uint32_t lastPersistMs = 0;
static Preferences prefs;

// Acumuladores para promedios (a nivel de namespace para evitar variables estáticas en funciones)
static double speedAccum = 0.0;
static uint32_t speedSamples = 0;
static double currentAccum = 0.0;
static uint32_t currentSamples = 0;
static double tempAccum = 0.0;
static uint32_t tempSamples = 0;

// Namespace para NVS
static const char* NVS_NAMESPACE = "telemetry";

// -----------------------
// Funciones auxiliares
// -----------------------
static uint32_t calculateChecksum(const VehicleData& d) {
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&d);
    // Calcular checksum excluyendo el campo checksum
    size_t len = offsetof(VehicleData, checksum);
    uint32_t hash = 2166136261u; // FNV-1a offset basis
    for (size_t i = 0; i < len; i++) {
        hash ^= bytes[i];
        hash *= 16777619u; // FNV-1a prime
    }
    return hash;
}

static bool isCorrupted(const VehicleData& d) {
    return (d.magic != 0xDEADBEEF || d.checksum != calculateChecksum(d));
}

// -----------------------
// API pública - Implementaciones
// -----------------------

void init() {
    loadFromStorage();
    
    // Inicializar valores de sesión
    data.sessionDistanceKm = 0;
    data.sessionEnergyKwh = 0;
    data.sessionRegenKwh = 0;
    data.sessionRuntimeSeconds = 0;
    data.avgSpeedKmh = 0;
    data.maxSpeedKmh = 0;
    data.lastUpdateMs = millis();
    
    initialized = true;
    lastPersistMs = millis();
    Logger::info("Telemetry: Init OK");
}

void update() {
    if (!cfg.enabled) return;
    
    uint32_t now = millis();
    if (now - data.lastUpdateMs < cfg.updateIntervalMs) return;
    
    // Actualizar tiempo de sesión
    uint32_t elapsed = (now - data.lastUpdateMs) / 1000;
    data.sessionRuntimeSeconds += elapsed;
    data.runtimeSeconds += elapsed;
    
    data.lastUpdateMs = now;

    // Guardado periódico
    if (cfg.persistToStorage && (now - lastPersistMs) > (cfg.persistIntervalSec * 1000UL)) {
        saveToStorage();
        lastPersistMs = now;
    }
}

const VehicleData& getData() { 
    return data; 
}

const Config& getConfig() { 
    return cfg; 
}

void setConfig(const Config& c) { 
    cfg = c; 
}

void resetSession() {
    data.sessionDistanceKm = 0;
    data.sessionEnergyKwh = 0;
    data.sessionRegenKwh = 0;
    data.sessionRuntimeSeconds = 0;
    data.avgSpeedKmh = 0;
    data.maxSpeedKmh = 0;
    Logger::info("Telemetry: Session reset");
}

void resetTrip() {
    data.tripDistanceKm = 0;
    Logger::info("Telemetry: Trip reset");
}

void saveToStorage() {
    data.checksum = calculateChecksum(data);
    
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        Logger::error("Telemetry: Failed to open NVS for writing");
        return;
    }
    
    // Guardar datos como blob
    size_t written = prefs.putBytes("data", &data, sizeof(VehicleData));
    prefs.end();
    
    if (written == sizeof(VehicleData)) {
        Logger::debug("Telemetry: Datos guardados en storage");
    } else {
        Logger::errorf("Telemetry: Error guardando datos (%u/%u bytes)", 
                      written, sizeof(VehicleData));
    }
}

void loadFromStorage() {
    if (!prefs.begin(NVS_NAMESPACE, true)) {
        Logger::warn("Telemetry: NVS no existe, usando valores por defecto");
        data = VehicleData();
        return;
    }
    
    size_t len = prefs.getBytesLength("data");
    
    if (len != sizeof(VehicleData)) {
        Logger::warnf("Telemetry: Tamaño datos inválido (%u vs %u)", 
                     len, sizeof(VehicleData));
        prefs.end();
        data = VehicleData();
        saveToStorage();
        return;
    }
    
    VehicleData temp;
    prefs.getBytes("data", &temp, sizeof(VehicleData));
    prefs.end();
    
    if (isCorrupted(temp)) {
        Logger::warn("Telemetry: Datos corruptos, restaurando valores por defecto");
        data = VehicleData();
        saveToStorage();
    } else {
        data = temp;
        Logger::infof("Telemetry: Cargado %.1f km, %lu horas", 
                      data.totalDistanceKm, 
                      static_cast<unsigned long>(data.runtimeSeconds / 3600));
    }
}

void setEnabled(bool enable) { 
    cfg.enabled = enable; 
}

bool initOK() { 
    return initialized; 
}

float getAvgConsumptionWhKm() {
    if (data.sessionDistanceKm <= MIN_DISTANCE_THRESHOLD) return 0.0f;
    return static_cast<float>((data.sessionEnergyKwh * 1000.0) / data.sessionDistanceKm);
}

float getEstimatedRangeKm() {
    float consumption = getAvgConsumptionWhKm();
    if (data.stateOfChargePercent <= 0 || consumption <= 0) return 0.0f;
    float remainingKwh = cfg.batteryCapacityKwh * (data.stateOfChargePercent / 100.0f);
    return (remainingKwh * 1000.0f) / consumption;
}

String exportToJson() {
    String json = "{";
    json += "\"distanceKm\":" + String(data.totalDistanceKm, 2) + ",";
    json += "\"tripKm\":" + String(data.tripDistanceKm, 2) + ",";
    json += "\"energyConsumedKwh\":" + String(data.energyConsumedKwh, 3) + ",";
    json += "\"regenEnergyKwh\":" + String(data.regenEnergyKwh, 3) + ",";
    json += "\"avgSpeedKmh\":" + String(data.avgSpeedKmh, 1) + ",";
    json += "\"maxSpeedKmh\":" + String(data.maxSpeedKmh, 1) + ",";
    json += "\"socPercent\":" + String(data.stateOfChargePercent, 1) + ",";
    json += "\"runtimeHours\":" + String(static_cast<unsigned long>(data.runtimeSeconds / 3600)) + ",";
    json += "\"consumptionWhKm\":" + String(getAvgConsumptionWhKm(), 1) + ",";
    json += "\"rangeKm\":" + String(getEstimatedRangeKm(), 1);
    json += "}";
    return json;
}

// -----------------------
// Funciones para actualizar métricas desde sensores
// -----------------------

void updateSpeed(float speedKmh) {
    data.currentSpeedKmh = speedKmh;
    if (speedKmh > data.maxSpeedKmh) {
        data.maxSpeedKmh = speedKmh;
    }
    // Promedio usando acumuladores a nivel de namespace
    speedAccum += speedKmh;
    speedSamples++;
    data.avgSpeedKmh = static_cast<float>(speedAccum / speedSamples);
}

void addDistance(float distanceKm) {
    if (distanceKm > 0) {
        data.totalDistanceKm += distanceKm;
        data.sessionDistanceKm += distanceKm;
        data.tripDistanceKm += distanceKm;
    }
}

void addEnergyConsumed(float energyKwh) {
    if (energyKwh > 0) {
        data.energyConsumedKwh += energyKwh;
        data.sessionEnergyKwh += energyKwh;
    }
}

void addEnergyRegenerated(float energyKwh) {
    if (energyKwh > 0) {
        data.regenEnergyKwh += energyKwh;
        data.sessionRegenKwh += energyKwh;
        data.regenActivations++;
    }
}

void updateBattery(float voltage, float current, float soc) {
    data.stateOfChargePercent = soc;
    
    if (voltage > 0) {
        if (data.minBatteryVoltage == 0 || voltage < data.minBatteryVoltage) {
            data.minBatteryVoltage = voltage;
        }
        if (voltage > data.maxBatteryVoltage) {
            data.maxBatteryVoltage = voltage;
        }
    }
    
    float absCurrent = fabs(current);
    if (absCurrent > data.maxBatteryCurrent) {
        data.maxBatteryCurrent = absCurrent;
    }
    
    // Promedio usando acumuladores a nivel de namespace
    currentAccum += absCurrent;
    currentSamples++;
    data.avgBatteryCurrent = static_cast<float>(currentAccum / currentSamples);
}

void updateTemperature(float motorTemp, float batteryTemp) {
    if (motorTemp > data.maxMotorTemp) {
        data.maxMotorTemp = motorTemp;
    }
    
    // Promedio usando acumuladores a nivel de namespace
    tempAccum += motorTemp;
    tempSamples++;
    data.avgMotorTemp = static_cast<float>(tempAccum / tempSamples);
    
    if (batteryTemp > data.maxBatteryTemp) {
        data.maxBatteryTemp = batteryTemp;
    }
}

void updateRegenEfficiency(float efficiency) {
    data.regenEfficiencyPercent = efficiency;
}

} // namespace Telemetry
