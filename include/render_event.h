#ifndef RENDER_EVENT_H
#define RENDER_EVENT_H

#include <stdint.h>

/**
 * @file render_event.h
 * @brief Thread-safe render event system for TFT_eSPI
 *
 * TFT_eSPI is NOT thread-safe. All drawing operations must happen from
 * a single context (HUDManager::update). This header defines the event
 * system that allows other managers to request rendering without directly
 * touching the TFT.
 *
 * CRITICAL: Never call tft.* methods outside HUDManager::update()!
 */

namespace RenderEvent {

/**
 * @brief Types of render events
 */
enum class Type : uint8_t {
  NONE = 0,          ///< No event
  SHOW_ERROR,        ///< Display error screen
  CLEAR_ERROR,       ///< Clear error screen and return to normal
  FORCE_REDRAW,      ///< Force complete screen redraw
  UPDATE_BRIGHTNESS, ///< Update backlight brightness
};

/**
 * @brief Maximum length for error messages
 */
static constexpr size_t MAX_ERROR_MSG_LEN = 128;

/**
 * @brief Render event data structure
 *
 * This structure is sent through a FreeRTOS queue from any context
 * to the render thread (HUDManager::update).
 */
struct Event {
  Type type;                          ///< Event type
  char errorMessage[MAX_ERROR_MSG_LEN]; ///< Error message (for SHOW_ERROR)
  uint8_t brightness;                 ///< Brightness value (for UPDATE_BRIGHTNESS)

  Event() : type(Type::NONE), errorMessage{0}, brightness(0) {}
};

} // namespace RenderEvent

#endif // RENDER_EVENT_H
