#include "config_manager.h"

namespace ConfigManager {

// Instancia global de Preferences
static Preferences prefs;

// Namespace en EEPROM
const char* NAMESPACE = "coche";

// Keys para cada parámetro
const char* KEY_CRC              = "crc";
const char* KEY_POWER_HOLD       = "pwr_hold";
const char* KEY_SHUTDOWN         = "shutdown";
const char* KEY_REGEN_LEVEL      = "regen_lvl";
const char* KEY_LED_BRIGHT       = "led_brght";
const char* KEY_LED_PATTERN      = "led_ptrn";
const char* KEY_LED_SPEED        = "led_spd";
const char* KEY_LED_R            = "led_r";
const char* KEY_LED_G            = "led_g";
const char* KEY_LED_B            = "led_b";
const char* KEY_ENC_CENTER       = "enc_cntr";
const char* KEY_ENC_MIN          = "enc_min";
const char* KEY_ENC_MAX          = "enc_max";
const char* KEY_SENS_WHEELS      = "sns_whls";
const char* KEY_SENS_CURRENT     = "sns_curr";
const char* KEY_SENS_ENCODER     = "sns_enc";
const char* KEY_MENU_TIMEOUT     = "mnu_tout";
const char* KEY_MENU_PIN         = "mnu_pin";
const char* KEY_ABS_THRESH       = "abs_thrs";
const char* KEY_TCS_THRESH       = "tcs_thrs";

// ============================================
// IMPLEMENTACIÓN
// ============================================

void init() {
    // Inicializar Preferences (EEPROM)
    prefs.begin(NAMESPACE, false); // false = read/write mode
}

uint32_t calculateCRC(const Config& config) {
    // CRC32 simple (suma de bytes XOR posición)
    const uint8_t* data = reinterpret_cast<const uint8_t*>(&config);
    size_t size = sizeof(Config) - sizeof(uint32_t); // Excluir campo CRC
    
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < size; i++) {
        crc ^= data[i];
        crc = (crc << 1) | (crc >> 31); // Rotate left
    }
    return crc;
}

bool load(Config& config) {
    // Cargar configuración desde EEPROM
    if (!prefs.isKey(KEY_CRC)) {
        // No hay configuración guardada
        return false;
    }
    
    // Cargar todos los parámetros
    config.powerHoldDelay     = prefs.getUShort(KEY_POWER_HOLD, 5000);
    config.shutdownDelay      = prefs.getUShort(KEY_SHUTDOWN, 3000);
    config.regenBrakeLevel    = prefs.getUChar(KEY_REGEN_LEVEL, 30);
    config.ledBrightness      = prefs.getUChar(KEY_LED_BRIGHT, 128);
    config.ledPattern         = prefs.getUChar(KEY_LED_PATTERN, 0);
    config.ledSpeed           = prefs.getUChar(KEY_LED_SPEED, 5);
    config.ledColorR          = prefs.getUChar(KEY_LED_R, 255);
    config.ledColorG          = prefs.getUChar(KEY_LED_G, 255);
    config.ledColorB          = prefs.getUChar(KEY_LED_B, 255);
    config.encoderCenter      = prefs.getShort(KEY_ENC_CENTER, 600);
    config.encoderMinLeft     = prefs.getShort(KEY_ENC_MIN, 0);
    config.encoderMaxRight    = prefs.getShort(KEY_ENC_MAX, 1200);
    config.sensorWheelsEnabled  = prefs.getBool(KEY_SENS_WHEELS, true);
    config.sensorCurrentEnabled = prefs.getBool(KEY_SENS_CURRENT, true);
    config.sensorEncoderEnabled = prefs.getBool(KEY_SENS_ENCODER, true);
    config.menuTimeout        = prefs.getUShort(KEY_MENU_TIMEOUT, 30000);
    config.menuPIN            = prefs.getUShort(KEY_MENU_PIN, 8989);
    config.absThreshold       = prefs.getUChar(KEY_ABS_THRESH, 15);
    config.tcsThreshold       = prefs.getUChar(KEY_TCS_THRESH, 20);
    
    // Verificar CRC
    uint32_t storedCRC = prefs.getUInt(KEY_CRC, 0);
    uint32_t calculatedCRC = calculateCRC(config);
    
    if (storedCRC != calculatedCRC) {
        // CRC inválido - configuración corrupta
        return false;
    }
    
    config.crc = calculatedCRC;
    return true;
}

bool save(const Config& config) {
    // Calcular y guardar CRC
    Config configWithCRC = config;
    configWithCRC.crc = calculateCRC(config);
    
    // Guardar todos los parámetros
    prefs.putUShort(KEY_POWER_HOLD,     configWithCRC.powerHoldDelay);
    prefs.putUShort(KEY_SHUTDOWN,       configWithCRC.shutdownDelay);
    prefs.putUChar(KEY_REGEN_LEVEL,     configWithCRC.regenBrakeLevel);
    prefs.putUChar(KEY_LED_BRIGHT,      configWithCRC.ledBrightness);
    prefs.putUChar(KEY_LED_PATTERN,     configWithCRC.ledPattern);
    prefs.putUChar(KEY_LED_SPEED,       configWithCRC.ledSpeed);
    prefs.putUChar(KEY_LED_R,           configWithCRC.ledColorR);
    prefs.putUChar(KEY_LED_G,           configWithCRC.ledColorG);
    prefs.putUChar(KEY_LED_B,           configWithCRC.ledColorB);
    prefs.putShort(KEY_ENC_CENTER,      configWithCRC.encoderCenter);
    prefs.putShort(KEY_ENC_MIN,         configWithCRC.encoderMinLeft);
    prefs.putShort(KEY_ENC_MAX,         configWithCRC.encoderMaxRight);
    prefs.putBool(KEY_SENS_WHEELS,      configWithCRC.sensorWheelsEnabled);
    prefs.putBool(KEY_SENS_CURRENT,     configWithCRC.sensorCurrentEnabled);
    prefs.putBool(KEY_SENS_ENCODER,     configWithCRC.sensorEncoderEnabled);
    prefs.putUShort(KEY_MENU_TIMEOUT,   configWithCRC.menuTimeout);
    prefs.putUShort(KEY_MENU_PIN,       configWithCRC.menuPIN);
    prefs.putUChar(KEY_ABS_THRESH,      configWithCRC.absThreshold);
    prefs.putUChar(KEY_TCS_THRESH,      configWithCRC.tcsThreshold);
    prefs.putUInt(KEY_CRC,              configWithCRC.crc);
    
    return true;
}

void reset() {
    // Borrar toda la configuración EEPROM
    prefs.clear();
}

bool isValid() {
    // Verificar si existe configuración válida
    if (!prefs.isKey(KEY_CRC)) {
        return false;
    }
    
    // Cargar y verificar CRC
    Config temp;
    return load(temp);
}

} // namespace ConfigManager
