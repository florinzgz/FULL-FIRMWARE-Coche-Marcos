// System-wide configuration constants for the vehicle firmware
#pragma once

#include "version.h"  // For FIRMWARE_VERSION
#include "pins.h"     // For PIN_TFT_RST

// Watchdog timeout configuration (reserved for future use)
// Note: Current implementation uses fixed 30-second hardware timeout internally
#define WATCHDOG_TIMEOUT_MS 5000

// Logging level configuration (reserved for future use)
// 0=DEBUG, 1=INFO, 2=WARNING, 3=ERROR
#define LOG_LEVEL 1

// System timing
#define SYSTEM_TICK_MS 10

// Add any other system-wide configuration constants here
