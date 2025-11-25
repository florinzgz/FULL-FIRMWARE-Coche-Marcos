#include "temperature.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include "pins.h"
#include "logger.h"
#include "storage.h"
#include "settings.h"
#include "system.h"   // para logError()

extern Storage::Config cfg;

static OneWire oneWire(PIN_ONEWIRE);
static DallasTemperature sensors(&oneWire);

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

void Sensors::initTemperature() {
    sensors.begin();

    int count = sensors.getDeviceCount();
    
    if (count != NUM_TEMPS) {
        Logger::warnf("DS18B20: detectados %d, esperados %d", count, NUM_TEMPS);
    }

    // 🔒 CORRECCIÓN 4.1: Almacenar direcciones ROM específicas
    // Usar el mínimo para evitar buffer overflow
    int sensorsToInit = (count < NUM_TEMPS) ? count : NUM_TEMPS;

    for(int i = 0; i < sensorsToInit; i++) {
        // Obtener dirección ROM del sensor
        if (sensors.getAddress(tempSensorAddrs[i], i)) {
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
            System::logError(400 + i); // registrar fallo persistente
            Logger::errorf("DS18B20 init FAIL idx %d - no se pudo obtener ROM", i);
        }
        lastTemp[i] = 0.0f;
    }

    // Marcar el resto como fallo si count < NUM_TEMPS
    for(int i = sensorsToInit; i < NUM_TEMPS; i++) {
        addressesStored[i] = false;
        sensorOk[i] = false;
        System::logError(400 + i);
        Logger::errorf("DS18B20 init FAIL idx %d - sensor no detectado", i);
        lastTemp[i] = 0.0f;
    }

    // 🔒 Configurar modo asíncrono (no bloqueante)
    sensors.setWaitForConversion(false);

    initialized = (count > 0);
    Logger::infof("Temperature sensors init: %d/%d OK", sensorsToInit, NUM_TEMPS);
}

void Sensors::updateTemperature() {
    uint32_t now = millis();

    if(!cfg.tempSensorsEnabled) {
        for(int i=0; i<NUM_TEMPS; i++) {
            lastTemp[i] = 0.0f;
            sensorOk[i] = false;
        }
        requestPending = false;
        return;
    }

    // 🔒 CORRECCIÓN 4.2: Conversión asíncrona con timeout
    if (!requestPending) {
        // Verificar si es momento de iniciar nueva lectura (~1 Hz)
        if (now - lastUpdateMs < UPDATE_INTERVAL_MS) return;  // 🔒 Using constant
        
        // Iniciar request asíncrono (no bloqueante)
        sensors.requestTemperatures();
        requestPending = true;
        requestTime = now;
        return;
    }
    
    // Esperar al menos 750ms para conversión 12-bit
    if (now - requestTime < 750) {
        return;
    }
    
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

    for(int i = 0; i < NUM_TEMPS; i++) {
        if(!sensorOk[i] || !addressesStored[i]) continue;

        // 🔒 Leer temperatura usando dirección ROM específica
        float t = sensors.getTempC(tempSensorAddrs[i]);

        // Validación y fallback
        if(t == DEVICE_DISCONNECTED_C || !isfinite(t)) {
            System::logError(400 + i);
            Logger::errorf("DS18B20 idx %d: lectura inválida (%.2f)", i, t);
            // mantener último valor válido
            continue;
        }

        // Clamps
        t = constrain(t, TEMP_MIN_CELSIUS, TEMP_MAX_CELSIUS);  // 🔒 Using constants

        // 🔒 CORRECCIÓN 4.3: Filtro EMA con constante configurable
        lastTemp[i] = lastTemp[i] + EMA_FILTER_ALPHA * (t - lastTemp[i]);  // 🔒 Using constant
    }
}

float Sensors::getTemperature(int channel) {
    // 🔒 v2.4.1: Validación de rango completa
    if(channel >= 0 && channel < NUM_TEMPS) return lastTemp[channel];
    return 0.0f;
}

bool Sensors::isTemperatureSensorOk(int channel) {
    // 🔒 v2.4.1: Validación de rango completa
    if(channel >= 0 && channel < NUM_TEMPS) return sensorOk[channel];
    return false;
}

bool Sensors::temperatureInitOK() {
    return initialized;
}

// 🔒 MEJORA OPCIONAL: Función de diagnóstico avanzado
Sensors::TemperatureStatus Sensors::getTemperatureStatus() {
    TemperatureStatus status;
    status.sensorsDetected = sensors.getDeviceCount();
    status.sensorsWorking = 0;
    status.criticalTempDetected = false;
    status.maxTemp = -999.0f;
    status.lastUpdateMs = lastUpdateMs;
    
    for(int i = 0; i < NUM_TEMPS; i++) {
        if(sensorOk[i]) {
            status.sensorsWorking++;
            
            // Actualizar temperatura máxima
            if(lastTemp[i] > status.maxTemp) {
                status.maxTemp = lastTemp[i];
            }
            
            // Detectar temperatura crítica (>85°C para motores)
            if(lastTemp[i] > TEMP_CRITICAL_CELSIUS) {
                status.criticalTempDetected = true;
            }
        }
    }
    
    return status;
}