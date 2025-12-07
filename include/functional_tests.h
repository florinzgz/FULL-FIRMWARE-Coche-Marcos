#pragma once

// ============================================================================
// functional_tests.h - Comprehensive Functional Testing Module
// ============================================================================
// This module provides comprehensive functional testing for all critical
// subsystems before deployment. Tests verify that all components work
// correctly under normal operating conditions.
//
// Usage:
//   FunctionalTests::init();
//   FunctionalTests::runAllTests();
//   if (FunctionalTests::allTestsPassed()) { /* deploy */ }
// ============================================================================

#include <Arduino.h>

namespace FunctionalTests {

// Test result structure
struct TestResult {
    const char* testName;
    bool passed;
    const char* failureReason;
    uint32_t executionTimeMs;
};

// Test categories
enum class TestCategory {
    DISPLAY_TESTS,  // Renamed from DISPLAY to avoid Arduino.h conflict
    SENSORS,
    MOTORS,
    SAFETY,
    COMMUNICATION,
    STORAGE,
    ALL
};

// ============================================================================
// Public API
// ============================================================================

/**
 * @brief Initialize the functional testing system
 */
void init();

/**
 * @brief Run all functional tests
 * @return true if all tests passed, false otherwise
 */
bool runAllTests();

/**
 * @brief Run tests for a specific category
 * @param category Test category to run
 * @return true if all tests in category passed
 */
bool runCategoryTests(TestCategory category);

/**
 * @brief Check if all tests have passed
 * @return true if all executed tests passed
 */
bool allTestsPassed();

/**
 * @brief Get the number of tests that passed
 * @return Number of passed tests
 */
uint32_t getPassedCount();

/**
 * @brief Get the number of tests that failed
 * @return Number of failed tests
 */
uint32_t getFailedCount();

/**
 * @brief Print test summary to Serial
 */
void printSummary();

/**
 * @brief Get detailed test results
 * @param results Output array for test results
 * @param maxResults Maximum number of results to return
 * @return Number of results written
 */
uint32_t getResults(TestResult* results, uint32_t maxResults);

// ============================================================================
// Individual Test Functions (can be called directly for debugging)
// ============================================================================

// Display Tests
bool testDisplayInit();
bool testDisplayBacklight();
bool testDisplayTouch();
bool testDisplayRendering();

// Sensor Tests
bool testCurrentSensors();
bool testTemperatureSensors();
bool testWheelSensors();
bool testObstacleSensors();

// Motor Tests
bool testSteeringMotor();
bool testTractionControl();
bool testRelaySequence();

// Safety Tests
bool testWatchdogFeed();
bool testEmergencyStop();
bool testABSSystem();
bool testTCSSystem();

// Communication Tests
bool testI2CBus();
bool testBluetoothConnection();
bool testWiFiConnection();

// Storage Tests
bool testEEPROMReadWrite();
bool testConfigPersistence();

} // namespace FunctionalTests
