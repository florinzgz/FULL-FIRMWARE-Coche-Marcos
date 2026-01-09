#pragma once
#include "current.h"
#include "temperature.h"
#include "wheels.h"
#include <Arduino.h> // Para millis()
#include <cmath>     // Para isfinite
#include <stddef.h>  // Para size_t
#include <stdint.h>

// API global de sensores
namespace Sensors {
// Inicializa todos los sensores
void init();

// Actualiza lecturas de todos los sensores
void update();

// Devuelve true si la inicialización fue correcta
bool initOK();

// Devuelve velocidad de rueda en km/h (idx = 0..3)
float getWheelSpeed(int idx);

// Devuelve voltaje de batería (V)
float getVoltage(int idx);

// ========================================================================
// 🆕 v2.8.0: Sensor Redundante para Seguridad Crítica
// ========================================================================

/**
 * @brief Estructura para manejo de sensores redundantes
 * Proporciona tolerancia a fallos para sensores críticos del vehículo
 * mediante lecturas primaria/secundaria con validación cruzada.
 */
struct RedundantSensor {
  float primaryValue;    // Valor del sensor primario
  float secondaryValue;  // Valor del sensor secundario (backup)
  bool primaryValid;     // Sensor primario válido
  bool secondaryValid;   // Sensor secundario válido
  float maxDeviation;    // Máxima desviación permitida entre sensores
  uint32_t lastUpdateMs; // Timestamp última actualización

  RedundantSensor()
      : primaryValue(0.0f), secondaryValue(0.0f), primaryValid(false),
        secondaryValid(false), maxDeviation(5.0f), lastUpdateMs(0) {}

  /**
   * @brief Obtiene un valor seguro del sensor
   * @return Promedio si ambos válidos y concordantes,
   *         valor del sensor válido si solo uno funciona,
   *         0.0f si ninguno válido
   */
  float getSafeValue() const {
    // Ninguno válido - retornar valor seguro
    if (!primaryValid && !secondaryValid) { return 0.0f; }

    // Solo uno válido - usar ese
    if (!primaryValid) return secondaryValid ? secondaryValue : 0.0f;
    if (!secondaryValid) return primaryValue;

    // Ambos válidos - verificar concordancia
    float deviation = std::fabs(primaryValue - secondaryValue);
    if (deviation <= maxDeviation) {
      // Valores concordantes - retornar promedio
      return (primaryValue + secondaryValue) / 2.0f;
    }

    // Valores discordantes - preferir primario pero marcar warning
    // En implementación real, esto debería loggear un error
    return primaryValue;
  }

  /**
   * @brief Verifica si hay discrepancia entre sensores
   */
  bool hasDiscrepancy() const {
    if (!primaryValid || !secondaryValid) return false;
    return std::fabs(primaryValue - secondaryValue) > maxDeviation;
  }

  /**
   * @brief Verifica si al menos un sensor funciona
   */
  bool isOperational() const { return primaryValid || secondaryValid; }

  /**
   * @brief Actualiza valores del sensor redundante
   */
  void update(float primary, bool primaryOk, float secondary,
              bool secondaryOk) {
    primaryValue = primary;
    primaryValid = primaryOk && std::isfinite(primary);
    secondaryValue = secondary;
    secondaryValid = secondaryOk && std::isfinite(secondary);
    lastUpdateMs = millis();
  }
};

// ========================================================================
// Diagnóstico unificado de sensores
// ========================================================================

// Constantes para diagnóstico de sensores
constexpr float INVALID_TEMPERATURE =
    -999.0f; // Valor de temperatura inválida/no inicializada
constexpr uint8_t MIN_WHEEL_SENSORS_CRITICAL =
    2; // Mínimo sensores de rueda para operación segura
constexpr size_t SENSOR_DIAG_BUFFER_MIN =
    32; // Tamaño mínimo buffer para diagnóstico
constexpr int BATTERY_CHANNEL_IDX = 4; // Índice del canal de batería (INA226)

// Estado consolidado de todos los sensores
struct SystemStatus {
  // Contadores de sensores funcionando
  uint8_t currentSensorsOK;     // INA226 sensores OK (0-6)
  uint8_t temperatureSensorsOK; // DS18B20 sensores OK (0-5)
  uint8_t wheelSensorsOK;       // Sensores de rueda OK (0-4)

