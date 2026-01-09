// managers/PowerManager.h
// Power management - integrates power system
#pragma once

#include "../../include/power_mgmt.h"

namespace PowerManager {
inline bool init() {
  PowerMgmt::init();
  return PowerMgmt::initOK();
}

inline void update() { PowerMgmt::update(); }
} // namespace PowerManager
