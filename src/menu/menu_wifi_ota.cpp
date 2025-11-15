#include "menu_wifi_ota.h"
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "config_storage.h"
// #include "display.h"  // Display module not yet implemented
// #include "audio.h"  // Audio module not yet implemented

// Static member initialization
unsigned long MenuWiFiOTA::lastUpdate = 0;
bool MenuWiFiOTA::updateAvailable = false;
String MenuWiFiOTA::latestVersion = "";
bool MenuWiFiOTA::isUpdating = false;
int MenuWiFiOTA::updateProgress = 0;

void MenuWiFiOTA::init() {
    lastUpdate = millis();
    updateAvailable = false;
    isUpdating = false;
    updateProgress = 0;
    
    // Auto-connect if WiFi enabled in config
    auto config = ConfigStorage::getCurrentConfig();
    if (config.wifi_enabled && !isConnected()) {
        connectWiFi();
    }
}

void MenuWiFiOTA::update() {
    // Update at 1Hz
    if (millis() - lastUpdate < 1000) return;
    lastUpdate = millis();
    
    // Handle OTA events (non-blocking)
    ArduinoOTA.handle();
}

void MenuWiFiOTA::draw() {
    // Display module not yet implemented
    // Display::clear();
    
    // Title
    // Display::setTextSize(2);
    // Display::setTextColor(TFT_CYAN);
    // Display::setCursor(10, 10);
    // Display::print("WiFi & OTA Status");
    
    drawConnectionStatus();
    drawSignalStrength();
    drawOTAStatus();
    drawControlButtons();
}

void MenuWiFiOTA::drawConnectionStatus() {
    // Display module not yet implemented
    // Display::setTextSize(1);
    
    // WiFi Status
    // int y = 50;
    // Display::setTextColor(TFT_WHITE);
    // Display::setCursor(10, y);
    // Display::print("WiFi Status:");
    
    // if (isConnected()) {
    //     Display::setTextColor(TFT_GREEN);
    //     Display::setCursor(120, y);
    //     Display::print("Connected");
    //     
    //     // SSID
    //     y += 20;
    //     Display::setTextColor(TFT_WHITE);
    //     Display::setCursor(10, y);
    //     Display::print("SSID:");
    //     Display::setCursor(120, y);
    //     Display::print(getSSID());
    //     
    //     // IP Address
    //     y += 20;
    //     Display::setCursor(10, y);
    //     Display::print("IP:");
    //     Display::setCursor(120, y);
    //     Display::print(getIP());
    // } else {
    //     Display::setTextColor(TFT_RED);
    //     Display::setCursor(120, y);
    //     Display::print("Disconnected");
    // }
}

void MenuWiFiOTA::drawSignalStrength() {
    if (!isConnected()) return;
    
    // Display module not yet implemented
    // int y = 110;
    // int rssi = getRSSI();
    // 
    // Display::setTextColor(TFT_WHITE);
    // Display::setCursor(10, y);
    // Display::print("Signal:");
    // Display::setCursor(120, y);
    // Display::print(rssi);
    // Display::print(" dBm");
    // 
    // // Quality bar
    // y += 20;
    // int quality = 0;
    // uint16_t color = TFT_RED;
    // 
    // if (rssi > -50) {
    //     quality = 100;
    //     color = TFT_GREEN;
    // } else if (rssi > -60) {
    //     quality = 75;
    //     color = TFT_GREENYELLOW;
    // } else if (rssi > -70) {
    //     quality = 50;
    //     color = TFT_YELLOW;
    // } else {
    //     quality = 25;
    //     color = TFT_ORANGE;
    // }
    // 
    // Display::fillRect(120, y, quality * 2, 10, color);
    // Display::drawRect(120, y, 200, 10, TFT_WHITE);
}

