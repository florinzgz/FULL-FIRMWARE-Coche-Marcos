#include "boot_guard.h"
#include "obstacle_config.h"
#include "pins.h"
#include <Arduino.h>
#include <esp_system.h>
#include <rom/rtc.h>

// ============================================================================
// 🔒 v2.17.1: Boot Counter for Bootloop Detection
// ============================================================================
// Uses RTC_NOINIT_ATTR to persist data across soft resets
// RTC memory survives warm resets but not power cycles
// ============================================================================

#define BOOTLOOP_DETECTION_THRESHOLD 3
#define BOOTLOOP_DETECTION_WINDOW_MS 60000 // 60 seconds

struct BootCounterData {
  uint32_t magic;         // Magic number to validate data
  uint8_t bootCount;      // Number of boots
  uint32_t firstBootMs;   // Timestamp of first boot in sequence
  uint32_t lastBootMs;    // Timestamp of last boot
  bool safeModeRequested; // Flag to enter safe mode
  uint8_t resetMarker;    // Persisted reset marker for diagnostics
};

// RTC memory - survives warm reset, cleared on power cycle
static RTC_NOINIT_ATTR BootCounterData bootCounterData;
static constexpr uint32_t BOOT_COUNTER_MAGIC = 0xB007C047; // "BOOT COTR"

void BootGuard::applyXshutStrappingGuard() {
  // v2.15.0: TOFSense-M S migration - No XSHUT pins needed (UART sensor)
  // VL53L5X I2C sensors required XSHUT pins for power sequencing
  // TOFSense-M S uses UART and doesn't have XSHUT pins
  // This function is now a no-op but retained for backward compatibility
  static bool alreadyApplied = false;
#if defined(ESP32) || defined(ESP_PLATFORM)
  static portMUX_TYPE bootGuardMux = portMUX_INITIALIZER_UNLOCKED;
  portENTER_CRITICAL(&bootGuardMux);
#endif
  if (alreadyApplied) {
#if defined(ESP32) || defined(ESP_PLATFORM)
    portEXIT_CRITICAL(&bootGuardMux);
#endif
    return;
  }
  alreadyApplied = true;
#if defined(ESP32) || defined(ESP_PLATFORM)
  portEXIT_CRITICAL(&bootGuardMux);
#endif

  // No action needed - TOFSense-M S doesn't use XSHUT pins
  // v2.17.3: Removed Logger call - can be called before Logger::init()
  Serial.println(
      "[BootGuard] XSHUT guard skipped (TOFSense-M S has no XSHUT pins)");
}

// ============================================================================
// Boot Counter Implementation
// ============================================================================

void BootGuard::initBootCounter() {
  // v2.17.3: Use Serial instead of Logger - can be called before Logger::init()
  // Check if RTC memory has valid data
  if (bootCounterData.magic != BOOT_COUNTER_MAGIC) {
    // First boot or power cycle - initialize
    bootCounterData.magic = BOOT_COUNTER_MAGIC;
    bootCounterData.bootCount = 0;
    bootCounterData.firstBootMs = 0;
    bootCounterData.lastBootMs = 0;
    bootCounterData.safeModeRequested = false;
    bootCounterData.resetMarker = static_cast<uint8_t>(RESET_MARKER_NONE);
    Serial.println(
        "[BootGuard] Boot counter initialized (power cycle or first boot)");
  } else {
    Serial.printf("[BootGuard] Boot counter found - %d previous boots\n",
                  bootCounterData.bootCount);
  }
}

void BootGuard::incrementBootCounter() {
  // v2.17.3: Use Serial instead of Logger - can be called before Logger::init()
  uint32_t now = millis();

  // If this is the first boot in the sequence, record timestamp
  if (bootCounterData.bootCount == 0) {
    bootCounterData.firstBootMs = now;
    bootCounterData.lastBootMs = now;
    bootCounterData.bootCount = 1;
    Serial.println("[BootGuard] Starting new boot sequence");
    return;
  }

  // Check if we're still within the detection window
  uint32_t timeSinceFirst = now - bootCounterData.firstBootMs;

  if (timeSinceFirst > BOOTLOOP_DETECTION_WINDOW_MS) {
    // Outside window - reset counter
    bootCounterData.firstBootMs = now;
    bootCounterData.lastBootMs = now;
    bootCounterData.bootCount = 1;
    bootCounterData.safeModeRequested = false;
    Serial.println(
        "[BootGuard] Boot detection window expired - resetting counter");
    return;
  }

  // Within window - increment counter
  bootCounterData.bootCount++;
  bootCounterData.lastBootMs = now;

  Serial.printf("[BootGuard] Boot #%d within %lu ms\n",
                bootCounterData.bootCount, timeSinceFirst);

  // Check for bootloop condition
  if (bootCounterData.bootCount >= BOOTLOOP_DETECTION_THRESHOLD) {
    bootCounterData.safeModeRequested = true;
    Serial.printf(
        "[BootGuard] WARNING: BOOTLOOP DETECTED - %d boots in %lu ms\n",
        bootCounterData.bootCount, timeSinceFirst);
    Serial.println("[BootGuard] Safe mode will be activated");
  }
}

void BootGuard::clearBootCounter() {
  // v2.17.3: Use Serial instead of Logger - Logger may not be initialized yet
  // Called from loop() to indicate successful boot
  if (bootCounterData.bootCount > 0) {
    Serial.printf("[BootGuard] Boot successful - clearing counter (was %d)\n",
                  bootCounterData.bootCount);
  }

  bootCounterData.bootCount = 0;
  bootCounterData.firstBootMs = 0;
  bootCounterData.lastBootMs = 0;
  bootCounterData.safeModeRequested = false;
}

uint8_t BootGuard::getBootCount() { return bootCounterData.bootCount; }

bool BootGuard::isBootloopDetected() {
  return bootCounterData.bootCount >= BOOTLOOP_DETECTION_THRESHOLD;
}

bool BootGuard::shouldEnterSafeMode() {
  return bootCounterData.safeModeRequested;
}

void BootGuard::setResetMarker(ResetMarker marker) {
  if (bootCounterData.magic != BOOT_COUNTER_MAGIC) {
    bootCounterData.magic = BOOT_COUNTER_MAGIC;
    bootCounterData.bootCount = 0;
    bootCounterData.firstBootMs = 0;
    bootCounterData.lastBootMs = 0;
    bootCounterData.safeModeRequested = false;
  }
  bootCounterData.resetMarker = static_cast<uint8_t>(marker);
}

BootGuard::ResetMarker BootGuard::getResetMarker() {
  return static_cast<ResetMarker>(bootCounterData.resetMarker);
}

void BootGuard::clearResetMarker() {
  bootCounterData.resetMarker = static_cast<uint8_t>(RESET_MARKER_NONE);
}

void BootGuard::logResetMarker() {
  ResetMarker marker = getResetMarker();
  switch (marker) {
  case RESET_MARKER_EXPLICIT_RESTART:
    Serial.println("[BootGuard] Reset marker: explicit restart");
    break;
  case RESET_MARKER_WATCHDOG_LOOP:
    Serial.println("[BootGuard] Reset marker: watchdog timeout expected");
    break;
  case RESET_MARKER_I2C_PREINIT:
    Serial.println("[BootGuard] Reset marker: I2C used before init");
    break;
  case RESET_MARKER_NULL_POINTER:
    Serial.println("[BootGuard] Reset marker: null pointer guard hit");
    break;
  case RESET_MARKER_NONE:
  default:
    break;
  }
}
