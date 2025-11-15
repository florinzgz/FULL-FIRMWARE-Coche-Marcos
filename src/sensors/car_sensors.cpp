#include "car_sensors.h"
#include "current.h"
#include "temperature.h"
#include "pedal.h"
#include "steering.h"
#include "shifter.h"
#include "wheels.h"
#include <Arduino.h>

// Variables estáticas
CarData CarSensors::lastData = {};
uint32_t CarSensors::lastSecondaryRead = 0;

void CarSensors::init() {
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
}

CarData CarSensors::readAll() {
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
    
    // Copiar datos
    data.batteryVoltage = lastData.batteryVoltage;
    data.batteryPercent = calculateBatteryPercent(data.batteryVoltage);
    data.batteryCurrent = lastData.batteryCurrent;
    data.batteryPower = data.batteryVoltage * data.batteryCurrent;
    
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
    // Leer 4 sensores INA226 de motores (canales 0-3)
    for (int i = 0; i < 4; i++) {
        lastData.motorCurrent[i] = Sensors::getCurrent(i);
    }
    
    // Leer sensor INA226 batería (canal 4)
    lastData.batteryCurrent = Sensors::getCurrent(4);
    lastData.batteryVoltage = Sensors::getVoltage(4);
    
    // Leer sensor INA226 dirección (canal 5)
    lastData.steeringCurrent = Sensors::getCurrent(5);
}

void CarSensors::readTemperatureSensors() {
    // Leer 4 sensores de temperatura de motores
    for (int i = 0; i < 4; i++) {
        lastData.motorTemp[i] = Sensors::getTemperature(i);
    }
    
    // Leer temperatura ambiente (sensor 4)
    lastData.ambientTemp = Sensors::getTemperature(4);
    
    // Leer temperatura controlador (sensor 5)
    // Nota: Solo hay 5 sensores (0-4), usando 4 para controlador si disponible
    lastData.controllerTemp = Sensors::getTemperature(4);
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
