#include "hud_limp_indicator.h"
#include "limp_mode.h"

namespace HudLimpIndicator {

// ========================================
// Layout Constants
// ========================================
static constexpr int16_t INDICATOR_X = 360;     // Top-right corner
static constexpr int16_t INDICATOR_Y = 10;      // Below top edge
static constexpr int16_t INDICATOR_WIDTH = 110; // Compact size
static constexpr int16_t INDICATOR_HEIGHT = 40; // Single line + padding

// ========================================
// Colors
// ========================================
static constexpr uint16_t COLOR_BACKGROUND = TFT_BLACK;
static constexpr uint16_t COLOR_NORMAL = TFT_DARKGREEN; // Not shown
static constexpr uint16_t COLOR_DEGRADED = TFT_YELLOW;  // Warning
static constexpr uint16_t COLOR_LIMP = TFT_ORANGE;      // Caution
static constexpr uint16_t COLOR_CRITICAL = TFT_RED;     // Danger

// ========================================
// State
// ========================================
static TFT_eSPI *tft = nullptr;
static LimpMode::LimpState lastState = LimpMode::LimpState::NORMAL;
static bool initialized = false;

// ========================================
// Internal Helpers
// ========================================

/**
 * @brief Get display color for limp state
 */
static uint16_t getColorForState(LimpMode::LimpState state) {
  switch (state) {
  case LimpMode::LimpState::NORMAL:
    return COLOR_NORMAL;
  case LimpMode::LimpState::DEGRADED:
    return COLOR_DEGRADED;
  case LimpMode::LimpState::LIMP:
    return COLOR_LIMP;
  case LimpMode::LimpState::CRITICAL:
    return COLOR_CRITICAL;
  default:
    return COLOR_CRITICAL;
  }
}

/**
 * @brief Get text label for limp state
 */
static const char *getTextForState(LimpMode::LimpState state) {
  switch (state) {
  case LimpMode::LimpState::NORMAL:
    return nullptr; // Don't show
  case LimpMode::LimpState::DEGRADED:
    return "DEG";
  case LimpMode::LimpState::LIMP:
    return "LIMP";
  case LimpMode::LimpState::CRITICAL:
    return "SAFE";
  default:
    return "ERR";
  }
}

/**
 * @brief Draw the indicator box
 */
static void drawIndicator(LimpMode::LimpState state) {
  if (!tft) return;

  const char *text = getTextForState(state);

  // NORMAL state: Don't show anything
  if (text == nullptr || state == LimpMode::LimpState::NORMAL) {
    // Clear the area
    tft->fillRect(INDICATOR_X, INDICATOR_Y, INDICATOR_WIDTH, INDICATOR_HEIGHT,
                  COLOR_BACKGROUND);
    return;
  }

  uint16_t color = getColorForState(state);

  // Draw background
  tft->fillRect(INDICATOR_X, INDICATOR_Y, INDICATOR_WIDTH, INDICATOR_HEIGHT,
                COLOR_BACKGROUND);

  // Draw border
  tft->drawRect(INDICATOR_X, INDICATOR_Y, INDICATOR_WIDTH, INDICATOR_HEIGHT,
                color);
  tft->drawRect(INDICATOR_X + 1, INDICATOR_Y + 1, INDICATOR_WIDTH - 2,
                INDICATOR_HEIGHT - 2, color);

  // Draw text centered
  tft->setTextColor(color, COLOR_BACKGROUND);
  tft->setTextDatum(MC_DATUM); // Middle center

  // Use font 4 for visibility (26 pixel height)
  tft->setFreeFont(&FreeSansBold18pt7b);

  int16_t textX = INDICATOR_X + INDICATOR_WIDTH / 2;
  int16_t textY = INDICATOR_Y + INDICATOR_HEIGHT / 2;

  tft->drawString(text, textX, textY);

  // Reset text datum
  tft->setTextDatum(TL_DATUM);
}

// ========================================
// Public API
// ========================================

void init(TFT_eSPI *tftDisplay) {
  tft = tftDisplay;
  lastState = LimpMode::LimpState::NORMAL;
  initialized = true;

  // Initial draw (clear area since we start in NORMAL)
  if (tft) {
    tft->fillRect(INDICATOR_X, INDICATOR_Y, INDICATOR_WIDTH, INDICATOR_HEIGHT,
                  COLOR_BACKGROUND);
  }
}

void draw() {
  if (!initialized || !tft) return;

  // Read current state from limp mode
  LimpMode::LimpState currentState = LimpMode::getState();

  // Only redraw if state changed
  if (currentState != lastState) {
    drawIndicator(currentState);
    lastState = currentState;
  }
}

void forceRedraw() {
  if (!initialized || !tft) return;

  // Read current state and force redraw
  LimpMode::LimpState currentState = LimpMode::getState();
  drawIndicator(currentState);
  lastState = currentState;
}

void clear() {
  if (!tft) return;

  // Clear indicator area
  tft->fillRect(INDICATOR_X, INDICATOR_Y, INDICATOR_WIDTH, INDICATOR_HEIGHT,
                COLOR_BACKGROUND);

  // Reset cached state so next draw() will redraw
  lastState = LimpMode::LimpState::NORMAL;
}

} // namespace HudLimpIndicator
