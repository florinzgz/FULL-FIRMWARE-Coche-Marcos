/**
 * @file eeprom_persistence.cpp
 * @brief Implementation of EEPROM persistence system for car configurations
 */

#include "eeprom_persistence.h"
#include "logger.h"

// Static members initialization
Preferences EEPROMPersistence::prefs;
bool EEPROMPersistence::initialized = false;

// Default configurations
EEPROMPersistence::EncoderConfig EEPROMPersistence::getDefaultEncoderConfig() {
    return {600, 0, 1200, false};  // center=600, left=0, right=1200, not calibrated
}

EEPROMPersistence::SensorStates EEPROMPersistence::getDefaultSensorStates() {
    return {true, true, true, true, true, true, true, true, true, true, true};  // All enabled
}

EEPROMPersistence::PowerConfig EEPROMPersistence::getDefaultPowerConfig() {
    return {5000, 100, 500, 3000, true};  // Standard delays, autoShutdown enabled
}

EEPROMPersistence::LEDConfig EEPROMPersistence::getDefaultLEDConfig() {
    return {0, 128, 128, 0xFF0000, true};  // Pattern 0, mid brightness/speed, red, enabled
}

EEPROMPersistence::WiFiConfig EEPROMPersistence::getDefaultWiFiConfig() {
    WiFiConfig cfg;
    strncpy(cfg.ssid, "", sizeof(cfg.ssid));
    strncpy(cfg.password, "", sizeof(cfg.password));
    cfg.autoConnect = false;
    cfg.otaEnabled = true;
    return cfg;
}

EEPROMPersistence::GeneralSettings EEPROMPersistence::getDefaultGeneralSettings() {
    return {true, 30, true, 15, true, true, true, 1};  // PIN enabled, 30s timeout, audio on, vol 15, all safety on, Normal mode
}

bool EEPROMPersistence::init() {
    if (initialized) return true;
    
    // Verify NVS by trying to open/close one namespace
    if (!prefs.begin(NS_ENCODER, false)) {
        Logger::error("EEPROM: Failed to initialize NVS");
        return false;
    }
    prefs.end();
    
    initialized = true;
    Logger::info("EEPROM persistence initialized OK");
    return true;
}

bool EEPROMPersistence::saveAll() {
    if (!initialized && !init()) return false;
    
    EncoderConfig encoder;
    SensorStates sensors;
    PowerConfig power;
    LEDConfig leds;
    WiFiConfig wifi;
    GeneralSettings general;
    
    // Load current values to preserve unchanged data
    loadEncoderConfig(encoder);
    loadSensorStates(sensors);
    loadPowerConfig(power);
    loadLEDConfig(leds);
    loadWiFiConfig(wifi);
    loadGeneralSettings(general);
    
    // Save all configurations
    bool success = true;
    success &= saveEncoderConfig(encoder);
    success &= saveSensorStates(sensors);
    success &= savePowerConfig(power);
    success &= saveLEDConfig(leds);
    success &= saveWiFiConfig(wifi);
    success &= saveGeneralSettings(general);
    
    if (success) {
        Logger::info("EEPROM: All configurations saved");
    } else {
        Logger::error("EEPROM: Failed to save some configurations");
    }
    
    return success;
}

bool EEPROMPersistence::loadAll() {
    if (!initialized && !init()) return false;
    
    EncoderConfig encoder;
    SensorStates sensors;
    PowerConfig power;
    LEDConfig leds;
    WiFiConfig wifi;
    GeneralSettings general;
    
    bool success = true;
    success &= loadEncoderConfig(encoder);
    success &= loadSensorStates(sensors);
    success &= loadPowerConfig(power);
    success &= loadLEDConfig(leds);
    success &= loadWiFiConfig(wifi);
    success &= loadGeneralSettings(general);
    
    if (success) {
        Logger::info("EEPROM: All configurations loaded");
    } else {
        Logger::warn("EEPROM: Some configurations missing, using defaults");
    }
    
    return success;
}

bool EEPROMPersistence::factoryReset() {
    if (!initialized && !init()) return false;
    
    bool success = true;
    
    // Clear all namespaces - uses the static constexpr constants defined in the header
    // to ensure consistency with the save/load functions
    static const char* const ALL_NAMESPACES[] = {
        NS_ENCODER, NS_SENSORS, NS_POWER, NS_LEDS, NS_WIFI, NS_GENERAL
    };
    static constexpr size_t NUM_NAMESPACES = sizeof(ALL_NAMESPACES) / sizeof(ALL_NAMESPACES[0]);
    
    for (size_t i = 0; i < NUM_NAMESPACES; i++) {
        if (prefs.begin(ALL_NAMESPACES[i], false)) {
            prefs.clear();
            prefs.end();
        }
    }
    
    // Save defaults
    success &= saveEncoderConfig(getDefaultEncoderConfig());
    success &= saveSensorStates(getDefaultSensorStates());
    success &= savePowerConfig(getDefaultPowerConfig());
    success &= saveLEDConfig(getDefaultLEDConfig());
    success &= saveWiFiConfig(getDefaultWiFiConfig());
    success &= saveGeneralSettings(getDefaultGeneralSettings());
    
    if (success) {
        Logger::info("EEPROM: Factory reset complete");
    } else {
        Logger::error("EEPROM: Factory reset failed");
    }
    
    return success;
}

