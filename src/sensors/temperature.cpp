#include "temperature.h"
#include "logger.h"
#include "pins.h"
#include "settings.h"
#include "storage.h"
#include "system.h" // para logError()
#include <DallasTemperature.h>
#include <OneWire.h>

extern Storage::Config cfg;

#ifndef DISABLE_SENSORS
// Only instantiate in full vehicle mode
// These global constructors configure GPIO pins and could cause bootloop
static OneWire oneWire(PIN_ONEWIRE);
static DallasTemperature sensors(&oneWire);
#endif

static float lastTemp[Sensors::NUM_TEMPS];
static bool sensorOk[Sensors::NUM_TEMPS];

// 🔒 CORRECCIÓN 4.1: Almacenar direcciones ROM específicas de cada sensor
static DeviceAddress tempSensorAddrs[Sensors::NUM_TEMPS];
static bool addressesStored[Sensors::NUM_TEMPS] = {false};

static uint32_t lastUpdateMs = 0;

// 🔒 CORRECCIÓN 4.2: Variables para conversión asíncrona
static bool requestPending = false;
static unsigned long requestTime = 0;
static const unsigned long CONVERSION_TIMEOUT_MS = 1000; // 1 segundo timeout

// Flag de inicialización global
static bool initialized = false;

// 🔒 CORRECCIÓN CRÍTICA: Mutex para proteger acceso concurrente a lastTemp[]
// Patrón consistente con current.cpp (líneas 16, 56-78)
static SemaphoreHandle_t tempMutex = nullptr;

void Sensors::initTemperature() {
#ifdef DISABLE_SENSORS
  Logger::info("Temperature: Skipped in DISABLE_SENSORS mode");
  initialized = true;
  return;
#else
  // 🔒 CORRECCIÓN CRÍTICA: Crear mutex si no existe
  if (tempMutex == nullptr) {
    tempMutex = xSemaphoreCreateMutex();
    if (tempMutex == nullptr) {
      Logger::error("Temperature: No se pudo crear mutex");
      System::logError(398);
      return;
    }
  }

  sensors.begin();

  int count = sensors.getDeviceCount();

  if (count != NUM_TEMPS) {
    Logger::warnf("DS18B20: detectados %d, esperados %d", count, NUM_TEMPS);
  }

  // Timeout per-sensor para prevenir bloqueo en un sensor lento
  // pero permitir que sensores subsecuentes se inicialicen
  const uint32_t PER_SENSOR_TIMEOUT_MS = 500; // 500ms por sensor

  // 🔒 CORRECCIÓN 4.1: Almacenar direcciones ROM específicas
  // Usar el mínimo para evitar buffer overflow
  int sensorsToInit = (count < NUM_TEMPS) ? count : NUM_TEMPS;

  for (int i = 0; i < sensorsToInit; i++) {
    uint32_t sensorStartTime = millis(); // Tiempo de inicio para este sensor

    // Obtener dirección ROM del sensor
    bool gotAddress = sensors.getAddress(tempSensorAddrs[i], i);
    uint32_t addressDuration = millis() - sensorStartTime;

    if (addressDuration > PER_SENSOR_TIMEOUT_MS) {
      Logger::warnf("DS18B20 %d getAddress timeout (%u ms) - continuando", i,
                    addressDuration);
      addressesStored[i] = false;
      sensorOk[i] = false;
      lastTemp[i] = 0.0f;
      continue;
    }

    if (gotAddress) {
      // Configurar resolución máxima (12-bit = 0.0625°C, 750ms conversión)
      sensors.setResolution(tempSensorAddrs[i], 12);

      addressesStored[i] = true;
      sensorOk[i] = true;

      // Log dirección ROM para identificación
      Logger::infof("DS18B20 %d: ROM=0x%02X%02X%02X%02X%02X%02X%02X%02X", i,
                    tempSensorAddrs[i][0], tempSensorAddrs[i][1],
                    tempSensorAddrs[i][2], tempSensorAddrs[i][3],
                    tempSensorAddrs[i][4], tempSensorAddrs[i][5],
                    tempSensorAddrs[i][6], tempSensorAddrs[i][7]);
    } else {
      addressesStored[i] = false;
      sensorOk[i] = false;
      Logger::warnf("DS18B20 %d no detectado - continuando", i);
      // NO marcar como error crítico
    }
    lastTemp[i] = 0.0f;
  }

  // Marcar el resto como fallo si count < NUM_TEMPS
  for (int i = sensorsToInit; i < NUM_TEMPS; i++) {
    addressesStored[i] = false;
    sensorOk[i] = false;
    Logger::warnf("DS18B20 %d no detectado - continuando", i);
    lastTemp[i] = 0.0f;
  }

  // 🔒 Configurar modo asíncrono (no bloqueante)
  sensors.setWaitForConversion(false);

  initialized = (count > 0);
  Logger::infof("Temperature sensors init: %d/%d OK", sensorsToInit, NUM_TEMPS);

  if (count < NUM_TEMPS) {
    Logger::warn(
        "DS18B20 init: algunos sensores no disponibles - modo degradado");
  }
#endif
}

