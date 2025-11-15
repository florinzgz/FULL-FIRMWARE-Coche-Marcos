#ifndef CAR_SENSORS_H
#define CAR_SENSORS_H

#include "display_types.h"

/**
 * @brief Recolector centralizado de datos de sensores
 * 
 * Lee todos los sensores del coche y devuelve estructura CarData unificada.
 * Optimizado para llamada única por ciclo de loop.
 */
class CarSensors {
public:
    /**
     * @brief Inicializa todos los sensores
     */
    static void init();
    
    /**
     * @brief Lee todos los sensores y devuelve datos actualizados
     * @return CarData con lecturas actuales
     */
    static CarData readAll();
    
    /**
     * @brief Lee solo sensores críticos (alta frecuencia)
     * Velocidad, corriente, voltaje
     * @return CarData con lecturas críticas
     */
    static CarData readCritical();
    
    /**
     * @brief Lee sensores secundarios (baja frecuencia)
     * Temperatura, odómetro
     */
    static void readSecondary();
    
    /**
     * @brief Calcula velocidad desde encoders
     * @return Velocidad en km/h
     */
    static float calculateSpeed();
    
    /**
     * @brief Calcula RPM estimado
     * @return RPM del motor
     */
    static float calculateRPM();
    
    /**
     * @brief Calcula porcentaje de batería
     * @param voltage Voltaje actual
     * @return Porcentaje (0-100)
     */
    static float calculateBatteryPercent(float voltage);

private:
    static CarData lastData;
    static uint32_t lastSecondaryRead;
    
    // Lectura de sensores específicos
    static void readINA226Sensors();
    static void readTemperatureSensors();
    static void readEncoders();
    static void readPedal();
    static void readSteering();
    static void readGear();
    static void readSystemStatus();
};

#endif // CAR_SENSORS_H
