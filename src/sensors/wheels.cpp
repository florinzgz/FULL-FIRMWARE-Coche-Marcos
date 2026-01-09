#include "wheels.h"
#include <Arduino.h>
#include <cmath>     // 🔒 For std::isfinite validation
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
            // 🔒 v2.4.1: Lectura atómica de pulses para evitar race conditions
            noInterrupts();
            unsigned long currentPulses = pulses[i];
            pulses[i] = 0;
            interrupts();
            
            float revs = (float)currentPulses / PULSES_PER_REV;

            // 🔒 SECURITY FIX: Prevent distance overflow (unsigned long max ~4.3 billion mm = 4300 km)
            // Check if adding new distance would overflow
            unsigned long newDistanceMm = (unsigned long)(revs * WHEEL_CIRCUM_MM);
            if (distance[i] > (ULONG_MAX - newDistanceMm)) {
                // Would overflow, reset distance counter with warning
                Logger::warnf("Wheel %d distance counter overflow, resetting (was %lu mm)", i, distance[i]);
                distance[i] = newDistanceMm;
            } else {
                distance[i] += newDistanceMm;
            }

            if(currentPulses > 0) {
                float mm_per_s = (revs * WHEEL_CIRCUM_MM) / (dt / 1000.0f);
                float kmh = (mm_per_s / 1000.0f) * 3.6f;
                
                // 🔒 SECURITY FIX: Validate calculated speed before using
                if (!std::isfinite(kmh) || kmh < 0.0f) {
                    Logger::warnf("Wheel %d: invalid speed calculation %.2f, setting to 0", i, kmh);
                    kmh = 0.0f;
                }
                
                // 🔒 Clamp a velocidad máxima definida en constants.h
                if(kmh > WHEEL_MAX_SPEED_KMH) kmh = WHEEL_MAX_SPEED_KMH;
                speed[i] = kmh;
                wheelOk[i] = true;
            } else {
                // Sin pulsos en este intervalo → velocidad 0, pero sensor vivo
                speed[i] = 0.0f;
                wheelOk[i] = true;
            }

            lastUpdate[i] = now;
        }
    }
}

float Sensors::getWheelSpeed(int wheel) {
    // 🔒 v2.4.1: Validación de rango completa
    if(wheel >= 0 && wheel < NUM_WHEELS) return speed[wheel];
    return 0.0f;
}

unsigned long Sensors::getWheelDistance(int wheel) {
    // 🔒 v2.4.1: Validación de rango completa
    if(wheel >= 0 && wheel < NUM_WHEELS) return distance[wheel];
    return 0;
}

bool Sensors::isWheelSensorOk(int wheel) {
    // 🔒 v2.4.1: Validación de rango completa
    if(wheel >= 0 && wheel < NUM_WHEELS) return wheelOk[wheel];
    return false;
}

// 🔎 Nuevo: función de estado de inicialización global
bool Sensors::wheelsInitOK() {
    return initialized;
}