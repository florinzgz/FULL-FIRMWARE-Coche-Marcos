// Temperature sensor implementation with mutex protection
// This file handles temperature sensor readings with thread-safe access

#include "temperature.h"
#include "Logger.h"
#include <Arduino.h>

// Static member initialization
float Temperature::lastTemp[MAX_CHANNELS] = {0.0f};
SemaphoreHandle_t Temperature::mutex = NULL;

void Temperature::init() {
    // Initialize mutex for thread-safe access
    mutex = xSemaphoreCreateMutex();
    
    // Initialize temperature sensors
    for (int i = 0; i < MAX_CHANNELS; i++) {
        lastTemp[i] = 0.0f;
    }
    
    Logger::info("Temperature sensors initialized");
}

float Temperature::read(uint8_t channel) {
    if (channel >= MAX_CHANNELS) {
        Logger::error("Invalid temperature channel");
        return 0.0f;
    }
    
    float temp = 0.0f;
    
    // 🔒 MEJORA: Mutex con timeout para evitar deadlocks
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        // Leer temperatura del sensor
        temp = readSensor(channel);
        lastTemp[channel] = temp;
        xSemaphoreGive(mutex);
    } else {
        // En caso de timeout, retornar último valor sin mutex (mejor que bloquear)
        // 🔒 MEJORA: Throttled logging para evitar spam en logs
        static uint32_t lastTimeoutLog = 0;
        if (millis() - lastTimeoutLog > 5000) {  // Log cada 5 segundos máximo
            Logger::warn("Temperature: read mutex timeout");
            lastTimeoutLog = millis();
        }
        temp = lastTemp[channel];
    }
    
    return temp;
}

float Temperature::readSensor(uint8_t channel) {
    // Implementación específica del sensor
    // Esto es un placeholder - implementar según el hardware real
    return analogRead(channel) * 0.1f;
}
