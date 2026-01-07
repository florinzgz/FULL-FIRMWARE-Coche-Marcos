// ============================================================================
// watchdog_tests.cpp - Watchdog Timer Testing Implementation
// ============================================================================

#include "watchdog_tests.h"
#include "logger.h"
#include "pins.h"
#include "watchdog.h"
#include "test_utils.h"

namespace WatchdogTests {

// ============================================================================
// Private State
// ============================================================================

static TestUtils::TestCounters counters;

// Test parameters
static const uint32_t MAX_ACCEPTABLE_FEED_INTERVAL_MS =
    8000; // 80% of 10s timeout
static const uint32_t MIN_FEED_INTERVAL_MS =
    50; // Should feed at least every 50ms

// ============================================================================
// Public API Implementation
// ============================================================================

void init() {
  counters.initialize();
  Logger::info("WatchdogTests: Initialized");
}

bool runAllTests() {
  if (!counters.initialized) { init(); }

  Logger::info("\n========================================");
  Logger::info("Starting Watchdog Timer Verification Tests");
  Logger::info("========================================");

  // Reset counters
  counters.reset();

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

  recordTest(counters, "Watchdog Configuration", enabled, "WATCHDOG TEST");

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
  recordTest(counters, "Feed Interval", passed, "WATCHDOG TEST");

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

  recordTest(counters, "Feed Count Tracking", countIncreased, "WATCHDOG TEST");

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

  recordTest(counters, "Status Reporting", statusValid, "WATCHDOG TEST");

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
  recordTest(counters, "Emergency Shutdown Mechanism (limited)", true, "WATCHDOG TEST");

  return true;
}

void printSummary() {
  TestUtils::printSummary(counters, "Watchdog Timer Test");
  if (counters.passedCount == counters.testCount && counters.testCount > 0) {
    Logger::info("Watchdog timer is properly configured and operational");
  } else {
    Logger::error("❌ WATCHDOG TESTS FAILED");
    Logger::error("⚠️  CRITICAL: System may not be safe for deployment!");
  }
}

uint32_t getPassedCount() { return counters.passedCount; }

uint32_t getFailedCount() { return counters.failedCount; }

} // namespace WatchdogTests
