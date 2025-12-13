#pragma once

// ============================================================================
// version.h - Firmware version information
// ============================================================================
// Centralized firmware version definition for use throughout the codebase
// ============================================================================

// Firmware version string (semantic versioning)
#define FIRMWARE_VERSION "2.10.2"
#define FIRMWARE_VERSION_MAJOR 2
#define FIRMWARE_VERSION_MINOR 10
#define FIRMWARE_VERSION_PATCH 2

// Build date (automatically set at compile time)
#define FIRMWARE_BUILD_DATE __DATE__
#define FIRMWARE_BUILD_TIME __TIME__

// Hardware target
#define HARDWARE_TARGET "ESP32-S3-DevKitC-1"

// Complete version string with build info
#define FIRMWARE_VERSION_FULL                                                  \
  "v" FIRMWARE_VERSION " (" FIRMWARE_BUILD_DATE " " FIRMWARE_BUILD_TIME ")"
