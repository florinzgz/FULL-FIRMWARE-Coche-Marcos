// ============================================================================
// test_runner.cpp - Test Execution Coordinator Implementation
// ============================================================================

#include "test_runner.h"
#include "logger.h"

// Include test modules based on compile-time flags
#ifdef ENABLE_FUNCTIONAL_TESTS
#include "functional_tests.h"
#endif

#ifdef ENABLE_MEMORY_STRESS_TESTS
#include "memory_stress_test.h"
#endif

#ifdef ENABLE_HARDWARE_FAILURE_TESTS
#include "hardware_failure_tests.h"
#endif

#ifdef ENABLE_WATCHDOG_TESTS
#include "watchdog_tests.h"
#endif

namespace TestRunner {

// ============================================================================
// Private State
// ============================================================================

static uint32_t totalTests = 0;
static uint32_t totalPassed = 0;
static uint32_t totalFailed = 0;

// ============================================================================
// Public API Implementation
// ============================================================================

bool runPreDeploymentTests() {
    Logger::info("\n");
    Logger::info("╔════════════════════════════════════════════════════════════╗");
    Logger::info("║     PRE-DEPLOYMENT COMPREHENSIVE TEST SUITE                ║");
    Logger::info("║     ESP32-S3 Car Control System v2.10.0+                  ║");
    Logger::info("╚════════════════════════════════════════════════════════════╝");
    Logger::info("");
    
    totalTests = 0;
    totalPassed = 0;
    totalFailed = 0;
    
    bool allPassed = true;
    
    // ========================================================================
    // 1. FUNCTIONAL TESTS
    // ========================================================================
#ifdef ENABLE_FUNCTIONAL_TESTS
    Logger::info("\n┌────────────────────────────────────────────────────────────┐");
    Logger::info("│ 1/4: FUNCTIONAL TESTING                                    │");
    Logger::info("└────────────────────────────────────────────────────────────┘");
    
    FunctionalTests::init();
    bool functionalOk = FunctionalTests::runAllTests();
    
    totalPassed += FunctionalTests::getPassedCount();
    totalFailed += FunctionalTests::getFailedCount();
    totalTests += FunctionalTests::getPassedCount() + FunctionalTests::getFailedCount();
    
    allPassed &= functionalOk;
#else
    Logger::info("\n⏭️  FUNCTIONAL TESTS: Skipped (not enabled)");
#endif
    
    // ========================================================================
    // 2. MEMORY STRESS TESTS
    // ========================================================================
#ifdef ENABLE_MEMORY_STRESS_TESTS
    Logger::info("\n┌────────────────────────────────────────────────────────────┐");
    Logger::info("│ 2/4: MEMORY STRESS TESTING                                 │");
    Logger::info("└────────────────────────────────────────────────────────────┘");
    
    MemoryStressTest::init();
    bool memoryOk = MemoryStressTest::runAllTests();

    totalPassed += MemoryStressTest::getPassedCount();
    totalFailed += MemoryStressTest::getFailedCount();
    totalTests += MemoryStressTest::getPassedCount() + MemoryStressTest::getFailedCount();

    allPassed &= memoryOk;
#else
    Logger::info("\n⏭️  MEMORY STRESS TESTS: Skipped (not enabled)");
#endif
    
    // ========================================================================
    // 3. HARDWARE FAILURE TESTS
    // ========================================================================
#ifdef ENABLE_HARDWARE_FAILURE_TESTS
    Logger::info("\n┌────────────────────────────────────────────────────────────┐");
    Logger::info("│ 3/4: HARDWARE FAILURE SCENARIO TESTING                     │");
    Logger::info("└────────────────────────────────────────────────────────────┘");
    
    HardwareFailureTests::init();
    bool hardwareOk = HardwareFailureTests::runAllTests();

    totalPassed += HardwareFailureTests::getPassedCount();
    totalFailed += HardwareFailureTests::getFailedCount();
    totalTests += HardwareFailureTests::getPassedCount() + HardwareFailureTests::getFailedCount();

    allPassed &= hardwareOk;
#else
    Logger::info("\n⏭️  HARDWARE FAILURE TESTS: Skipped (not enabled)");
#endif
    
    // ========================================================================
    // 4. WATCHDOG TIMER TESTS
    // ========================================================================
#ifdef ENABLE_WATCHDOG_TESTS
    Logger::info("\n┌────────────────────────────────────────────────────────────┐");
    Logger::info("│ 4/4: WATCHDOG TIMER VERIFICATION                           │");
    Logger::info("└────────────────────────────────────────────────────────────┘");
    
    WatchdogTests::init();
    bool watchdogOk = WatchdogTests::runAllTests();

    totalPassed += WatchdogTests::getPassedCount();
    totalFailed += WatchdogTests::getFailedCount();
    totalTests += WatchdogTests::getPassedCount() + WatchdogTests::getFailedCount();

    allPassed &= watchdogOk;
#else
    Logger::info("\n⏭️  WATCHDOG TESTS: Skipped (not enabled)");
#endif
    
    // ========================================================================
    // OVERALL SUMMARY
    // ========================================================================
    printOverallSummary();
    
    return allPassed;
}

bool isTestModeEnabled() {
#if defined(ENABLE_FUNCTIONAL_TESTS) || \
    defined(ENABLE_MEMORY_STRESS_TESTS) || \
    defined(ENABLE_HARDWARE_FAILURE_TESTS) || \
    defined(ENABLE_WATCHDOG_TESTS)
    return true;
#else
    return false;
#endif
}

void printOverallSummary() {
    Logger::info("\n");
    Logger::info("╔════════════════════════════════════════════════════════════╗");
    Logger::info("║           OVERALL TEST SUMMARY                             ║");
    Logger::info("╚════════════════════════════════════════════════════════════╝");
    Logger::info("");
    
    Logger::infof("📊 Total Tests Run: %lu", totalTests);
    Logger::infof("✅ Tests Passed: %lu", totalPassed);
    Logger::infof("❌ Tests Failed: %lu", totalFailed);
    
    if (totalTests > 0) {
        float passRate = (totalPassed * 100.0f) / totalTests;
        Logger::infof("📈 Pass Rate: %.1f%%", passRate);
    }
    
    Logger::info("");
    
    if (totalFailed == 0 && totalTests > 0) {
        Logger::info("╔════════════════════════════════════════════════════════════╗");
        Logger::info("║  ✅ ✅ ✅  ALL TESTS PASSED  ✅ ✅ ✅                      ║");
        Logger::info("║                                                            ║");
        Logger::info("║  System is READY for production deployment                ║");
        Logger::info("║                                                            ║");
        Logger::info("║  Next Steps:                                               ║");
        Logger::info("║  1. Review test output for any warnings                   ║");
        Logger::info("║  2. Complete sign-off checklist in DEPLOYMENT_TESTING_GUIDE.md ║");
        Logger::info("║  3. Deploy to production hardware                         ║");
        Logger::info("╚════════════════════════════════════════════════════════════╝");
    } else if (totalTests == 0) {
        Logger::warn("╔════════════════════════════════════════════════════════════╗");
        Logger::warn("║  ⚠️  WARNING: No tests were run                            ║");
        Logger::warn("║                                                            ║");
        Logger::warn("║  Enable test modules in platformio.ini to run tests       ║");
        Logger::warn("╚════════════════════════════════════════════════════════════╝");
    } else {
        Logger::error("╔════════════════════════════════════════════════════════════╗");
        Logger::error("║  ❌ ❌ ❌  TESTS FAILED  ❌ ❌ ❌                          ║");
        Logger::error("║                                                            ║");
        Logger::error("║  System is NOT READY for deployment                       ║");
        Logger::error("║                                                            ║");
        Logger::error("║  Required Actions:                                         ║");
        Logger::error("║  1. Review failed tests in detail                         ║");
        Logger::error("║  2. Fix identified issues                                 ║");
        Logger::error("║  3. Re-run tests until all pass                           ║");
        Logger::error("║  4. DO NOT deploy until all tests pass                    ║");
        Logger::error("╚════════════════════════════════════════════════════════════╝");
    }
    
    Logger::info("");
}

} // namespace TestRunner
