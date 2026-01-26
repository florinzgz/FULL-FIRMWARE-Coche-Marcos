#pragma once

// ============================================================================
// version.h - Firmware version information
// ============================================================================
// Centralized firmware version definition for use throughout the codebase
// ============================================================================

// Firmware version string (semantic versioning)
// v2.17.4: Bootloop fix - Increased INT_WDT to 10000ms for maximum safety margin
// v2.17.3: Bootloop fix - Disabled PSRAM memtest, increased INT_WDT to 5000ms
// v2.11.3: Stack overflow fixes - IPC stack increase, logger buffer reduction,
// watchdog feeds
#define FIRMWARE_VERSION "2.17.4"
#define FIRMWARE_VERSION_MAJOR 2
#define FIRMWARE_VERSION_MINOR 17
#define FIRMWARE_VERSION_PATCH 4

// Build date (automatically set at compile time)
#define FIRMWARE_BUILD_DATE __DATE__
#define FIRMWARE_BUILD_TIME __TIME__

// Hardware target
#define HARDWARE_TARGET "ESP32-S3-DevKitC-1"

// Complete version string with build info
#define FIRMWARE_VERSION_FULL                                                  \
  "v" FIRMWARE_VERSION " (" FIRMWARE_BUILD_DATE " " FIRMWARE_BUILD_TIME ")"
