// managers/HUDManager.h
// HUD management - integrates display system
#pragma once

#include "../../include/hud.h"

// Create a namespace HUDManager that wraps the HUD namespace functions
// This avoids conflicts with the existing HUDManager class
namespace HUDManager {
inline bool init() {
  HUD::init();
  // HUD::init() returns void and has no initOK() method
  // Assume initialization always succeeds
  return true;
}

inline void update() { HUD::update(); }

inline void showError() { HUD::showError(); }
} // namespace HUDManager
