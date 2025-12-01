#pragma once
#include <Arduino.h>

namespace Storage {

    // Versión de estructura de datos guardada
    const uint16_t kConfigVersion = 6;   // ⚠️ v6: added touch_enabled flag for SPI bus stability

    struct ErrorLog {
        uint16_t code;       // código de error
        uint32_t timestamp;  // marca de tiempo (millis o RTC)
    };
    
    // 🔒 v2.4.2: Estructura para odómetro y mantenimiento
    struct OdometerData {
        float totalKm;           // Odómetro total (km)
        float tripKm;            // Odómetro parcial (km)
        float lastServiceKm;     // Km del último mantenimiento
        uint32_t lastServiceDate;// Fecha último mantenimiento (timestamp)
        uint32_t engineHours;    // Horas de motor acumuladas (segundos)
    };

    struct Config {
        // Calibración pedal
        int pedalMin;
        int pedalMax;
        uint8_t pedalCurve;

        // Freno regenerativo
        uint8_t regenPercent;

        // INA226 (coeficiente shunt por canal)
        float shuntCoeff[6];

        // Encoder dirección
        int32_t steerZeroOffset;

        // HUD opciones
        bool showTemps;
        bool showEffort;
        uint8_t displayBrightness;  // Brillo de pantalla (0-255)

        // Módulos habilitados
        bool audioEnabled;
        bool lightsEnabled;
        bool multimediaEnabled;
        bool tractionEnabled;     // 🔎 NUEVO: habilitar/deshabilitar módulo de tracción

        // Nuevos flags para tolerancia a fallos
        bool wheelSensorsEnabled;
        bool tempSensorsEnabled;
        bool currentSensorsEnabled;
        bool steeringEnabled;
        
        // 🔒 v2.8.6: Touch screen configuration
        bool touchEnabled;            // Enable/disable touchscreen functionality
        
        // 🔒 v2.4.2: Odómetro y mantenimiento
        OdometerData odometer;
        uint16_t maintenanceIntervalKm;   // Intervalo mantenimiento (km) - default 500
        uint16_t maintenanceIntervalDays; // Intervalo mantenimiento (días) - default 180

        // Log persistente de errores
        static constexpr int MAX_ERRORS = 16;
        ErrorLog errors[MAX_ERRORS];
        int errorCount;

        // Versión + checksum
        uint16_t version;
        uint32_t checksum;
    };

    void init();
    void load(Config &cfg);
    bool save(const Config &cfg);
    void resetToFactory();
    void defaults(Config &cfg);

    // Helpers
    uint32_t computeChecksum(const Config &cfg);
    
    // 🔒 v2.4.2: Función para verificar corrupción de EEPROM
    bool isCorrupted();
    
    // 🔒 v2.4.2: Funciones de odómetro y mantenimiento
    void updateOdometer(float distanceKm);
    void resetTripOdometer();
    void recordMaintenance();
    bool isMaintenanceDue();
    float getKmUntilService();
}  // namespace Storage

// Global config instance
extern Storage::Config cfg;