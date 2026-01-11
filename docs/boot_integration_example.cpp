/**
 * @file boot_integration_example.cpp
 * @brief Example of integrating Phase 12 boot validation into main.cpp
 * 
 * This file shows how to instrument your setup() function to use
 * the boot sequence validator for comprehensive boot testing.
 * 
 * USAGE:
 * Copy the relevant sections into your main.cpp setup() function.
 */

// Add to top of main.cpp:
#include "boot_sequence_test.h"

// ============================================================================
// Example: Instrumented setup() with boot validation
// ============================================================================

void setup() {
  // =========================================================================
  // PHASE 12: Initialize boot sequence validation
  // This must be the FIRST line in setup()
  // =========================================================================
  #ifdef ENABLE_BOOT_VALIDATION
  BootSequenceTest::init();
  #endif
  
  // =========================================================================
  // Early UART initialization
  // =========================================================================
  Serial.begin(115200);
  while (!Serial && millis() < 2000) {
    ;
  }
  
  Serial.println("[BOOT] Starting vehicle firmware...");
  
  // =========================================================================
  // STAGE: Boot Guard (Bootloop Protection)
  // =========================================================================
  BootGuard::initBootCounter();
  BootGuard::incrementBootCounter();
  
  if (BootGuard::isBootloopDetected()) {
    Serial.println("[BOOT] ⚠️  BOOTLOOP DETECTED - Safe mode will be activated");
  }
  
  // =========================================================================
  // STAGE: System Init
  // =========================================================================
  #ifdef ENABLE_BOOT_VALIDATION
  BootSequenceTest::markStageStart(BootSequenceTest::BootStage::SYSTEM_INIT);
  #endif
  
  System::init();
  
  #ifdef ENABLE_BOOT_VALIDATION
  BootSequenceTest::markStageComplete(BootSequenceTest::BootStage::SYSTEM_INIT, true);
  #endif
  
  // =========================================================================
  // STAGE: Storage Init
  // =========================================================================
  #ifdef ENABLE_BOOT_VALIDATION
  BootSequenceTest::markStageStart(BootSequenceTest::BootStage::STORAGE_INIT);
  #endif
  
  Storage::init();
  
  #ifdef ENABLE_BOOT_VALIDATION
  BootSequenceTest::markStageComplete(BootSequenceTest::BootStage::STORAGE_INIT, true);
  #endif
  
  // =========================================================================
  // STAGE: Watchdog Init
  // =========================================================================
  #ifdef ENABLE_BOOT_VALIDATION
  BootSequenceTest::markStageStart(BootSequenceTest::BootStage::WATCHDOG_INIT);
  #endif
  
  Watchdog::init();
  Watchdog::feed();
  
  #ifdef ENABLE_BOOT_VALIDATION
  BootSequenceTest::markStageComplete(BootSequenceTest::BootStage::WATCHDOG_INIT, true);
  #endif
  
  // =========================================================================
  // STAGE: Logger Init
  // =========================================================================
  #ifdef ENABLE_BOOT_VALIDATION
  BootSequenceTest::markStageStart(BootSequenceTest::BootStage::LOGGER_INIT);
  #endif
  
  Logger::init();
  Logger::info("Boot sequence started");
  Watchdog::feed();
  
  #ifdef ENABLE_BOOT_VALIDATION
  BootSequenceTest::markStageComplete(BootSequenceTest::BootStage::LOGGER_INIT, true);
  #endif
  
  // =========================================================================
  // STAGE: PSRAM Check
  // =========================================================================
  #ifdef ENABLE_BOOT_VALIDATION
  BootSequenceTest::markStageStart(BootSequenceTest::BootStage::PSRAM_CHECK);
  #endif
  
  bool psramOk = psramFound();
  if (psramOk) {
    Logger::info("PSRAM detected and enabled");
  } else {
    Logger::warn("PSRAM not available - using internal RAM only");
  }
  
  #ifdef ENABLE_BOOT_VALIDATION
  BootSequenceTest::markStageComplete(BootSequenceTest::BootStage::PSRAM_CHECK, psramOk);
  #endif
  
  // =========================================================================
  // STAGE: Sensor Init
  // =========================================================================
  #ifdef ENABLE_BOOT_VALIDATION
  BootSequenceTest::markStageStart(BootSequenceTest::BootStage::SENSOR_INIT);
  #endif
  
  bool sensorOk = true;
  
  #ifndef STANDALONE_DISPLAY
  // Full vehicle mode - initialize sensors
  Serial.println("[INIT] Sensor Manager initialization...");
  sensorOk = SensorManager::init();
  if (!sensorOk) {
    Logger::error("Sensor Manager initialization failed");
  } else {
    Logger::info("Sensor Manager initialized");
  }
  Watchdog::feed();
  #else
  // Standalone mode - skip sensors
  Serial.println("🧪 STANDALONE: Skipping sensor initialization");
  #endif
  
  #ifdef ENABLE_BOOT_VALIDATION
  BootSequenceTest::markStageComplete(BootSequenceTest::BootStage::SENSOR_INIT, sensorOk);
  #endif
  
  // =========================================================================
  // STAGE: LimpMode Init (if available)
  // =========================================================================
  #ifdef ENABLE_BOOT_VALIDATION
  BootSequenceTest::markStageStart(BootSequenceTest::BootStage::LIMP_MODE_INIT);
  #endif
  
  #ifndef STANDALONE_DISPLAY
  LimpMode::init(); // LimpMode has void init, assume success
  Logger::info("LimpMode initialized");
  #endif
  
  #ifdef ENABLE_BOOT_VALIDATION
  BootSequenceTest::markStageComplete(BootSequenceTest::BootStage::LIMP_MODE_INIT, true);
  #endif
  
  // =========================================================================
  // STAGE: HUD Init
  // =========================================================================
  #ifdef ENABLE_BOOT_VALIDATION
  BootSequenceTest::markStageStart(BootSequenceTest::BootStage::HUD_INIT);
  #endif
  
  Serial.println("[INIT] HUD Manager initialization...");
  bool hudOk = HUDManager::init();
  
  if (!hudOk) {
    Serial.println("[ERROR] HUD Manager initialization failed");
    Logger::error("HUD Manager initialization failed");
  } else {
    Logger::info("HUD Manager initialized");
  }
  Watchdog::feed();
  
  #ifdef ENABLE_BOOT_VALIDATION
  BootSequenceTest::markStageComplete(BootSequenceTest::BootStage::HUD_INIT, hudOk);
  #endif
  
  // =========================================================================
  // STAGE: Compositor Init
  // =========================================================================
  #ifdef ENABLE_BOOT_VALIDATION
  BootSequenceTest::markStageStart(BootSequenceTest::BootStage::COMPOSITOR_INIT);
  #endif
  
  // Compositor is initialized as part of HUDManager::init()
  // Just verify it's ready
  bool compositorOk = HudCompositor::isInitialized();
  
  #ifdef ENABLE_BOOT_VALIDATION
  BootSequenceTest::markStageComplete(BootSequenceTest::BootStage::COMPOSITOR_INIT, compositorOk);
  #endif
  
  // =========================================================================
  // STAGE: Managers Init
  // =========================================================================
  #ifdef ENABLE_BOOT_VALIDATION
  BootSequenceTest::markStageStart(BootSequenceTest::BootStage::MANAGERS_INIT);
  #endif
  
  bool managersOk = true;
  
  #ifndef STANDALONE_DISPLAY
  // Initialize remaining managers
  if (!PowerManager::init()) {
    Logger::error("Power Manager initialization failed");
    managersOk = false;
  }
  
  if (!SafetyManager::init()) {
    Logger::error("Safety Manager initialization failed");
    managersOk = false;
  }
  
  if (!ControlManager::init()) {
    Logger::error("Control Manager initialization failed");
    managersOk = false;
  }
  
  // Non-critical managers
  bool safeMode = BootGuard::shouldEnterSafeMode();
  if (!safeMode) {
    TelemetryManager::init();
    ModeManager::init();
  }
  #endif
  
  #ifdef ENABLE_BOOT_VALIDATION
  BootSequenceTest::markStageComplete(BootSequenceTest::BootStage::MANAGERS_INIT, managersOk);
  #endif
  
  // =========================================================================
  // STAGE: Boot Complete
  // =========================================================================
  Serial.println("[BOOT] System initialization complete");
  Logger::info("Vehicle firmware ready");
  
  #ifdef ENABLE_BOOT_VALIDATION
  BootSequenceTest::markStageStart(BootSequenceTest::BootStage::BOOT_COMPLETE);
  BootSequenceTest::markStageComplete(BootSequenceTest::BootStage::BOOT_COMPLETE, true);
  
  // =========================================================================
  // PHASE 12: Run comprehensive boot validation
  // =========================================================================
  Serial.println("\n[BOOT] Running Phase 12 boot validation...");
  
  bool bootValidationPassed = BootSequenceTest::runComprehensiveCheck();
  
  if (bootValidationPassed) {
    Serial.println("[BOOT] ✓✓✓ Boot validation PASSED ✓✓✓");
    Logger::info("Phase 12 boot validation: PASSED");
  } else {
    Serial.println("[BOOT] ✗✗✗ Boot validation FAILED ✗✗✗");
    Logger::error("Phase 12 boot validation: FAILED - Check serial output");
  }
  
  // Print detailed results
  BootSequenceTest::printResults();
  #endif
}

// ============================================================================
// Example: Instrumented loop() with boot counter clear
// ============================================================================

void loop() {
  Watchdog::feed();
  
  // Clear boot counter after first successful loop iteration
  static bool firstLoop = true;
  if (firstLoop) {
    BootGuard::clearBootCounter();
    Logger::info("Main loop: Boot successful - boot counter cleared");
    
    #ifdef ENABLE_BOOT_VALIDATION
    // Also log final boot status
    if (BootSequenceTest::isBootHealthy()) {
      Logger::info("Boot health check: HEALTHY");
    }
    #endif
    
    firstLoop = false;
  }
  
  // Rest of loop() implementation...
  // ...
}

// ============================================================================
// How to enable boot validation
// ============================================================================

/*
 * Add to platformio.ini:
 * 
 * [env:esp32-s3-n32r16v-boot-test]
 * extends = env:esp32-s3-n32r16v
 * build_flags =
 *     ${env:esp32-s3-n32r16v.build_flags}
 *     -DENABLE_BOOT_VALIDATION
 * 
 * Then build with:
 *     pio run -e esp32-s3-n32r16v-boot-test -t upload
 * 
 * Monitor output with:
 *     pio device monitor -b 115200
 */
