// managers/HUDManager.h
// HUD management - integrates display system
// 🔒 CRITICAL: This is a legacy wrapper namespace
// The actual implementation is in include/hud_manager.h (class-based)
#pragma once

#include "../../include/hud_manager.h"

// Legacy namespace wrapper for backwards compatibility
// Forwards calls to the class-based HUDManager
namespace HUDManager {

// Forward to class-based HUDManager::init()
inline bool init() {
  ::HUDManager::init();
  return ::HUDManager::initOK();
}

// Forward to class-based HUDManager::update()
inline void update() { ::HUDManager::update(); }

// Forward to class-based HUDManager::showError()
// Uses generic error message since old code doesn't provide details
inline void showError() { ::HUDManager::showError("Critical error detected"); }

} // namespace HUDManager
