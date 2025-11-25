#include "wifi_manager.h"
#include "logger.h"
#include "alerts.h"

namespace WiFiManager {
    // WiFi credentials - CHANGE THESE!
    // Note: Variable names use _CONFIG suffix to avoid conflicts with build flag macros
#ifdef WIFI_SSID
    const char* WIFI_SSID_CONFIG = WIFI_SSID;
#else
    const char* WIFI_SSID_CONFIG = "YOUR_WIFI_SSID";
#endif

#ifdef WIFI_PASSWORD
    const char* WIFI_PASSWORD_CONFIG = WIFI_PASSWORD;
#else
    const char* WIFI_PASSWORD_CONFIG = "YOUR_WIFI_PASSWORD";
#endif

    const char* OTA_HOSTNAME_CONFIG = "coche-inteligente";
    
#ifdef OTA_PASSWORD
    const char* OTA_PASSWORD_CONFIG = OTA_PASSWORD;
#else
    const char* OTA_PASSWORD_CONFIG = "admin123";  // Change this!
#endif
    
    // Status variables
    bool connected = false;
    unsigned long lastReconnectAttempt = 0;
    const unsigned long RECONNECT_INTERVAL = 30000; // 30 seconds
    
    // WiFi connection state (non-blocking)
    static bool connectionInProgress = false;
    static unsigned long connectionStartTime = 0;
    static const unsigned long CONNECTION_TIMEOUT = 10000; // 10 seconds
    
    void init() {
        Logger::infof("WiFi: Iniciando conexión a %s", WIFI_SSID_CONFIG);
        
        // Set WiFi mode
        WiFi.mode(WIFI_STA);
        WiFi.setHostname(OTA_HOSTNAME_CONFIG);
        
        // Start connection (non-blocking)
        WiFi.begin(WIFI_SSID_CONFIG, WIFI_PASSWORD_CONFIG);
        
        // Set connection state for non-blocking check in update()
        connectionInProgress = true;
        connectionStartTime = millis();
        
        Logger::info("WiFi: Conexión iniciada (modo no bloqueante)");
    }
    
    void update() {
        // Handle initial connection attempt (non-blocking)
        if (connectionInProgress) {
            unsigned long now = millis();
            
            if (WiFi.status() == WL_CONNECTED) {
                // Connection successful
                connectionInProgress = false;
                connected = true;
                
                Logger::infof("WiFi: Conectado! IP: %s", WiFi.localIP().toString().c_str());
                Logger::infof("WiFi: RSSI: %d dBm", WiFi.RSSI());
                
                // Start mDNS
                if (MDNS.begin(OTA_HOSTNAME_CONFIG)) {
                    Logger::infof("WiFi: mDNS iniciado: %s.local", OTA_HOSTNAME_CONFIG);
                    MDNS.addService("http", "tcp", 80);
                }
                
                // Configure OTA
                ArduinoOTA.setHostname(OTA_HOSTNAME_CONFIG);
                ArduinoOTA.setPassword(OTA_PASSWORD_CONFIG);
                
                ArduinoOTA.onStart(onOTAStart);
                ArduinoOTA.onEnd(onOTAEnd);
                ArduinoOTA.onProgress(onOTAProgress);
                ArduinoOTA.onError(onOTAError);
                
                ArduinoOTA.begin();
                Logger::info("WiFi: OTA habilitado");
                
                // Play connection sound
                Alerts::play(Audio::AUDIO_MODULO_OK);
            } else if (now - connectionStartTime > CONNECTION_TIMEOUT) {
                // Connection timeout
                connectionInProgress = false;
                connected = false;
                Logger::error("WiFi: Fallo al conectar (timeout)");
            }
            // else: still connecting, check again in next update
            return; // Don't handle other WiFi logic during initial connection
        }
        
        // Handle OTA updates
        if (connected) {
            ArduinoOTA.handle();
        }
        
        // Auto-reconnect
        if (WiFi.status() != WL_CONNECTED && !connected) {
            unsigned long now = millis();
            if (now - lastReconnectAttempt > RECONNECT_INTERVAL) {
                lastReconnectAttempt = now;
                Logger::info("WiFi: Intentando reconectar...");
                WiFi.disconnect();
                WiFi.begin(WIFI_SSID_CONFIG, WIFI_PASSWORD_CONFIG);
            }
        } else if (WiFi.status() == WL_CONNECTED && !connected) {
            connected = true;
            Logger::infof("WiFi: Reconectado! IP: %s", WiFi.localIP().toString().c_str());
            Alerts::play(Audio::AUDIO_MODULO_OK);
        } else if (WiFi.status() != WL_CONNECTED && connected) {
            connected = false;
            Logger::warn("WiFi: Conexión perdida");
        }
    }
    
    bool isConnected() {
        return connected && (WiFi.status() == WL_CONNECTED);
    }
    
    String getIPAddress() {
        if (isConnected()) {
            return WiFi.localIP().toString();
        }
        return "0.0.0.0";
    }
    
    int getRSSI() {
        if (isConnected()) {
            return WiFi.RSSI();
        }
        return -100;
    }
    
    // OTA Callbacks
    void onOTAStart() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else { // U_SPIFFS
            type = "filesystem";
        }
        
        Logger::infof("OTA: Iniciando actualización %s", type.c_str());
        Alerts::play(Audio::AUDIO_MODULO_OK);
        
        // Optionally, shut down other systems
        // HUD::showOTAProgress(0);
    }
    
    void onOTAEnd() {
        Logger::info("OTA: Actualización completada");
        Alerts::play(Audio::AUDIO_MODULO_OK);
    }
    
    void onOTAProgress(unsigned int progress, unsigned int total) {
        static unsigned int lastPercent = 0;
        unsigned int percent = (progress / (total / 100));
        
        if (percent != lastPercent && percent % 10 == 0) {
            Logger::infof("OTA: Progreso: %u%%", percent);
            lastPercent = percent;
        }
        
        // Update HUD if needed
        // HUD::showOTAProgress(percent);
    }
    
    void onOTAError(ota_error_t error) {
        const char* errorMsg = "Desconocido";
        switch (error) {
            case OTA_AUTH_ERROR:    errorMsg = "Auth Failed"; break;
            case OTA_BEGIN_ERROR:   errorMsg = "Begin Failed"; break;
            case OTA_CONNECT_ERROR: errorMsg = "Connect Failed"; break;
            case OTA_RECEIVE_ERROR: errorMsg = "Receive Failed"; break;
            case OTA_END_ERROR:     errorMsg = "End Failed"; break;
        }
        
        Logger::errorf("OTA: Error: %s", errorMsg);
    }
}
