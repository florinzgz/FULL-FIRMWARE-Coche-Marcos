#include "hud_graphics_telemetry.h"
#include "hud_layer.h"     // 🚨 CRITICAL FIX: For RenderContext
#include "safe_draw.h"     // 🚨 CRITICAL FIX: For coordinate-safe drawing
#include "hud_compositor.h"

namespace HudGraphicsTelemetry {

// ========================================
// Layout Constants
// ========================================
static constexpr int16_t TELEMETRY_X = 10;       // Fixed X position
static constexpr int16_t TELEMETRY_Y = 10;       // Fixed Y position
static constexpr int16_t TELEMETRY_WIDTH = 220;  // Fixed width
static constexpr int16_t TELEMETRY_HEIGHT = 120; // Fixed height

static constexpr int16_t LINE_HEIGHT = 12; // Height per line
static constexpr int16_t TEXT_SIZE = 1;    // Text size (small font)
static constexpr int16_t MARGIN_X = 6;     // Left margin inside box
static constexpr int16_t MARGIN_Y = 6;     // Top margin inside box

// ========================================
// Colors
// ========================================
static constexpr uint16_t COLOR_BACKGROUND = TFT_BLACK;
static constexpr uint16_t COLOR_TEXT = TFT_WHITE;
static constexpr uint16_t COLOR_LABEL = TFT_CYAN;
static constexpr uint16_t COLOR_BORDER_NORMAL = TFT_WHITE;
static constexpr uint16_t COLOR_BORDER_ERROR = TFT_RED;

// FPS color thresholds
static constexpr uint16_t COLOR_FPS_GOOD = TFT_GREEN;  // FPS > 25
static constexpr uint16_t COLOR_FPS_WARN = TFT_YELLOW; // FPS 15-25
static constexpr uint16_t COLOR_FPS_BAD = TFT_RED;     // FPS < 15

// ========================================
// State
// ========================================
static TFT_eSPI *tft = nullptr;       // For backward compatibility
static TFT_eSprite *sprite = nullptr; // For compositor mode
static bool initialized = false;
static bool visible = false; // Telemetry visibility flag

// ========================================
// Helper Functions
// ========================================

/**
 * @brief Get FPS color based on value
 */
static uint16_t getFpsColor(uint32_t fps) {
  if (fps > 25) {
    return COLOR_FPS_GOOD;
  } else if (fps >= 15) {
    return COLOR_FPS_WARN;
  } else {
    return COLOR_FPS_BAD;
  }
}

/**
 * @brief Draw the telemetry panel
 * @param stats Render statistics to display
 * @param target Target to draw to (sprite if available, otherwise tft)
 */
static void drawTelemetry(const HudCompositor::RenderStats &stats,
                          TFT_eSprite *target) {
  // Use sprite if available, otherwise fall back to TFT
  // 🚨 CRITICAL FIX: Create safe RenderContext
  HudLayer::RenderContext ctx(target, true, 0, 0,
                               target ? target->width() : 480,
                               target ? target->height() : 320);
  TFT_eSPI *drawTarget = SafeDraw::getDrawTarget(ctx);
  if (!drawTarget) return;

  // Clear the area
  drawTarget->fillRect(TELEMETRY_X, TELEMETRY_Y, TELEMETRY_WIDTH,
                       TELEMETRY_HEIGHT, COLOR_BACKGROUND);

  // Draw border (red if shadow errors, white otherwise)
  uint16_t borderColor =
      (stats.shadowMismatches > 0) ? COLOR_BORDER_ERROR : COLOR_BORDER_NORMAL;
  drawTarget->drawRect(TELEMETRY_X, TELEMETRY_Y, TELEMETRY_WIDTH,
                       TELEMETRY_HEIGHT, borderColor);

  // Set text properties
  drawTarget->setTextSize(TEXT_SIZE);
  drawTarget->setTextDatum(TL_DATUM); // Top-left alignment

  int16_t cursorY = TELEMETRY_Y + MARGIN_Y;
  int16_t cursorX = TELEMETRY_X + MARGIN_X;

  // Buffer for formatting (static to avoid stack allocation overhead)
  static char buf[32];

  // FPS (color-coded)
  drawTarget->setTextColor(COLOR_LABEL, COLOR_BACKGROUND);
  drawTarget->drawString("FPS:", cursorX, cursorY);
  snprintf(buf, sizeof(buf), "%u", stats.fps);
  drawTarget->setTextColor(getFpsColor(stats.fps), COLOR_BACKGROUND);
  drawTarget->drawString(buf, cursorX + 100, cursorY);
  cursorY += LINE_HEIGHT;

  // Frame time
  drawTarget->setTextColor(COLOR_LABEL, COLOR_BACKGROUND);
  drawTarget->drawString("Frame time:", cursorX, cursorY);
  snprintf(buf, sizeof(buf), "%u ms", stats.avgFrameTimeMs);
  drawTarget->setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
  drawTarget->drawString(buf, cursorX + 100, cursorY);
  cursorY += LINE_HEIGHT;

  // Dirty rects
  drawTarget->setTextColor(COLOR_LABEL, COLOR_BACKGROUND);
  drawTarget->drawString("Dirty rects:", cursorX, cursorY);
  snprintf(buf, sizeof(buf), "%u", stats.dirtyRectCount);
  drawTarget->setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
  drawTarget->drawString(buf, cursorX + 100, cursorY);
  cursorY += LINE_HEIGHT;

  // Dirty area
  drawTarget->setTextColor(COLOR_LABEL, COLOR_BACKGROUND);
  drawTarget->drawString("Dirty area:", cursorX, cursorY);
  snprintf(buf, sizeof(buf), "%u px", stats.dirtyPixels);
  drawTarget->setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
  drawTarget->drawString(buf, cursorX + 100, cursorY);
  cursorY += LINE_HEIGHT;

  // Bandwidth (convert bytes to KB/s)
  // Use 64-bit arithmetic to prevent overflow
  drawTarget->setTextColor(COLOR_LABEL, COLOR_BACKGROUND);
  drawTarget->drawString("Bandwidth:", cursorX, cursorY);
  uint32_t bandwidthKBps =
      (uint32_t)(((uint64_t)stats.bytesPushed * stats.fps) / 1024);
  snprintf(buf, sizeof(buf), "%u KB/s", bandwidthKBps);
  drawTarget->setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
  drawTarget->drawString(buf, cursorX + 100, cursorY);
  cursorY += LINE_HEIGHT;

  // PSRAM usage
  drawTarget->setTextColor(COLOR_LABEL, COLOR_BACKGROUND);
  drawTarget->drawString("PSRAM:", cursorX, cursorY);
  uint32_t psramKB = stats.psramUsedBytes / 1024;
  snprintf(buf, sizeof(buf), "%u KB", psramKB);
  drawTarget->setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
  drawTarget->drawString(buf, cursorX + 100, cursorY);
  cursorY += LINE_HEIGHT;

  // Shadow mode status
  drawTarget->setTextColor(COLOR_LABEL, COLOR_BACKGROUND);
  drawTarget->drawString("Shadow:", cursorX, cursorY);
  drawTarget->setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
  drawTarget->drawString(stats.shadowEnabled ? "ON" : "OFF", cursorX + 100,
                         cursorY);
  cursorY += LINE_HEIGHT;

  // Shadow blocks (only if shadow enabled)
  if (stats.shadowEnabled) {
    drawTarget->setTextColor(COLOR_LABEL, COLOR_BACKGROUND);
    drawTarget->drawString("Shadow blocks:", cursorX, cursorY);
    snprintf(buf, sizeof(buf), "%u", stats.shadowBlocksCompared);
    drawTarget->setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
    drawTarget->drawString(buf, cursorX + 100, cursorY);
    cursorY += LINE_HEIGHT;

    // Shadow errors (red if > 0)
    drawTarget->setTextColor(COLOR_LABEL, COLOR_BACKGROUND);
    drawTarget->drawString("Shadow errors:", cursorX, cursorY);
    snprintf(buf, sizeof(buf), "%u", stats.shadowMismatches);
    uint16_t errorColor = (stats.shadowMismatches > 0) ? TFT_RED : COLOR_TEXT;
    drawTarget->setTextColor(errorColor, COLOR_BACKGROUND);
    drawTarget->drawString(buf, cursorX + 100, cursorY);
  }
}

// ========================================
// Public API
// ========================================

void init(TFT_eSPI *tftDisplay) {
  tft = tftDisplay;
  SafeDraw::init(tft);  // 🚨 CRITICAL FIX: Initialize SafeDraw
  sprite = nullptr; // Will be set by compositor if used
  initialized = true;
  visible = false; // Hidden by default

  // Clear the area
  if (tft) {
    tft->fillRect(TELEMETRY_X, TELEMETRY_Y, TELEMETRY_WIDTH, TELEMETRY_HEIGHT,
                  COLOR_BACKGROUND);
  }
}

void setVisible(bool v) { visible = v; }

bool isVisible() { return visible; }

// ========================================
// Compositor Integration
// ========================================

/**
 * @brief Layer renderer implementation for compositor
 */
class GraphicsTelemetryRenderer : public HudLayer::LayerRenderer {
public:
  void render(HudLayer::RenderContext &ctx) override {
    if (!ctx.isValid()) return;
    if (!visible) return; // Don't render if not visible

    // Store sprite for potential legacy use
    sprite = ctx.sprite;

    // Get current render stats from compositor
    const HudCompositor::RenderStats &stats = HudCompositor::getRenderStats();

    // Always redraw when visible (stats change every frame)
    drawTelemetry(stats, ctx.sprite);

    // Mark telemetry area as dirty
    ctx.markDirty(TELEMETRY_X, TELEMETRY_Y, TELEMETRY_WIDTH, TELEMETRY_HEIGHT);
  }

  bool isActive() const override {
    // Only active when initialized and visible
    return initialized && visible;
  }
};

// Singleton instance for compositor registration
static GraphicsTelemetryRenderer rendererInstance;

/**
 * @brief Get the layer renderer for compositor registration
 * @return Pointer to the layer renderer instance
 */
HudLayer::LayerRenderer *getRenderer() { return &rendererInstance; }

} // namespace HudGraphicsTelemetry
