#pragma once

// ============================================================================
// watchdog_tests.h - Watchdog Timer Verification Tests
// ============================================================================
// Tests watchdog timer functionality:
// - Watchdog feed interval verification
// - Timeout behavior testing
// - Emergency shutdown verification
// - Panic handler execution
// ============================================================================

#include <Arduino.h>

namespace WatchdogTests {

/**
 * @brief Initialize watchdog testing
 */
void init();

/**
 * @brief Run all watchdog verification tests
 * @return true if all tests passed
 */
bool runAllTests();

/**
 * @brief Test watchdog feed interval monitoring
 * Verifies that feed() is called regularly
 * @return true if feed interval is acceptable
 */
bool testFeedInterval();

/**
 * @brief Test watchdog feed count tracking
 * @return true if feed counting works correctly
 */
bool testFeedCounting();

/**
 * @brief Test watchdog status reporting
 * @return true if status queries work correctly
 */
bool testStatusReporting();

/**
 * @brief Test emergency shutdown mechanism
 * Verifies panic handler would execute correctly
 * Note: Does NOT trigger actual watchdog timeout
 * @return true if shutdown mechanism is valid
 */
bool testEmergencyShutdown();

/**
 * @brief Verify watchdog is enabled and configured
 * @return true if watchdog is properly configured
 */
bool testWatchdogConfiguration();

/**
 * @brief Print test summary
 */
void printSummary();

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

} // namespace WatchdogTests
