#pragma once

// ============================================================================
// memory_stress_test.h - Memory Stress Testing Module
// ============================================================================
// Tests memory allocation, deallocation, fragmentation, and leak detection.
// Validates that the system handles low memory conditions gracefully.
// ============================================================================

#include <Arduino.h>

namespace MemoryStressTest {

// Test result structure
struct MemoryTestResult {
    const char* testName;
    bool passed;
    uint32_t freeHeapBefore;
    uint32_t freeHeapAfter;
    int32_t heapDelta;
    uint32_t executionTimeMs;
};

/**
 * @brief Initialize memory stress testing
 */
void init();

/**
 * @brief Run all memory stress tests
 * @return true if all tests passed
 */
bool runAllTests();

/**
 * @brief Test repeated initialization for memory leaks
 * Tests all modules that allocate memory during init()
 * @return true if no leaks detected
 */
bool testRepeatedInitialization();

/**
 * @brief Test malloc failure handling
 * Simulates low memory conditions
 * @return true if system handles failures gracefully
 */
bool testMallocFailures();

/**
 * @brief Test heap fragmentation
 * Performs many small allocations/deallocations
 * @return true if fragmentation is within acceptable limits
 */
bool testHeapFragmentation();

/**
 * @brief Monitor heap usage over time
 * @param durationMs How long to monitor
 * @return true if heap remains stable
 */
bool testHeapStability(uint32_t durationMs);

/**
 * @brief Get current free heap size
 * @return Free heap in bytes
 */
uint32_t getFreeHeap();

/**
 * @brief Get minimum free heap since last reset
 * @return Minimum free heap in bytes
 */
uint32_t getMinFreeHeap();

/**
 * @brief Print memory statistics
 */
void printMemoryStats();

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

} // namespace MemoryStressTest
