#pragma once

// ============================================================================
// hardware_failure_tests.h - Hardware Failure Scenario Testing
// ============================================================================
// Tests system behavior under hardware failure conditions:
// - I2C bus failures and recovery
// - Sensor disconnections
// - Display disconnections
// - Power supply variations
// ============================================================================

#include <Arduino.h>

namespace HardwareFailureTests {

/**
 * @brief Initialize hardware failure testing
 */
void init();

/**
 * @brief Run all hardware failure scenario tests
 * @return true if all tests passed
 */
bool runAllTests();

/**
 * @brief Test I2C bus recovery mechanisms
 * @return true if recovery works correctly
 */
bool testI2CBusRecovery();

/**
 * @brief Test sensor disconnection handling
 * @return true if system handles disconnection gracefully
 */
bool testSensorDisconnection();

/**
 * @brief Test display communication failure handling
 * @return true if system handles display failure
 */
bool testDisplayFailure();

/**
 * @brief Test power supply variation handling
 * @return true if system handles voltage variations
 */
bool testPowerVariations();

/**
 * @brief Print test summary
 */
void printSummary();

} // namespace HardwareFailureTests
