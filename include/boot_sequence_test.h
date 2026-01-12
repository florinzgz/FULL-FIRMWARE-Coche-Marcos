#pragma once
/**
 * @file boot_sequence_test.h
 * @brief Phase 12 - Boot Sequence Validation Test
 * 
 * Comprehensive boot sequence validator that can run on actual hardware
 * to verify that the firmware boots correctly and all subsystems initialize
 * in the proper order without crashes or hangs.
 * 
 * This test is designed to be run as part of pre-deployment validation.
 */

#include <Arduino.h>
#include <stdint.h>

namespace BootSequenceTest {

/**
 * @brief Boot stage identifiers
 */
enum class BootStage : uint8_t {
  PRE_INIT = 0,
  SYSTEM_INIT,
  STORAGE_INIT,
  WATCHDOG_INIT,
  LOGGER_INIT,
  I2C_RECOVERY_INIT,
  PSRAM_CHECK,
  SENSOR_INIT,
  LIMP_MODE_INIT,
  HUD_INIT,
  COMPOSITOR_INIT,
  MANAGERS_INIT,
  BOOT_COMPLETE,
  MAX_STAGES
};

/**
 * @brief Boot stage result
 */
struct StageResult {
  BootStage stage;
  bool success;
  uint32_t startMs;
  uint32_t durationMs;
  const char* errorMsg;
};

/**
 * @brief Boot validation result
 */
struct ValidationResult {
  bool allStagesPassed;
  uint32_t totalBootTimeMs;
  uint8_t failedStageCount;
  StageResult stages[static_cast<uint8_t>(BootStage::MAX_STAGES)];
};

/**
 * @brief Initialize the boot sequence test
 * 
 * Must be called very early in setup(), before any other initialization.
 */
void init();

/**
 * @brief Mark a boot stage as started
 * @param stage The boot stage that is starting
 */
void markStageStart(BootStage stage);

/**
 * @brief Mark a boot stage as completed
 * @param stage The boot stage that completed
 * @param success Whether the stage completed successfully
 * @param errorMsg Optional error message if stage failed
 */
void markStageComplete(BootStage stage, bool success, const char* errorMsg = nullptr);

/**
 * @brief Get the current boot stage
 * @return Current boot stage
 */
BootStage getCurrentStage();

/**
 * @brief Get validation results
 * @return Reference to validation results
 */
const ValidationResult& getResults();

/**
 * @brief Print validation results to Serial
 */
void printResults();

/**
 * @brief Check if boot sequence is healthy
 * @return true if all stages passed, false otherwise
 */
bool isBootHealthy();

/**
 * @brief Get name of a boot stage
 * @param stage Boot stage
 * @return Stage name as string
 */
const char* getStageName(BootStage stage);

/**
 * @brief Validate that the boot sequence completed in a reasonable time
 * @param maxBootTimeMs Maximum acceptable boot time in milliseconds
 * @return true if boot time is acceptable
 */
bool validateBootTime(uint32_t maxBootTimeMs = 10000);

/**
 * @brief Run comprehensive boot health check
 * 
 * This should be called after all initialization is complete.
 * It validates:
 * - All stages completed successfully
 * - Boot time is reasonable
 * - No stages skipped
 * - Proper ordering
 * 
 * @return true if all checks pass
 */
bool runComprehensiveCheck();

} // namespace BootSequenceTest
