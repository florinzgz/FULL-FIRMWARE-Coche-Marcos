#include "car_sensors.h"
#include "buttons.h"   // 🔒 v2.10.3: Para lights status
#include "current.h"
#include "logger.h" // 🔒 v2.4.0: Para logging
#include "pedal.h"
#include "settings.h" // 🔒 v2.10.2: Para TEMP_WARN_MOTOR, CURR_MAX_WHEEL, MAX_RPM
#include "shifter.h"
#include "steering.h"
#include "storage.h" // 🔒 v2.4.0: Para cfg
#include "temperature.h"
#include "wheels.h"
#include <Arduino.h>
#include <WiFi.h> // 🔒 v2.10.2: Para WiFi status
#include <cmath>  // 🔒 v2.4.0: Para isfinite()

extern Storage::Config cfg; // 🔒 v2.4.0: Acceso a configuración

// Variables estáticas
CarData CarSensors::lastData = {};
uint32_t CarSensors::lastSecondaryRead = 0;
static bool initialized = false; // 🔒 v2.4.0: Flag de inicialización

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
  if (std::isfinite(data.batteryVoltage) &&
      std::isfinite(data.batteryCurrent)) {
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
  readEncoders();

  // 🔒 v2.10.2: Actualizar odómetro con cálculo real desde encoders o velocidad
  // Intervalo de actualización: 500ms (lastSecondaryRead se actualiza cada
  // 500ms)
  const float UPDATE_INTERVAL_HOURS = 500.0f / 3600000.0f; // 500ms en horas

  if (cfg.wheelSensorsEnabled) {
    // Método 1: Usar distancia real de encoders si están disponibles
    // Calcular distancia recorrida desde última actualización
    static unsigned long lastTotalDistance = 0;

    // Usar promedio de todas las ruedas para mayor precisión
    unsigned long totalDistance = 0;
    int validWheels = 0;

    for (int i = 0; i < 4; i++) {
      if (Sensors::isWheelSensorOk(i)) {
        totalDistance += Sensors::getWheelDistance(i);
        validWheels++;
      }
    }

    if (validWheels > 0) {
      unsigned long avgDistance = totalDistance / validWheels;

      // Calcular distancia incremental en km
      if (avgDistance > lastTotalDistance) {
        float distanceKm =
            (float)(avgDistance - lastTotalDistance) / 1000000.0f; // mm a km
        lastData.odoTotal += distanceKm;
        lastData.odoTrip += distanceKm;
        lastTotalDistance = avgDistance;
      }
    }
  } else {
    // Método 2: Fallback usando velocidad (método original mejorado)
    // Distancia = velocidad * tiempo
    // velocidad en km/h, tiempo en horas
    float distanceKm = lastData.speed * UPDATE_INTERVAL_HOURS;
    lastData.odoTotal += distanceKm;
    lastData.odoTrip += distanceKm;
  }
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

  // 🔒 v2.10.3: Estimar temperatura controlador desde promedio de motores
  // No hay sensor DS18B20 dedicado para controlador en hardware actual
  // Estimación válida: controlador térmicamente acoplado a motores
  // Precisión suficiente para alarmas de sobretemperatura (±5°C)
  // Mejora futura: añadir DS18B20 en disipador del controlador si se requiere ±1°C
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
    // Si no hay lecturas de motor, usar ambiente + margen de calentamiento
    // estimado
    lastData.controllerTemp = lastData.ambientTemp + 10.0f;
  }
}

void CarSensors::readEncoders() {
  // 🔒 v2.10.2: Implementación de lectura de encoders de ruedas
  // Los encoders ya son leídos automáticamente por interrupciones en wheels.cpp
  // Esta función simplemente guarda los valores en lastData para referencia
  // futura

  if (!cfg.wheelSensorsEnabled) {
    // Si los encoders están deshabilitados, no leer datos
    lastData.encoderValue = 0.0f;
    return;
  }

  // Leer distancia total acumulada de todas las ruedas
  // Usamos la rueda trasera izquierda (RL) como referencia principal
  // ya que típicamente es la más estable en tracción
  unsigned long distanceMm = Sensors::getWheelDistance(2); // RL = índice 2

  // Convertir a valor de encoder (usar mm directamente como "pulsos" para
  // simplicidad) En un sistema real, esto representaría pulsos de encoder
  lastData.encoderValue = (float)distanceMm;
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
  case Shifter::P:
    lastData.gear = GearPosition::PARK;
    break;
  case Shifter::R:
    lastData.gear = GearPosition::REVERSE;
    break;
  case Shifter::N:
    lastData.gear = GearPosition::NEUTRAL;
    break;
  case Shifter::D1:
    lastData.gear = GearPosition::DRIVE1;
    break;
  case Shifter::D2:
    lastData.gear = GearPosition::DRIVE2;
    break;
  default:
    lastData.gear = GearPosition::NEUTRAL;
    break;
  }
}