void MenuWiFiOTA::drawOTAStatus() {
    // Display module not yet implemented
    // int y = 160;
    // 
    // Display::setTextColor(TFT_WHITE);
    // Display::setCursor(10, y);
    // Display::print("Firmware:");
    // Display::setCursor(120, y);
    // Display::print(getCurrentVersion());
    // 
    // if (updateAvailable) {
    //     y += 20;
    //     Display::setTextColor(TFT_GREEN);
    //     Display::setCursor(10, y);
    //     Display::print("Update available:");
    //     Display::setCursor(120, y);
    //     Display::print(latestVersion);
    // }
    // 
    // if (isUpdating) {
    //     y += 30;
    //     Display::setTextColor(TFT_CYAN);
    //     Display::setCursor(10, y);
    //     Display::print("Updating...");
    //     
    //     // Progress bar
    //     y += 20;
    //     Display::fillRect(10, y, updateProgress * 3, 15, TFT_BLUE);
    //     Display::drawRect(10, y, 300, 15, TFT_WHITE);
    // }
}

void MenuWiFiOTA::drawControlButtons() {
    // Display module not yet implemented
    // int y = 250;
    // 
    // // Connect/Disconnect button
    // if (isConnected()) {
    //     Display::fillRoundRect(10, y, 140, 40, 5, TFT_ORANGE);
    //     Display::setTextColor(TFT_WHITE);
    //     Display::setCursor(25, y + 12);
    //     Display::print("Disconnect");
    // } else {
    //     Display::fillRoundRect(10, y, 140, 40, 5, TFT_GREEN);
    //     Display::setTextColor(TFT_WHITE);
    //     Display::setCursor(35, y + 12);
    //     Display::print("Connect");
    // }
    // 
    // // Check Updates button
    // Display::fillRoundRect(160, y, 140, 40, 5, TFT_BLUE);
    // Display::setTextColor(TFT_WHITE);
    // Display::setCursor(165, y + 12);
    // Display::print("Check Updates");
    // 
    // // Back button
    // y = 300;
    // Display::fillRoundRect(10, y, 140, 40, 5, TFT_RED);
    // Display::setTextColor(TFT_WHITE);
    // Display::setCursor(50, y + 12);
    // Display::print("Back");
}

bool MenuWiFiOTA::handleTouch(int16_t x, int16_t y) {
    // Connect/Disconnect button
    if (y >= 250 && y <= 290) {
        if (x >= 10 && x <= 150) {
            if (isConnected()) {
                disconnectWiFi();
            } else {
                connectWiFi();
            }
            return true;
        }
        
        // Check Updates button
        if (x >= 160 && x <= 300) {
            checkForUpdates();
            return true;
        }
    }
    
    // Back button
    if (y >= 300 && y <= 340 && x >= 10 && x <= 150) {
        return false; // Exit menu
    }
    
    return true; // Stay in menu
}

void MenuWiFiOTA::connectWiFi() {
    auto config = ConfigStorage::getCurrentConfig();
    
    // Use credentials from config or default
    WiFi.begin("SSID_FROM_CONFIG", "PASSWORD_FROM_CONFIG");
    
    // Audio::playAlert(Audio::ALERT_BEEP);  // Audio module not implemented yet
}

void MenuWiFiOTA::disconnectWiFi() {
    WiFi.disconnect();
    // Audio::playAlert(Audio::ALERT_BEEP);  // Audio module not implemented yet
}

void MenuWiFiOTA::checkForUpdates() {
    if (!isConnected()) {
        // Audio::playAlert(Audio::ALERT_ERROR);  // Audio module not implemented yet
        return;
    }
    
    // TODO: Query GitHub releases or update server
    // For now, simulate check
    // Audio::playAlert(Audio::ALERT_BEEP);  // Audio module not implemented yet
    
    // Example: updateAvailable = true; latestVersion = "v2.0.1";
}

void MenuWiFiOTA::installUpdate() {
    // Safety checks
    if (!updateAvailable) return;
    if (isUpdating) return;
    
    // TODO: Check car is stopped
    // TODO: Check battery > 50%
    
    isUpdating = true;
    // Audio::playAlert(Audio::ALERT_BEEP);  // Audio module not implemented yet
    
    // ArduinoOTA will handle the actual update
}

bool MenuWiFiOTA::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

int MenuWiFiOTA::getRSSI() {
    return WiFi.RSSI();
}

String MenuWiFiOTA::getSSID() {
    return WiFi.SSID();
}

String MenuWiFiOTA::getIP() {
    return WiFi.localIP().toString();
}

String MenuWiFiOTA::getCurrentVersion() {
    return "v1.0.0"; // TODO: Get from firmware
}

String MenuWiFiOTA::getLatestVersion() {
    return latestVersion;
}
