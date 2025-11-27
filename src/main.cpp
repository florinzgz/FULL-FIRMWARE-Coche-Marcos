#include <Arduino.h>
#include <math.h>  // For sinf() in demo mode

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
#include "telemetry.h"           // 🆕 v2.8.0: Sistema de telemetría

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
    // 🔒 v2.4.2: Serial no es crítico - continuar sin espera bloqueante
    // Si Serial no está listo, los logs simplemente no se mostrarán
    // pero el sistema no se bloqueará

    Debug::setLevel(2); // nivel DEBUG

    // --- Inicialización básica ---
    System::init();
    Storage::init();
    Logger::init();
    
#ifdef STANDALONE_DISPLAY
    Logger::info("🧪 STANDALONE_DISPLAY MODE: Skipping sensor initialization");
    
    // Initialize only display components
    HUDManager::init();
    
    // Show logo briefly with non-blocking timing
    HUDManager::showLogo();
    unsigned long logoStart = millis();
    while (millis() - logoStart < 1500) { 
        // Keep main loop responsive during logo display
        yield(); // Allow background tasks to run
    }
    
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
    initialData.motorTemp[0] = 42.0f;
    initialData.motorTemp[1] = 42.0f;
    initialData.motorTemp[2] = 42.0f;
    initialData.motorTemp[3] = 42.0f;
    initialData.ambientTemp = 25.0f;
    initialData.controllerTemp = 38.0f;
    initialData.throttlePercent = 0;
    initialData.pedalPosition = 50.0f;  // Simulated pedal at 50% for standalone mode
    initialData.pedalPercent = 50.0f;   // Simulated pedal percentage for HUD pedal bar
    initialData.steeringAngle = 0.0f;
    initialData.gear = GearPosition::PARK;
    initialData.motorCurrent[0] = 2.0f;
    initialData.motorCurrent[1] = 2.0f;
    initialData.motorCurrent[2] = 2.0f;
    initialData.motorCurrent[3] = 2.0f;
    initialData.steeringCurrent = 0.5f;
    initialData.batteryPower = initialData.batteryVoltage * initialData.batteryCurrent;
    initialData.odoTotal = 0.0f;
    initialData.odoTrip = 0.0f;
    initialData.encoderValue = 0.0f;
    initialData.status.fourWheelDrive = true;
    initialData.status.lights = false;
    initialData.status.parkingBrake = false;
    initialData.status.bluetooth = false;
    initialData.status.wifi = false;
    initialData.status.warnings = false;
    
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
    
    // --- Telemetry System ---
    Telemetry::init();           // 🆕 v2.8.0: Sistema de telemetría
    
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
    // 🧪 STANDALONE MODE: Update HUD with animated simulated values
    if (now - lastHudUpdate >= HUD_UPDATE_INTERVAL) {
        lastHudUpdate = now;
        
        // Demo time for animation (cycles every 30 seconds)
        static uint32_t demoStartTime = millis();
        float demoTime = (float)((now - demoStartTime) % 30000) / 1000.0f;
        float wave = (sinf(demoTime * 0.4f) + 1.0f) / 2.0f;  // 0 to 1 oscillation
        float steerWave = sinf(demoTime * 0.5f);             // -1 to 1 for steering
        
        // Create animated simulated car data
        CarData data;
        data.speed = 5.0f + wave * 45.0f;            // 5 to 50 km/h
        data.rpm = 600 + (int)(wave * 2400.0f);      // 600 to 3000 RPM
        data.batteryVoltage = 24.0f + wave * 1.0f;   // 24.0 to 25.0V variation
        data.batteryCurrent = 1.0f + wave * 8.0f;    // 1 to 9A (proportional to speed)
        data.batteryPercent = 85 + (int)(wave * 10); // 85% to 95%
        data.motorTemp[0] = 40.0f + wave * 15.0f;    // Motor FL temp
        data.motorTemp[1] = 40.0f + wave * 15.0f;    // Motor FR temp
        data.motorTemp[2] = 38.0f + wave * 12.0f;    // Motor RL temp
        data.motorTemp[3] = 38.0f + wave * 12.0f;    // Motor RR temp
        data.ambientTemp = 25.0f;                    // 25°C ambient (static)
        data.controllerTemp = 35.0f + wave * 10.0f;  // 35-45°C controller
        data.motorCurrent[0] = 1.0f + wave * 4.0f;   // Motor FL current
        data.motorCurrent[1] = 1.0f + wave * 4.0f;   // Motor FR current
        data.motorCurrent[2] = 1.0f + wave * 3.0f;   // Motor RL current
        data.motorCurrent[3] = 1.0f + wave * 3.0f;   // Motor RR current
        data.steeringCurrent = 0.2f + fabsf(steerWave) * 1.0f;  // Steering current (fabsf for float)
        data.pedalPercent = 10.0f + wave * 70.0f;    // 10% to 80% pedal
        data.steeringAngle = steerWave * 15.0f;      // -15° to +15°
        data.gear = (data.speed > 3.0f) ? GearPosition::DRIVE1 : GearPosition::PARK;
        data.batteryPower = data.batteryVoltage * data.batteryCurrent;
        data.odoTotal = 1234.5f + (now - demoStartTime) / 60000.0f;  // Slowly increment
        data.odoTrip = 56.7f + (now - demoStartTime) / 120000.0f;
        data.encoderValue = 2048.0f + steerWave * 500.0f;  // Encoder follows steering
        
        // Animated SystemStatus
        data.status.fourWheelDrive = true;                           // 4x4 active
        data.status.lights = (demoTime > 5.0f && demoTime < 20.0f);  // Lights cycle
        data.status.parkingBrake = (data.speed < 10.0f);
        data.status.wifi = false;                                    // WiFi off
        data.status.bluetooth = false;                               // Bluetooth off
        data.status.warnings = (data.motorTemp[0] > 52.0f) ? 1 : 0;  // Temp warning
        
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
    
    // Telemetry
    Telemetry::update();         // 🆕 v2.8.0: Sistema de telemetría

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