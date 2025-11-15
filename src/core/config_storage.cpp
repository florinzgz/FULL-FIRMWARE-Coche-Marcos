#include "config_storage.h"

namespace ConfigStorage {
    static Preferences preferences;
    static Config currentConfig;
    static bool initialized = false;
    
    void init() {
        if (initialized) return;
        
        preferences.begin(NAMESPACE, false); // Read/write mode
        
        // Try to load existing config
        if (!load(currentConfig)) {
            // No valid config found, use defaults
            currentConfig = DEFAULT_CONFIG;
            currentConfig.checksum = calculateChecksum(currentConfig);
            save(currentConfig);
        }
        
        initialized = true;
    }
    
    bool load(Config& config) {
        if (!preferences.isKey("checksum")) {
            return false; // No config stored
        }
        
        // Load all values
        config.sensorFL_enabled = preferences.getBool("sFL", DEFAULT_CONFIG.sensorFL_enabled);
        config.sensorFR_enabled = preferences.getBool("sFR", DEFAULT_CONFIG.sensorFR_enabled);
        config.sensorRL_enabled = preferences.getBool("sRL", DEFAULT_CONFIG.sensorRL_enabled);
        config.sensorRR_enabled = preferences.getBool("sRR", DEFAULT_CONFIG.sensorRR_enabled);
        config.ina226_enabled = preferences.getBool("sINA", DEFAULT_CONFIG.ina226_enabled);
        
        config.encoder_center = preferences.getShort("enc_c", DEFAULT_CONFIG.encoder_center);
        config.encoder_left_limit = preferences.getShort("enc_l", DEFAULT_CONFIG.encoder_left_limit);
        config.encoder_right_limit = preferences.getShort("enc_r", DEFAULT_CONFIG.encoder_right_limit);
        
        config.led_pattern = preferences.getUChar("led_pat", DEFAULT_CONFIG.led_pattern);
        config.led_brightness = preferences.getUChar("led_bri", DEFAULT_CONFIG.led_brightness);
        config.led_speed = preferences.getUChar("led_spd", DEFAULT_CONFIG.led_speed);
        config.led_color = preferences.getUInt("led_col", DEFAULT_CONFIG.led_color);
        
        config.power_hold_delay = preferences.getUShort("pwr_hold", DEFAULT_CONFIG.power_hold_delay);
        config.aux_delay = preferences.getUShort("pwr_aux", DEFAULT_CONFIG.aux_delay);
        config.traction_delay = preferences.getUShort("pwr_trac", DEFAULT_CONFIG.traction_delay);
        
        config.abs_enabled = preferences.getBool("abs", DEFAULT_CONFIG.abs_enabled);
        config.tcs_enabled = preferences.getBool("tcs", DEFAULT_CONFIG.tcs_enabled);
        config.regen_enabled = preferences.getBool("regen", DEFAULT_CONFIG.regen_enabled);
        config.wifi_enabled = preferences.getBool("wifi", DEFAULT_CONFIG.wifi_enabled);
        
        config.checksum = preferences.getUInt("checksum", 0);
        
        // Validate checksum
        return isValid(config);
    }
    
    bool save(const Config& config) {
        // Calculate and update checksum
        Config temp = config;
        temp.checksum = calculateChecksum(temp);
        
        // Save all values
        preferences.putBool("sFL", temp.sensorFL_enabled);
        preferences.putBool("sFR", temp.sensorFR_enabled);
        preferences.putBool("sRL", temp.sensorRL_enabled);
        preferences.putBool("sRR", temp.sensorRR_enabled);
        preferences.putBool("sINA", temp.ina226_enabled);
        
        preferences.putShort("enc_c", temp.encoder_center);
        preferences.putShort("enc_l", temp.encoder_left_limit);
        preferences.putShort("enc_r", temp.encoder_right_limit);
        
        preferences.putUChar("led_pat", temp.led_pattern);
        preferences.putUChar("led_bri", temp.led_brightness);
        preferences.putUChar("led_spd", temp.led_speed);
        preferences.putUInt("led_col", temp.led_color);
        
        preferences.putUShort("pwr_hold", temp.power_hold_delay);
        preferences.putUShort("pwr_aux", temp.aux_delay);
        preferences.putUShort("pwr_trac", temp.traction_delay);
        
        preferences.putBool("abs", temp.abs_enabled);
        preferences.putBool("tcs", temp.tcs_enabled);
        preferences.putBool("regen", temp.regen_enabled);
        preferences.putBool("wifi", temp.wifi_enabled);
        
        preferences.putUInt("checksum", temp.checksum);
        
        // Update current config
        currentConfig = temp;
        
        return true;
    }
    
    void resetToDefaults() {
        preferences.clear();
        currentConfig = DEFAULT_CONFIG;
        currentConfig.checksum = calculateChecksum(currentConfig);
        save(currentConfig);
    }
    
    bool isValid(const Config& config) {
        uint32_t calculated = calculateChecksum(config);
        return calculated == config.checksum;
    }
    
    uint32_t calculateChecksum(const Config& config) {
        // Simple checksum: XOR all fields
        uint32_t checksum = 0;
        
        const uint8_t* data = reinterpret_cast<const uint8_t*>(&config);
        size_t size = sizeof(Config) - sizeof(uint32_t); // Exclude checksum field
        
        for (size_t i = 0; i < size; i++) {
            checksum ^= (data[i] << (i % 24));
        }
        
        return checksum;
    }
    
    Config& getCurrentConfig() {
        if (!initialized) {
            init();
        }
        return currentConfig;
    }
}
