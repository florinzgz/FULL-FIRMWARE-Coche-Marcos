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
    // Non-blocking: allow Serial to initialize naturally
    unsigned long serialStart = millis();
    while (!Serial && (millis() - serialStart < 500)) {
        // Wait up to 500ms for Serial without blocking
    }

    Debug::setLevel(2); // nivel DEBUG

    // --- Inicialización básica ---
    System::init();
    Storage::init();
    Logger::init();
    
#ifdef STANDALONE_DISPLAY
    Logger::info("🧪 STANDALONE_DISPLAY MODE: Skipping sensor initialization");
    
    // Initialize only display components
    HUDManager::init();
    
    // Show logo briefly
    HUDManager::showLogo();
    delay(1500);
    
    // Go directly to dashboard
    HUDManager::showMenu(MenuType::DASHBOARD);
    
    // CRITICAL: Initialize CarData with simulated values BEFORE calling update()
    // Otherwise, HUD::update() may use uninitialized data causing garbage values or crashes
    CarData initialData;
    initialData.speed = 12.0f;
    initialData.rpm = 850;  // Note: RPM is placeholder, proportional to speed
    initialData.batteryVoltage = 24.5f;
    initialData.batteryCurrent = 2.3f;
    initialData.batteryPercent = 87;
    initialData.motorTemp = 42.0f;
    initialData.batteryTemp = 38.0f;
    initialData.roll = 0.5f;
    initialData.pitch = -1.2f;
    initialData.pedalPercent = 0;
    initialData.steeringAngle = 0.0f;
    initialData.gear = 1;
    initialData.wheelFL_rpm = 15.0f;
    initialData.wheelFR_rpm = 15.0f;
    initialData.wheelRL_rpm = 15.0f;
    initialData.wheelRR_rpm = 15.0f;
    initialData.wheelFL_temp = 0.0f;
    initialData.wheelFR_temp = 0.0f;
    initialData.wheelRL_temp = 0.0f;
    initialData.wheelRR_temp = 0.0f;
    initialData.wheelFL_effort = 0.0f;
    initialData.wheelFR_effort = 0.0f;
    initialData.wheelRL_effort = 0.0f;
    initialData.wheelRR_effort = 0.0f;
    initialData.absActive = false;
    initialData.tcsActive = false;
    initialData.fourWDActive = true;
    initialData.lightsOn = false;
    initialData.musicPlaying = false;
    
    HUDManager::updateCarData(initialData);
    HUDManager::update();  // Render first frame immediately
    
    Logger::info("🧪 STANDALONE MODE: Dashboard active with simulated values");
    
#else
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
    // Non-blocking: logo display time handled by first loop iterations
    // Removed blocking delay(2000) - logo will show during system checks

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
#endif
}

void loop() {
    static uint32_t lastHudUpdate = 0;
    const uint32_t HUD_UPDATE_INTERVAL = 33; // 30 FPS (~33ms per frame)
    
    uint32_t now = millis();
    
#ifdef STANDALONE_DISPLAY
    // 🧪 STANDALONE MODE: Update HUD with simulated values
    if (now - lastHudUpdate >= HUD_UPDATE_INTERVAL) {
        lastHudUpdate = now;
        
        // Create simulated car data
        CarData data;
        data.speed = 12.0f;                    // 12 km/h
        data.rpm = 850;                        // Idle RPM
        data.batteryVoltage = 24.5f;           // 24.5V
        data.batteryCurrent = 2.3f;            // 2.3A
        data.batteryPercent = 87;              // 87%
        data.motorTemp = 42.0f;                // 42°C
        data.batteryTemp = 38.0f;              // 38°C
        data.roll = 0.5f;                      // 0.5° roll
        data.pitch = -1.2f;                    // -1.2° pitch
        data.pedalPercent = 0;                 // No pedal input
        data.steeringAngle = 0.0f;             // Centered
        data.gear = 1;                         // Gear 1
        data.wheelFL_rpm = 15.0f;              // Front-left wheel RPM
        data.wheelFR_rpm = 15.0f;              // Front-right wheel RPM
        data.wheelRL_rpm = 15.0f;              // Rear-left wheel RPM
        data.wheelRR_rpm = 15.0f;              // Rear-right wheel RPM
        data.wheelFL_temp = 0.0f;              // Temperature disabled (will show "--")
        data.wheelFR_temp = 0.0f;
        data.wheelRL_temp = 0.0f;
        data.wheelRR_temp = 0.0f;
        data.wheelFL_effort = 0.0f;            // Effort disabled (will show "--")
        data.wheelFR_effort = 0.0f;
        data.wheelRL_effort = 0.0f;
        data.wheelRR_effort = 0.0f;
        data.absActive = false;
        data.tcsActive = false;
        data.fourWDActive = true;              // 4x4 active
        data.lightsOn = false;
        data.musicPlaying = false;
        
        HUDManager::updateCarData(data);
        HUDManager::update();
    }
    
    // Minimal loop - no sensors, no control systems
    delay(1);  // Prevent watchdog issues in standalone mode
    
#else
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

    // Non-blocking main loop - no delay needed
    // Loop runs as fast as possible, modules control their own update rates:
    // - HUD: 30 FPS (33ms) via internal timing
    // - Sensors: 20-50 Hz via updateCurrent/Temperature/Wheels timing
    // - Control modules: update every iteration (non-blocking)
#endif
}