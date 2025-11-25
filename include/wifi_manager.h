#pragma once
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>

namespace WiFiManager {
    // WiFi configuration
    // Note: These names are prefixed to avoid conflicts with build flags
    extern const char* WIFI_SSID_CONFIG;
    extern const char* WIFI_PASSWORD_CONFIG;
    extern const char* OTA_HOSTNAME_CONFIG;
    extern const char* OTA_PASSWORD_CONFIG;
    
    // Status
    extern bool connected;
    extern unsigned long lastReconnectAttempt;
    
    // Functions
    void init();
    void update();
    bool isConnected();
    String getIPAddress();
    int getRSSI();
    
    // OTA callbacks
    void onOTAStart();
    void onOTAEnd();
    void onOTAProgress(unsigned int progress, unsigned int total);
    void onOTAError(ota_error_t error);
}
