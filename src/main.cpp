#include <Arduino.h>
#include <math.h> // For sinf() in demo mode

// Configuración
#include "pins.h"
#include "settings.h"
#include "version.h" // 🔒 v2.10.2: Firmware version information

// ============================================================================
// Demo Mode Animation Constants (STANDALONE_DISPLAY)
// ============================================================================
#ifdef STANDALONE_DISPLAY
// Timing
static const uint32_t DEMO_CYCLE_MS =
    30000; // Full animation cycle duration (30 seconds)

// Wave frequencies (radians per second factor)
static const float DEMO_SPEED_WAVE_FREQ =
    0.4f; // Speed/RPM oscillation frequency
static const float DEMO_STEER_WAVE_FREQ =
    0.5f; // Steering oscillation frequency

// Speed and RPM ranges
static const float DEMO_MIN_SPEED = 5.0f;    // Minimum speed (km/h)
static const float DEMO_SPEED_RANGE = 45.0f; // Speed variation range (km/h)
static const float DEMO_MIN_RPM = 600.0f;    // Minimum RPM
static const float DEMO_RPM_RANGE = 2400.0f; // RPM variation range

// Pedal and steering
static const float DEMO_MIN_PEDAL = 10.0f;   // Minimum pedal position (%)
static const float DEMO_PEDAL_RANGE = 70.0f; // Pedal variation range (%)
static const float DEMO_MAX_STEER_ANGLE =
    15.0f; // Maximum steering angle (degrees)
static const float DEMO_ENCODER_RANGE = 500.0f; // Encoder value variation range

// Battery
static const float DEMO_MIN_VOLTAGE = 24.0f;  // Minimum battery voltage (V)
static const float DEMO_VOLTAGE_RANGE = 1.0f; // Voltage variation range (V)
static const float DEMO_MIN_CURRENT = 1.0f;   // Minimum battery current (A)
static const float DEMO_CURRENT_RANGE = 8.0f; // Current variation range (A)

// Temperatures
static const float DEMO_MIN_FRONT_TEMP = 40.0f; // Minimum front motor temp (°C)
static const float DEMO_FRONT_TEMP_RANGE =
    15.0f; // Front temp variation range (°C)
static const float DEMO_MIN_REAR_TEMP = 38.0f; // Minimum rear motor temp (°C)
static const float DEMO_REAR_TEMP_RANGE =
    12.0f; // Rear temp variation range (°C)
static const float DEMO_MIN_CONTROLLER_TEMP =
    35.0f; // Minimum controller temp (°C)
static const float DEMO_CONTROLLER_TEMP_RANGE =
    10.0f; // Controller temp variation range (°C)

// Motor currents
static const float DEMO_MIN_FRONT_CURRENT =
    1.0f; // Minimum front motor current (A)
static const float DEMO_FRONT_CURRENT_RANGE =
    4.0f; // Front motor current variation (A)
static const float DEMO_MIN_REAR_CURRENT =
    1.0f; // Minimum rear motor current (A)
static const float DEMO_REAR_CURRENT_RANGE =
    3.0f; // Rear motor current variation (A)
static const float DEMO_MIN_STEER_CURRENT =
    0.2f; // Minimum steering current (A)
static const float DEMO_STEER_CURRENT_RANGE =
    1.0f; // Steering current variation (A)

// Feature timing (seconds within demo cycle)
static const float DEMO_LIGHTS_ON_TIME = 5.0f;   // Lights turn on at this time
static const float DEMO_LIGHTS_OFF_TIME = 20.0f; // Lights turn off at this time

// Thresholds
static const float DEMO_GEAR_CHANGE_SPEED =
    3.0f; // Speed threshold for P↔D1 gear change
static const float DEMO_TEMP_WARNING_THRESHOLD =
    52.0f; // Temp threshold for warning indicator
#endif

// Núcleo
#include "bluetooth_controller.h"
#include "i2c_recovery.h"
#include "logger.h"
#include "storage.h"
#include "system.h"
#include "telemetry.h" // 🆕 v2.8.0: Sistema de telemetría
#include "watchdog.h"
#include "wifi_manager.h"

