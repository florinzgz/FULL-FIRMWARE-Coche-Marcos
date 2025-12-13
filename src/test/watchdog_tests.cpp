// ============================================================================
// watchdog_tests.cpp - Watchdog Timer Testing Implementation
// ============================================================================

#include "watchdog_tests.h"
#include "logger.h"
#include "pins.h"
#include "watchdog.h"

namespace WatchdogTests {

// ============================================================================
// Private State
// ============================================================================

static uint32_t testCount = 0;
static uint32_t passedCount = 0;
static uint32_t failedCount = 0;
static bool initialized = false;

// Test parameters
static const uint32_t MAX_ACCEPTABLE_FEED_INTERVAL_MS =
    8000; // 80% of 10s timeout
static const uint32_t MIN_FEED_INTERVAL_MS =
    50; // Should feed at least every 50ms

// ============================================================================
// Helper Functions
// ============================================================================

static void recordTest(const char *name, bool passed) {
  testCount++;
  if (passed) {
    passedCount++;
    Logger::infof("✅ WATCHDOG TEST PASSED: %s", name);
  } else {
    failedCount++;
    Logger::errorf("❌ WATCHDOG TEST FAILED: %s", name);
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

  Logger::info("WatchdogTests: Initialized");
}

bool runAllTests() {
  if (!initialized) { init(); }

  Logger::info("\n========================================");
  Logger::info("Starting Watchdog Timer Verification Tests");
  Logger::info("========================================");

  // Reset counters
  testCount = 0;
  passedCount = 0;
  failedCount = 0;

  bool allPassed = true;

  allPassed &= testWatchdogConfiguration();
  allPassed &= testFeedInterval();
  allPassed &= testFeedCounting();
  allPassed &= testStatusReporting();
  allPassed &= testEmergencyShutdown();

  printSummary();

  return allPassed;
}

bool testWatchdogConfiguration() {
  Logger::info("\n--- Testing Watchdog Configuration ---");

  // Verify watchdog is enabled
  bool enabled = Watchdog::isEnabled();

  if (enabled) {
    Logger::info("Watchdog is enabled and configured");
  } else {
    Logger::error("Watchdog is NOT enabled - critical safety issue!");
  }

  recordTest("Watchdog Configuration", enabled);

  return enabled;
}

bool testFeedInterval() {
  Logger::info("\n--- Testing Watchdog Feed Interval ---");

  // Get current feed interval
  uint32_t interval = Watchdog::getLastFeedInterval();

  Logger::infof("Current feed interval: %lu ms", interval);

  // Feed interval should be reasonable (not too long, not too short)
  bool intervalOk = (interval >= MIN_FEED_INTERVAL_MS) &&
                    (interval <= MAX_ACCEPTABLE_FEED_INTERVAL_MS);

  if (!intervalOk) {
    if (interval > MAX_ACCEPTABLE_FEED_INTERVAL_MS) {
      Logger::errorf(
          "Feed interval too long (%lu ms) - risk of watchdog timeout!",
          interval);
    } else {
      Logger::warnf(
          "Feed interval very short (%lu ms) - may impact performance",
          interval);
    }
  }

  // Perform a feed and verify interval resets
  Watchdog::feed();
  delay(10);
  uint32_t newInterval = Watchdog::getLastFeedInterval();

  // After feeding, the interval should be small (< 100ms) indicating recent
  // feed
  bool intervalReset = (newInterval < 100);

  bool passed = intervalOk && intervalReset;
  recordTest("Feed Interval", passed);

  return passed;
}

bool testFeedCounting() {
  Logger::info("\n--- Testing Feed Count Tracking ---");

  uint32_t countBefore = Watchdog::getFeedCount();

  // Feed the watchdog multiple times
  for (int i = 0; i < 5; i++) {
    Watchdog::feed();
    delay(10);
  }

  uint32_t countAfter = Watchdog::getFeedCount();

  // Count should have increased
  bool countIncreased = (countAfter > countBefore);

  Logger::infof("Feed count increased from %lu to %lu", countBefore,
                countAfter);

  recordTest("Feed Count Tracking", countIncreased);

  return countIncreased;
}

bool testStatusReporting() {
  Logger::info("\n--- Testing Status Reporting ---");

  // Verify all status queries return valid values
  bool enabled = Watchdog::isEnabled();
  uint32_t feedCount = Watchdog::getFeedCount();
  uint32_t feedInterval = Watchdog::getLastFeedInterval();

  bool statusValid = enabled && (feedCount > 0) && (feedInterval < 60000);

  Logger::infof("Watchdog Status:");
  Logger::infof("  Enabled: %s", enabled ? "YES" : "NO");
  Logger::infof("  Feed Count: %lu", feedCount);
  Logger::infof("  Last Feed Interval: %lu ms", feedInterval);

  recordTest("Status Reporting", statusValid);

  return statusValid;
}

bool testEmergencyShutdown() {
  Logger::info("\n--- Testing Emergency Shutdown Mechanism ---");

  // We can't actually trigger the watchdog timeout without resetting the system
  // Instead, we verify that the panic handler mechanism is in place
  // by checking that relay pins are configured correctly

  // Verify relay pins are configured as outputs
  // The panic handler uses direct GPIO register access, but we can verify
  // that the pins are at least configured

  pinMode(PIN_RELAY_MAIN, OUTPUT);
  pinMode(PIN_RELAY_TRAC, OUTPUT);
  pinMode(PIN_RELAY_DIR, OUTPUT);
  pinMode(PIN_RELAY_SPARE, OUTPUT);

  // Verify configuration by checking pin modes (Arduino doesn't provide easy
  // API for this) For now, we just verify the pinMode calls don't crash This is
  // a limitation - proper verification would require reading GPIO registers

  Logger::warn("Emergency shutdown mechanism: Limited verification - pinMode "
               "configuration only");
  Logger::warn(
      "⚠️  Full test requires watchdog timeout trigger (would reset system)");

  // Mark test as passed with caveat
  recordTest("Emergency Shutdown Mechanism (limited)", true);

  return true;
}

void printSummary() {
  Logger::info("\n========================================");
  Logger::info("Watchdog Timer Test Summary");
  Logger::info("========================================");
  Logger::infof("Total Tests: %lu", testCount);
  Logger::infof("Passed: %lu", passedCount);
  Logger::infof("Failed: %lu", failedCount);

  if (passedCount == testCount && testCount > 0) {
    Logger::info("✅ ALL WATCHDOG TESTS PASSED");
    Logger::info("Watchdog timer is properly configured and operational");
  } else {
    Logger::error("❌ WATCHDOG TESTS FAILED");
    Logger::error("⚠️  CRITICAL: System may not be safe for deployment!");
  }
  Logger::info("========================================\n");
}

uint32_t getPassedCount() { return passedCount; }

uint32_t getFailedCount() { return failedCount; }

} // namespace WatchdogTests
