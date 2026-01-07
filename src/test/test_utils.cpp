// ============================================================================
// test_utils.cpp - Common Test Utilities Implementation
// ============================================================================

#include "test_utils.h"
#include "logger.h"

namespace TestUtils {

void recordTest(TestCounters &counters, const char *name, bool passed,
                const char *category) {
  counters.testCount++;
  if (passed) {
    counters.passedCount++;
    Logger::infof("✅ %s PASSED: %s", category, name);
  } else {
    counters.failedCount++;
    Logger::errorf("❌ %s FAILED: %s", category, name);
  }
}

void printSummary(const TestCounters &counters, const char *suiteName) {
  Logger::info("\n========================================");
  Logger::infof("%s Summary", suiteName);
  Logger::info("========================================");
  Logger::infof("Total Tests: %lu", counters.testCount);
  Logger::infof("Passed: %lu", counters.passedCount);
  Logger::infof("Failed: %lu", counters.failedCount);

  if (counters.passedCount == counters.testCount && counters.testCount > 0) {
    Logger::infof("✅ ALL %s PASSED", suiteName);
  } else {
    Logger::errorf("❌ %s FAILED", suiteName);
  }
  Logger::info("========================================\n");
}

} // namespace TestUtils