// Entradas
#include "buttons.h"
#include "pedal.h"
#include "shifter.h"
#include "steering.h"

// Sensores
#include "current.h"
#include "temperature.h"
#include "wheels.h"

// Control
#include "relays.h"
#include "steering_motor.h"
#include "traction.h"

// Advanced Safety Systems
#include "abs_system.h"
#include "obstacle_detection.h"
#include "obstacle_safety.h"
#include "regen_ai.h"
#include "tcs_system.h"

// HUD y Audio
#include "alerts.h"
#include "car_sensors.h"
#include "dfplayer.h"
#include "hud.h"
#include "hud_manager.h"
#include "menu_hidden.h" // 🆕 v2.9.4: Para calibración táctil directa
#include "queue.h"

// Utils
#include "debug.h"
#include "filters.h"
#include "math_utils.h"

// Testing (conditionally included)
#if defined(ENABLE_FUNCTIONAL_TESTS) || defined(ENABLE_MEMORY_STRESS_TESTS) || \
    defined(ENABLE_HARDWARE_FAILURE_TESTS) || defined(ENABLE_WATCHDOG_TESTS)
#include "test_runner.h"
#endif

// ============================================================================
// Boot Timing Constants (Hardware requirements)
// ============================================================================
// These values are required by the ST7796S display controller
static constexpr uint32_t SERIAL_WAIT_MAX_MS =
    1000; // Max wait for Serial connection
static constexpr uint32_t SERIAL_POLL_INTERVAL_MS =
    50; // Poll interval for Serial ready
static constexpr uint32_t TFT_RESET_PULSE_MS =
    10; // Reset pulse width (hardware minimum 10µs, using 10ms for safety
        // margin)
static constexpr uint32_t TFT_RESET_RECOVERY_MS =
    50; // Post-reset recovery time (min 5ms, using 50ms for margin)

