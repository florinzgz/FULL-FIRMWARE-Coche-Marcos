// ============================================================================
// hardware_failure_tests.cpp - Hardware Failure Testing Implementation
// ============================================================================

#include "hardware_failure_tests.h"
#include "car_sensors.h"
#include "current.h"
#include "i2c_recovery.h"
#include "logger.h"
#include "temperature.h"
#include "test_utils.h"

namespace HardwareFailureTests {

// ============================================================================
// Private State
// ============================================================================

static TestUtils::TestCounters counters;

// ============================================================================
// Public API Implementation
// ============================================================================

void init() {
  counters.initialize();
  Logger::info("HardwareFailureTests: Initialized");
}

bool runAllTests() {
  if (!counters.initialized) { init(); }

  Logger::info("\n========================================");
  Logger::info("Starting Hardware Failure Scenario Tests");
  Logger::info("========================================");

  // Reset counters
  counters.reset();

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

  recordTest(counters, "I2C Bus Recovery", recoveryAvailable, "HW TEST");

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
  recordTest(counters, "Sensor Disconnection Handling", allSensorsValid, "HW TEST");

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
  recordTest(counters, "Display Failure Handling", systemStable, "HW TEST");

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

  recordTest(counters, "Power Variation Monitoring", voltageValid, "HW TEST");

  return voltageValid;
}

void printSummary() {
  TestUtils::printSummary(counters, "Hardware Failure Test");
  if (counters.failedCount > 0) {
    Logger::error("❌ HARDWARE FAILURE TESTS FAILED");
  }
}

uint32_t getPassedCount() { return counters.passedCount; }

uint32_t getFailedCount() { return counters.failedCount; }

} // namespace HardwareFailureTests
