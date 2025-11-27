#include "sensors.h"
#include "logger.h"
#include "storage.h"
#include "pedal.h"
#include "shifter.h"
#include "steering.h"
#include "buttons.h"
#include <Arduino.h>
#include <cstdio>

extern Storage::Config cfg;

namespace Sensors {

    static bool ok = false;

    void init() {
        // Aquí inicializarías tus sensores reales
        ok = true;
        Logger::info("Sensors init OK");
    }

    void update() {
        // Aquí refrescarías lecturas reales
    }

    bool initOK() {
        return ok;
    }
    
    // ========================================================================
    // Diagnóstico unificado de sensores
    // ========================================================================
    
    SystemStatus getSystemStatus() {
        SystemStatus status;
        status.lastUpdateMs = millis();
        
        // Contar sensores de corriente INA226 OK
        status.currentSensorsOK = 0;
        if (cfg.currentSensorsEnabled) {
            for (int i = 0; i < NUM_CURRENTS; i++) {
                if (isCurrentSensorOk(i)) {
                    status.currentSensorsOK++;
                }
            }
            // Canal de batería
            status.batteryMonitorOK = isCurrentSensorOk(BATTERY_CHANNEL_IDX);
        }
        status.currentSensorsTotal = NUM_CURRENTS;
        
        // Contar sensores de temperatura DS18B20 OK
        status.temperatureSensorsOK = 0;
        status.maxTemperature = INVALID_TEMPERATURE;
        status.temperatureWarning = false;
        
        if (cfg.tempSensorsEnabled) {
            bool foundValidTemp = false;
            for (int i = 0; i < NUM_TEMPS; i++) {
                if (isTemperatureSensorOk(i)) {
                    status.temperatureSensorsOK++;
                    float temp = getTemperature(i);
                    if (!foundValidTemp) {
                        status.maxTemperature = temp;
                        foundValidTemp = true;
                    } else if (temp > status.maxTemperature) {
                        status.maxTemperature = temp;
                    }
                    if (temp > TEMP_CRITICAL_CELSIUS) {
                        status.temperatureWarning = true;
                    }
                }
            }
        }
        status.temperatureSensorsTotal = NUM_TEMPS;
        
        // Contar sensores de rueda OK
        status.wheelSensorsOK = 0;
        if (cfg.wheelSensorsEnabled) {
            for (int i = 0; i < NUM_WHEELS; i++) {
                if (isWheelSensorOk(i)) {
                    status.wheelSensorsOK++;
                }
            }
        }
        status.wheelSensorsTotal = NUM_WHEELS;
        
        // Determinar estado general
        // Sensores críticos: al menos batería + MIN_WHEEL_SENSORS_CRITICAL ruedas
        status.criticalSensorsOK = status.batteryMonitorOK && 
                                   (status.wheelSensorsOK >= MIN_WHEEL_SENSORS_CRITICAL);
        
        // Todos los sensores habilitados están OK
        bool currentOK = !cfg.currentSensorsEnabled || (status.currentSensorsOK == status.currentSensorsTotal);
        bool tempOK = !cfg.tempSensorsEnabled || (status.temperatureSensorsOK == status.temperatureSensorsTotal);
        bool wheelOK = !cfg.wheelSensorsEnabled || (status.wheelSensorsOK == status.wheelSensorsTotal);
        status.allSensorsHealthy = currentOK && tempOK && wheelOK;
        
        return status;
    }
    
    bool getSensorDiagnosticText(uint8_t sensorType, uint8_t sensorIdx, char* buffer, size_t bufSize) {
        if (buffer == nullptr || bufSize < SENSOR_DIAG_BUFFER_MIN) return false;
        
        switch (sensorType) {
            case 0: // Corriente (INA226)
                if (sensorIdx >= NUM_CURRENTS) {
                    snprintf(buffer, bufSize, "INA%d: INVALID", sensorIdx);
                    return false;
                }
                if (!cfg.currentSensorsEnabled) {
                    snprintf(buffer, bufSize, "INA%d: DISABLED", sensorIdx);
                    return false;
                }
                if (isCurrentSensorOk(sensorIdx)) {
                    float current = getCurrent(sensorIdx);
                    float voltage = getVoltage(sensorIdx);
                    snprintf(buffer, bufSize, "INA%d: %.1fA %.1fV OK", sensorIdx, current, voltage);
                    return true;
                } else {
                    snprintf(buffer, bufSize, "INA%d: FAIL", sensorIdx);
                    return false;
                }
                
            case 1: // Temperatura (DS18B20)
                if (sensorIdx >= NUM_TEMPS) {
                    snprintf(buffer, bufSize, "TEMP%d: INVALID", sensorIdx);
                    return false;
                }
                if (!cfg.tempSensorsEnabled) {
                    snprintf(buffer, bufSize, "TEMP%d: DISABLED", sensorIdx);
                    return false;
                }
                if (isTemperatureSensorOk(sensorIdx)) {
                    float temp = getTemperature(sensorIdx);
                    const char* status = temp > TEMP_CRITICAL_CELSIUS ? "CRIT" : "OK";
                    snprintf(buffer, bufSize, "TEMP%d: %.1fC %s", sensorIdx, temp, status);
                    return true;
                } else {
                    snprintf(buffer, bufSize, "TEMP%d: FAIL", sensorIdx);
                    return false;
                }
                
            case 2: // Rueda
                if (sensorIdx >= NUM_WHEELS) {
                    snprintf(buffer, bufSize, "WHL%d: INVALID", sensorIdx);
                    return false;
                }
                if (!cfg.wheelSensorsEnabled) {
                    snprintf(buffer, bufSize, "WHL%d: DISABLED", sensorIdx);
                    return false;
                }
                if (isWheelSensorOk(sensorIdx)) {
                    float speed = getWheelSpeed(sensorIdx);
                    snprintf(buffer, bufSize, "WHL%d: %.1fkm/h OK", sensorIdx, speed);
                    return true;
                } else {
                    snprintf(buffer, bufSize, "WHL%d: FAIL", sensorIdx);
                    return false;
                }
                
            default:
                snprintf(buffer, bufSize, "UNKNOWN SENSOR");
                return false;
        }
    }
    
    // ========================================================================
    // Diagnóstico de dispositivos de entrada
    // ========================================================================
    
    InputDeviceStatus getInputDeviceStatus() {
        InputDeviceStatus status;
        
        // Estado del pedal
        status.pedalOK = Pedal::initOK();
        auto pedalState = Pedal::get();
        status.pedalValid = pedalState.valid;
        status.pedalPercent = pedalState.percent;
        status.pedalRaw = pedalState.raw;
        
        // Estado del shifter (palanca de cambios)
        status.shifterOK = Shifter::initOK();
        auto shifterState = Shifter::get();
        status.shifterGear = static_cast<uint8_t>(shifterState.gear);
        
        // Estado del steering (encoder de dirección)
        status.steeringOK = Steering::initOK();
        auto steerState = Steering::get();
        status.steeringCentered = steerState.centered;
        status.steeringValid = steerState.valid;
        status.steeringAngle = steerState.angleDeg;
        status.steeringTicks = steerState.ticks;
        
        // Estado de los botones
        status.buttonsOK = Buttons::initOK();
        auto btnsState = Buttons::get();
        status.lightsActive = btnsState.lights;
        status.multimediaActive = btnsState.multimedia;
        status.mode4x4Active = btnsState.mode4x4;
        
        // Estado general - todos los inputs críticos deben estar OK
        status.allInputsOK = status.pedalOK && status.shifterOK && 
                            status.steeringOK && status.buttonsOK;
        
        return status;
    }
}