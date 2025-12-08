# ESP32-S3 Car Control System - Pre-Deployment Testing Guide

**Version:** 2.10.0+  
**Date:** 2025-12-07  
**Document Status:** Production

---

## Table of Contents

1. [Overview](#overview)
2. [Test Categories](#test-categories)
3. [Running Tests](#running-tests)
4. [Test Environments](#test-environments)
5. [Expected Results](#expected-results)
6. [Troubleshooting](#troubleshooting)
7. [Sign-Off Checklist](#sign-off-checklist)

---

## Overview

This guide provides comprehensive instructions for pre-deployment testing of the ESP32-S3 Car Control System firmware. All tests must pass before deploying to production hardware.

### Testing Objectives

✅ **Functional Testing** - Verify all subsystems work correctly  
✅ **Memory Stress Testing** - Ensure no memory leaks or fragmentation  
✅ **Hardware Failure Scenarios** - Validate graceful degradation  
✅ **Watchdog Timeout Verification** - Confirm safety mechanisms work

### Safety-Critical Requirements

⚠️ **WARNING**: This firmware controls a vehicle. All safety-critical systems MUST be tested and verified before deployment:

- Emergency stop functionality
- Watchdog timer operation
- Relay sequencing
- ABS/TCS systems
- Obstacle detection

---

## Test Categories

### 1. Functional Tests

**Purpose:** Verify all subsystems operate correctly under normal conditions

**Modules Tested:**
- Display system (TFT, touch, backlight)
- Current sensors (INA226)
- Temperature sensors (DS18B20)
- Wheel speed sensors
- Obstacle detection (VL53L5CX)
- Steering motor control
- Traction control
- Relay sequencing
- Safety systems (ABS, TCS)
- Communication (I2C, Bluetooth, WiFi)
- Storage (EEPROM, config persistence)

**Header:** `include/functional_tests.h`  
**Implementation:** `src/test/functional_tests.cpp`

### 2. Memory Stress Tests

**Purpose:** Detect memory leaks and validate heap stability

**Tests Performed:**
- Repeated module initialization (leak detection)
- Heap fragmentation testing
- Heap stability monitoring (5 second cycles)
- Malloc failure handling (nullptr checks)

**Header:** `include/memory_stress_test.h`  
**Implementation:** `src/test/memory_stress_test.cpp`

### 3. Hardware Failure Tests

**Purpose:** Ensure graceful degradation under hardware failures

**Scenarios Tested:**
- I2C bus failure and recovery
- Sensor disconnection handling
- Display communication failure
- Power supply variation monitoring

**Header:** `include/hardware_failure_tests.h`  
**Implementation:** `src/test/hardware_failure_tests.cpp`

### 4. Watchdog Timer Tests

**Purpose:** Verify watchdog protection is active and functional

**Tests Performed:**
- Watchdog configuration verification
- Feed interval monitoring
- Feed count tracking
- Status reporting
- Emergency shutdown mechanism validation

**Header:** `include/watchdog_tests.h`  
**Implementation:** `src/test/watchdog_tests.cpp`

---

## Running Tests

### Method 1: PlatformIO Test Environment

The easiest way to run all tests is using the pre-deployment test environment:

```bash
# Build and upload test firmware
pio run -e esp32-s3-devkitc-predeployment -t upload

# Monitor test output
pio device monitor -e esp32-s3-devkitc-predeployment
```

### Method 2: Manual Test Execution

If you need to run tests manually from code:

```cpp
#include "functional_tests.h"
#include "memory_stress_test.h"
#include "hardware_failure_tests.h"
#include "watchdog_tests.h"

void runPreDeploymentTests() {
    // Initialize all test modules
    FunctionalTests::init();
    MemoryStressTest::init();
    HardwareFailureTests::init();
    WatchdogTests::init();
    
    // Run all tests
    bool functionalOk = FunctionalTests::runAllTests();
    bool memoryOk = MemoryStressTest::runAllTests();
    bool hardwareOk = HardwareFailureTests::runAllTests();
    bool watchdogOk = WatchdogTests::runAllTests();
    
    // Overall result
    bool allTestsPassed = functionalOk && memoryOk && 
                          hardwareOk && watchdogOk;
    
    if (allTestsPassed) {
        Logger::info("✅ ALL PRE-DEPLOYMENT TESTS PASSED");
        Logger::info("System is READY for production deployment");
    } else {
        Logger::error("❌ PRE-DEPLOYMENT TESTS FAILED");
        Logger::error("System is NOT READY for deployment");
        Logger::error("Review test output and fix issues before deploying");
    }
}
```

### Method 3: Hidden Menu Test Access

Tests can also be triggered from the hidden menu on the device:

1. Enter hidden menu (touch sequence: top-left → top-right → bottom-right → bottom-left)
2. Navigate to "System Tests" option
3. Select test category to run
4. View results on display

---

## Test Environments

### Pre-Deployment Environment

**Environment Name:** `esp32-s3-devkitc-predeployment`

**Configuration:**
- All test modules enabled
- Maximum debug output
- Extended stack sizes for test execution
- Test mode flags enabled

**Build Flags:**
```ini
-DTEST_MODE
-DENABLE_FUNCTIONAL_TESTS
-DENABLE_MEMORY_STRESS_TESTS
-DENABLE_HARDWARE_FAILURE_TESTS
-DENABLE_WATCHDOG_TESTS
-DCORE_DEBUG_LEVEL=5
```

### Standard Test Environment

**Environment Name:** `esp32-s3-devkitc-test`

**Configuration:**
- Standalone display mode
- All LED/sensor tests enabled
- Moderate debug output

---

## Expected Results

### Functional Tests

**Expected Pass Rate:** 100%

**Typical Output:**
```
========================================
Starting Comprehensive Functional Tests
========================================

--- Display Tests ---
✅ TEST PASSED: Display Init (5ms)
✅ TEST PASSED: Display Backlight (52ms)
✅ TEST PASSED: Display Touch (3ms)
✅ TEST PASSED: Display Rendering (1ms)

--- Sensor Tests ---
✅ TEST PASSED: Current Sensors (125ms)
✅ TEST PASSED: Temperature Sensors (850ms)
✅ TEST PASSED: Wheel Sensors (15ms)
✅ TEST PASSED: Obstacle Sensors (45ms)

[... more tests ...]

========================================
Functional Test Summary
========================================
Total Tests: 22
Passed: 22 (100.0%)
Failed: 0 (0.0%)
✅ ALL TESTS PASSED - System ready for deployment
========================================
```

### Memory Stress Tests

**Expected Behavior:**
- No memory leaks (heap delta < 128 bytes tolerance)
- Heap remains stable during operation
- Minimum free heap > 100KB

**Typical Output:**
```
========================================
Starting Memory Stress Tests
========================================

--- Memory Statistics ---
Total Heap: 327680 bytes
Free Heap: 270532 bytes (82.6%)
Min Free Heap: 268420 bytes
Largest Free Block: 245760 bytes
-------------------------

--- Testing Repeated Initialization (Memory Leak Detection) ---
✅ MEMORY TEST PASSED: Shifter Repeated Init (delta: -64 bytes, 125ms)
✅ MEMORY TEST PASSED: Current Sensors Repeated Init (delta: 0 bytes, 1520ms)

--- Testing Heap Fragmentation ---
Largest free block before: 245760 bytes
Largest free block after: 245120 bytes
✅ MEMORY TEST PASSED: Heap Fragmentation (delta: -32 bytes, 245ms)

--- Testing Heap Stability ---
✅ MEMORY TEST PASSED: Heap Stability (delta: -16 bytes, 5002ms)

========================================
Memory Stress Test Summary
========================================
Total Tests: 4
Passed: 4
Failed: 0

--- Memory Statistics ---
Total Heap: 327680 bytes
Free Heap: 270480 bytes (82.6%)
Min Free Heap: 268350 bytes
Largest Free Block: 245120 bytes
-------------------------

✅ ALL MEMORY TESTS PASSED
========================================
```

### Hardware Failure Tests

**Expected Pass Rate:** 100%

**Note:** Some tests may show warnings if hardware is not fully connected, but should still pass.

### Watchdog Tests

**Expected Pass Rate:** 100%

**Critical Requirements:**
- Watchdog MUST be enabled
- Feed interval MUST be < 8000ms
- Feed count MUST increase
- Emergency shutdown mechanism MUST be present

---

## Troubleshooting

### Common Issues

#### 1. Display Tests Fail

**Symptoms:**
- Display backlight test fails
- Touch test fails
- Rendering tests fail

**Solutions:**
- Check SPI connections (pins 10, 11, 12, 13, 14, 16)
- Verify backlight pin (GPIO 42) connection
- Check touch CS pin (GPIO 21) connection
- Ensure display power supply is adequate

#### 2. Sensor Tests Fail

**Symptoms:**
- Current sensor tests fail
- Temperature sensor tests fail
- Wheel sensor tests fail

**Solutions:**
- Verify I2C bus connections (SDA: GPIO 8, SCL: GPIO 9)
- Check sensor power supplies
- Run I2C bus scan to verify sensor addresses
- Check for I2C pull-up resistors (4.7kΩ recommended)

#### 3. Memory Tests Show Leaks

**Symptoms:**
- Heap delta > 128 bytes on repeated init
- Free heap decreases over time
- Heap fragmentation excessive

**Solutions:**
- Review recent code changes
- Check for missing delete/free calls
- Use heap profiler to identify leak source
- Verify all malloc/new calls have corresponding free/delete

#### 4. Watchdog Tests Fail

**Symptoms:**
- Watchdog not enabled
- Feed interval too long
- Feed count not increasing

**Solutions:**
- Verify Watchdog::init() is called in setup()
- Ensure Watchdog::feed() is called in main loop
- Check for blocking delays in main loop
- Reduce maximum loop iteration time

---

## Sign-Off Checklist

Before deploying firmware to production, complete this checklist:

### Pre-Deployment Testing

- [ ] All functional tests pass (100%)
- [ ] All memory stress tests pass (100%)
- [ ] All hardware failure tests pass (100%)
- [ ] All watchdog tests pass (100%)
- [ ] No memory leaks detected (< 128 byte tolerance)
- [ ] Free heap remains > 100KB under normal operation
- [ ] Watchdog feed interval < 8 seconds
- [ ] Emergency shutdown mechanism verified

### Safety-Critical Verification

- [ ] Emergency stop button tested and functional
- [ ] Watchdog timeout triggers emergency shutdown
- [ ] Relay sequence operates in correct order (Main → Trac → Dir)
- [ ] ABS system responds to wheel slip correctly
- [ ] TCS system prevents wheel spin correctly
- [ ] Obstacle detection alerts function correctly
- [ ] Brake light activation works correctly
- [ ] All safety warnings display properly

### Hardware Integration

- [ ] All sensors report valid readings
- [ ] Display renders correctly (no ghosting/artifacts)
- [ ] Touch input responds accurately
- [ ] Motor control operates smoothly
- [ ] Steering control is responsive and accurate
- [ ] Battery monitoring shows correct voltage/current
- [ ] Temperature monitoring shows realistic values

### Performance Verification

- [ ] System boots successfully in < 5 seconds
- [ ] Main loop iteration time < 100ms
- [ ] Display refresh rate adequate (no visible lag)
- [ ] Touch response latency < 100ms
- [ ] Sensor update rate meets specifications
- [ ] No unexpected system resets observed

### Documentation

- [ ] Test results documented
- [ ] Any test failures investigated and resolved
- [ ] Configuration changes documented
- [ ] Known issues/limitations documented
- [ ] Deployment notes prepared

### Final Approval

**Tested By:** ________________  
**Date:** ________________  
**Signature:** ________________  

**Approved By:** ________________  
**Date:** ________________  
**Signature:** ________________  

---

## Appendix: Test Result Analysis

### Interpreting Test Results

**PASSED (✅):**
- Test completed successfully
- All criteria met
- No issues detected

**FAILED (❌):**
- Test did not complete successfully
- One or more criteria not met
- Issue must be investigated and resolved

### Performance Metrics

| Metric | Target | Warning | Critical |
|--------|--------|---------|----------|
| Free Heap | > 150KB | < 150KB | < 100KB |
| Feed Interval | < 5s | 5-8s | > 8s |
| Test Pass Rate | 100% | 95-99% | < 95% |
| Memory Leak | 0 bytes | < 128 bytes | > 128 bytes |
| Boot Time | < 3s | 3-5s | > 5s |

### Test Execution Log Template

```
========================================
PRE-DEPLOYMENT TEST EXECUTION LOG
========================================
Date: YYYY-MM-DD
Time: HH:MM:SS
Firmware Version: X.X.X
Hardware Version: X.X
Tester: [Name]

FUNCTIONAL TESTS:
- Total: XX
- Passed: XX
- Failed: XX
- Pass Rate: XX%

MEMORY STRESS TESTS:
- Total: XX
- Passed: XX
- Failed: XX
- Free Heap: XXXXX bytes
- Min Free Heap: XXXXX bytes

HARDWARE FAILURE TESTS:
- Total: XX
- Passed: XX
- Failed: XX

WATCHDOG TESTS:
- Total: XX
- Passed: XX
- Failed: XX
- Watchdog Enabled: YES/NO
- Feed Interval: XXXXms

OVERALL RESULT: PASS/FAIL

NOTES:
[Any observations, warnings, or issues]

SIGN-OFF:
Tester: ________________
Date: ________________
========================================
```

---

**End of Document**
