#include "watchdog.h"
#include "logger.h"
#include "pins.h"
#include "power_mgmt.h"
// Arduino framework provides automatic watchdog management (less configurable than ESP-IDF)

namespace Watchdog {

// Arduino framework handles watchdog automatically
// Note: API remains active for compatibility but actual watchdog is managed by Arduino
static bool apiEnabled = false; // Tracks API state, not actual watchdog state
static uint32_t lastFeedTime = 0;
static uint32_t feedCount = 0;

void init() {
  // Watchdog disabled for Arduino framework compatibility
  // Arduino manages watchdog automatically
  apiEnabled = true;
  lastFeedTime = millis();
  feedCount = 0;

  Logger::info("Watchdog: Disabled (Arduino framework compatibility)");
}

void feed() {
  if (!apiEnabled) return;

  // No-op for Arduino - watchdog is automatic
  uint32_t now = millis();
  uint32_t interval = now - lastFeedTime;
  lastFeedTime = now;
  feedCount++;

  // Log every 100 feeds for compatibility
  if (feedCount % 100 == 0) {
    Logger::infof("WDT feed %lu (interval: %lums) [Arduino auto]", feedCount, interval);
  }
}

void disable() {
  // Already disabled for Arduino
  apiEnabled = false;
  Logger::info("Watchdog: Already disabled (Arduino framework)");
}

bool isEnabled() { return apiEnabled; }

uint32_t getFeedCount() { return feedCount; }

uint32_t getLastFeedInterval() { return millis() - lastFeedTime; }

} // namespace Watchdog
