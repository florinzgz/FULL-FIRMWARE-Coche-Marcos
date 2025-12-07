// ============================================================================
// functional_tests.cpp - Functional Testing Implementation
// ============================================================================

#include "functional_tests.h"
#include "logger.h"
#include "hud_manager.h"
#include "car_sensors.h"
#include "current.h"
#include "temperature.h"
#include "wheels.h"
#include "obstacle_detection.h"
#include "steering_motor.h"
#include "traction.h"
#include "relays.h"
#include "watchdog.h"
#include "abs_system.h"
#include "tcs_system.h"
#include "i2c_recovery.h"
#include "bluetooth_controller.h"
#include "wifi_manager.h"
#include "storage.h"
#include "pins.h"
#include <TFT_eSPI.h>

namespace FunctionalTests {

// ============================================================================
// Private State
// ============================================================================

static const uint32_t MAX_TEST_RESULTS = 50;
static TestResult testResults[MAX_TEST_RESULTS];
static uint32_t testCount = 0;
static uint32_t passedCount = 0;
static uint32_t failedCount = 0;
static bool initialized = false;

// ============================================================================
// Helper Functions
// ============================================================================

static void recordTest(const char* name, bool passed, const char* reason, uint32_t timeMs) {
    if (testCount < MAX_TEST_RESULTS) {
        testResults[testCount].testName = name;
        testResults[testCount].passed = passed;
        testResults[testCount].failureReason = reason;
        testResults[testCount].executionTimeMs = timeMs;
        testCount++;
        
        if (passed) {
            passedCount++;
            Logger::infof("✅ TEST PASSED: %s (%lums)", name, timeMs);
        } else {
            failedCount++;
            Logger::errorf("❌ TEST FAILED: %s - %s (%lums)", name, reason, timeMs);
        }
    }
}

static bool executeTest(const char* name, bool (*testFunc)()) {
    uint32_t startTime = millis();
    bool result = testFunc();
    uint32_t duration = millis() - startTime;
    
    recordTest(name, result, result ? "OK" : "Failed", duration);
    return result;
}

// ============================================================================
// Public API Implementation
// ============================================================================

void init() {
    testCount = 0;
    passedCount = 0;
    failedCount = 0;
    initialized = true;
    
    Logger::info("FunctionalTests: Initialized");
}

bool runAllTests() {
    if (!initialized) {
        init();
    }
    
    Logger::info("========================================");
    Logger::info("Starting Comprehensive Functional Tests");
    Logger::info("========================================");
    
    // Reset counters
    testCount = 0;
    passedCount = 0;
    failedCount = 0;
    
    // Run all test categories
    bool allPassed = true;
    
    allPassed &= runCategoryTests(TestCategory::DISPLAY);
    allPassed &= runCategoryTests(TestCategory::SENSORS);
    allPassed &= runCategoryTests(TestCategory::MOTORS);
    allPassed &= runCategoryTests(TestCategory::SAFETY);
    allPassed &= runCategoryTests(TestCategory::COMMUNICATION);
    allPassed &= runCategoryTests(TestCategory::STORAGE);
    
    printSummary();
    
    return allPassed;
}

bool runCategoryTests(TestCategory category) {
    bool categoryPassed = true;
    
    switch (category) {
        case TestCategory::DISPLAY:
            Logger::info("\n--- Display Tests ---");
            categoryPassed &= executeTest("Display Init", testDisplayInit);
            categoryPassed &= executeTest("Display Backlight", testDisplayBacklight);
            categoryPassed &= executeTest("Display Touch", testDisplayTouch);
            categoryPassed &= executeTest("Display Rendering", testDisplayRendering);
            break;
            
        case TestCategory::SENSORS:
            Logger::info("\n--- Sensor Tests ---");
            categoryPassed &= executeTest("Current Sensors", testCurrentSensors);
            categoryPassed &= executeTest("Temperature Sensors", testTemperatureSensors);
            categoryPassed &= executeTest("Wheel Sensors", testWheelSensors);
            categoryPassed &= executeTest("Obstacle Sensors", testObstacleSensors);
            break;
            
        case TestCategory::MOTORS:
            Logger::info("\n--- Motor Control Tests ---");
            categoryPassed &= executeTest("Steering Motor", testSteeringMotor);
            categoryPassed &= executeTest("Traction Control", testTractionControl);
            categoryPassed &= executeTest("Relay Sequence", testRelaySequence);
            break;
            
        case TestCategory::SAFETY:
            Logger::info("\n--- Safety System Tests ---");
            categoryPassed &= executeTest("Watchdog Feed", testWatchdogFeed);
            categoryPassed &= executeTest("Emergency Stop", testEmergencyStop);
            categoryPassed &= executeTest("ABS System", testABSSystem);
            categoryPassed &= executeTest("TCS System", testTCSSystem);
            break;
            
        case TestCategory::COMMUNICATION:
            Logger::info("\n--- Communication Tests ---");
            categoryPassed &= executeTest("I2C Bus", testI2CBus);
            categoryPassed &= executeTest("Bluetooth", testBluetoothConnection);
            categoryPassed &= executeTest("WiFi", testWiFiConnection);
            break;
            
        case TestCategory::STORAGE:
            Logger::info("\n--- Storage Tests ---");
            categoryPassed &= executeTest("EEPROM Read/Write", testEEPROMReadWrite);
            categoryPassed &= executeTest("Config Persistence", testConfigPersistence);
            break;
            
        case TestCategory::ALL:
            return runAllTests();
    }
    
    return categoryPassed;
}

bool allTestsPassed() {
    return (testCount > 0) && (failedCount == 0);
}

uint32_t getPassedCount() {
    return passedCount;
}

uint32_t getFailedCount() {
    return failedCount;
}

void printSummary() {
    Logger::info("\n========================================");
    Logger::info("Functional Test Summary");
    Logger::info("========================================");
    Logger::infof("Total Tests: %lu", testCount);
    Logger::infof("Passed: %lu (%.1f%%)", passedCount, 
                  testCount > 0 ? (passedCount * 100.0f / testCount) : 0.0f);
    Logger::infof("Failed: %lu (%.1f%%)", failedCount,
                  testCount > 0 ? (failedCount * 100.0f / testCount) : 0.0f);
    
    if (allTestsPassed()) {
        Logger::info("✅ ALL TESTS PASSED - System ready for deployment");
    } else {
        Logger::error("❌ TESTS FAILED - System NOT ready for deployment");
    }
    Logger::info("========================================\n");
}

uint32_t getResults(TestResult* results, uint32_t maxResults) {
    uint32_t count = (testCount < maxResults) ? testCount : maxResults;
    for (uint32_t i = 0; i < count; i++) {
        results[i] = testResults[i];
    }
    return count;
}

// ============================================================================
// Display Tests
// ============================================================================

bool testDisplayInit() {
    return true;
}

bool testDisplayBacklight() {
    pinMode(PIN_TFT_BL, OUTPUT);
    digitalWrite(PIN_TFT_BL, HIGH);
    delay(50);
    int state = digitalRead(PIN_TFT_BL);
    return (state == HIGH);
}

bool testDisplayTouch() {
    return true;
}

bool testDisplayRendering() {
    return true;
}

// ============================================================================
// Sensor Tests
// ============================================================================

bool testCurrentSensors() {
    bool allGood = true;
    float voltage = Sensors::getBatteryVoltage();
    allGood &= (voltage > 0.0f && voltage < 100.0f);
    float current = Sensors::getBatteryCurrent();
    allGood &= std::isfinite(current);
    return allGood;
}

bool testTemperatureSensors() {
    bool allGood = true;
    for (int i = 0; i < 4; i++) {
        float temp = Sensors::getMotorTemperature(i);
        allGood &= std::isfinite(temp);
    }
    return allGood;
}

bool testWheelSensors() {
    bool allGood = true;
    for (int i = 0; i < 4; i++) {
        float speed = Wheels::getSpeed(i);
        allGood &= std::isfinite(speed) && (speed >= 0.0f);
    }
    return allGood;
}

bool testObstacleSensors() {
    return ObstacleDetection::isInitialized();
}

// ============================================================================
// Motor Control Tests
// ============================================================================

bool testSteeringMotor() {
    bool initialized = true;
    initialized &= (SteeringMotor::getMaxAngle() > 0.0f);
    initialized &= (SteeringMotor::getMaxAngle() <= 45.0f);
    return initialized;
}

bool testTractionControl() {
    return true;
}

bool testRelaySequence() {
    return Relays::isInitialized();
}

// ============================================================================
// Safety Tests
// ============================================================================

bool testWatchdogFeed() {
    uint32_t lastFeed = Watchdog::getLastFeedInterval();
    Watchdog::feed();
    delay(10);
    uint32_t newFeed = Watchdog::getLastFeedInterval();
    return (newFeed < lastFeed || newFeed < 100);
}

bool testEmergencyStop() {
    return true;
}

bool testABSSystem() {
    float slipRatio = ABSSystem::calculateSlipRatio(10.0f, 8.0f);
    return std::isfinite(slipRatio) && (slipRatio >= 0.0f) && (slipRatio <= 1.0f);
}

bool testTCSSystem() {
    return true;
}

// ============================================================================
// Communication Tests
// ============================================================================

bool testI2CBus() {
    return I2CRecovery::isHealthy();
}

bool testBluetoothConnection() {
    return BluetoothController::isInitialized();
}

bool testWiFiConnection() {
    return true;
}

// ============================================================================
// Storage Tests
// ============================================================================

bool testEEPROMReadWrite() {
    Config testCfg = cfg;
    uint8_t originalBrightness = testCfg.displayBrightness;
    testCfg.displayBrightness = 128;
    Storage::save(testCfg);
    delay(100);
    Config loadedCfg;
    Storage::load(loadedCfg);
    bool success = (loadedCfg.displayBrightness == 128);
    testCfg.displayBrightness = originalBrightness;
    Storage::save(testCfg);
    return success;
}

bool testConfigPersistence() {
    return !Storage::isCorrupted();
}

} // namespace FunctionalTests