// Encoder Configuration
bool EEPROMPersistence::saveEncoderConfig(const EncoderConfig& config) {
    if (!prefs.begin(NS_ENCODER, false)) return false;
    
    prefs.putShort("center", config.centerPosition);
    prefs.putShort("left", config.leftLimit);
    prefs.putShort("right", config.rightLimit);
    prefs.putBool("calibrated", config.calibrated);
    
    prefs.end();
    return true;
}

bool EEPROMPersistence::loadEncoderConfig(EncoderConfig& config) {
    if (!prefs.begin(NS_ENCODER, true)) {
        config = getDefaultEncoderConfig();
        return false;
    }
    
    EncoderConfig defaults = getDefaultEncoderConfig();
    config.centerPosition = prefs.getShort("center", defaults.centerPosition);
    config.leftLimit = prefs.getShort("left", defaults.leftLimit);
    config.rightLimit = prefs.getShort("right", defaults.rightLimit);
    config.calibrated = prefs.getBool("calibrated", defaults.calibrated);
    
    prefs.end();
    return true;
}

// Sensor States
bool EEPROMPersistence::saveSensorStates(const SensorStates& states) {
    if (!prefs.begin(NS_SENSORS, false)) return false;
    
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

// Power Configuration
bool EEPROMPersistence::savePowerConfig(const PowerConfig& config) {
    if (!prefs.begin(NS_POWER, false)) return false;
    
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
        config = getDefaultPowerConfig();
        return false;
    }
    
    PowerConfig defaults = getDefaultPowerConfig();
    config.powerHoldDelay = prefs.getUShort("hold", defaults.powerHoldDelay);
    config.auxDelay = prefs.getUShort("aux", defaults.auxDelay);
    config.motorDelay = prefs.getUShort("motor", defaults.motorDelay);
    config.shutdownDelay = prefs.getUShort("shut", defaults.shutdownDelay);
    config.autoShutdown = prefs.getBool("auto", defaults.autoShutdown);
    
    prefs.end();
    return true;
}

// LED Configuration
bool EEPROMPersistence::saveLEDConfig(const LEDConfig& config) {
    if (!prefs.begin(NS_LEDS, false)) return false;
    
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
        config = getDefaultLEDConfig();
        return false;
    }
    
    LEDConfig defaults = getDefaultLEDConfig();
    config.pattern = prefs.getUChar("pattern", defaults.pattern);
    config.brightness = prefs.getUChar("bright", defaults.brightness);
    config.speed = prefs.getUChar("speed", defaults.speed);
    config.color = prefs.getUInt("color", defaults.color);
    config.enabled = prefs.getBool("enabled", defaults.enabled);
    
    prefs.end();
    return true;
}

// WiFi Configuration
bool EEPROMPersistence::saveWiFiConfig(const WiFiConfig& config) {
    if (!prefs.begin(NS_WIFI, false)) return false;
    
    prefs.putString("ssid", config.ssid);
    prefs.putString("pass", config.password);
    prefs.putBool("auto", config.autoConnect);
    prefs.putBool("ota", config.otaEnabled);
    
    prefs.end();
    return true;
}

bool EEPROMPersistence::loadWiFiConfig(WiFiConfig& config) {
    if (!prefs.begin(NS_WIFI, true)) {
        config = getDefaultWiFiConfig();
        return false;
    }
    
    WiFiConfig defaults = getDefaultWiFiConfig();
    
    String ssid = prefs.getString("ssid", defaults.ssid);
    String pass = prefs.getString("pass", defaults.password);
    strncpy(config.ssid, ssid.c_str(), sizeof(config.ssid) - 1);
    config.ssid[sizeof(config.ssid) - 1] = '\0';
    strncpy(config.password, pass.c_str(), sizeof(config.password) - 1);
    config.password[sizeof(config.password) - 1] = '\0';
    
    config.autoConnect = prefs.getBool("auto", defaults.autoConnect);
    config.otaEnabled = prefs.getBool("ota", defaults.otaEnabled);
    
    prefs.end();
    return true;
}

// General Settings
bool EEPROMPersistence::saveGeneralSettings(const GeneralSettings& settings) {
    if (!prefs.begin(NS_GENERAL, false)) return false;
    
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
        settings = getDefaultGeneralSettings();
        return false;
    }
    
    GeneralSettings defaults = getDefaultGeneralSettings();
    settings.hiddenMenuPIN = prefs.getBool("pinEnabled", defaults.hiddenMenuPIN);
    settings.menuTimeout = prefs.getUShort("timeout", defaults.menuTimeout);
    settings.audioEnabled = prefs.getBool("audio", defaults.audioEnabled);
    settings.volume = prefs.getUChar("volume", defaults.volume);
    settings.absEnabled = prefs.getBool("abs", defaults.absEnabled);
    settings.tcsEnabled = prefs.getBool("tcs", defaults.tcsEnabled);
    settings.regenEnabled = prefs.getBool("regen", defaults.regenEnabled);
    settings.driveMode = prefs.getUChar("drive", defaults.driveMode);
    
    prefs.end();
    return true;
}
