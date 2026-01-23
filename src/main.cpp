// main.cpp - Entry point for the vehicle firmware
// Handles initialization, mode management, and main control loop

#include "SystemConfig.h"
#include "hud_manager.h"
#include "logger.h"
#include "managers/ControlManager.h"
#include "managers/ModeManager.h"
#include "managers/PowerManager.h"
#include "managers/SafetyManager.h"
#include "managers/SensorManager.h"
#include "managers/TelemetryManager.h"
#include "pins.h"
#include "watchdog.h"
#include <Arduino.h>

// Include core system components for proper boot sequence
#include "boot_guard.h" // 🔒 v2.17.1: Boot counter for bootloop detection
#include "i2c_recovery.h"
#include "storage.h"
#include "system.h"

// ============================================================================
// Critical error recovery configuration - v2.11.5
// ============================================================================
namespace CriticalErrorConfig {
constexpr uint8_t MAX_RETRIES = 3;
constexpr uint32_t RETRY_DELAY_MS = 5000;
} // namespace CriticalErrorConfig

// Forward declarations
void initializeSystem();
void handleCriticalError(const char *errorMsg);

void setup() {
  // 🔒 v2.11.6: BOOTLOOP FIX - Early UART diagnostic output
  // Initialize Serial first for all modes
  Serial.begin(115200);

#ifdef STANDALONE_DISPLAY
  // Early diagnostic output for standalone mode
  delay(100); // Give UART time to stabilize
  Serial.println("\n\n=== ESP32-S3 EARLY BOOT ===");
  Serial.println("[STANDALONE] Mode active");
  Serial.flush();
#endif

  while (!Serial && millis() < 2000) {
    ;
  }

  Serial.println("[BOOT] Enabling TFT backlight...");
  pinMode(PIN_TFT_BL, OUTPUT);
  digitalWrite(PIN_TFT_BL, HIGH);
  // PWM setup in HUDManager::init will override this, but keep the backlight on
  // immediately so boot progress is visible even if init fails.

  Serial.println("[BOOT] Resetting TFT display...");
  pinMode(PIN_TFT_RST, OUTPUT);
  digitalWrite(PIN_TFT_RST, HIGH);
  delay(DisplayBootConfig::TFT_RESET_STABILIZATION_MS);
  digitalWrite(PIN_TFT_RST, LOW);
  delay(DisplayBootConfig::TFT_RESET_PULSE_MS);
  digitalWrite(PIN_TFT_RST, HIGH);
  delay(DisplayBootConfig::TFT_RESET_RECOVERY_MS);

  // 🔍 DIAGNOSTIC MARKER A: Serial initialized
  Serial.write('A');
  Serial.flush();
  delay(10);

  Serial.println("[BOOT] Starting vehicle firmware...");
  Serial.print("[BOOT] Firmware version: ");
  Serial.println(FIRMWARE_VERSION);

  // 🔒 v2.17.1: Initialize and check boot counter BEFORE any other init
  BootGuard::initBootCounter();
  BootGuard::logResetMarker();
  BootGuard::clearResetMarker();
  BootGuard::incrementBootCounter();

  if (BootGuard::isBootloopDetected()) {
    Serial.println("[BOOT] ⚠️  BOOTLOOP DETECTED - Safe mode will be activated");
    Serial.printf("[BOOT] Boot count: %d within detection window\n",
                  BootGuard::getBootCount());
  }

  // 🔍 DIAGNOSTIC MARKER B: Boot counter initialized
  Serial.write('B');
  Serial.flush();
  delay(10);

  // Critical boot sequence
  System::init();
  Storage::init();
  Watchdog::init();
  Watchdog::feed();
  I2CRecovery::init();

  // 🔍 DIAGNOSTIC MARKER C: Core systems initialized
  Serial.write('C');
  Serial.flush();
  delay(10);

  Logger::init();
  Logger::info("Boot sequence started");
  Watchdog::feed();

  // 🔍 DIAGNOSTIC MARKER D: Before initializeSystem (includes HUD init)
  Serial.write('D');
  Serial.flush();
  delay(10);

  initializeSystem();

  // 🔍 DIAGNOSTIC MARKER E: System initialization complete
  Serial.write('E');
  Serial.flush();
  delay(10);

  Serial.println("[BOOT] System initialization complete");
  Logger::info("Vehicle firmware ready");
}

