#include "wheels.h"
#include <Arduino.h>
#include "pins.h"
#include "settings.h"
#include "logger.h"
#include "storage.h"
#include "system.h"   // 🔎 para logError()

#define PULSES_PER_REV 6              // 6 tornillos por rueda
#define WHEEL_CIRCUM_MM 1100          // Circunferencia real en mm (110 cm)
#define SENSOR_TIMEOUT_MS 1000        // 🔎 si no hay pulsos en 1s → fallo

extern Storage::Config cfg;

static volatile unsigned long pulses[Sensors::NUM_WHEELS];
static unsigned long lastUpdate[Sensors::NUM_WHEELS];
static float speed[Sensors::NUM_WHEELS];
static unsigned long distance[Sensors::NUM_WHEELS];
static bool wheelOk[Sensors::NUM_WHEELS];

// 🔎 Nuevo: flag de inicialización global
static bool initialized = false;

void IRAM_ATTR wheelISR0() { pulses[0]++; }
void IRAM_ATTR wheelISR1() { pulses[1]++; }
void IRAM_ATTR wheelISR2() { pulses[2]++; }
void IRAM_ATTR wheelISR3() { pulses[3]++; }

void Sensors::initWheels() {
    for(int i=0; i<NUM_WHEELS; i++) {
        pulses[i] = 0;
        distance[i] = 0;
        speed[i] = 0;
        lastUpdate[i] = millis();
        wheelOk[i] = true;
    }

    attachInterrupt(digitalPinToInterrupt(PIN_WHEEL0), wheelISR0, RISING);
    // PIN_WHEEL1 ahora en MCP23017 GPIOB0 - se lee por polling en update()
    attachInterrupt(digitalPinToInterrupt(PIN_WHEEL2), wheelISR2, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_WHEEL3), wheelISR3, RISING);

    Logger::info("Wheel sensors init OK");
    initialized = true;   // marcar como inicializado
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
                // Clamp a velocidad máxima definida
                if(kmh > MAX_SPEED_KMH) kmh = MAX_SPEED_KMH;
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