  // Totales configurados
  uint8_t currentSensorsTotal; // Total configurados
  uint8_t temperatureSensorsTotal;
  uint8_t wheelSensorsTotal;

  // Estado general
  bool allSensorsHealthy; // Todos los sensores habilitados están OK
  bool criticalSensorsOK; // Sensores críticos funcionan

  // Banderas específicas
  bool batteryMonitorOK;   // INA226 canal batería OK
  bool temperatureWarning; // Temperatura crítica detectada
  float maxTemperature;    // Temperatura máxima actual

  // Timestamp última actualización
  uint32_t lastUpdateMs;

  SystemStatus()
      : currentSensorsOK(0), temperatureSensorsOK(0), wheelSensorsOK(0),
        currentSensorsTotal(NUM_CURRENTS), temperatureSensorsTotal(NUM_TEMPS),
        wheelSensorsTotal(NUM_WHEELS), allSensorsHealthy(false),
        criticalSensorsOK(false), batteryMonitorOK(false),
        temperatureWarning(false), maxTemperature(INVALID_TEMPERATURE),
        lastUpdateMs(0) {}
};

/**
 * @brief Obtiene estado consolidado de todos los sensores
 * @return SystemStatus con información de salud de todos los sensores
 */
SystemStatus getSystemStatus();

/**
 * @brief Obtiene texto de diagnóstico para un sensor específico
 * @param sensorType Tipo: 0=corriente, 1=temperatura, 2=rueda
 * @param sensorIdx Índice del sensor
 * @param buffer Buffer para el texto
 * @param bufSize Tamaño del buffer
 * @return true si el sensor está OK
 */
bool getSensorDiagnosticText(uint8_t sensorType, uint8_t sensorIdx,
                             char *buffer, size_t bufSize);

// ========================================================================
// Diagnóstico de dispositivos de entrada
// ========================================================================

// Estado de dispositivos de entrada (pedal, shifter, steering, buttons)
struct InputDeviceStatus {
  // Pedal
  bool pedalOK;       // Pedal inicializado
  bool pedalValid;    // Lectura válida
  float pedalPercent; // Porcentaje actual
  int pedalRaw;       // Valor ADC crudo

  // Shifter (palanca de cambios)
  bool shifterOK;      // Shifter inicializado
  uint8_t shifterGear; // Marcha actual (0=P, 1=D2, 2=D1, 3=N, 4=R)

  // Steering (encoder de dirección)
  bool steeringOK;       // Encoder inicializado
  bool steeringCentered; // Centrado completado
  bool steeringValid;    // Lectura válida
  float steeringAngle;   // Ángulo actual
  long steeringTicks;    // Ticks del encoder

  // Buttons
  bool buttonsOK;        // Botones inicializados
  bool lightsActive;     // Luces activadas
  bool multimediaActive; // Multimedia activado
  bool mode4x4Active;    // Modo 4x4 activado

  // Estado general
  bool allInputsOK; // Todos los inputs funcionando

  InputDeviceStatus()
      : pedalOK(false), pedalValid(false), pedalPercent(0.0f), pedalRaw(0),
        shifterOK(false), shifterGear(0), steeringOK(false),
        steeringCentered(false), steeringValid(false), steeringAngle(0.0f),
        steeringTicks(0), buttonsOK(false), lightsActive(false),
        multimediaActive(false), mode4x4Active(false), allInputsOK(false) {}
};

/**
 * @brief Obtiene estado de todos los dispositivos de entrada
 * @return InputDeviceStatus con información de pedal, shifter, steering,
 * buttons
 */
InputDeviceStatus getInputDeviceStatus();
} // namespace Sensors