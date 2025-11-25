#include "car_sensors.h"
#include "current.h"
#include "temperature.h"
#include "pedal.h"
#include "steering.h"
#include "shifter.h"
#include "wheels.h"
#include "storage.h"        // 🔒 v2.4.0: Para cfg
#include "logger.h"         // 🔒 v2.4.0: Para logging
#include <Arduino.h>
#include <cmath>            // 🔒 v2.4.0: Para isfinite()

extern Storage::Config cfg; // 🔒 v2.4.0: Acceso a configuración

// Variables estáticas
CarData CarSensors::lastData = {};
uint32_t CarSensors::lastSecondaryRead = 0;
static bool initialized = false;  // 🔒 v2.4.0: Flag de inicialización

void CarSensors::init() {
    Logger::info("CarSensors: Inicializando sensores...");
    
    // Inicializar sensores de corriente (INA226)
    Sensors::initCurrent();
    
    // Inicializar sensores de temperatura
    Sensors::initTemperature();
    
    // Inicializar otros sensores
    Pedal::init();
    Steering::init();
    Sensors::initWheels();
    
    // Inicializar datos
    memset(&lastData, 0, sizeof(CarData));
    lastData.gear = GearPosition::PARK;
    lastSecondaryRead = 0;
    
    initialized = true;
    Logger::info("CarSensors: Inicialización completada");
}

CarData CarSensors::readAll() {
    // 🔒 v2.4.0: Guard de inicialización
    if (!initialized) {
        Logger::warn("CarSensors::readAll() llamado sin init");
        return lastData;
    }
    
    CarData data = lastData;
    
    // Leer sensores críticos (alta frecuencia)
    readINA226Sensors();
    readPedal();
    readSteering();
    readGear();
    
    // Calcular velocidad y RPM
    data.speed = calculateSpeed();
    data.rpm = calculateRPM();
    
    // Leer sensores secundarios cada 500ms
    uint32_t now = millis();
    if (now - lastSecondaryRead >= 500) {
        readSecondary();
        lastSecondaryRead = now;
    }
    
    // Copiar datos con validación
    data.batteryVoltage = lastData.batteryVoltage;
    data.batteryPercent = calculateBatteryPercent(data.batteryVoltage);
    data.batteryCurrent = lastData.batteryCurrent;
    
    // 🔒 v2.4.0: Validar cálculo de potencia
    if (std::isfinite(data.batteryVoltage) && std::isfinite(data.batteryCurrent)) {
        data.batteryPower = data.batteryVoltage * data.batteryCurrent;
    } else {
        data.batteryPower = 0.0f;
    }
    
    for (int i = 0; i < 4; i++) {
        data.motorCurrent[i] = lastData.motorCurrent[i];
        data.motorTemp[i] = lastData.motorTemp[i];
    }
    
    data.steeringCurrent = lastData.steeringCurrent;
    data.ambientTemp = lastData.ambientTemp;
    data.controllerTemp = lastData.controllerTemp;
    
    data.gear = lastData.gear;
    data.throttlePercent = lastData.throttlePercent;
    data.steeringAngle = lastData.steeringAngle;
    
    data.status = lastData.status;
    data.odoTotal = lastData.odoTotal;
    data.odoTrip = lastData.odoTrip;
    
    return data;
}

CarData CarSensors::readCritical() {
    CarData data = lastData;
    
    readINA226Sensors();
    data.speed = calculateSpeed();
    data.rpm = calculateRPM();
    data.batteryVoltage = lastData.batteryVoltage;
    data.batteryPercent = calculateBatteryPercent(data.batteryVoltage);
    data.batteryCurrent = lastData.batteryCurrent;
    data.batteryPower = data.batteryVoltage * data.batteryCurrent;
    
    for (int i = 0; i < 4; i++) {
        data.motorCurrent[i] = lastData.motorCurrent[i];
    }
    
    return data;
}

void CarSensors::readSecondary() {
    readTemperatureSensors();
    readSystemStatus();
    
    // Actualizar odómetro (simplificado)
    // TODO: Implementar cálculo real desde encoders
    lastData.odoTotal += lastData.speed * 0.0001389;  // km en 500ms
    lastData.odoTrip += lastData.speed * 0.0001389;
}

void CarSensors::readINA226Sensors() {
    // 🔒 v2.4.0: Verificar si sensores están habilitados
    if (!cfg.currentSensorsEnabled) {
        for (int i = 0; i < 4; i++) {
            lastData.motorCurrent[i] = 0.0f;
        }
        lastData.batteryCurrent = 0.0f;
        lastData.batteryVoltage = 0.0f;
        lastData.steeringCurrent = 0.0f;
        return;
    }
    
    // Leer 4 sensores INA226 de motores (canales 0-3)
    for (int i = 0; i < 4; i++) {
        float current = Sensors::getCurrent(i);
        // 🔒 v2.4.0: Validar lectura
        if (std::isfinite(current)) {
            lastData.motorCurrent[i] = current;
        } else {
            lastData.motorCurrent[i] = 0.0f;
        }
    }
    
    // Leer sensor INA226 batería (canal 4)
    float battCurrent = Sensors::getCurrent(4);
    float battVoltage = Sensors::getVoltage(4);
    
    // 🔒 v2.4.0: Validar lecturas batería
    lastData.batteryCurrent = std::isfinite(battCurrent) ? battCurrent : 0.0f;
    lastData.batteryVoltage = std::isfinite(battVoltage) ? battVoltage : 0.0f;
    
    // Leer sensor INA226 dirección (canal 5)
    float steerCurrent = Sensors::getCurrent(5);
    lastData.steeringCurrent = std::isfinite(steerCurrent) ? steerCurrent : 0.0f;
}

