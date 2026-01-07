#pragma once

// ============================================================================
// test_utils.h - Common Test Utilities
// ============================================================================
// Shared test infrastructure to avoid code duplication across test modules
// ============================================================================

#include <Arduino.h>

namespace TestUtils {

/**
 * @brief Test counters structure
 */
struct TestCounters {
  uint32_t testCount = 0;
  uint32_t passedCount = 0;
  uint32_t failedCount = 0;
  bool initialized = false;

  void reset() {
    testCount = 0;
    passedCount = 0;
    failedCount = 0;
  }

  void initialize() {
    reset();
    initialized = true;
  }
};

/**
 * @brief Record a test result
 * @param counters Test counters structure
 * @param name Test name
 * @param passed Whether the test passed
 * @param category Test category (e.g., "HW TEST", "WATCHDOG TEST")
 */
void recordTest(TestCounters &counters, const char *name, bool passed,
                const char *category = "TEST");

/**
 * @brief Print test summary
 * @param counters Test counters structure
 * @param suiteName Test suite name (e.g., "Hardware Failure Test",
 * "Watchdog Timer Test")
 */
void printSummary(const TestCounters &counters, const char *suiteName);

} // namespace TestUtils
