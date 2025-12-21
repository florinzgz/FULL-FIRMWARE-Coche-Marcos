// core/SystemConfig.h
// System-wide configuration constants for the vehicle firmware
#pragma once

#include "../../include/version.h"  // For FIRMWARE_VERSION
#include "../../include/pins.h"     // For PIN_TFT_RST

// Watchdog configuration
// Timeout in milliseconds (note: actual watchdog uses 30 seconds internally)
// This value is passed to Watchdog::init() but currently not used by the implementation
#define WATCHDOG_TIMEOUT_MS 5000

// Logging configuration
// 0=DEBUG, 1=INFO, 2=WARNING, 3=ERROR
// This value is passed to Logger::init() but currently not used by the implementation
#define LOG_LEVEL 1

// System timing
#define SYSTEM_TICK_MS 10

// Add any other system-wide configuration constants here