void CarSensors::readTemperatureSensors() {
    // 🔒 v2.4.0: Verificar si sensores están habilitados
    if (!cfg.tempSensorsEnabled) {
        for (int i = 0; i < 4; i++) {
            lastData.motorTemp[i] = 0.0f;
        }
        lastData.ambientTemp = 0.0f;
        lastData.controllerTemp = 0.0f;
        return;
    }
    
    // Leer 4 sensores de temperatura de motores
    for (int i = 0; i < 4; i++) {
        float temp = Sensors::getTemperature(i);
        // 🔒 v2.4.0: Validar lectura
        if (std::isfinite(temp) && temp > -50.0f && temp < 150.0f) {
            lastData.motorTemp[i] = temp;
        }
        // Si inválido, mantener último valor
    }
    
    // Leer temperatura ambiente (sensor 4)
    float ambTemp = Sensors::getTemperature(4);
    if (std::isfinite(ambTemp) && ambTemp > -50.0f && ambTemp < 80.0f) {
        lastData.ambientTemp = ambTemp;
    }
    
    // 🔒 v2.4.0: Estimar temperatura controlador como promedio de temperaturas de motores
    // NOTA: No hay sensor dedicado para controlador. Esta es una estimación
    // basada en que el controlador está cerca de los motores y se calienta proporcionalmente.
    // TODO: Añadir sensor DS18B20 dedicado para controlador si se requiere precisión.
    float motorAvg = 0.0f;
    int validCount = 0;
    for (int i = 0; i < 4; i++) {
        if (lastData.motorTemp[i] > 0.0f) {
            motorAvg += lastData.motorTemp[i];
            validCount++;
        }
    }
    if (validCount > 0) {
        lastData.controllerTemp = motorAvg / validCount;
    } else {
        // Si no hay lecturas de motor, usar ambiente + margen de calentamiento estimado
        lastData.controllerTemp = lastData.ambientTemp + 10.0f;
    }
}

void CarSensors::readEncoders() {
    // TODO: Implementar lectura de encoders de ruedas
    // Por ahora placeholder
}

void CarSensors::readPedal() {
    lastData.throttlePercent = Pedal::get().percent;
}

void CarSensors::readSteering() {
    lastData.steeringAngle = Steering::get().angleDeg;
}

void CarSensors::readGear() {
    Shifter::Gear shifterGear = Shifter::get().gear;
    
    // Mapear Shifter::Gear a GearPosition
    switch (shifterGear) {
        case Shifter::P: lastData.gear = GearPosition::PARK; break;
        case Shifter::R: lastData.gear = GearPosition::REVERSE; break;
        case Shifter::N: lastData.gear = GearPosition::NEUTRAL; break;
        case Shifter::D1: lastData.gear = GearPosition::DRIVE1; break;
        case Shifter::D2: lastData.gear = GearPosition::DRIVE2; break;
        default: lastData.gear = GearPosition::NEUTRAL; break;
    }
}

void CarSensors::readSystemStatus() {
    // TODO: Leer estados del sistema
    // Por ahora valores por defecto
    lastData.status.lights = false;
    lastData.status.fourWheelDrive = true;
    lastData.status.parkingBrake = (lastData.gear == GearPosition::PARK);
    lastData.status.bluetooth = false;  // BT clásico deshabilitado en ESP32-S3
    lastData.status.wifi = false;       // TODO: Leer estado WiFi
    lastData.status.warnings = false;
}

float CarSensors::calculateSpeed() {
    // TODO: Calcular velocidad real desde encoders
    // Por ahora basado en corriente promedio (aproximación)
    float avgCurrent = 0;
    for (int i = 0; i < 4; i++) {
        avgCurrent += lastData.motorCurrent[i];
    }
    avgCurrent /= 4.0;
    
    // Aproximación lineal: 10A = 20 km/h (ajustar según calibración real)
    float speed = (avgCurrent / 10.0) * 20.0;
    return constrain(speed, 0.0, 30.0);  // Límite 30 km/h
}

float CarSensors::calculateRPM() {
    // Estimación basada en velocidad
    // Asumiendo relación transmisión y diámetro rueda
    // TODO: Calcular con datos reales
    return lastData.speed * 7.33;  // Aproximación
}

float CarSensors::calculateBatteryPercent(float voltage) {
    // Batería 24V: rango 21V (0%) a 28V (100%)
    const float V_MIN = 21.0;
    const float V_MAX = 28.0;
    
    float percent = ((voltage - V_MIN) / (V_MAX - V_MIN)) * 100.0;
    return constrain(percent, 0.0, 100.0);
}

// 🔒 v2.5.0: Estado de inicialización
bool CarSensors::initOK() {
    return initialized;
}
