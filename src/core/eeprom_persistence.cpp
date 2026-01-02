#include "eeprom_persistence.h"
#include "logger.h"

Preferences EEPROMPersistence::prefs;
bool EEPROMPersistence::initialized = false;

// Configuración por defecto
EEPROMPersistence::EncoderConfig EEPROMPersistence::getDefaultEncoderConfig() { return {600, 0, 1200, false}; }
EEPROMPersistence::SensorStates EEPROMPersistence::getDefaultSensorStates()   { return {true, true, true, true, true, true, true, true, true, true, true}; }
EEPROMPersistence::PowerConfig EEPROMPersistence::getDefaultPowerConfig()     { return {5000, 100, 500, 3000, true}; }
EEPROMPersistence::LEDConfig EEPROMPersistence::getDefaultLEDConfig()         { return {1, 128, 128, 0xFF0000, true}; }
EEPROMPersistence::GeneralSettings EEPROMPersistence::getDefaultGeneralSettings() { return {true, 30, true, 15, true, true, true, 1}; }

bool EEPROMPersistence::init() {
    if (initialized) return true;
    if (!prefs.begin(NS_ENCODER, false)) {
        Logger::error("EEPROM: cannot open NS_ENCODER");
        return false;
    }
    prefs.end();
    initialized = true;
    Logger::info("EEPROM persistence initialized OK");
    return true;
}

bool EEPROMPersistence::saveAll(
    EncoderConfig& encoder, SensorStates& sensors, PowerConfig& power, LEDConfig& leds, GeneralSettings& general
) {
    if (!initialized && !init()) return false;
    bool success = true;
    success &= saveEncoderConfig(encoder);
    success &= saveSensorStates(sensors);
    success &= savePowerConfig(power);
    success &= saveLEDConfig(leds);
    success &= saveGeneralSettings(general);
    Logger::info(success ? "EEPROM: All configurations saved" : "EEPROM: Failed to save some configurations");
    return success;
}

bool EEPROMPersistence::loadAll(
    EncoderConfig& encoder, SensorStates& sensors, PowerConfig& power, LEDConfig& leds, GeneralSettings& general
) {
    if (!initialized && !init()) return false;
    bool success = true;
    success &= loadEncoderConfig(encoder);
    success &= loadSensorStates(sensors);
    success &= loadPowerConfig(power);
    success &= loadLEDConfig(leds);
    success &= loadGeneralSettings(general);
    Logger::info(success ? "EEPROM: All configurations loaded" : "EEPROM: Some configurations missing, using defaults");
    return success;
}

bool EEPROMPersistence::factoryReset() {
    if (!initialized && !init()) return false;
    bool success = true;
    static const char* const ALL_NAMESPACES[] = {
        NS_ENCODER, NS_SENSORS, NS_POWER, NS_LEDS, NS_GENERAL
    };
    static constexpr size_t NUM_NAMESPACES = sizeof(ALL_NAMESPACES) / sizeof(ALL_NAMESPACES[0]);
    static_assert(NUM_NAMESPACES == 5, "ALL_NAMESPACES debe ser 5 tras eliminar WiFi");
    for (size_t i = 0; i < NUM_NAMESPACES; i++) {
        if (prefs.begin(ALL_NAMESPACES[i], false)) {
            prefs.clear();
            prefs.end();
        }
    }
    success &= saveEncoderConfig(getDefaultEncoderConfig());
    success &= saveSensorStates(getDefaultSensorStates());
    success &= savePowerConfig(getDefaultPowerConfig());
    success &= saveLEDConfig(getDefaultLEDConfig());
    success &= saveGeneralSettings(getDefaultGeneralSettings());
    Logger::info(success ? "EEPROM: Factory reset complete" : "EEPROM: Factory reset failed");
    return success;
}

