#include "hud_limp_indicator.h"
#include "hud_layer.h"
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
static TFT_eSPI *tft = nullptr; // For backward compatibility
static TFT_eSprite *sprite = nullptr; // For compositor mode
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
 * @param target Target to draw to (sprite if available, otherwise tft)
 */
static void drawIndicator(LimpMode::LimpState state, TFT_eSprite *target) {
  // Use sprite if available, otherwise fall back to TFT
  TFT_eSPI *drawTarget = target ? (TFT_eSPI*)target : tft;
  if (!drawTarget) return;

  const char *text = getTextForState(state);

  // NORMAL state: Don't show anything
  if (text == nullptr || state == LimpMode::LimpState::NORMAL) {
    // Clear the area
    drawTarget->fillRect(INDICATOR_X, INDICATOR_Y, INDICATOR_WIDTH, INDICATOR_HEIGHT,
                  COLOR_BACKGROUND);
    return;
  }

  uint16_t color = getColorForState(state);

  // Draw background
  drawTarget->fillRect(INDICATOR_X, INDICATOR_Y, INDICATOR_WIDTH, INDICATOR_HEIGHT,
                COLOR_BACKGROUND);

  // Draw border
  drawTarget->drawRect(INDICATOR_X, INDICATOR_Y, INDICATOR_WIDTH, INDICATOR_HEIGHT,
                color);
  drawTarget->drawRect(INDICATOR_X + 1, INDICATOR_Y + 1, INDICATOR_WIDTH - 2,
                INDICATOR_HEIGHT - 2, color);

  // Draw text centered
  drawTarget->setTextColor(color, COLOR_BACKGROUND);
  drawTarget->setTextDatum(MC_DATUM); // Middle center

  // Use font 4 for visibility (26 pixel height)
  drawTarget->setFreeFont(&FreeSansBold18pt7b);

  int16_t textX = INDICATOR_X + INDICATOR_WIDTH / 2;
  int16_t textY = INDICATOR_Y + INDICATOR_HEIGHT / 2;

  drawTarget->drawString(text, textX, textY);

  // Reset text datum
  drawTarget->setTextDatum(TL_DATUM);
}

// ========================================
// Public API
// ========================================

void init(TFT_eSPI *tftDisplay) {
  tft = tftDisplay;
  sprite = nullptr; // Will be set by compositor if used
  lastState = LimpMode::LimpState::NORMAL;
  initialized = true;

  // Initial draw (clear area since we start in NORMAL)
  if (tft) {
    tft->fillRect(INDICATOR_X, INDICATOR_Y, INDICATOR_WIDTH, INDICATOR_HEIGHT,
                  COLOR_BACKGROUND);
  }
}

void draw() {
  if (!initialized) return;
  if (!tft && !sprite) return;

  // Read current state from limp mode
  LimpMode::LimpState currentState = LimpMode::getState();

  // Only redraw if state changed
  if (currentState != lastState) {
    drawIndicator(currentState, sprite);
    lastState = currentState;
  }
}

void forceRedraw() {
  if (!initialized) return;
  if (!tft && !sprite) return;

  // Read current state and force redraw
  LimpMode::LimpState currentState = LimpMode::getState();
  drawIndicator(currentState, sprite);
  lastState = currentState;
}

void clear() {
  if (!tft && !sprite) return;

  // Use sprite if available, otherwise fall back to TFT
  TFT_eSPI *drawTarget = sprite ? (TFT_eSPI*)sprite : tft;

  // Clear indicator area
  drawTarget->fillRect(INDICATOR_X, INDICATOR_Y, INDICATOR_WIDTH, INDICATOR_HEIGHT,
                COLOR_BACKGROUND);

  // Reset cached state so next draw() will redraw
  lastState = LimpMode::LimpState::NORMAL;
}

// ========================================
// Compositor Integration
// ========================================

/**
 * @brief Layer renderer implementation for compositor
 */
class LimpIndicatorRenderer : public HudLayer::LayerRenderer {
public:
  void render(HudLayer::RenderContext &ctx) override {
    if (!ctx.isValid()) return;
    
    // Store sprite for legacy draw() calls
    sprite = ctx.sprite;

    // Read current state from limp mode
    LimpMode::LimpState currentState = LimpMode::getState();

    // Only redraw if state changed or context is dirty
    if (currentState != lastState || ctx.dirty) {
      drawIndicator(currentState, ctx.sprite);
      lastState = currentState;
    }
  }

  bool isActive() const override {
    // Always active to show limp state
    return initialized;
  }
};

// Singleton instance for compositor registration
static LimpIndicatorRenderer rendererInstance;

/**
 * @brief Get the layer renderer for compositor registration
 * @return Pointer to the layer renderer instance
 */
HudLayer::LayerRenderer *getRenderer() {
  return &rendererInstance;
}

} // namespace HudLimpIndicator
