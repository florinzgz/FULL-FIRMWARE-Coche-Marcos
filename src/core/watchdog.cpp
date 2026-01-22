#include "watchdog.h"
#include "logger.h"
#include "pins.h"
#include "power_mgmt.h"
// Arduino framework - watchdog is handled automatically

namespace Watchdog {

// Arduino framework handles watchdog automatically
// These functions are disabled for Arduino compatibility
static bool initialized = false;
static uint32_t lastFeedTime = 0;
static uint32_t feedCount = 0;

void init() {
  // Watchdog disabled for Arduino framework compatibility
  // Arduino manages watchdog automatically
  initialized = true;
  lastFeedTime = millis();
  feedCount = 0;

  Logger::info("Watchdog: Disabled (Arduino framework compatibility)");
}

void feed() {
  if (!initialized) return;

  // No-op for Arduino - watchdog is automatic
  uint32_t now = millis();
  uint32_t interval = now - lastFeedTime;
  lastFeedTime = now;
  feedCount++;

  // Log cada 100 feeds for compatibility
  if (feedCount % 100 == 0) {
    Logger::infof("WDT feed %lu (interval: %lums) [Arduino auto]", feedCount, interval);
  }
}

void disable() {
  // Already disabled for Arduino
  initialized = false;
  Logger::info("Watchdog: Already disabled (Arduino framework)");
}

bool isEnabled() { return initialized; }

uint32_t getFeedCount() { return feedCount; }

uint32_t getLastFeedInterval() { return millis() - lastFeedTime; }

} // namespace Watchdog
