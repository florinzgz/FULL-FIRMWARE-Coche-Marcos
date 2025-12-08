#pragma once

// ============================================================================
// test_runner.h - Test Execution Coordinator
// ============================================================================
// Coordinates execution of all pre-deployment tests
// Integrates with main.cpp to run tests on startup when enabled
// ============================================================================

#include <Arduino.h>

namespace TestRunner {

/**
 * @brief Run all pre-deployment tests
 * Called from setup() when test mode is enabled
 * @return true if all tests passed
 */
bool runPreDeploymentTests();

/**
 * @brief Check if test mode is enabled
 * @return true if any test flags are defined
 */
bool isTestModeEnabled();

/**
 * @brief Print overall test summary
 */
void printOverallSummary();

} // namespace TestRunner