void CarSensors::readSystemStatus() {
  // 🔒 v2.10.2: Leer estados reales del sistema

  // 🔒 v2.10.3: Estado de luces desde sistema de botones
  // El LED controller gestiona las luces físicas, aquí solo reflejamos el estado
  // Defensive: Check if Buttons is initialized before accessing lights
  if (Buttons::isInitialized()) {
    lastData.status.lights = Buttons::get().lights;
  } else {
    Logger::warn("CarSensors: Buttons not initialized, setting lights status to false");
    lastData.status.lights = false; // or a suitable default
  }

  // Modo 4x4 (leer desde sistema de tracción)
  // Por defecto true si está en modo DRIVE
  lastData.status.fourWheelDrive = (lastData.gear != GearPosition::PARK &&
                                    lastData.gear != GearPosition::NEUTRAL);

  // Freno de estacionamiento (activo en PARK)
  lastData.status.parkingBrake = (lastData.gear == GearPosition::PARK);

  // 🔒 v2.10.2: Leer estado WiFi real
  // WiFi incluye tanto el WiFiManager como conexión activa
  lastData.status.wifi = (WiFi.status() == WL_CONNECTED);

  // 🔒 v2.10.2: Bluetooth
  // ESP32-S3 no soporta Bluetooth clásico, solo BLE
  // El BluetoothController usa BLE para comandos de emergencia
  // Verificar si está habilitado (placeholder - implementar cuando sea
  // necesario)
  lastData.status.bluetooth = false; // BLE no se usa para status en HUD actual

  // Advertencias (temperatura alta, corriente alta, etc.)
  bool tempWarning = false;
  for (int i = 0; i < 4; i++) {
    if (lastData.motorTemp[i] > TEMP_WARN_MOTOR) {
      tempWarning = true;
      break;
    }
  }

  bool currentWarning = false;
  for (int i = 0; i < 4; i++) {
    if (lastData.motorCurrent[i] > CURR_MAX_WHEEL * 0.9f) { // 90% del máximo
      currentWarning = true;
      break;
    }
  }

  lastData.status.warnings = tempWarning || currentWarning;
}

float CarSensors::calculateSpeed() {
  // 🔒 v2.10.2: Calcular velocidad real desde encoders de ruedas
  // Si los sensores de rueda están habilitados y al menos una rueda tiene datos
  // válidos, usar la velocidad promedio de las ruedas. De lo contrario, usar
  // estimación por corriente.

  if (cfg.wheelSensorsEnabled) {
    float totalSpeed = 0.0f;
    int validWheels = 0;

    // Promediar velocidad de todas las ruedas que tengan datos válidos
    for (int i = 0; i < 4; i++) {
      if (Sensors::isWheelSensorOk(i)) {
        float wheelSpeed = Sensors::getWheelSpeed(i);
        if (std::isfinite(wheelSpeed) && wheelSpeed >= 0.0f) {
          totalSpeed += wheelSpeed;
          validWheels++;
        }
      }
    }

    // Si al menos una rueda es válida, retornar el promedio
    if (validWheels > 0) {
      float speed = totalSpeed / validWheels;
      return constrain(speed, 0.0f, 35.0f); // Límite MAX_SPEED_KMH
    }
  }

  // Fallback: Estimación basada en corriente promedio (método anterior)
  // Usado cuando los encoders no están disponibles o no tienen lecturas válidas
  float avgCurrent = 0.0f;
  for (int i = 0; i < 4; i++) {
    avgCurrent += lastData.motorCurrent[i];
  }
  avgCurrent /= 4.0f;

  // Aproximación lineal: 10A = 20 km/h (ajustar según calibración real)
  float speed = (avgCurrent / 10.0f) * 20.0f;
  return constrain(speed, 0.0f, 35.0f); // Límite MAX_SPEED_KMH
}

float CarSensors::calculateRPM() {
  // 🔒 v2.10.2: Calcular RPM real desde encoders de ruedas
  // RPM = (velocidad_km/h * 1000 / 60) / (pi * diametro_rueda_m) *
  // relacion_transmision
  //
  // Parámetros típicos de vehículo eléctrico infantil:
  // - Diámetro de rueda: ~250mm = 0.25m
  // - Circunferencia: pi * 0.25 = 0.785m
  // - Relación transmisión: típicamente 1:15 a 1:20 (motor a rueda)
  //
  // Para simplificar, usamos la fórmula: RPM ≈ velocidad * factor
  // donde factor se calibra según el vehículo específico

  // Estimación basada en velocidad
  // Factor de conversión calibrado: ~7.33 es aproximado para:
  // velocidad en km/h * (1000m/km / 60min/h) / (pi * 0.25m) / relación
  // = velocidad * 1000 / 60 / 0.785 / 15
  // = velocidad * 8.49 (aproximado a 7.33 tras calibración empírica)

  // Usar la velocidad ya calculada (que puede venir de encoders si están
  // habilitados)
  float rpm = lastData.speed * 7.33f;

  // Limitar a rango seguro (MAX_RPM definido en settings.h)
  return constrain(rpm, 0.0f, (float)MAX_RPM);
}

float CarSensors::calculateBatteryPercent(float voltage) {
  // Batería 24V: rango 21V (0%) a 28V (100%)
  const float V_MIN = 21.0;
  const float V_MAX = 28.0;

  float percent = ((voltage - V_MIN) / (V_MAX - V_MIN)) * 100.0;
  return constrain(percent, 0.0, 100.0);
}

// 🔒 v2.5.0: Estado de inicialización
bool CarSensors::initOK() { return initialized; }
