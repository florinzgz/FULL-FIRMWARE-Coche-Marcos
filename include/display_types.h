#ifndef DISPLAY_TYPES_H
#define DISPLAY_TYPES_H

#include <stdint.h>

// Tipos de menú
enum class MenuType {
    NONE,
    DASHBOARD,
    SETTINGS,
    CALIBRATION,
    HARDWARE_TEST,
    WIFI_CONFIG,
    INA226_MONITOR,
    STATISTICS,
    QUICK_MENU,
    HIDDEN_MENU      // Menú oculto con calibraciones y datos detallados
};

// Niveles de alerta
enum class AlertLevel {
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

// Posiciones de marcha
enum class GearPosition {
    PARK,      // P
    NEUTRAL,   // N
    REVERSE,   // R
    DRIVE1,    // D1
    DRIVE2     // D2
};

// Estados del sistema
struct SystemStatus {
    bool lights;
    bool fourWheelDrive;
    bool parkingBrake;
    bool bluetooth;
    bool wifi;
    bool warnings;
};

// Datos del coche (Car Data) - Integración con sensores reales
struct CarData {
    // Velocidad y odometría
    float speed;              // km/h (calculado desde sensores rueda)
    float rpm;                // RPM motor (estimado desde velocidad)
    float odoTotal;           // Odómetro total en km
    float odoTrip;            // Odómetro parcial en km
    
    // Energía (INA226 sensores)
    float voltage;            // Voltaje batería 24V (INA226 canal 4) - alias de batteryVoltage
    float batteryVoltage;     // Voltaje batería 24V (INA226 canal 4)
    float batteryPercent;     // Porcentaje batería (0-100, calculado)
    float current;            // Corriente total batería - alias de batteryCurrent
    float batteryCurrent;     // Corriente total batería
    float batteryPower;       // Potencia instantánea (W)
    
    // Temperaturas (DS18B20)
    float temperature;        // Temperatura principal (motor/pedal) - alias de motorTemp[0]
    float motorTemp[4];       // Temperatura 4 motores tracción
    float ambientTemp;        // Temperatura ambiente
    float controllerTemp;     // Temperatura controlador
    
    // Corrientes motores (INA226 canales 0-3)
    float motorCurrent[4];    // Corriente 4 motores tracción (FL, FR, RL, RR)
    float steeringCurrent;    // Corriente motor dirección (INA226 canal 5)
    
    // Estado
    GearPosition gear;        // Marcha actual (shifter)
    SystemStatus status;      // Estado del sistema
    
    // Pedal y dirección
    float pedalPosition;      // Posición pedal acelerador (0-100%) - alias de throttlePercent
    float throttlePercent;    // Posición pedal acelerador (0-100%)
    float encoderValue;       // Valor crudo del encoder del pedal
    float steeringAngle;      // Ángulo volante (grados, desde encoder)
};

// Datos del vehículo (alias para compatibilidad con display firmware)
typedef CarData VehicleData;

// Datos de un widget wheel (selector circular)
struct WheelWidget {
    int16_t x, y;
    int16_t radius;
    int value;
    int minValue;
    int maxValue;
    const char* label;
    bool touched;
};

// Datos de un slider
struct SliderWidget {
    int16_t x, y;
    int16_t width;
    int16_t height;
    int value;
    int minValue;
    int maxValue;
    const char* label;
    bool touched;
};

// Datos de un botón
struct ButtonWidget {
    int16_t x, y;
    int16_t width;
    int16_t height;
    const char* label;
    bool pressed;
    bool enabled;
};

// Tipo de gesto táctil
enum class GestureType {
    NONE,
    TAP,
    LONG_PRESS,
    SWIPE_LEFT,
    SWIPE_RIGHT,
    SWIPE_UP,
    SWIPE_DOWN
};

// Datos de un gesto
struct TouchGesture {
    GestureType type;
    int16_t startX, startY;
    int16_t endX, endY;
    uint32_t duration;
};

#endif // DISPLAY_TYPES_H
