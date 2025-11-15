#pragma once
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>

namespace WiFiManager {
    // WiFi configuration
    extern const char* WIFI_SSID;
    extern const char* WIFI_PASSWORD;
    extern const char* OTA_HOSTNAME;
    extern const char* OTA_PASSWORD;
    
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
