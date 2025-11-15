#ifndef MENU_WIFI_OTA_H
#define MENU_WIFI_OTA_H

#include <Arduino.h>

class MenuWiFiOTA {
public:
    static void init();
    static void update();
    static void draw();
    static bool handleTouch(int16_t x, int16_t y);
    
private:
    static void drawConnectionStatus();
    static void drawSignalStrength();
    static void drawOTAStatus();
    static void drawControlButtons();
    
    static void connectWiFi();
    static void disconnectWiFi();
    static void checkForUpdates();
    static void installUpdate();
    
    static bool isConnected();
    static int getRSSI();
    static String getSSID();
    static String getIP();
    static String getCurrentVersion();
    static String getLatestVersion();
    
    static unsigned long lastUpdate;
    static bool updateAvailable;
    static String latestVersion;
    static bool isUpdating;
    static int updateProgress;
};

#endif // MENU_WIFI_OTA_H
