#pragma once

#include <stdint.h>

namespace BootGuard {
enum ResetMarker : uint8_t {
  RESET_MARKER_NONE = 0,
  RESET_MARKER_EXPLICIT_RESTART = 1,
  RESET_MARKER_WATCHDOG_LOOP = 2,
  RESET_MARKER_I2C_PREINIT = 3,
  RESET_MARKER_NULL_POINTER = 4,
  RESET_MARKER_MAX_VALUE = RESET_MARKER_NULL_POINTER,
};

// Ensure XSHUT lines that land on strapping pins are forced HIGH early in boot
void applyXshutStrappingGuard();

// 🔒 v2.17.1: Boot counter for bootloop detection
void initBootCounter();
void incrementBootCounter();
void clearBootCounter();
uint8_t getBootCount();
bool isBootloopDetected();  // Returns true if >3 boots in 60 seconds
bool shouldEnterSafeMode(); // Returns true if bootloop detected

void setResetMarker(ResetMarker marker);
ResetMarker getResetMarker();
void clearResetMarker();
void logResetMarker();
} // namespace BootGuard
