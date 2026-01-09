// managers/TelemetryManager.h
// Telemetry management - integrates telemetry system
#pragma once

#include "../../include/telemetry.h"

namespace TelemetryManager {
inline bool init() {
  Telemetry::init();
  return Telemetry::initOK();
}

inline void update() { Telemetry::update(); }
} // namespace TelemetryManager