void Sensors::updateTemperature() {
#ifdef DISABLE_SENSORS
  // In disabled mode, return default values
  for (int i = 0; i < NUM_TEMPS; i++) {
    lastTemp[i] = 0.0f;
    sensorOk[i] = false;
  }
  return;
#else
  uint32_t now = millis();

  if (!cfg.tempSensorsEnabled) {
    for (int i = 0; i < NUM_TEMPS; i++) {
      lastTemp[i] = 0.0f;
      sensorOk[i] = false;
    }
    requestPending = false;
    return;
  }

  // 🔒 CORRECCIÓN 4.2: Conversión asíncrona con timeout
  if (!requestPending) {
    // Verificar si es momento de iniciar nueva lectura (~1 Hz)
    if (now - lastUpdateMs < UPDATE_INTERVAL_MS) return; // 🔒 Using constant

    // Iniciar request asíncrono (no bloqueante)
    sensors.requestTemperatures();
    requestPending = true;
    requestTime = now;
    return;
  }

  // Esperar al menos 750ms para conversión 12-bit
  if (now - requestTime < 750) { return; }

  // Timeout de seguridad
  if (now - requestTime > CONVERSION_TIMEOUT_MS) {
    Logger::warn("DS18B20: timeout en conversión");
    System::logError(450); // código: timeout conversión
    requestPending = false;
    lastUpdateMs = now;
    return;
  }

  // Conversión completa, leer resultados
  requestPending = false;
  lastUpdateMs = now;

  for (int i = 0; i < NUM_TEMPS; i++) {
    if (!sensorOk[i] || !addressesStored[i]) continue;

    // 🔒 Leer temperatura usando dirección ROM específica
    float t = sensors.getTempC(tempSensorAddrs[i]);

    // Validación y fallback
    if (t == DEVICE_DISCONNECTED_C || !isfinite(t)) {
      System::logError(400 + i);
      Logger::errorf("DS18B20 idx %d: lectura inválida (%.2f)", i, t);
      // mantener último valor válido
      continue;
    }

    // Clamps
    t = constrain(t, TEMP_MIN_CELSIUS, TEMP_MAX_CELSIUS); // 🔒 Using constants

    // 🔒 CORRECCIÓN CRÍTICA: Proteger escritura en lastTemp[] con mutex
    // Timeout de 10ms para evitar bloqueos
    if (tempMutex != nullptr &&
        xSemaphoreTake(tempMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      // 🔒 CORRECCIÓN 4.3: Filtro EMA con constante configurable
      lastTemp[i] = lastTemp[i] +
                    EMA_FILTER_ALPHA * (t - lastTemp[i]); // 🔒 Using constant
      xSemaphoreGive(tempMutex);
    } else {
      Logger::warn("Temperature: mutex timeout en updateTemperature");
    }
  }
#endif
}

float Sensors::getTemperature(int channel) {
  // 🔒 v2.4.1: Validación de rango completa
  if (channel < 0 || channel >= NUM_TEMPS) return 0.0f;

  // 🔒 CORRECCIÓN CRÍTICA: Proteger lectura de lastTemp[] con mutex
  // Timeout de 5ms (más corto que escritura)
  float temp = 0.0f;
  if (tempMutex != nullptr &&
      xSemaphoreTake(tempMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
    temp = lastTemp[channel];
    xSemaphoreGive(tempMutex);
  } else {
    // 🔒 MEJORA: En caso de timeout, evitar torn read y usar throttled logging
    static uint32_t lastTimeoutLog = 0;
    if (millis() - lastTimeoutLog > 5000) { // Log cada 5 segundos máximo
      Logger::warn("Temperature: read mutex timeout");
      lastTimeoutLog = millis();
    }
    // Retornar último valor sin mutex (mejor que bloquear completamente)
    // Nota: Esto es un torn read potencial pero preferible a un deadlock
    temp = lastTemp[channel];
  }
  return temp;
}

bool Sensors::isTemperatureSensorOk(int channel) {
  // 🔒 v2.4.1: Validación de rango completa
  if (channel >= 0 && channel < NUM_TEMPS) return sensorOk[channel];
  return false;
}

bool Sensors::temperatureInitOK() { return initialized; }

// 🔒 MEJORA OPCIONAL: Función de diagnóstico avanzado
Sensors::TemperatureStatus Sensors::getTemperatureStatus() {
  TemperatureStatus status;
#ifdef DISABLE_SENSORS
  status.sensorsDetected = 0;
  status.sensorsWorking = 0;
  status.criticalTempDetected = false;
  status.maxTemp = -999.0f;
  status.lastUpdateMs = 0;
#else
  status.sensorsDetected = sensors.getDeviceCount();
  status.sensorsWorking = 0;
  status.criticalTempDetected = false;
  status.maxTemp = -999.0f;
  status.lastUpdateMs = lastUpdateMs;

  for (int i = 0; i < NUM_TEMPS; i++) {
    if (sensorOk[i]) {
      status.sensorsWorking++;

      // Actualizar temperatura máxima
      if (lastTemp[i] > status.maxTemp) { status.maxTemp = lastTemp[i]; }

      // Detectar temperatura crítica (>85°C para motores)
      if (lastTemp[i] > TEMP_CRITICAL_CELSIUS) {
        status.criticalTempDetected = true;
      }
    }
  }
#endif

  return status;
}