void setup() {
  // ========================================================================
  // 🔒 v2.8.1: CRITICAL EARLY BOOT DIAGNOSTICS
  // These must run FIRST to diagnose blank screen / no boot issues
  // ========================================================================

  // 1. Initialize Serial IMMEDIATELY for debugging
  Serial.begin(115200);

  // 2. Wait briefly for Serial to stabilize (but don't block forever)
  // This helps capture boot messages on freshly flashed devices
  uint32_t serialStart = millis();
  while (!Serial && (millis() - serialStart < SERIAL_WAIT_MAX_MS)) {
    delay(SERIAL_POLL_INTERVAL_MS);
  }

  // 3. First sign of life - print boot message
  Serial.println();
  Serial.println("========================================");
  Serial.printf("ESP32-S3 Car Control System %s\n", FIRMWARE_VERSION_FULL);
  Serial.println("========================================");
  Serial.printf("CPU Freq: %d MHz\n", getCpuFrequencyMhz());
  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
  
  // Check PSRAM availability (returns 0 if not present/enabled)
  size_t psramSize = ESP.getPsramSize();
  if (psramSize > 0) {
    Serial.printf("PSRAM: %d bytes (Free: %d bytes)\n", psramSize, ESP.getFreePsram());
  } else {
    Serial.println("PSRAM: Not available or not enabled");
  }
  
  Serial.printf("Stack high water mark: %d bytes\n",
                uxTaskGetStackHighWaterMark(NULL));
  Serial.printf("Configured loop stack: %d bytes\n", CONFIG_ARDUINO_LOOP_STACK_SIZE);
  Serial.printf("Configured main task stack: %d bytes\n", CONFIG_ESP_MAIN_TASK_STACK_SIZE);
  Serial.println("Boot sequence starting...");
  Serial.flush();

  // 4. CRITICAL: Enable TFT backlight IMMEDIATELY
  // This ensures the screen is not blank even if other init fails
  // User can see that the ESP32 is at least starting
  Serial.println("[BOOT] Enabling TFT backlight...");
  pinMode(PIN_TFT_BL, OUTPUT);
  digitalWrite(PIN_TFT_BL, HIGH); // Backlight ON
  Serial.println("[BOOT] Backlight enabled on GPIO42");

  // 5. Initialize TFT reset to ensure display is ready
  // ST7796S datasheet requires min 10µs reset pulse, 5ms recovery.
  // We intentionally use millisecond delays (10ms pulse, 50ms recovery) for
  // extra safety margin and robustness.
  Serial.println("[BOOT] Resetting TFT display...");
  pinMode(PIN_TFT_RST, OUTPUT);
  digitalWrite(PIN_TFT_RST, LOW);
  delay(TFT_RESET_PULSE_MS); // Reset pulse (intentional extra margin)
  digitalWrite(PIN_TFT_RST, HIGH);
  delay(TFT_RESET_RECOVERY_MS); // Wait for display to initialize (intentional
                                // extra margin)
  Serial.println("[BOOT] TFT reset complete");

  // 🔒 v2.4.2: Serial no es crítico - continuar sin espera bloqueante
  // Si Serial no está listo, los logs simplemente no se mostrarán
  // pero el sistema no se bloqueará

  Debug::setLevel(2); // nivel DEBUG
  Serial.println("[BOOT] Debug level set to 2");

  // --- Inicialización básica ---
  Serial.println("[BOOT] Initializing System...");
  System::init();
  Serial.printf("[STACK] After System::init - Free: %d bytes\n",
                uxTaskGetStackHighWaterMark(NULL));

  Serial.println("[BOOT] Initializing Storage...");
  Storage::init();
  Serial.printf("[STACK] After Storage::init - Free: %d bytes\n",
                uxTaskGetStackHighWaterMark(NULL));
  
  // 🔒 v2.10.5: Feed watchdog early to prevent boot loop during initialization
  // Watchdog has 10s timeout - must feed during long setup() to prevent reset
  Serial.println("[BOOT] Initializing Watchdog early...");
  Watchdog::init();
  Watchdog::feed();
  Serial.println("[BOOT] Watchdog initialized and fed");

  // 🔒 CRITICAL FIX: Load configuration from EEPROM
  // Without this, cfg.displayBrightness is uninitialized (0), causing screen to
  // turn off when HUDManager reconfigures the backlight to PWM mode
  Serial.println("[BOOT] Loading configuration from EEPROM...");
  if (Storage::isCorrupted()) {
    Serial.println(
        "[BOOT] EEPROM corrupted or uninitialized - applying defaults");
    Storage::defaults(cfg);
    Storage::save(cfg);
  } else {
    Storage::load(cfg);
  }
  Serial.printf("[BOOT] Display brightness loaded: %d\n",
                cfg.displayBrightness);

  // 🔒 v2.9.9: Additional safety check - ensure brightness is valid after load
  // This is a failsafe in case EEPROM data is corrupted or from an incompatible
  // version
  if (cfg.displayBrightness == 0 || cfg.displayBrightness > 255) {
    Serial.printf("[BOOT] WARNING: Invalid brightness value (%d), forcing to "
                  "default (%d)\n",
                  cfg.displayBrightness, DISPLAY_BRIGHTNESS_DEFAULT);
    cfg.displayBrightness = DISPLAY_BRIGHTNESS_DEFAULT;
    Storage::save(cfg); // Save the corrected value
    Serial.println("[BOOT] Brightness corrected and saved to EEPROM");
  }

  Serial.println("[BOOT] Initializing Logger...");
  Logger::init();
  Serial.printf("[STACK] After Logger::init - Free: %d bytes\n",
                uxTaskGetStackHighWaterMark(NULL));
  Watchdog::feed();  // Feed watchdog after logger init

#ifdef STANDALONE_DISPLAY
  Serial.println(
      "[BOOT] STANDALONE_DISPLAY MODE: Skipping sensor initialization");
  Logger::info("🧪 STANDALONE_DISPLAY MODE: Skipping sensor initialization");
  
  // 🔒 v2.10.5: Initialize watchdog in standalone mode too
  Watchdog::feed();  // Feed watchdog in standalone mode

  // Initialize only display components
  Serial.println("[BOOT] Initializing HUD Manager (display only)...");
  HUDManager::init();
  Watchdog::feed();  // Feed after HUD init

  // Show logo briefly with non-blocking timing
  Serial.println("[BOOT] Showing logo...");
  HUDManager::showLogo();
  unsigned long logoStart = millis();
  while (millis() - logoStart < 1500) {
    // Keep main loop responsive during logo display
    Watchdog::feed();  // Feed watchdog during logo display
    yield(); // Allow background tasks to run
    delay(10); // Small delay to prevent tight loop
  }

  // Go directly to dashboard
  HUDManager::showMenu(MenuType::DASHBOARD);
  Watchdog::feed();  // Feed after showing dashboard

  // CRITICAL: Initialize CarData with simulated values BEFORE calling update()
  // Otherwise, HUD::update() may use uninitialized data causing garbage values
  // or crashes
  CarData initialData;
  initialData.speed = 12.0f;
  initialData.rpm = 850; // Note: RPM is placeholder, proportional to speed
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
  initialData.pedalPosition =
      50.0f; // Simulated pedal at 50% for standalone mode
  initialData.pedalPercent =
      50.0f; // Simulated pedal percentage for HUD pedal bar
  initialData.steeringAngle = 0.0f;
  initialData.gear = GearPosition::PARK;
  initialData.motorCurrent[0] = 2.0f;
  initialData.motorCurrent[1] = 2.0f;
  initialData.motorCurrent[2] = 2.0f;
  initialData.motorCurrent[3] = 2.0f;
  initialData.steeringCurrent = 0.5f;
  initialData.batteryPower =
      initialData.batteryVoltage * initialData.batteryCurrent;
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
  HUDManager::update(); // Render first frame immediately

  Logger::info("🧪 STANDALONE MODE: Dashboard active with simulated values");
  Serial.println("[BOOT] STANDALONE MODE: Setup complete!");

#else
  // ========================================================================
  // 🔒 v2.8.1: FULL INITIALIZATION MODE WITH DIAGNOSTIC OUTPUT
  // Each step prints to Serial so we can identify where hangs occur
  // ========================================================================

  Serial.println("[BOOT] FULL MODE: Starting hardware initialization...");

  // CRITICAL: Watchdog already initialized above - feed it regularly
  Watchdog::feed();
  
  Serial.println("[BOOT] Initializing I2C Recovery...");
  I2CRecovery::init();
  Serial.printf("[STACK] After I2CRecovery::init - Free: %d bytes\n",
                uxTaskGetStackHighWaterMark(NULL));
  Watchdog::feed();  // Feed after I2C init

  // Initialize WiFi and OTA (before sensors for telemetry)
  Serial.println("[BOOT] Initializing WiFi Manager...");
  WiFiManager::init();
  Serial.printf("[STACK] After WiFiManager::init - Free: %d bytes\n",
                uxTaskGetStackHighWaterMark(NULL));
  Watchdog::feed();  // Feed after WiFi init (non-blocking but important)

  Serial.println("[BOOT] Initializing Relays...");
  Relays::init();
  Serial.printf("[STACK] After Relays::init - Free: %d bytes\n",
                uxTaskGetStackHighWaterMark(NULL));
  Watchdog::feed();  // Feed after relays init

  // Initialize unified sensor reader
  Serial.println("[BOOT] Initializing Car Sensors...");
  CarSensors::init();
  Serial.printf("[STACK] After CarSensors::init - Free: %d bytes\n",
                uxTaskGetStackHighWaterMark(NULL));
  Watchdog::feed();  // Feed after car sensors init

  // Initialize advanced HUD manager
  Serial.println("[BOOT] Initializing HUD Manager...");
  HUDManager::init();
  Serial.printf("[STACK] After HUDManager::init - Free: %d bytes\n",
                uxTaskGetStackHighWaterMark(NULL));
  Watchdog::feed();  // Feed after HUD init

  Serial.println("[BOOT] Initializing DFPlayer Audio...");
  Audio::DFPlayer::init();
  Audio::AudioQueue::init();
  Serial.printf("[STACK] After Audio init - Free: %d bytes\n",
                uxTaskGetStackHighWaterMark(NULL));
  Watchdog::feed();  // Feed after audio init

  Serial.println("[BOOT] Initializing Current Sensors (INA226)...");
  Sensors::initCurrent();
  Serial.printf("[STACK] After Current Sensors - Free: %d bytes\n",
                uxTaskGetStackHighWaterMark(NULL));
  Watchdog::feed();  // Feed after current sensors init (I2C operations)

  Serial.println("[BOOT] Initializing Temperature Sensors (DS18B20)...");
  Sensors::initTemperature();
  Serial.printf("[STACK] After Temperature Sensors - Free: %d bytes\n",
                uxTaskGetStackHighWaterMark(NULL));
  Watchdog::feed();  // Feed after temperature sensors init

  Serial.println("[BOOT] Initializing Wheel Sensors...");
  Sensors::initWheels();
  Serial.printf("[STACK] After Wheel Sensors - Free: %d bytes\n",
                uxTaskGetStackHighWaterMark(NULL));
  Watchdog::feed();  // Feed after wheel sensors init

  Serial.println("[BOOT] Initializing Pedal...");
  Pedal::init();
  Serial.println("[BOOT] Initializing Steering...");
  Steering::init();
  Serial.println("[BOOT] Initializing Buttons...");
  Buttons::init();
  Serial.println("[BOOT] Initializing Shifter...");
  Shifter::init();
  Watchdog::feed();  // Feed after input devices init

  Serial.println("[BOOT] Initializing Traction...");
  Traction::init();
  Serial.println("[BOOT] Initializing Steering Motor...");
  SteeringMotor::init();
  Watchdog::feed();  // Feed after control systems init

  // --- Advanced Safety Systems ---
  Serial.println("[BOOT] Initializing ABS System...");
  ABSSystem::init();
  Serial.println("[BOOT] Initializing TCS System...");
  TCSSystem::init();
  Serial.println("[BOOT] Initializing Regen AI...");
  RegenAI::init();
  Watchdog::feed();  // Feed after safety systems init

  // --- Obstacle Detection System ---
  Serial.println("[BOOT] Initializing Obstacle Detection...");
  ObstacleDetection::init();
  Serial.printf("[STACK] After Obstacle Detection - Free: %d bytes\n",
                uxTaskGetStackHighWaterMark(NULL));
  Watchdog::feed();  // Feed after obstacle detection init (I2C intensive)

  Serial.println("[BOOT] Initializing Obstacle Safety...");
  ObstacleSafety::init();
  Watchdog::feed();  // Feed after obstacle safety init

  // --- Telemetry System ---
  Serial.println("[BOOT] Initializing Telemetry...");
  Telemetry::init(); // 🆕 v2.8.0: Sistema de telemetría
  Watchdog::feed();  // Feed after telemetry init

  // --- Bluetooth Emergency Override Controller ---
  Serial.println("[BOOT] Initializing Bluetooth Controller...");
  BluetoothController::init();
  Serial.printf("[STACK] After Bluetooth - Free: %d bytes\n",
                uxTaskGetStackHighWaterMark(NULL));
  Watchdog::feed();  // Feed after Bluetooth init

  Serial.println("[BOOT] All modules initialized. Starting self-test...");
  Watchdog::feed();  // Feed before self-test

  // --- Logo de arranque ---
  HUDManager::showLogo();
  Alerts::play(Audio::AUDIO_INICIO);
  // Non-blocking: logo display time handled by first loop iterations
  // Removed blocking delay(2000) - logo will show during system checks
  Watchdog::feed();  // Feed after showing logo

  // --- Chequeo rápido ---
  auto health = System::selfTest();
  Watchdog::feed();  // Feed after self-test
  if (health.ok) {
    Serial.println("[BOOT] Self-test PASSED!");
    Steering::center();
    HUDManager::showReady();
    Relays::enablePower();
    Alerts::play(Audio::AUDIO_MODULO_OK);
  } else {
    Serial.println("[BOOT] Self-test FAILED!");
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
  Serial.println("[BOOT] Setup complete! Entering main loop...");
#endif

  // ========================================================================
  // 🧪 PRE-DEPLOYMENT TESTING (if enabled)
  // ========================================================================
#if defined(ENABLE_FUNCTIONAL_TESTS) || defined(ENABLE_MEMORY_STRESS_TESTS) || \
    defined(ENABLE_HARDWARE_FAILURE_TESTS) || defined(ENABLE_WATCHDOG_TESTS)

  Serial.println("\n[BOOT] Pre-deployment testing enabled - running tests...");
  Logger::info("Starting pre-deployment test suite");

  // Run all enabled tests
  bool testsOk = TestRunner::runPreDeploymentTests();

  if (testsOk) {
    Serial.println("\n✅ ALL PRE-DEPLOYMENT TESTS PASSED");
    Serial.println("System ready for production deployment");
  } else {
    Serial.println("\n❌ PRE-DEPLOYMENT TESTS FAILED");
    Serial.println("⚠️  DO NOT DEPLOY - Fix issues and retest");
  }

  Serial.println("\n[BOOT] Test execution complete. System will continue "
                 "normal operation...");
  delay(3000); // Give user time to see test results
#endif
}

// ============================================================================
// 🆕 v2.9.4: Función para activar calibración táctil desde botón físico
// ============================================================================
// Esta función se llama cuando el usuario mantiene presionado el botón 4X4
// durante 5 segundos. Permite calibrar el touch sin necesidad de que funcione.
//
// Precondiciones:
// - El sistema debe estar completamente inicializado
// - TFT debe estar inicializado (HUDManager::init() ya ejecutado)
// - MenuHidden debe estar inicializado (MenuHidden::init() ya ejecutado)
//
// Comportamiento:
// - Cierra el menú oculto si está abierto
// - Cancela cualquier calibración en curso
// - Inicia la calibración táctil interactiva
// - Reproduce sonido de confirmación (AUDIO_MENU_OCULTO)
//
// Llamada desde: buttons.cpp al detectar presión de 5s en botón 4X4
void activateTouchCalibration() {
  Logger::info("activateTouchCalibration() llamada desde botón físico");
  MenuHidden::startTouchCalibrationDirectly();
}

void loop() {
  static uint32_t lastHudUpdate = 0;
  const uint32_t HUD_UPDATE_INTERVAL = 33; // 30 FPS (~33ms per frame)

  uint32_t now = millis();

#ifdef STANDALONE_DISPLAY
  // 🧪 STANDALONE MODE: Update HUD with animated simulated values
  if (now - lastHudUpdate >= HUD_UPDATE_INTERVAL) {
    lastHudUpdate = now;

    // Demo time for animation (uses named constants for timing)
    static uint32_t demoStartTime = millis();
    float demoTime = (float)((now - demoStartTime) % DEMO_CYCLE_MS) / 1000.0f;
    float wave = (sinf(demoTime * DEMO_SPEED_WAVE_FREQ) + 1.0f) /
                 2.0f; // 0 to 1 oscillation
    float steerWave =
        sinf(demoTime * DEMO_STEER_WAVE_FREQ); // -1 to 1 for steering

    // Create animated simulated car data using named constants
    CarData data;
    data.speed = DEMO_MIN_SPEED + wave * DEMO_SPEED_RANGE;
    data.rpm = DEMO_MIN_RPM + (int)(wave * DEMO_RPM_RANGE);
    data.batteryVoltage = DEMO_MIN_VOLTAGE + wave * DEMO_VOLTAGE_RANGE;
    data.batteryCurrent = DEMO_MIN_CURRENT + wave * DEMO_CURRENT_RANGE;
    data.batteryPercent = 85 + (int)(wave * 10);
    data.motorTemp[0] = DEMO_MIN_FRONT_TEMP + wave * DEMO_FRONT_TEMP_RANGE;
    data.motorTemp[1] = DEMO_MIN_FRONT_TEMP + wave * DEMO_FRONT_TEMP_RANGE;
    data.motorTemp[2] = DEMO_MIN_REAR_TEMP + wave * DEMO_REAR_TEMP_RANGE;
    data.motorTemp[3] = DEMO_MIN_REAR_TEMP + wave * DEMO_REAR_TEMP_RANGE;
    data.ambientTemp = 25.0f;
    data.controllerTemp =
        DEMO_MIN_CONTROLLER_TEMP + wave * DEMO_CONTROLLER_TEMP_RANGE;
    data.motorCurrent[0] =
        DEMO_MIN_FRONT_CURRENT + wave * DEMO_FRONT_CURRENT_RANGE;
    data.motorCurrent[1] =
        DEMO_MIN_FRONT_CURRENT + wave * DEMO_FRONT_CURRENT_RANGE;
    data.motorCurrent[2] =
        DEMO_MIN_REAR_CURRENT + wave * DEMO_REAR_CURRENT_RANGE;
    data.motorCurrent[3] =
        DEMO_MIN_REAR_CURRENT + wave * DEMO_REAR_CURRENT_RANGE;
    data.steeringCurrent =
        DEMO_MIN_STEER_CURRENT + fabsf(steerWave) * DEMO_STEER_CURRENT_RANGE;
    data.pedalPercent = DEMO_MIN_PEDAL + wave * DEMO_PEDAL_RANGE;
    data.steeringAngle = steerWave * DEMO_MAX_STEER_ANGLE;
    data.gear = (data.speed > DEMO_GEAR_CHANGE_SPEED) ? GearPosition::DRIVE1
                                                      : GearPosition::PARK;
    data.batteryPower = data.batteryVoltage * data.batteryCurrent;
    data.odoTotal = 1234.5f + (now - demoStartTime) / 60000.0f;
    data.odoTrip = 56.7f + (now - demoStartTime) / 120000.0f;
    data.encoderValue = 2048.0f + steerWave * DEMO_ENCODER_RANGE;

    // Animated SystemStatus using named timing constants
    data.status.fourWheelDrive = true;
    data.status.lights =
        (demoTime > DEMO_LIGHTS_ON_TIME && demoTime < DEMO_LIGHTS_OFF_TIME);
    data.status.parkingBrake = (data.speed < DEMO_GEAR_CHANGE_SPEED);
    data.status.wifi = false;
    data.status.bluetooth = false;
    data.status.warnings =
        (data.motorTemp[0] > DEMO_TEMP_WARNING_THRESHOLD) ? 1 : 0;

    HUDManager::updateCarData(data);
    HUDManager::update();
  }

  // Minimal loop - no sensors, no control systems
  delay(1); // Prevent watchdog issues in standalone mode

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

  // Handle 4x4 button toggle
  if (Buttons::toggled4x4()) {
    bool new4x4State = Buttons::get().mode4x4;
    Traction::setMode4x4(new4x4State);
    Logger::infof("4x4 button toggled: %s", new4x4State ? "ON" : "OFF");
  }

  // Sensores
  Sensors::updateCurrent();
  Sensors::updateTemperature();
  Sensors::updateWheels();

  // Control
  Traction::setDemand(Pedal::get().percent);
  Traction::update();

  // 🔒 v2.9.1: Solo actualizar SteeringMotor si está inicializado correctamente
  // Evita el warning "[WARN] SteeringMotor update llamado sin init" cuando
  // el PCA9685 de dirección no está conectado o falló la inicialización
  if (SteeringMotor::initOK()) {
    SteeringMotor::setDemandAngle(Steering::get().angleDeg);
    SteeringMotor::update();
  }

  Relays::update();

  // Advanced Safety Systems
  ABSSystem::update();
  TCSSystem::update();
  RegenAI::update();

  // Obstacle Detection and Safety
  ObstacleDetection::update();
  ObstacleSafety::update();

  // Telemetry
  Telemetry::update(); // 🆕 v2.8.0: Sistema de telemetría

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