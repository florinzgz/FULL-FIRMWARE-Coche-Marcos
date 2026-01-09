#pragma once

// ============================================================================
// audio_validation_tests.h - Audio System Validation Tests
// ============================================================================
// Validates all 68 audio tracks are properly defined and functional
// ============================================================================

#include <Arduino.h>

namespace AudioValidationTests {

/**
 * @brief Test result structure
 */
struct TestResult {
  const char *testName;
  bool passed;
  const char *failureReason;
};

/**
 * @brief Initialize the audio validation test system
 */
void init();

/**
 * @brief Run all audio validation tests
 * @return true if all tests passed
 */
bool runAllTests();

/**
 * @brief Print test summary to serial
 */
void printSummary();

/**
 * @brief Get test results
 * @param count Output parameter for number of results
 * @return Array of test results
 */
const TestResult *getResults(uint32_t &count);

} // namespace AudioValidationTests
