#ifndef EEPROM_PERSISTENCE_H
#define EEPROM_PERSISTENCE_H

#include <Preferences.h>

// --- Definición de estructuras de configuración ---
struct EncoderConfig {
    int16_t centerPosition;
    int16_t leftLimit;
    int16_t rightLimit;
    bool calibrated;
};

struct SensorStates {
    bool wheelFL;
    bool wheelFR;
    bool wheelRL;
    bool wheelRR;
    bool encoder;
    bool ina226FL;
    bool ina226FR;
    bool ina226RL;
    bool ina226RR;
    bool ina226Bat;
    bool ina226Steer;
};

struct PowerConfig {
    uint16_t powerHoldDelay;
    uint16_t auxDelay;
    uint16_t motorDelay;
    uint16_t shutdownDelay;
    bool autoShutdown;
};

struct LEDConfig {
    uint8_t pattern;
    uint8_t brightness;
    uint8_t speed;
    uint32_t color;
    bool enabled;
};

struct GeneralSettings {
    bool hiddenMenuPIN;
    uint16_t menuTimeout;
    bool audioEnabled;
    uint8_t volume;
    bool absEnabled;
    bool tcsEnabled;
    bool regenEnabled;
    uint8_t driveMode;
};

// --- Namespaces para Preferences ---
#define NS_ENCODER  "ENCODER"
#define NS_SENSORS  "SENSORS"
#define NS_POWER    "POWER"
#define NS_LEDS     "LEDS"
#define NS_GENERAL  "GENERAL"

class EEPROMPersistence {
public:
    // Inicialización global del sistema de persistencia
    static bool init();

    // Carga y guardado de toda la configuración (referencia)
    static bool saveAll(EncoderConfig&, SensorStates&, PowerConfig&, LEDConfig&, GeneralSettings&);
    static bool loadAll(EncoderConfig&, SensorStates&, PowerConfig&, LEDConfig&, GeneralSettings&);

    // Factory Reset (todo a valores fábrica)
    static bool factoryReset();

    // Operaciones por cada módulo
    static bool saveEncoderConfig(const EncoderConfig&);
    static bool loadEncoderConfig(EncoderConfig&);
    
    static bool saveSensorStates(const SensorStates&);
    static bool loadSensorStates(SensorStates&);
    
    static bool savePowerConfig(const PowerConfig&);
    static bool loadPowerConfig(PowerConfig&);
    
    static bool saveLEDConfig(const LEDConfig&);
    static bool loadLEDConfig(LEDConfig&);
    
    static bool saveGeneralSettings(const GeneralSettings&);
    static bool loadGeneralSettings(GeneralSettings&);

    // Defaults
    static EncoderConfig getDefaultEncoderConfig();
    static SensorStates getDefaultSensorStates();
    static PowerConfig getDefaultPowerConfig();
    static LEDConfig getDefaultLEDConfig();
    static GeneralSettings getDefaultGeneralSettings();

    // Estado global (para debug o pruebas)
    static Preferences prefs;
    static bool initialized;
};

#endif // EEPROM_PERSISTENCE_H