void loop() {
  Watchdog::feed();

  // 🔒 v2.17.1: Clear boot counter after successful first loop iteration
  // This indicates the system booted successfully and is stable
  static bool firstLoop = true;
  if (firstLoop) {
    BootGuard::clearBootCounter();
    Logger::info("Main loop: Boot successful - boot counter cleared");
    firstLoop = false;
  }

  // 🔍 VERIFICATION: Periodic memory monitoring (every 30 seconds)
  static uint32_t lastMemoryLog = 0;
  uint32_t now = millis();
  if (now - lastMemoryLog >= 30000) {
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t freePsram = ESP.getFreePsram();
    Logger::infof("Memory: Heap=%u KB, PSRAM=%u KB", freeHeap / 1024,
                  freePsram / 1024);
    lastMemoryLog = now;
  }

#ifdef STANDALONE_DISPLAY
  // ===========================
  // STANDALONE DISPLAY MODE
  // ===========================
  HUDManager::update();
  delay(33); // ~30 FPS

#else
  // ===========================
  // FULL VEHICLE MODE
  // ===========================
  PowerManager::update();
  SensorManager::update();
  SafetyManager::update();

  VehicleMode currentMode = ModeManager::getCurrentMode();
  ModeManager::update();

  ControlManager::update();

  TelemetryManager::update();
  HUDManager::update();

  delay(SYSTEM_TICK_MS);
#endif
}

void initializeSystem() {
  Watchdog::feed();

  // 🔒 v2.17.1: Check if safe mode is requested due to bootloop
  bool safeMode = BootGuard::shouldEnterSafeMode();

  if (safeMode) {
    Serial.println("⚠️⚠️⚠️ SAFE MODE ACTIVATED ⚠️⚠️⚠️");
    Serial.println(
        "[SAFE MODE] Bootloop detected - initializing only critical systems");
    Logger::error("SAFE MODE: Bootloop detected - minimal initialization");
  }

#ifdef STANDALONE_DISPLAY
  // ===========================
  // STANDALONE DISPLAY INIT
  // ===========================
  Serial.println("🧪 STANDALONE DISPLAY MODE");
  Serial.flush();
  delay(100); // Ensure UART message is sent

  Serial.println("[INIT] HUD Manager initialization...");
  Serial.flush();

  if (!HUDManager::init()) {
    Serial.println("[ERROR] HUD Manager initialization failed");
    Serial.flush();
    handleCriticalError("HUD Manager initialization failed");
  }

  Logger::info("HUD Manager initialized (standalone)");
  Watchdog::feed();

  Serial.println("🧪 STANDALONE: Skipping other managers");
  Serial.flush();
  return; // ¡MUY IMPORTANTE!

#else
  // ===========================
  // FULL VEHICLE INIT
  // ===========================

  // CRITICAL SYSTEMS - Always initialize
  Serial.println("[INIT] Power Manager initialization...");
  if (!PowerManager::init()) {
    handleCriticalError("Power Manager initialization failed");
  }
  Logger::info("Power Manager initialized");
  Watchdog::feed();

  Serial.println("[INIT] Sensor Manager initialization...");
  if (!SensorManager::init()) {
    handleCriticalError("Sensor Manager initialization failed");
  }
  Logger::info("Sensor Manager initialized");
  Watchdog::feed();

  Serial.println("[INIT] Safety Manager initialization...");
  if (!SafetyManager::init()) {
    handleCriticalError("Safety Manager initialization failed");
  }
  Logger::info("Safety Manager initialized");
  Watchdog::feed();

  // HUD - Try to initialize even in safe mode (for error display)
  Serial.println("[INIT] HUD Manager initialization...");
  if (!HUDManager::init()) {
    if (!safeMode) {
      handleCriticalError("HUD Manager initialization failed");
    } else {
      Logger::warn(
          "Safe Mode: HUD initialization failed - continuing without display");
    }
  } else {
    Logger::info("HUD Manager initialized");
  }
  Watchdog::feed();

  // Control Manager - Critical
  Serial.println("[INIT] Control Manager initialization...");
  if (!ControlManager::init()) {
    handleCriticalError("Control Manager initialization failed");
  }
  Logger::info("Control Manager initialized");
  Watchdog::feed();

  // NON-CRITICAL SYSTEMS - Skip in safe mode
  if (safeMode) {
    Serial.println("[SAFE MODE] Skipping Telemetry Manager (non-critical)");
    Serial.println("[SAFE MODE] Skipping Mode Manager (non-critical)");
    Logger::warn("Safe Mode: Non-critical managers disabled");
  } else {
    Serial.println("[INIT] Telemetry Manager initialization...");
    if (!TelemetryManager::init()) {
      handleCriticalError("Telemetry Manager initialization failed");
    }
    Logger::info("Telemetry Manager initialized");
    Watchdog::feed();

    Serial.println("[INIT] Mode Manager initialization...");
    if (!ModeManager::init()) {
      handleCriticalError("Mode Manager initialization failed");
    }
    Logger::info("Mode Manager initialized");
    Watchdog::feed();
  }

#endif
}

