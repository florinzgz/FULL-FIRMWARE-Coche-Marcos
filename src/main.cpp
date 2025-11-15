#include <Arduino.h>

// Configuración
#include "pins.h"
#include "settings.h"

// Núcleo
#include "system.h"
#include "storage.h"
#include "logger.h"
#include "wifi_manager.h"
#include "watchdog.h"
#include "i2c_recovery.h"
#include "bluetooth_controller.h"

// Entradas
#include "pedal.h"
#include "steering.h"
#include "buttons.h"
#include "shifter.h"

// Sensores
#include "current.h"
#include "temperature.h"
#include "wheels.h"

// Control
#include "relays.h"
#include "traction.h"
#include "steering_motor.h"

// Advanced Safety Systems
#include "abs_system.h"
#include "tcs_system.h"
#include "regen_ai.h"

// HUD y Audio
#include "hud.h"
#include "hud_manager.h"
#include "car_sensors.h"
#include "dfplayer.h"
#include "queue.h"
#include "alerts.h"

// Utils
#include "debug.h"
#include "filters.h"
#include "math_utils.h"

void setup() {
    Serial.begin(115200);
    delay(500);

    Debug::setLevel(2); // nivel DEBUG

    // --- Inicialización básica ---
    System::init();
    Storage::init();
    Logger::init();
    
    // CRITICAL: Initialize Watchdog and I²C Recovery FIRST
    Watchdog::init();
    I2CRecovery::init();
    
    // Initialize WiFi and OTA (before sensors for telemetry)
    WiFiManager::init();
    
    Relays::init();
    
    // Initialize unified sensor reader
    CarSensors::init();
    
    // Initialize advanced HUD manager
    HUDManager::init();
    
    Audio::DFPlayer::init();
    Audio::AudioQueue::init();

    Sensors::initCurrent();
    Sensors::initTemperature();
    Sensors::initWheels();

    Pedal::init();
    Steering::init();
    Buttons::init();
    Shifter::init();

    Traction::init();
    SteeringMotor::init();

    // --- Advanced Safety Systems ---
    ABSSystem::init();
    TCSSystem::init();
    RegenAI::init();
    
    // --- Bluetooth Emergency Override Controller ---
    BluetoothController::init();

    // --- Logo de arranque ---
    HUDManager::showLogo();
    Alerts::play(Audio::AUDIO_INICIO);
    delay(2000);

    // --- Chequeo rápido ---
    auto health = System::selfTest();
    if (health.ok) {
        Steering::center();
        HUDManager::showReady();
        Relays::enablePower();
        Alerts::play(Audio::AUDIO_MODULO_OK);
    } else {
        HUDManager::showError("System check failed");
        Alerts::play(Audio::AUDIO_ERROR_GENERAL);

        // Opcional: imprimir detalle de qué falló
        Serial.println("---- SELFTEST FAIL ----");
        Serial.printf("Steering OK: %s\n", health.steeringOK ? "YES" : "NO");
        Serial.printf("Current OK: %s\n", health.currentOK ? "YES" : "NO");
        Serial.printf("Temps OK: %s\n", health.tempsOK ? "YES" : "NO");
        Serial.printf("Wheels OK: %s\n", health.wheelsOK ? "YES" : "NO");
    }
    
    // Show advanced dashboard
    HUDManager::showMenu(MenuType::DASHBOARD);
}

void loop() {
    static uint32_t lastHudUpdate = 0;
    const uint32_t HUD_UPDATE_INTERVAL = 33; // 30 FPS (~33ms per frame)
    
    uint32_t now = millis();
    
    // CRITICAL: Feed watchdog at start of every loop iteration
    Watchdog::feed();
    
    // PRIORITY 1: Bluetooth Emergency Override (HIGHEST PRIORITY)
    BluetoothController::update();
    
    // Entradas
    Pedal::update();
    Steering::update();
    Buttons::update();
    Shifter::update();

    // Sensores
    Sensors::updateCurrent();
    Sensors::updateTemperature();
    Sensors::updateWheels();

    // Control
    Traction::setDemand(Pedal::get().percent);
    Traction::update();
    SteeringMotor::setDemandAngle(Steering::get().angleDeg);
    SteeringMotor::update();
    Relays::update();

    // Advanced Safety Systems
    ABSSystem::update();
    TCSSystem::update();
    RegenAI::update();

    // HUD - Update at fixed 30 FPS for fluid rendering
    if (now - lastHudUpdate >= HUD_UPDATE_INTERVAL) {
        lastHudUpdate = now;
        
        // Read all car sensors and update HUD
        CarData data = CarSensors::readAll();
        HUDManager::updateCarData(data);
        HUDManager::update();
    }

    // Audio
    Audio::AudioQueue::update();

    // WiFi and OTA
    WiFiManager::update();

    // Sistema
    System::update();
    // Logger::update(); // Logger no tiene método update

    delay(10); // ~100 Hz main loop, HUD updates at 30 Hz internally
}