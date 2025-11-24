#include "wheels.h"
#include <Arduino.h>
#include "pins.h"
#include "settings.h"
#include "constants.h"  // 🔒 Usar constantes centralizadas
#include "logger.h"
#include "storage.h"
#include "system.h"

extern Storage::Config cfg;

static volatile unsigned long pulses[Sensors::NUM_WHEELS];
static unsigned long lastUpdate[Sensors::NUM_WHEELS];
static float speed[Sensors::NUM_WHEELS];
static unsigned long distance[Sensors::NUM_WHEELS];
static bool wheelOk[Sensors::NUM_WHEELS];

// 🔎 Nuevo: flag de inicialización global
static bool initialized = false;

// 🔒 CORRECCIÓN 4.10: Debounce para ISRs de ruedas
// Sensores inductivos pueden generar rebotes a alta velocidad
static volatile unsigned long lastPulseTime[Sensors::NUM_WHEELS] = {0};
static constexpr unsigned long DEBOUNCE_US = 500;  // 500µs debounce (máx ~2000 Hz)

void IRAM_ATTR wheelISR0() {
    unsigned long now = micros();
    if (now - lastPulseTime[0] > DEBOUNCE_US) {
        pulses[0]++;
        lastPulseTime[0] = now;
    }
}

void IRAM_ATTR wheelISR1() {
    unsigned long now = micros();
    if (now - lastPulseTime[1] > DEBOUNCE_US) {
        pulses[1]++;
        lastPulseTime[1] = now;
    }
}

void IRAM_ATTR wheelISR2() {
    unsigned long now = micros();
    if (now - lastPulseTime[2] > DEBOUNCE_US) {
        pulses[2]++;
        lastPulseTime[2] = now;
    }
}

void IRAM_ATTR wheelISR3() {
    unsigned long now = micros();
    if (now - lastPulseTime[3] > DEBOUNCE_US) {
        pulses[3]++;
        lastPulseTime[3] = now;
    }
}

void Sensors::initWheels() {
    for(int i=0; i<NUM_WHEELS; i++) {
        pulses[i] = 0;
        distance[i] = 0;
        speed[i] = 0;
        lastUpdate[i] = millis();
        wheelOk[i] = true;
    }

    // 🔒 Configurar interrupciones para sensores en GPIOs directos
    attachInterrupt(digitalPinToInterrupt(PIN_WHEEL_FL), wheelISR0, RISING);
    // 🔒 NOTA: PIN_WHEEL_FR (GPIO 36) ahora se lee directamente, no vía MCP23017
    // El comentario anterior sobre MCP23017 GPIOB0 era incorrecto según pins.h actualizado
    attachInterrupt(digitalPinToInterrupt(PIN_WHEEL_FR), wheelISR1, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_WHEEL_RL), wheelISR2, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_WHEEL_RR), wheelISR3, RISING);

    Logger::info("Wheel sensors init OK - 4 interrupts configured");
    initialized = true;
}

void Sensors::updateWheels() {
    if(!cfg.wheelSensorsEnabled) {
        // Si está desactivado → todo a neutro
        for(int i=0; i<NUM_WHEELS; i++) {
            speed[i] = 0.0f;
            wheelOk[i] = false;
        }
        return;
    }

    unsigned long now = millis();
    for(int i=0; i<NUM_WHEELS; i++) {
        unsigned long dt = now - lastUpdate[i];

        // 🔎 Timeout → sensor no responde
        if(dt > SENSOR_TIMEOUT_MS) {
            speed[i] = 0.0f;
            wheelOk[i] = false;
            System::logError(500 + i); // ej. 500=FL, 501=FR, 502=RL, 503=RR
            continue;
        }

        if(dt >= 100) { // cada 100ms
            float revs = (float)pulses[i] / PULSES_PER_REV;

            // Distancia acumulada en mm
            distance[i] += (unsigned long)(revs * WHEEL_CIRCUM_MM);

            if(pulses[i] > 0) {
                float mm_per_s = (revs * WHEEL_CIRCUM_MM) / (dt / 1000.0f);
                float kmh = (mm_per_s / 1000.0f) * 3.6f;
                // 🔒 Clamp a velocidad máxima definida en constants.h
                if(kmh > WHEEL_MAX_SPEED_KMH) kmh = WHEEL_MAX_SPEED_KMH;
                speed[i] = kmh;
                wheelOk[i] = true;
            } else {
                // Sin pulsos en este intervalo → velocidad 0, pero sensor vivo
                speed[i] = 0.0f;
                wheelOk[i] = true;
            }

            pulses[i] = 0;
            lastUpdate[i] = now;
        }
    }
}

float Sensors::getWheelSpeed(int wheel) {
    if(wheel < NUM_WHEELS) return speed[wheel];
    return 0.0f;
}

unsigned long Sensors::getWheelDistance(int wheel) {
    if(wheel < NUM_WHEELS) return distance[wheel];
    return 0;
}

bool Sensors::isWheelSensorOk(int wheel) {
    if(wheel < NUM_WHEELS) return wheelOk[wheel];
    return false;
}

// 🔎 Nuevo: función de estado de inicialización global
bool Sensors::wheelsInitOK() {
    return initialized;
}