void handleCriticalError(const char *errorMsg) {
  Watchdog::feed();

  static uint8_t retryCount = 0;
  static const char *lastErrorSource = nullptr;

  Serial.print("[CRITICAL ERROR] ");
  Serial.println(errorMsg);
  Logger::error(errorMsg);

  if (lastErrorSource != errorMsg) {
    retryCount = 0;
    lastErrorSource = errorMsg;
  }

  retryCount++;

  if (retryCount >= CriticalErrorConfig::MAX_RETRIES) {
    Logger::errorf("Critical error: Max retries reached (%d/%d)", retryCount,
                   CriticalErrorConfig::MAX_RETRIES);
    Logger::error("Allowing watchdog timeout for system reset");
    BootGuard::setResetMarker(BootGuard::RESET_MARKER_WATCHDOG_LOOP);

#ifndef STANDALONE_DISPLAY
    HUDManager::showError("Critical error: System failed after max retries");
#endif

    Serial.println("[CRITICAL ERROR] Max retries - stopping watchdog feeds");
    Serial.flush();

    uint32_t lastPrintTime = millis();
    while (true) {
      yield();
      if (millis() - lastPrintTime >= 1000) {
        Serial.println("[CRITICAL ERROR] Waiting for watchdog reset...");
        lastPrintTime = millis();
      }
    }
  }

#ifndef STANDALONE_DISPLAY
  HUDManager::showError("Critical error detected");
#endif

  Serial.printf("[CRITICAL ERROR] Retry %d/%d in %lums\n", retryCount,
                CriticalErrorConfig::MAX_RETRIES,
                CriticalErrorConfig::RETRY_DELAY_MS);

  Serial.flush();

  uint32_t delayStart = millis();
  while (millis() - delayStart < CriticalErrorConfig::RETRY_DELAY_MS) {
    Watchdog::feed();
    yield();
  }

  Logger::info("Attempting system restart...");
  BootGuard::setResetMarker(BootGuard::RESET_MARKER_EXPLICIT_RESTART);
  Serial.println("[CRITICAL ERROR] Restarting...");
  Serial.flush();

  ESP.restart();

  while (true) {
    delay(1000);
  }
}
