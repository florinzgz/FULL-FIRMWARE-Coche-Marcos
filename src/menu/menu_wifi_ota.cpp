#include "menu_wifi_ota.h"
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "config_storage.h"
#include "version.h"        // 🔒 v2.10.2: Firmware version
#include "car_sensors.h"    // 🔒 v2.10.2: Para verificar estado del vehículo
#include "logger.h"         // 🔒 v2.10.2: Para logging
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
        Logger::warn("OTA: No se puede verificar actualizaciones - WiFi no conectado");
        // Audio::playAlert(Audio::ALERT_ERROR);  // Audio module not implemented yet
        return;
    }
    
    // 🔒 v2.10.2: Implementación básica de verificación de actualizaciones
    // En una implementación completa, esto haría una petición HTTP a GitHub Releases
    // o a un servidor de actualizaciones personalizado.
    // 
    // Ejemplo de implementación futura con HTTPClient:
    /*
    #include <HTTPClient.h>
    HTTPClient http;
    const char* updateURL = "https://api.github.com/repos/florinzgz/FULL-FIRMWARE-Coche-Marcos/releases/latest";
    
    http.begin(updateURL);
    http.addHeader("User-Agent", "ESP32-Car-OTA");
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        // Parse JSON para obtener "tag_name" (versión)
        // Comparar con FIRMWARE_VERSION actual
        // Si hay nueva versión: updateAvailable = true; latestVersion = tag_name;
    }
    
    http.end();
    */
    
    Logger::info("OTA: Verificación de actualizaciones (placeholder)");
    Logger::info("OTA: Versión actual: %s", FIRMWARE_VERSION);
    
    // Por ahora, simular que no hay actualizaciones disponibles
    // En producción, implementar la lógica HTTP anterior
    updateAvailable = false;
    latestVersion = "v" FIRMWARE_VERSION;  // Versión actual
    
    // Audio::playAlert(Audio::ALERT_BEEP);  // Audio module not implemented yet
    
    // Ejemplo para testing (descomentar para simular actualización disponible):
    // updateAvailable = true; 
    // latestVersion = "v2.11.0";
}

void MenuWiFiOTA::installUpdate() {
    // Safety checks
    if (!updateAvailable) {
        Logger::warn("OTA: No hay actualización disponible");
        return;
    }
    
    if (isUpdating) {
        Logger::warn("OTA: Actualización ya en progreso");
        return;
    }
    
    // 🔒 v2.10.2: Verificar que el vehículo esté detenido
    CarData data = CarSensors::readCritical();
    if (data.speed > 0.5f) {  // Tolerancia de 0.5 km/h para ruido de sensores
        Logger::error("OTA: ABORTADO - Vehículo en movimiento (velocidad: %.1f km/h)", data.speed);
        // Audio::playAlert(Audio::ALERT_ERROR);  // TODO: cuando audio esté disponible
        return;
    }
    
    // 🔒 v2.10.2: Verificar que el vehículo esté en PARK
    if (data.gear != GearPosition::PARK) {
        Logger::error("OTA: ABORTADO - Vehículo no está en PARK");
        // Audio::playAlert(Audio::ALERT_ERROR);  // TODO: cuando audio esté disponible
        return;
    }
    
    // 🔒 v2.10.2: Verificar nivel de batería > 50%
    if (data.batteryPercent < 50) {
        Logger::error("OTA: ABORTADO - Batería insuficiente (%d%%, requiere >50%%)", 
                     data.batteryPercent);
        // Audio::playAlert(Audio::ALERT_ERROR);  // TODO: cuando audio esté disponible
        return;
    }
    
    // Todas las verificaciones pasaron, proceder con actualización
    Logger::info("OTA: Iniciando actualización - Vehículo detenido, PARK, batería OK");
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
    // 🔒 v2.10.2: Retornar versión real del firmware desde version.h
    return "v" FIRMWARE_VERSION;
}

String MenuWiFiOTA::getLatestVersion() {
    return latestVersion;
}
