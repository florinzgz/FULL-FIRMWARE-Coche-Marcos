#include <Arduino.h>
#include "config.h"
#include "system.h"
#include "storage.h"
#include "watchdog.h"
#include "logger.h"
#include "network.h"
#include "mqtt.h"
#include "sensors.h"
#include "actuators.h"
#include "dashboard.h"

// Forward declarations
void initializeSystem();
void mainLoop();

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=================================");
    Serial.println("FULL FIRMWARE - Coche Marcos");
    Serial.println("Version: " FIRMWARE_VERSION);
    Serial.println("=================================");
    
    // Critical initialization sequence
    System::init();
    Storage::init();
    Watchdog::init();
    Watchdog::feed();
    Logger::init();
    Logger::info("Boot sequence started");
    Watchdog::feed();
    
    initializeSystem();
}

void loop() {
    mainLoop();
}

void initializeSystem() {
    Logger::info("Initializing system components...");
    Watchdog::feed();
    
    // Initialize network
    Network::init();
    Watchdog::feed();
    
    // Initialize MQTT
    MQTT::init();
    Watchdog::feed();
    
    // Initialize sensors
    Sensors::init();
    Watchdog::feed();
    
    // Initialize actuators
    Actuators::init();
    Watchdog::feed();
    
    // Initialize dashboard
    Dashboard::init();
    Watchdog::feed();
    
    Logger::info("System initialization complete");
}

void mainLoop() {
    Watchdog::feed();
    
    // Update network connection
    Network::update();
    
    // Update MQTT connection
    MQTT::update();
    
    // Read sensors
    Sensors::update();
    
    // Update actuators
    Actuators::update();
    
    // Update dashboard
    Dashboard::update();
    
    Watchdog::feed();
    delay(10);
}