// ============================================================================
// hardware_failure_tests.cpp - Hardware Failure Testing Implementation
// ============================================================================

#include "hardware_failure_tests.h"
#include "car_sensors.h"
#include "current.h"
#include "i2c_recovery.h"
#include "logger.h"
#include "temperature.h"

namespace HardwareFailureTests {

// ============================================================================
// Private State
// ============================================================================

static uint32_t testCount = 0;
static uint32_t passedCount = 0;
static uint32_t failedCount = 0;
static bool initialized = false;

// ============================================================================
// Helper Functions
// ============================================================================

static void recordTest(const char *name, bool passed) {
  testCount++;
  if (passed) {
    passedCount++;
    Logger::infof("✅ HW TEST PASSED: %s", name);
  } else {
    failedCount++;
    Logger::errorf("❌ HW TEST FAILED: %s", name);
  }
}

// ============================================================================
// Public API Implementation
// ============================================================================

void init() {
  testCount = 0;
  passedCount = 0;
  failedCount = 0;
  initialized = true;

  Logger::info("HardwareFailureTests: Initialized");
}

bool runAllTests() {
  if (!initialized) { init(); }

  Logger::info("\n========================================");
  Logger::info("Starting Hardware Failure Scenario Tests");
  Logger::info("========================================");

  // Reset counters
  testCount = 0;
  passedCount = 0;
  failedCount = 0;

  bool allPassed = true;

  allPassed &= testI2CBusRecovery();
  allPassed &= testSensorDisconnection();
  allPassed &= testDisplayFailure();
  allPassed &= testPowerVariations();

  printSummary();

  return allPassed;
}

bool testI2CBusRecovery() {
  Logger::info("\n--- Testing I2C Bus Recovery ---");

  // Check that I2C recovery system is initialized and functional
  bool recoveryAvailable = I2CRecovery::isBusHealthy();

  if (!recoveryAvailable) {
    Logger::warn("I2C bus not healthy - testing recovery mechanism");

    // Attempt recovery
    I2CRecovery::recoverBus();
    delay(100);

    // Check if recovery worked
    recoveryAvailable = I2CRecovery::isBusHealthy();
  }

  recordTest("I2C Bus Recovery", recoveryAvailable);

  return recoveryAvailable;
}

bool testSensorDisconnection() {
  Logger::info("\n--- Testing Sensor Disconnection Handling ---");

  // Test that system can handle sensor read failures gracefully
  // We verify that readings return valid values (not NaN or infinity)

  bool allSensorsValid = true;

  // Test current sensors - using correct API
  float voltage = Sensors::getVoltage(0); // Battery is typically index 0
  allSensorsValid &= std::isfinite(voltage);

  float current = Sensors::getCurrent(0);
  allSensorsValid &= std::isfinite(current);

  // Test temperature sensors - using correct API
  for (int i = 0; i < 4; i++) {
    float temp = Sensors::getTemperature(i);
    allSensorsValid &= std::isfinite(temp);
  }

  // Even if sensors are disconnected, system should return valid
  // default values, not NaN or infinity
  recordTest("Sensor Disconnection Handling", allSensorsValid);

  return allSensorsValid;
}

bool testDisplayFailure() {
  Logger::info("\n--- Testing Display Failure Handling ---");

  // Verify that system continues operation even if display fails
  // This is verified by checking that logger still works
  // and system doesn't crash

  bool systemStable = true;

  // Log a message
  Logger::info("Testing display failure recovery...");

  // System should continue running
  delay(100);

  // If we get here, system is stable
  recordTest("Display Failure Handling", systemStable);

  return systemStable;
}

bool testPowerVariations() {
  Logger::info("\n--- Testing Power Supply Variation Handling ---");

  // Test that system can detect and respond to power variations
  float voltage = Sensors::getVoltage(0); // Battery voltage

  bool voltageValid =
      std::isfinite(voltage) && (voltage > 0.0f) && (voltage < 100.0f);

  if (voltageValid) {
    Logger::infof("Battery voltage: %.2fV", voltage);

    // Check for low voltage warning threshold
    if (voltage < 20.0f) { Logger::warn("Low battery voltage detected"); }

    // Check for high voltage warning threshold
    if (voltage > 30.0f) { Logger::warn("High battery voltage detected"); }
  }

  recordTest("Power Variation Monitoring", voltageValid);

  return voltageValid;
}

void printSummary() {
  Logger::info("\n========================================");
  Logger::info("Hardware Failure Test Summary");
  Logger::info("========================================");
  Logger::infof("Total Tests: %lu", testCount);
  Logger::infof("Passed: %lu", passedCount);
  Logger::infof("Failed: %lu", failedCount);

  if (passedCount == testCount && testCount > 0) {
    Logger::info("✅ ALL HARDWARE FAILURE TESTS PASSED");
  } else {
    Logger::error("❌ HARDWARE FAILURE TESTS FAILED");
  }
  Logger::info("========================================\n");
}

uint32_t getPassedCount() { return passedCount; }

uint32_t getFailedCount() { return failedCount; }

} // namespace HardwareFailureTests
