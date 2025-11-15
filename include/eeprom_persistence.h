#ifndef EEPROM_PERSISTENCE_H
#define EEPROM_PERSISTENCE_H

#include <Arduino.h>
#include <Preferences.h>

/**
 * @brief Sistema de persistencia EEPROM para configuraciones del coche
 * 
 * Gestiona almacenamiento persistente de:
 * - Configuración encoder (calibración volante)
 * - Estados sensores (enable/disable)
 * - Configuración power management (tiempos relés)
 * - Configuración LEDs (patrón, brillo, velocidad, color)
 * - Configuración WiFi (SSID, password)
 * - Settings generales del sistema
 * 
 * Usa ESP32 Preferences (NVS) para almacenamiento no volátil
 */
class EEPROMPersistence {
public:
    /**
     * @brief Inicializa el sistema EEPROM
     * @return true si inicialización exitosa
     */
    static bool init();
    
    /**
     * @brief Guarda todas las configuraciones actuales
     * @return true si guardado exitoso
     */
    static bool saveAll();
    
    /**
     * @brief Carga todas las configuraciones guardadas
     * @return true si carga exitosa
     */
    static bool loadAll();
    
    /**
     * @brief Resetea todas las configuraciones a valores por defecto
     * @return true si reset exitoso
     */
    static bool factoryReset();
    
    // ========== ENCODER CALIBRATION ==========
    struct EncoderConfig {
        int16_t centerPosition;  // Posición centro volante
        int16_t leftLimit;       // Límite izquierdo
        int16_t rightLimit;      // Límite derecho
        bool calibrated;         // Flag calibración completada
    };
    
    static bool saveEncoderConfig(const EncoderConfig& config);
    static bool loadEncoderConfig(EncoderConfig& config);
    
    // ========== SENSOR STATES ==========
    struct SensorStates {
        bool wheelFL;      // Sensor rueda Front Left
        bool wheelFR;      // Sensor rueda Front Right
        bool wheelRL;      // Sensor rueda Rear Left
        bool wheelRR;      // Sensor rueda Rear Right
        bool encoder;      // Encoder volante
        bool ina226FL;     // INA226 Motor FL
        bool ina226FR;     // INA226 Motor FR
        bool ina226RL;     // INA226 Motor RL
        bool ina226RR;     // INA226 Motor RR
        bool ina226Bat;    // INA226 Batería
        bool ina226Steer;  // INA226 Motor dirección
    };
    
    static bool saveSensorStates(const SensorStates& states);
    static bool loadSensorStates(SensorStates& states);
    
    // ========== POWER MANAGEMENT CONFIG ==========
    struct PowerConfig {
        uint16_t powerHoldDelay;      // Delay Power Hold (ms)
        uint16_t auxDelay;            // Delay 12V Auxiliares (ms)
        uint16_t motorDelay;          // Delay 24V Motores (ms)
        uint16_t shutdownDelay;       // Delay shutdown total (ms)
        bool autoShutdown;            // Auto-shutdown habilitado
    };
    
    static bool savePowerConfig(const PowerConfig& config);
    static bool loadPowerConfig(PowerConfig& config);
    
    // ========== LED CONFIG ==========
    struct LEDConfig {
        uint8_t pattern;       // Patrón actual (0-9)
        uint8_t brightness;    // Brillo (0-255)
        uint8_t speed;         // Velocidad animación (0-255)
        uint32_t color;        // Color RGB personalizado
        bool enabled;          // LEDs habilitados
    };
    
    static bool saveLEDConfig(const LEDConfig& config);
    static bool loadLEDConfig(LEDConfig& config);
    
    // ========== WIFI CONFIG ==========
    struct WiFiConfig {
        char ssid[32];         // SSID WiFi
        char password[64];     // Password WiFi
        bool autoConnect;      // Auto-connect al arranque
        bool otaEnabled;       // OTA habilitado
    };
    
    static bool saveWiFiConfig(const WiFiConfig& config);
    static bool loadWiFiConfig(WiFiConfig& config);
    
    // ========== GENERAL SETTINGS ==========
    struct GeneralSettings {
        bool hiddenMenuPIN;      // PIN 8989 activado
        uint16_t menuTimeout;    // Timeout menú oculto (segundos)
        bool audioEnabled;       // Audio habilitado
        uint8_t volume;          // Volumen (0-30)
        bool absEnabled;         // ABS habilitado
        bool tcsEnabled;         // TCS habilitado
        bool regenEnabled;       // Regen brake habilitado
        uint8_t driveMode;       // Modo conducción (0=Eco, 1=Normal, 2=Sport)
    };
    
    static bool saveGeneralSettings(const GeneralSettings& settings);
    static bool loadGeneralSettings(GeneralSettings& settings);
    
    // ========== VERSIÓN CONFIGURACIÓN ==========
    static const uint8_t CONFIG_VERSION = 1;  // Versión actual del formato
    
private:
    static Preferences prefs;
    static bool initialized;
    
    // Nombres de namespaces NVS
    static constexpr const char* NS_ENCODER = "encoder";
    static constexpr const char* NS_SENSORS = "sensors";
    static constexpr const char* NS_POWER = "power";
    static constexpr const char* NS_LEDS = "leds";
    static constexpr const char* NS_WIFI = "wifi";
    static constexpr const char* NS_GENERAL = "general";
    
    // Valores por defecto
    static EncoderConfig getDefaultEncoderConfig();
    static SensorStates getDefaultSensorStates();
    static PowerConfig getDefaultPowerConfig();
    static LEDConfig getDefaultLEDConfig();
    static WiFiConfig getDefaultWiFiConfig();
    static GeneralSettings getDefaultGeneralSettings();
};

#endif // EEPROM_PERSISTENCE_H
