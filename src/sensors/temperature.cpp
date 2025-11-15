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

static uint32_t lastUpdateMs = 0;

// Flag de inicialización global
static bool initialized = false;

void Sensors::initTemperature() {
    sensors.begin();

    int count = sensors.getDeviceCount();
    if(count < NUM_TEMPS) {
        Logger::warnf("DS18B20: detectados %d de %d esperados", count, NUM_TEMPS);
    }

    for(int i=0; i<NUM_TEMPS; i++) {
        if(i < count) {
            sensorOk[i] = true;
            Logger::infof("DS18B20 init OK idx %d", i);
        } else {
            sensorOk[i] = false;
            System::logError(400+i); // registrar fallo persistente
            Logger::errorf("DS18B20 init FAIL idx %d", i);
        }
        lastTemp[i] = 0.0f;
    }

    initialized = (count > 0);
}

void Sensors::updateTemperature() {
    uint32_t now = millis();
    if(now - lastUpdateMs < 1000) return; // ~1 Hz
    lastUpdateMs = now;

    if(!cfg.tempSensorsEnabled) {
        for(int i=0; i<NUM_TEMPS; i++) {
            lastTemp[i] = 0.0f;
            sensorOk[i] = false;
        }
        return;
    }

    sensors.requestTemperatures();

    for(int i=0; i<NUM_TEMPS; i++) {
        if(!sensorOk[i]) continue;

        float t = sensors.getTempCByIndex(i);

        // Validación y fallback
        if(t == DEVICE_DISCONNECTED_C || !isfinite(t)) {
            System::logError(400+i);
            Logger::errorf("DS18B20 idx %d: lectura inválida", i);
            // mantener último valor válido
            continue;
        }

        // Clamps
        t = constrain(t, -40.0f, 150.0f);

        // Filtro EMA para suavizar
        lastTemp[i] = lastTemp[i] + 0.2f * (t - lastTemp[i]);
    }
}

float Sensors::getTemperature(int channel) {
    if(channel < NUM_TEMPS) return lastTemp[channel];
    return 0.0f;
}

bool Sensors::isTemperatureSensorOk(int channel) {
    if(channel < NUM_TEMPS) return sensorOk[channel];
    return false;
}

bool Sensors::temperatureInitOK() {
    return initialized;
}