// --- Encoder Configuration ---
bool EEPROMPersistence::saveEncoderConfig(const EncoderConfig& config) {
    if (!prefs.begin(NS_ENCODER, false)) {
        Logger::error("EEPROM: cannot open NS_ENCODER");
        return false;
    }
    prefs.putShort("center", config.centerPosition);
    prefs.putShort("left", config.leftLimit);
    prefs.putShort("right", config.rightLimit);
    prefs.putBool("calibrated", config.calibrated);
    prefs.end();
    return true;
}
bool EEPROMPersistence::loadEncoderConfig(EncoderConfig& config) {
    if (!prefs.begin(NS_ENCODER, true)) {
        Logger::error("EEPROM: cannot open NS_ENCODER");
        config = getDefaultEncoderConfig();
        return false;
    }
    EncoderConfig defaults = getDefaultEncoderConfig();
    config.centerPosition = prefs.getShort("center", defaults.centerPosition);
    if (config.centerPosition < 0 || config.centerPosition > 4095) config.centerPosition = defaults.centerPosition;
    config.leftLimit = prefs.getShort("left", defaults.leftLimit);
    if (config.leftLimit < 0 || config.leftLimit > 4095) config.leftLimit = defaults.leftLimit;
    config.rightLimit = prefs.getShort("right", defaults.rightLimit);
    if (config.rightLimit < 0 || config.rightLimit > 4095) config.rightLimit = defaults.rightLimit;
    config.calibrated = prefs.getBool("calibrated", defaults.calibrated);
    prefs.end();
    return true;
}

// --- Sensor States ---
bool EEPROMPersistence::saveSensorStates(const SensorStates& states) {
    if (!prefs.begin(NS_SENSORS, false)) {
        Logger::error("EEPROM: cannot open NS_SENSORS");
        return false;
    }
    prefs.putBool("wFL", states.wheelFL);
    prefs.putBool("wFR", states.wheelFR);
    prefs.putBool("wRL", states.wheelRL);
    prefs.putBool("wRR", states.wheelRR);
    prefs.putBool("enc", states.encoder);
    prefs.putBool("inaFL", states.ina226FL);
    prefs.putBool("inaFR", states.ina226FR);
    prefs.putBool("inaRL", states.ina226RL);
    prefs.putBool("inaRR", states.ina226RR);
    prefs.putBool("inaBat", states.ina226Bat);
    prefs.putBool("inaStr", states.ina226Steer);
    prefs.end();
    return true;
}
bool EEPROMPersistence::loadSensorStates(SensorStates& states) {
    if (!prefs.begin(NS_SENSORS, true)) {
        Logger::error("EEPROM: cannot open NS_SENSORS");
        states = getDefaultSensorStates();
        return false;
    }
    SensorStates defaults = getDefaultSensorStates();
    states.wheelFL = prefs.getBool("wFL", defaults.wheelFL);
    states.wheelFR = prefs.getBool("wFR", defaults.wheelFR);
    states.wheelRL = prefs.getBool("wRL", defaults.wheelRL);
    states.wheelRR = prefs.getBool("wRR", defaults.wheelRR);
    states.encoder = prefs.getBool("enc", defaults.encoder);
    states.ina226FL = prefs.getBool("inaFL", defaults.ina226FL);
    states.ina226FR = prefs.getBool("inaFR", defaults.ina226FR);
    states.ina226RL = prefs.getBool("inaRL", defaults.ina226RL);
    states.ina226RR = prefs.getBool("inaRR", defaults.ina226RR);
    states.ina226Bat = prefs.getBool("inaBat", defaults.ina226Bat);
    states.ina226Steer = prefs.getBool("inaStr", defaults.ina226Steer);
    prefs.end();
    return true;
}

// --- Power Configuration ---
bool EEPROMPersistence::savePowerConfig(const PowerConfig& config) {
    if (!prefs.begin(NS_POWER, false)) {
        Logger::error("EEPROM: cannot open NS_POWER");
        return false;
    }
    prefs.putUShort("hold", config.powerHoldDelay);
    prefs.putUShort("aux", config.auxDelay);
    prefs.putUShort("motor", config.motorDelay);
    prefs.putUShort("shut", config.shutdownDelay);
    prefs.putBool("auto", config.autoShutdown);
    prefs.end();
    return true;
}
bool EEPROMPersistence::loadPowerConfig(PowerConfig& config) {
    if (!prefs.begin(NS_POWER, true)) {
        Logger::error("EEPROM: cannot open NS_POWER");
        config = getDefaultPowerConfig();
        return false;
    }
    PowerConfig defaults = getDefaultPowerConfig();
    config.powerHoldDelay = prefs.getUShort("hold", defaults.powerHoldDelay);
    if (config.powerHoldDelay < 100 || config.powerHoldDelay > 10000) config.powerHoldDelay = defaults.powerHoldDelay;
    config.auxDelay = prefs.getUShort("aux", defaults.auxDelay);
    if (config.auxDelay < 10 || config.auxDelay > 10000) config.auxDelay = defaults.auxDelay;
    config.motorDelay = prefs.getUShort("motor", defaults.motorDelay);
    if (config.motorDelay < 10 || config.motorDelay > 10000) config.motorDelay = defaults.motorDelay;
    config.shutdownDelay = prefs.getUShort("shut", defaults.shutdownDelay);
    if (config.shutdownDelay < 10 || config.shutdownDelay > 15000) config.shutdownDelay = defaults.shutdownDelay;
    config.autoShutdown = prefs.getBool("auto", defaults.autoShutdown);
    prefs.end();
    return true;
}

