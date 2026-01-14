#include "hud_limp_diagnostics.h"
#include "hud_layer.h" // 🚨 CRITICAL FIX: For RenderContext
#include "limp_mode.h"
#include "safe_draw.h" // 🚨 CRITICAL FIX: For coordinate-safe drawing

namespace HudLimpDiagnostics {

// ========================================
// Layout Constants (ILI9488 480x320)
// ========================================
static constexpr int16_t DIAG_X = 260;      // Fixed X position
static constexpr int16_t DIAG_Y = 60;       // Fixed Y position
static constexpr int16_t DIAG_WIDTH = 210;  // Fixed width
static constexpr int16_t DIAG_HEIGHT = 180; // Fixed height

static constexpr int16_t LINE_HEIGHT = 18; // Height per line
static constexpr int16_t TEXT_SIZE = 1;    // Text size (small font)
static constexpr int16_t MARGIN_X = 8;     // Left margin inside box
static constexpr int16_t MARGIN_Y = 8;     // Top margin inside box

// ========================================
// Colors
// ========================================
static constexpr uint16_t COLOR_BACKGROUND = TFT_BLACK;
static constexpr uint16_t COLOR_OK = TFT_GREEN;
static constexpr uint16_t COLOR_FAIL = TFT_YELLOW;
static constexpr uint16_t COLOR_BORDER_NORMAL = TFT_WHITE;
static constexpr uint16_t COLOR_BORDER_CRITICAL = TFT_RED;
static constexpr uint16_t COLOR_TEXT = TFT_WHITE;

// ========================================
// State Cache
// ========================================
static TFT_eSPI *tft = nullptr;       // For backward compatibility
static TFT_eSprite *sprite = nullptr; // For compositor mode
static bool initialized = false;

// Cache for previous diagnostics to detect changes
static LimpMode::LimpState lastState = LimpMode::LimpState::NORMAL;
static LimpMode::Diagnostics lastDiagnostics;

// ========================================
// Helper Functions
// ========================================

/**
 * @brief Compare two diagnostics structures for equality
 */
static bool diagnosticsEqual(const LimpMode::Diagnostics &a,
                             const LimpMode::Diagnostics &b) {
  return (a.state == b.state && a.pedalValid == b.pedalValid &&
          a.steeringValid == b.steeringValid &&
          a.systemErrorCount == b.systemErrorCount &&
          a.batteryUndervoltage == b.batteryUndervoltage &&
          a.temperatureWarning == b.temperatureWarning &&
          a.powerLimit == b.powerLimit && a.maxSpeedLimit == b.maxSpeedLimit);
}

/**
 * @brief Draw the diagnostics panel
 * @param diag Diagnostics to draw
 * @param target Target to draw to (sprite if available, otherwise tft)
 *
 * Note: TFT_eSprite inherits from TFT_eSPI, so casting TFT_eSprite* to
 * TFT_eSPI* is safe and allows using the same drawing API for both.
 */
static void drawDiagnostics(const LimpMode::Diagnostics &diag,
                            TFT_eSprite *target) {
  // Use sprite if available, otherwise fall back to TFT
  // Safe cast: TFT_eSprite inherits from TFT_eSPI
  // 🚨 CRITICAL FIX: Create safe RenderContext
  HudLayer::RenderContext ctx(target, true, 0, 0,
                              target ? target->width() : 480,
                              target ? target->height() : 320);
  TFT_eSPI *drawTarget = SafeDraw::getDrawTarget(ctx);
  if (!drawTarget) return;

  // Clear the area
  drawTarget->fillRect(DIAG_X, DIAG_Y, DIAG_WIDTH, DIAG_HEIGHT,
                       COLOR_BACKGROUND);

  // Draw border (red if CRITICAL, white otherwise)
  uint16_t borderColor = (diag.state == LimpMode::LimpState::CRITICAL)
                             ? COLOR_BORDER_CRITICAL
                             : COLOR_BORDER_NORMAL;
  drawTarget->drawRect(DIAG_X, DIAG_Y, DIAG_WIDTH, DIAG_HEIGHT, borderColor);

  // Set text properties
  drawTarget->setTextSize(TEXT_SIZE);
  drawTarget->setTextDatum(TL_DATUM); // Top-left alignment

  int16_t cursorY = DIAG_Y + MARGIN_Y;
  int16_t cursorX = DIAG_X + MARGIN_X;

  // Title
  drawTarget->setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
  SafeDraw::drawString(ctx, "LIMP MODE", cursorX, cursorY);
  cursorY += LINE_HEIGHT + 4; // Extra space after title

  // Pedal status
  drawTarget->setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
  SafeDraw::drawString(ctx, "Pedal:", cursorX, cursorY);
  drawTarget->setTextColor(diag.pedalValid ? COLOR_OK : COLOR_FAIL,
                           COLOR_BACKGROUND);
  SafeDraw::drawString(ctx, diag.pedalValid ? "OK" : "FAIL", cursorX + 70,
                       cursorY);
  cursorY += LINE_HEIGHT;

  // Steering status
  drawTarget->setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
  SafeDraw::drawString(ctx, "Steering:", cursorX, cursorY);
  drawTarget->setTextColor(diag.steeringValid ? COLOR_OK : COLOR_FAIL,
                           COLOR_BACKGROUND);
  SafeDraw::drawString(ctx, diag.steeringValid ? "OK" : "FAIL", cursorX + 70,
                       cursorY);
  cursorY += LINE_HEIGHT;

  // Battery status
  drawTarget->setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
  SafeDraw::drawString(ctx, "Battery:", cursorX, cursorY);
  drawTarget->setTextColor(diag.batteryUndervoltage ? COLOR_FAIL : COLOR_OK,
                           COLOR_BACKGROUND);
  SafeDraw::drawString(ctx, diag.batteryUndervoltage ? "LOW" : "OK",
                       cursorX + 70, cursorY);
  cursorY += LINE_HEIGHT;

  // Temperature status
  drawTarget->setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
  SafeDraw::drawString(ctx, "Temp:", cursorX, cursorY);
  drawTarget->setTextColor(diag.temperatureWarning ? COLOR_FAIL : COLOR_OK,
                           COLOR_BACKGROUND);
  SafeDraw::drawString(ctx, diag.temperatureWarning ? "WARN" : "OK",
                       cursorX + 70, cursorY);
  cursorY += LINE_HEIGHT;

  // Error count
  char errorStr[16];
  snprintf(errorStr, sizeof(errorStr), "%d", diag.systemErrorCount);
  drawTarget->setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
  SafeDraw::drawString(ctx, "Errors:", cursorX, cursorY);
  drawTarget->setTextColor(diag.systemErrorCount > 0 ? COLOR_FAIL : COLOR_OK,
                           COLOR_BACKGROUND);
  SafeDraw::drawString(ctx, errorStr, cursorX + 70, cursorY);
  cursorY += LINE_HEIGHT + 4; // Extra space before limits

  // Power limit
  char powerStr[16];
  int powerPercent = (int)(diag.powerLimit * 100.0f);
  snprintf(powerStr, sizeof(powerStr), "%d %%", powerPercent);
  drawTarget->setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
  SafeDraw::drawString(ctx, "Power:", cursorX, cursorY);
  drawTarget->setTextColor(powerPercent < 100 ? COLOR_FAIL : COLOR_OK,
                           COLOR_BACKGROUND);
  SafeDraw::drawString(ctx, powerStr, cursorX + 70, cursorY);
  cursorY += LINE_HEIGHT;

  // Speed limit
  char speedStr[16];
  int speedPercent = (int)(diag.maxSpeedLimit * 100.0f);
  snprintf(speedStr, sizeof(speedStr), "%d %%", speedPercent);
  drawTarget->setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
  SafeDraw::drawString(ctx, "Speed:", cursorX, cursorY);
  drawTarget->setTextColor(speedPercent < 100 ? COLOR_FAIL : COLOR_OK,
                           COLOR_BACKGROUND);
  SafeDraw::drawString(ctx, speedStr, cursorX + 70, cursorY);
}

// ========================================
// Public API
// ========================================

void init(TFT_eSPI *tftDisplay) {
  tft = tftDisplay;
  SafeDraw::init(tft); // 🚨 CRITICAL FIX: Initialize SafeDraw
  sprite = nullptr;    // Will be set by compositor if used
  initialized = true;

  // Initialize cache to NORMAL state
  lastState = LimpMode::LimpState::NORMAL;
  memset(&lastDiagnostics, 0, sizeof(lastDiagnostics));
  lastDiagnostics.state = LimpMode::LimpState::NORMAL;
  lastDiagnostics.powerLimit = 1.0f;
  lastDiagnostics.maxSpeedLimit = 1.0f;

  // Clear the area (nothing to show in NORMAL state)
  if (tft) {
    tft->fillRect(DIAG_X, DIAG_Y, DIAG_WIDTH, DIAG_HEIGHT, COLOR_BACKGROUND);
  }
}

void draw() {
  if (!initialized) return;
  if (!tft && !sprite) return;

  // Get current diagnostics from LimpMode (single source of truth)
  LimpMode::Diagnostics currentDiag = LimpMode::getDiagnostics();

  // Use sprite if available, otherwise fall back to TFT
  // 🚨 CRITICAL FIX: Create safe RenderContext
  HudLayer::RenderContext ctx(sprite, true, 0, 0,
                              sprite ? sprite->width() : 480,
                              sprite ? sprite->height() : 320);
  TFT_eSPI *drawTarget = SafeDraw::getDrawTarget(ctx);

  // Only show when NOT in NORMAL state
  if (currentDiag.state == LimpMode::LimpState::NORMAL) {
    // If we were showing diagnostics before, clear the area
    if (lastState != LimpMode::LimpState::NORMAL) {
      drawTarget->fillRect(DIAG_X, DIAG_Y, DIAG_WIDTH, DIAG_HEIGHT,
                           COLOR_BACKGROUND);
      lastState = LimpMode::LimpState::NORMAL;
    }
    return;
  }

  // Check if diagnostics changed (avoid unnecessary redraws)
  if (diagnosticsEqual(currentDiag, lastDiagnostics)) {
    return; // No change, skip redraw
  }

  // Diagnostics changed, redraw
  drawDiagnostics(currentDiag, sprite);

  // Update cache
  lastState = currentDiag.state;
  lastDiagnostics = currentDiag;
}

void forceRedraw() {
  if (!initialized) return;
  if (!tft && !sprite) return;

  // Get current diagnostics
  LimpMode::Diagnostics currentDiag = LimpMode::getDiagnostics();

  // Use sprite if available, otherwise fall back to TFT
  // 🚨 CRITICAL FIX: Create safe RenderContext
  HudLayer::RenderContext ctx(sprite, true, 0, 0,
                              sprite ? sprite->width() : 480,
                              sprite ? sprite->height() : 320);
  TFT_eSPI *drawTarget = SafeDraw::getDrawTarget(ctx);

  // If in NORMAL state, just clear
  if (currentDiag.state == LimpMode::LimpState::NORMAL) {
    drawTarget->fillRect(DIAG_X, DIAG_Y, DIAG_WIDTH, DIAG_HEIGHT,
                         COLOR_BACKGROUND);
  } else {
    // Force redraw diagnostics
    drawDiagnostics(currentDiag, sprite);
  }

  // Update cache
  lastState = currentDiag.state;
  lastDiagnostics = currentDiag;
}

void clear() {
  if (!tft && !sprite) return;

  // Use sprite if available, otherwise fall back to TFT
  // 🚨 CRITICAL FIX: Create safe RenderContext
  HudLayer::RenderContext ctx(sprite, true, 0, 0,
                              sprite ? sprite->width() : 480,
                              sprite ? sprite->height() : 320);
  TFT_eSPI *drawTarget = SafeDraw::getDrawTarget(ctx);

  // Clear diagnostics area
  drawTarget->fillRect(DIAG_X, DIAG_Y, DIAG_WIDTH, DIAG_HEIGHT,
                       COLOR_BACKGROUND);

  // Reset cached state to force redraw on next draw()
  lastState = LimpMode::LimpState::NORMAL;
  memset(&lastDiagnostics, 0, sizeof(lastDiagnostics));
  lastDiagnostics.state = LimpMode::LimpState::NORMAL;
  lastDiagnostics.powerLimit = 1.0f;
  lastDiagnostics.maxSpeedLimit = 1.0f;
}

// ========================================
// Compositor Integration
// ========================================

/**
 * @brief Layer renderer implementation for compositor
 */
class LimpDiagnosticsRenderer : public HudLayer::LayerRenderer {
public:
  void render(HudLayer::RenderContext &ctx) override {
    if (!ctx.isValid()) return;

    // Store sprite for legacy draw() calls
    sprite = ctx.sprite;

    // Get current diagnostics from LimpMode (single source of truth)
    LimpMode::Diagnostics currentDiag = LimpMode::getDiagnostics();

    // Only show when NOT in NORMAL state
    if (currentDiag.state == LimpMode::LimpState::NORMAL) {
      // If we were showing diagnostics before, clear the area
      if (lastState != LimpMode::LimpState::NORMAL || ctx.dirty) {
        // 🚨 CRITICAL FIX: Use SafeDraw to safely clear rectangle
        // Direct ctx.sprite->fillRect() with screen coordinates causes
        // out-of-bounds writes that corrupt heap and trigger IPC0 crashes
        SafeDraw::fillRect(ctx, DIAG_X, DIAG_Y, DIAG_WIDTH, DIAG_HEIGHT,
                          COLOR_BACKGROUND);
        lastState = LimpMode::LimpState::NORMAL;

        // PHASE 8: Mark diagnostics area as dirty
        ctx.markDirty(DIAG_X, DIAG_Y, DIAG_WIDTH, DIAG_HEIGHT);
      }
      return;
    }

    // Check if diagnostics changed or context is dirty
    if (!diagnosticsEqual(currentDiag, lastDiagnostics) || ctx.dirty) {
      // Diagnostics changed or dirty, redraw
      drawDiagnostics(currentDiag, ctx.sprite);

      // Update cache
      lastState = currentDiag.state;
      lastDiagnostics = currentDiag;

      // PHASE 8: Mark diagnostics area as dirty
      ctx.markDirty(DIAG_X, DIAG_Y, DIAG_WIDTH, DIAG_HEIGHT);
    }
  }

  bool isActive() const override {
    // Always active to show diagnostics when needed
    return initialized;
  }
};

// Singleton instance for compositor registration
static LimpDiagnosticsRenderer rendererInstance;

/**
 * @brief Get the layer renderer for compositor registration
 * @return Pointer to the layer renderer instance
 */
HudLayer::LayerRenderer *getRenderer() { return &rendererInstance; }

} // namespace HudLimpDiagnostics
