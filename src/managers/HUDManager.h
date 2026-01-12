// managers/HUDManager.h
// HUD management - integrates display system
#pragma once

#include "../../include/hud.h"
#include "../../include/pins.h" // 🔒 v2.11.4: For PIN_TFT_BL
#include "../../include/settings.h" // 🔒 v2.11.4: For DISPLAY_BRIGHTNESS_DEFAULT
#include <TFT_eSPI.h>
#include <Arduino.h>

// 🔒 v2.11.4: CRITICAL FIX - Access to global tft object
// The TFT object is defined in hud_manager.cpp and must be initialized
extern TFT_eSPI tft;

// Create a namespace HUDManager that wraps both old and new HUD systems
// This avoids conflicts while ensuring proper initialization
namespace HUDManager {
inline bool init() {
  // 🔒 v2.11.4: CRITICAL FIX - Initialize TFT AND backlight before HUD::init()
  // This was broken when refactoring to namespace wrappers - TFT was never initialized!
  Serial.println("[HUDManager] Initializing display system...");
  Serial.flush();
  
  try {
    // Step 1: Initialize backlight PWM BEFORE TFT init
    Serial.println("[HUDManager] Configuring backlight...");
    ledcSetup(0, 5000, 8); // Channel 0, 5kHz, 8-bit resolution
    ledcAttachPin(PIN_TFT_BL, 0);
    ledcWrite(0, DISPLAY_BRIGHTNESS_DEFAULT); // Set default brightness
    delay(10); // Let PWM stabilize
    Serial.printf("[HUDManager] Backlight enabled at %d/255\n", DISPLAY_BRIGHTNESS_DEFAULT);
    
    // Step 2: Initialize TFT_eSPI
    Serial.println("[HUDManager] Initializing TFT...");
    tft.init();
    tft.setRotation(3); // Landscape mode: 480x320
    tft.fillScreen(TFT_BLACK);
    Serial.println("[HUDManager] TFT initialized: 480x320");
    
    // Step 3: Initialize HUD components (gauges, icons, wheels, touch)
    Serial.println("[HUDManager] Initializing HUD components...");
    HUD::init();
    
    Serial.println("[HUDManager] ✅ All systems initialized successfully");
    return true;
    
  } catch (const std::exception &e) {
    Serial.printf("[HUDManager] ❌ Init failed with exception: %s\n", e.what());
    return false;
  } catch (...) {
    Serial.println("[HUDManager] ❌ Init failed with unknown exception");
    return false;
  }
}

inline void update() { HUD::update(); }

inline void showError() { HUD::showError(); }
} // namespace HUDManager
