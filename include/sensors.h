#pragma once
#include "current.h"
#include "wheels.h"
#include "temperature.h"
#include <stdint.h>
#include <stddef.h>  // Para size_t

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
    // Diagnóstico unificado de sensores
    // ========================================================================
    
    // Constantes para diagnóstico de sensores
    constexpr float INVALID_TEMPERATURE = -999.0f;  // Valor de temperatura inválida/no inicializada
    constexpr uint8_t MIN_WHEEL_SENSORS_CRITICAL = 2;  // Mínimo sensores de rueda para operación segura
    constexpr size_t SENSOR_DIAG_BUFFER_MIN = 32;  // Tamaño mínimo buffer para diagnóstico
    
    // Estado consolidado de todos los sensores
    struct SystemStatus {
        // Contadores de sensores funcionando
        uint8_t currentSensorsOK;       // INA226 sensores OK (0-6)
        uint8_t temperatureSensorsOK;   // DS18B20 sensores OK (0-5)
        uint8_t wheelSensorsOK;         // Sensores de rueda OK (0-4)
        
        // Totales configurados
        uint8_t currentSensorsTotal;    // Total configurados
        uint8_t temperatureSensorsTotal;
        uint8_t wheelSensorsTotal;
        
        // Estado general
        bool allSensorsHealthy;         // Todos los sensores habilitados están OK
        bool criticalSensorsOK;         // Sensores críticos funcionan
        
        // Banderas específicas
        bool batteryMonitorOK;          // INA226 canal batería OK
        bool temperatureWarning;        // Temperatura crítica detectada
        float maxTemperature;           // Temperatura máxima actual
        
        // Timestamp última actualización
        uint32_t lastUpdateMs;
        
        SystemStatus() : currentSensorsOK(0), temperatureSensorsOK(0), wheelSensorsOK(0),
                        currentSensorsTotal(NUM_CURRENTS), temperatureSensorsTotal(NUM_TEMPS), 
                        wheelSensorsTotal(NUM_WHEELS),
                        allSensorsHealthy(false), criticalSensorsOK(false),
                        batteryMonitorOK(false), temperatureWarning(false),
                        maxTemperature(INVALID_TEMPERATURE), lastUpdateMs(0) {}
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
    bool getSensorDiagnosticText(uint8_t sensorType, uint8_t sensorIdx, char* buffer, size_t bufSize);
}