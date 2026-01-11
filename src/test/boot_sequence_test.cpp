/**
 * @file boot_sequence_test.cpp
 * @brief Phase 12 - Boot Sequence Validation Test Implementation
 */

#include "boot_sequence_test.h"
#include "logger.h"
#include <Arduino.h>

namespace BootSequenceTest {

// ============================================================================
// Private State
// ============================================================================

static ValidationResult results;
static BootStage currentStage = BootStage::PRE_INIT;
static bool initialized = false;
static uint32_t bootStartMs = 0;

// ============================================================================
// Stage Names
// ============================================================================

static const char* STAGE_NAMES[] = {
  "PRE_INIT",
  "SYSTEM_INIT",
  "STORAGE_INIT",
  "WATCHDOG_INIT",
  "LOGGER_INIT",
  "I2C_RECOVERY_INIT",
  "PSRAM_CHECK",
  "SENSOR_INIT",
  "LIMP_MODE_INIT",
  "HUD_INIT",
  "COMPOSITOR_INIT",
  "MANAGERS_INIT",
  "BOOT_COMPLETE",
  "MAX_STAGES"  // Not used as a stage, but included to match enum count
};

// Compile-time assertion to ensure array size matches enum count
// Array has 14 elements (0-12 are stages, 13 is MAX_STAGES sentinel)
// Enum has 14 values (PRE_INIT through BOOT_COMPLETE = 13, plus MAX_STAGES = 14)
static_assert(sizeof(STAGE_NAMES) / sizeof(STAGE_NAMES[0]) == 
              static_cast<size_t>(BootSequenceTest::BootStage::MAX_STAGES) + 1,
              "STAGE_NAMES array must have MAX_STAGES + 1 elements");

// ============================================================================
// Public API Implementation
// ============================================================================

void init() {
  if (initialized) {
    return; // Already initialized
  }
  
  bootStartMs = millis();
  
  // Initialize result structure
  results.allStagesPassed = true;
  results.totalBootTimeMs = 0;
  results.failedStageCount = 0;
  
  // Initialize all stage results
  for (uint8_t i = 0; i < static_cast<uint8_t>(BootStage::MAX_STAGES); i++) {
    results.stages[i].stage = static_cast<BootStage>(i);
    results.stages[i].success = false;
    results.stages[i].startMs = 0;
    results.stages[i].durationMs = 0;
    results.stages[i].errorMsg = nullptr;
  }
  
  currentStage = BootStage::PRE_INIT;
  initialized = true;
  
  Serial.println("[BootTest] Boot sequence validation initialized");
}

void markStageStart(BootStage stage) {
  if (!initialized) {
    init();
  }
  
  uint8_t stageIdx = static_cast<uint8_t>(stage);
  if (stageIdx >= static_cast<uint8_t>(BootStage::MAX_STAGES)) {
    return;
  }
  
  currentStage = stage;
  results.stages[stageIdx].startMs = millis();
  
  Serial.print("[BootTest] Stage started: ");
  Serial.println(STAGE_NAMES[stageIdx]);
}

void markStageComplete(BootStage stage, bool success, const char* errorMsg) {
  if (!initialized) {
    return;
  }
  
  uint8_t stageIdx = static_cast<uint8_t>(stage);
  if (stageIdx >= static_cast<uint8_t>(BootStage::MAX_STAGES)) {
    return;
  }
  
  uint32_t now = millis();
  results.stages[stageIdx].success = success;
  results.stages[stageIdx].durationMs = now - results.stages[stageIdx].startMs;
  results.stages[stageIdx].errorMsg = errorMsg;
  
  if (!success) {
    results.allStagesPassed = false;
    results.failedStageCount++;
    
    Serial.print("[BootTest] Stage FAILED: ");
    Serial.print(STAGE_NAMES[stageIdx]);
    if (errorMsg) {
      Serial.print(" - ");
      Serial.println(errorMsg);
    } else {
      Serial.println();
    }
  } else {
    Serial.print("[BootTest] Stage completed: ");
    Serial.print(STAGE_NAMES[stageIdx]);
    Serial.print(" (");
    Serial.print(results.stages[stageIdx].durationMs);
    Serial.println("ms)");
  }
  
  // Update total boot time if this is the final stage
  if (stage == BootStage::BOOT_COMPLETE) {
    results.totalBootTimeMs = millis() - bootStartMs;
  }
}

BootStage getCurrentStage() {
  return currentStage;
}

const ValidationResult& getResults() {
  return results;
}

const char* getStageName(BootStage stage) {
  uint8_t stageIdx = static_cast<uint8_t>(stage);
  if (stageIdx >= static_cast<uint8_t>(BootStage::MAX_STAGES)) {
    return "UNKNOWN";
  }
  return STAGE_NAMES[stageIdx];
}

bool isBootHealthy() {
  return results.allStagesPassed && results.failedStageCount == 0;
}

bool validateBootTime(uint32_t maxBootTimeMs) {
  if (results.totalBootTimeMs == 0) {
    results.totalBootTimeMs = millis() - bootStartMs;
  }
  
  bool timeOk = results.totalBootTimeMs <= maxBootTimeMs;
  
  Serial.print("[BootTest] Boot time: ");
  Serial.print(results.totalBootTimeMs);
  Serial.print("ms (max: ");
  Serial.print(maxBootTimeMs);
  Serial.print("ms) - ");
  Serial.println(timeOk ? "OK" : "TOO SLOW");
  
  return timeOk;
}

void printResults() {
  Serial.println("\n========================================");
  Serial.println("  BOOT SEQUENCE VALIDATION REPORT");
  Serial.println("========================================\n");
  
  Serial.print("Total Boot Time: ");
  Serial.print(results.totalBootTimeMs);
  Serial.println("ms");
  
  Serial.print("Overall Status: ");
  if (results.allStagesPassed) {
    Serial.println("✓ ALL STAGES PASSED");
  } else {
    Serial.print("✗ ");
    Serial.print(results.failedStageCount);
    Serial.println(" STAGE(S) FAILED");
  }
  
  Serial.println("\nStage Breakdown:");
  Serial.println("----------------------------------------");
  
  for (uint8_t i = 0; i < static_cast<uint8_t>(BootStage::MAX_STAGES); i++) {
    if (results.stages[i].startMs > 0) {
      Serial.print("  ");
      Serial.print(STAGE_NAMES[i]);
      Serial.print(": ");
      
      if (results.stages[i].success) {
        Serial.print("✓ ");
      } else {
        Serial.print("✗ ");
      }
      
      Serial.print(results.stages[i].durationMs);
      Serial.print("ms");
      
      if (results.stages[i].errorMsg) {
        Serial.print(" - ");
        Serial.print(results.stages[i].errorMsg);
      }
      
      Serial.println();
    }
  }
  
  Serial.println("========================================\n");
}

bool runComprehensiveCheck() {
  Serial.println("[BootTest] Running comprehensive boot health check...");
  
  bool allPassed = true;
  
  // Check 1: All stages completed successfully
  if (!results.allStagesPassed) {
    Serial.println("[BootTest] ✗ Some stages failed");
    allPassed = false;
  } else {
    Serial.println("[BootTest] ✓ All stages passed");
  }
  
  // Check 2: Boot time is reasonable (max 10 seconds)
  if (!validateBootTime(10000)) {
    Serial.println("[BootTest] ✗ Boot time too long");
    allPassed = false;
  } else {
    Serial.println("[BootTest] ✓ Boot time acceptable");
  }
  
  // Check 3: Check for skipped critical stages
  bool criticalStagesOk = true;
  BootStage criticalStages[] = {
    BootStage::SYSTEM_INIT,
    BootStage::WATCHDOG_INIT,
    BootStage::LOGGER_INIT
  };
  
  for (BootStage stage : criticalStages) {
    uint8_t idx = static_cast<uint8_t>(stage);
    if (results.stages[idx].startMs == 0) {
      Serial.print("[BootTest] ✗ Critical stage skipped: ");
      Serial.println(STAGE_NAMES[idx]);
      criticalStagesOk = false;
      allPassed = false;
    }
  }
  
  if (criticalStagesOk) {
    Serial.println("[BootTest] ✓ All critical stages executed");
  }
  
  // Check 4: Verify proper ordering
  bool orderingOk = true;
  uint32_t lastStartTime = 0;
  for (uint8_t i = 0; i < static_cast<uint8_t>(BootStage::MAX_STAGES); i++) {
    if (results.stages[i].startMs > 0) {
      if (results.stages[i].startMs < lastStartTime) {
        Serial.println("[BootTest] ✗ Stage ordering violation detected");
        orderingOk = false;
        allPassed = false;
        break;
      }
      lastStartTime = results.stages[i].startMs;
    }
  }
  
  if (orderingOk) {
    Serial.println("[BootTest] ✓ Stage ordering correct");
  }
  
  // Print detailed results
  printResults();
  
  // Final verdict
  Serial.println("[BootTest] ========================================");
  if (allPassed) {
    Serial.println("[BootTest] ✓✓✓ BOOT SEQUENCE VALIDATION PASSED ✓✓✓");
    Serial.println("[BootTest] Firmware is ready for operation");
  } else {
    Serial.println("[BootTest] ✗✗✗ BOOT SEQUENCE VALIDATION FAILED ✗✗✗");
    Serial.println("[BootTest] Issues must be resolved");
  }
  Serial.println("[BootTest] ========================================");
  
  return allPassed;
}

} // namespace BootSequenceTest
