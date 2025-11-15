#pragma once
#include <Arduino.h>

namespace Storage {

    // Versión de estructura de datos guardada
    const uint16_t kConfigVersion = 3;   // ⚠️ subir versión al cambiar estructura

    struct ErrorLog {
        uint16_t code;       // código de error
        uint32_t timestamp;  // marca de tiempo (millis o RTC)
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
}  // namespace Storage

// Global config instance
extern Storage::Config cfg;