// --- LED Configuration ---
bool EEPROMPersistence::saveLEDConfig(const LEDConfig& config) {
    if (!prefs.begin(NS_LEDS, false)) {
        Logger::error("EEPROM: cannot open NS_LEDS");
        return false;
    }
    prefs.putUChar("pattern", config.pattern);
    prefs.putUChar("bright", config.brightness);
    prefs.putUChar("speed", config.speed);
    prefs.putUInt("color", config.color);
    prefs.putBool("enabled", config.enabled);
    prefs.end();
    return true;
}
bool EEPROMPersistence::loadLEDConfig(LEDConfig& config) {
    if (!prefs.begin(NS_LEDS, true)) {
        Logger::error("EEPROM: cannot open NS_LEDS");
        config = getDefaultLEDConfig();
        return false;
    }
    LEDConfig defaults = getDefaultLEDConfig();
    config.pattern = prefs.getUChar("pattern", defaults.pattern);
    config.brightness = prefs.getUChar("bright", defaults.brightness);
    config.speed = prefs.getUChar("speed", defaults.speed);
    config.color = prefs.getUInt("color", defaults.color);
    config.enabled = prefs.getBool("enabled", defaults.enabled);
    // Extra validación de rango
    if (config.pattern > 10)     config.pattern = defaults.pattern;
    if (config.brightness > 255) config.brightness = defaults.brightness;
    if (config.speed > 255)      config.speed = defaults.speed;
    prefs.end();
    return true;
}

// --- General Settings ---
bool EEPROMPersistence::saveGeneralSettings(const GeneralSettings& settings) {
    if (!prefs.begin(NS_GENERAL, false)) {
        Logger::error("EEPROM: cannot open NS_GENERAL");
        return false;
    }
    prefs.putBool("pinEnabled", settings.hiddenMenuPIN);
    prefs.putUShort("timeout", settings.menuTimeout);
    prefs.putBool("audio", settings.audioEnabled);
    prefs.putUChar("volume", settings.volume);
    prefs.putBool("abs", settings.absEnabled);
    prefs.putBool("tcs", settings.tcsEnabled);
    prefs.putBool("regen", settings.regenEnabled);
    prefs.putUChar("drive", settings.driveMode);
    prefs.end();
    return true;
}
bool EEPROMPersistence::loadGeneralSettings(GeneralSettings& settings) {
    if (!prefs.begin(NS_GENERAL, true)) {
        Logger::error("EEPROM: cannot open NS_GENERAL");
        settings = getDefaultGeneralSettings();
        return false;
    }
    GeneralSettings defaults = getDefaultGeneralSettings();
    settings.hiddenMenuPIN = prefs.getBool("pinEnabled", defaults.hiddenMenuPIN);
    settings.menuTimeout = prefs.getUShort("timeout", defaults.menuTimeout);
    if (settings.menuTimeout < 5 || settings.menuTimeout > 600) settings.menuTimeout = defaults.menuTimeout;
    settings.audioEnabled = prefs.getBool("audio", defaults.audioEnabled);
    settings.volume = prefs.getUChar("volume", defaults.volume);
    if (settings.volume > 100) settings.volume = defaults.volume;
    settings.absEnabled = prefs.getBool("abs", defaults.absEnabled);
    settings.tcsEnabled = prefs.getBool("tcs", defaults.tcsEnabled);
    settings.regenEnabled = prefs.getBool("regen", defaults.regenEnabled);
    settings.driveMode = prefs.getUChar("drive", defaults.driveMode);
    if (settings.driveMode > 3) settings.driveMode = defaults.driveMode;
    prefs.end();
    return true;
}