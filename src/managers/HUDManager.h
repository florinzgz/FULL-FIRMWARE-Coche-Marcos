// managers/HUDManager.h
// HUD management - integrates display system
// 🔒 CRITICAL: This is a legacy wrapper for backwards compatibility
// The actual implementation is in include/hud_manager.h (class-based)
#pragma once

#include "../../include/hud_manager.h"

// Legacy free function wrappers for backwards compatibility
// These forward calls to the class-based HUDManager
// Note: We cannot use a namespace called HUDManager because that conflicts
// with the HUDManager class. Code that uses "HUDManager::init()" will
// now resolve to the static class method HUDManager::init() directly.

