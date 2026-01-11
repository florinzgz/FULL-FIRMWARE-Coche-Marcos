#include "hud_limp_diagnostics.h"
#include "limp_mode.h"

namespace HudLimpDiagnostics {

// ========================================
// Layout Constants (ILI9488 480x320)
// ========================================
static constexpr int16_t DIAG_X = 260;      // Fixed X position
static constexpr int16_t DIAG_Y = 60;       // Fixed Y position
static constexpr int16_t DIAG_WIDTH = 210;  // Fixed width
static constexpr int16_t DIAG_HEIGHT = 180; // Fixed height

static constexpr int16_t LINE_HEIGHT = 18;  // Height per line
static constexpr int16_t TEXT_SIZE = 1;     // Text size (small font)
static constexpr int16_t MARGIN_X = 8;      // Left margin inside box
static constexpr int16_t MARGIN_Y = 8;      // Top margin inside box

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
static TFT_eSPI *tft = nullptr;
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
 */
static void drawDiagnostics(const LimpMode::Diagnostics &diag) {
  if (!tft) return;

  // Clear the area
  tft->fillRect(DIAG_X, DIAG_Y, DIAG_WIDTH, DIAG_HEIGHT, COLOR_BACKGROUND);

  // Draw border (red if CRITICAL, white otherwise)
  uint16_t borderColor = (diag.state == LimpMode::LimpState::CRITICAL)
                             ? COLOR_BORDER_CRITICAL
                             : COLOR_BORDER_NORMAL;
  tft->drawRect(DIAG_X, DIAG_Y, DIAG_WIDTH, DIAG_HEIGHT, borderColor);

  // Set text properties
  tft->setTextSize(TEXT_SIZE);
  tft->setTextDatum(TL_DATUM); // Top-left alignment

  int16_t cursorY = DIAG_Y + MARGIN_Y;
  int16_t cursorX = DIAG_X + MARGIN_X;

  // Title
  tft->setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
  tft->drawString("LIMP MODE", cursorX, cursorY);
  cursorY += LINE_HEIGHT + 4; // Extra space after title

  // Pedal status
  tft->setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
  tft->drawString("Pedal:", cursorX, cursorY);
  tft->setTextColor(diag.pedalValid ? COLOR_OK : COLOR_FAIL, COLOR_BACKGROUND);
  tft->drawString(diag.pedalValid ? "OK" : "FAIL", cursorX + 70, cursorY);
  cursorY += LINE_HEIGHT;

  // Steering status
  tft->setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
  tft->drawString("Steering:", cursorX, cursorY);
  tft->setTextColor(diag.steeringValid ? COLOR_OK : COLOR_FAIL,
                    COLOR_BACKGROUND);
  tft->drawString(diag.steeringValid ? "OK" : "FAIL", cursorX + 70, cursorY);
  cursorY += LINE_HEIGHT;

  // Battery status
  tft->setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
  tft->drawString("Battery:", cursorX, cursorY);
  tft->setTextColor(diag.batteryUndervoltage ? COLOR_FAIL : COLOR_OK,
                    COLOR_BACKGROUND);
  tft->drawString(diag.batteryUndervoltage ? "LOW" : "OK", cursorX + 70,
                  cursorY);
  cursorY += LINE_HEIGHT;

  // Temperature status
  tft->setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
  tft->drawString("Temp:", cursorX, cursorY);
  tft->setTextColor(diag.temperatureWarning ? COLOR_FAIL : COLOR_OK,
                    COLOR_BACKGROUND);
  tft->drawString(diag.temperatureWarning ? "WARN" : "OK", cursorX + 70,
                  cursorY);
  cursorY += LINE_HEIGHT;

  // Error count
  char errorStr[16];
  snprintf(errorStr, sizeof(errorStr), "%d", diag.systemErrorCount);
  tft->setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
  tft->drawString("Errors:", cursorX, cursorY);
  tft->setTextColor(diag.systemErrorCount > 0 ? COLOR_FAIL : COLOR_OK,
                    COLOR_BACKGROUND);
  tft->drawString(errorStr, cursorX + 70, cursorY);
  cursorY += LINE_HEIGHT + 4; // Extra space before limits

  // Power limit
  char powerStr[16];
  int powerPercent = (int)(diag.powerLimit * 100.0f);
  snprintf(powerStr, sizeof(powerStr), "%d %%", powerPercent);
  tft->setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
  tft->drawString("Power:", cursorX, cursorY);
  tft->setTextColor(powerPercent < 100 ? COLOR_FAIL : COLOR_OK,
                    COLOR_BACKGROUND);
  tft->drawString(powerStr, cursorX + 70, cursorY);
  cursorY += LINE_HEIGHT;

  // Speed limit
  char speedStr[16];
  int speedPercent = (int)(diag.maxSpeedLimit * 100.0f);
  snprintf(speedStr, sizeof(speedStr), "%d %%", speedPercent);
  tft->setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
  tft->drawString("Speed:", cursorX, cursorY);
  tft->setTextColor(speedPercent < 100 ? COLOR_FAIL : COLOR_OK,
                    COLOR_BACKGROUND);
  tft->drawString(speedStr, cursorX + 70, cursorY);
}

// ========================================
// Public API
// ========================================

void init(TFT_eSPI *tftDisplay) {
  tft = tftDisplay;
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
  if (!initialized || !tft) return;

  // Get current diagnostics from LimpMode (single source of truth)
  LimpMode::Diagnostics currentDiag = LimpMode::getDiagnostics();

  // Only show when NOT in NORMAL state
  if (currentDiag.state == LimpMode::LimpState::NORMAL) {
    // If we were showing diagnostics before, clear the area
    if (lastState != LimpMode::LimpState::NORMAL) {
      tft->fillRect(DIAG_X, DIAG_Y, DIAG_WIDTH, DIAG_HEIGHT, COLOR_BACKGROUND);
      lastState = LimpMode::LimpState::NORMAL;
    }
    return;
  }

  // Check if diagnostics changed (avoid unnecessary redraws)
  if (diagnosticsEqual(currentDiag, lastDiagnostics)) {
    return; // No change, skip redraw
  }

  // Diagnostics changed, redraw
  drawDiagnostics(currentDiag);

  // Update cache
  lastState = currentDiag.state;
  lastDiagnostics = currentDiag;
}

void forceRedraw() {
  if (!initialized || !tft) return;

  // Get current diagnostics
  LimpMode::Diagnostics currentDiag = LimpMode::getDiagnostics();

  // If in NORMAL state, just clear
  if (currentDiag.state == LimpMode::LimpState::NORMAL) {
    tft->fillRect(DIAG_X, DIAG_Y, DIAG_WIDTH, DIAG_HEIGHT, COLOR_BACKGROUND);
  } else {
    // Force redraw diagnostics
    drawDiagnostics(currentDiag);
  }

  // Update cache
  lastState = currentDiag.state;
  lastDiagnostics = currentDiag;
}

void clear() {
  if (!tft) return;

  // Clear diagnostics area
  tft->fillRect(DIAG_X, DIAG_Y, DIAG_WIDTH, DIAG_HEIGHT, COLOR_BACKGROUND);

  // Reset cached state to force redraw on next draw()
  lastState = LimpMode::LimpState::NORMAL;
  memset(&lastDiagnostics, 0, sizeof(lastDiagnostics));
  lastDiagnostics.state = LimpMode::LimpState::NORMAL;
  lastDiagnostics.powerLimit = 1.0f;
  lastDiagnostics.maxSpeedLimit = 1.0f;
}

} // namespace HudLimpDiagnostics
