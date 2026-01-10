#include "render_engine.h"
#include "logger.h"
#include <Arduino.h>

namespace RenderEngine {

// Static data
static TFT_eSPI *tft = nullptr;
static LayerSprite layers[RenderLayer::COUNT];
static bool initialized = false;
static bool dmaEnabled = true;

// Frame timing
static uint32_t lastFrameTime = 0;
static uint32_t frameTime = 0;
static float avgFPS = 0.0f;
static const int FPS_SAMPLES = 10;
static float fpsBuffer[FPS_SAMPLES] = {0};
static int fpsIndex = 0;

void init(TFT_eSPI *display) {
  if (display == nullptr) {
    Logger::error("RenderEngine: Cannot initialize with null display");
    return;
  }

  tft = display;

  // Initialize all layer slots
  for (int i = 0; i < RenderLayer::COUNT; i++) {
    layers[i].sprite = nullptr;
    layers[i].x = 0;
    layers[i].y = 0;
    layers[i].w = 0;
    layers[i].h = 0;
    layers[i].dirty = false;
    layers[i].visible = false;
    layers[i].created = false;
  }

  initialized = true;
  lastFrameTime = millis();

  Logger::info("RenderEngine: Initialized - sprite-based layered rendering ready");
  
#ifdef BOARD_HAS_PSRAM
  size_t psramSize = ESP.getPsramSize();
  size_t freePsram = ESP.getFreePsram();
  Logger::infof("RenderEngine: PSRAM total=%d bytes, free=%d bytes", 
                psramSize, freePsram);
#else
  Logger::warn("RenderEngine: PSRAM not available - sprites will use regular RAM");
#endif
}

bool createLayer(RenderLayer::Layer layer, int16_t x, int16_t y, int16_t w,
                 int16_t h, uint8_t colorDepth) {
  if (!initialized) {
    Logger::error("RenderEngine: Not initialized");
    return false;
  }

  if (layer >= RenderLayer::COUNT) {
    Logger::errorf("RenderEngine: Invalid layer %d", layer);
    return false;
  }

  LayerSprite &ls = layers[layer];

  // Delete existing sprite if any
  if (ls.created && ls.sprite != nullptr) {
    delete ls.sprite;
    ls.sprite = nullptr;
    ls.created = false;
  }

  // Create new sprite with PSRAM allocation
  ls.sprite = new TFT_eSprite(tft);
  if (ls.sprite == nullptr) {
    Logger::errorf("RenderEngine: Failed to allocate sprite for layer %s",
                   RenderLayer::getLayerName(layer));
    return false;
  }

  // Set color depth (1, 8, 16, or 24 bits)
  ls.sprite->setColorDepth(colorDepth);

  // Create sprite buffer in PSRAM
  // TFT_eSPI automatically uses PSRAM if available when calling createSprite()
  // Note: ESP32-S3 with PSRAM OPI will allocate from PSRAM
  void *spriteBuffer = ls.sprite->createSprite(w, h);
  if (spriteBuffer == nullptr) {
    Logger::errorf("RenderEngine: Failed to create sprite buffer for layer %s "
                   "(size: %dx%d, depth: %d)",
                   RenderLayer::getLayerName(layer), w, h, colorDepth);
    delete ls.sprite;
    ls.sprite = nullptr;
    return false;
  }

  // Store layer info
  ls.x = x;
  ls.y = y;
  ls.w = w;
  ls.h = h;
  ls.dirty = true;
  ls.visible = true;
  ls.created = true;

  Logger::infof("RenderEngine: Created layer %s at (%d,%d) size %dx%d depth %d",
                RenderLayer::getLayerName(layer), x, y, w, h, colorDepth);

  return true;
}

void deleteLayer(RenderLayer::Layer layer) {
  if (layer >= RenderLayer::COUNT)
    return;

  LayerSprite &ls = layers[layer];
  if (ls.created && ls.sprite != nullptr) {
    ls.sprite->deleteSprite();
    delete ls.sprite;
    ls.sprite = nullptr;
    ls.created = false;
    Logger::infof("RenderEngine: Deleted layer %s",
                  RenderLayer::getLayerName(layer));
  }
}

void markDirty(RenderLayer::Layer layer) {
  if (layer >= RenderLayer::COUNT)
    return;
  if (layers[layer].created) {
    layers[layer].dirty = true;
  }
}

void markAllDirty() {
  for (int i = 0; i < RenderLayer::COUNT; i++) {
    if (layers[i].created) {
      layers[i].dirty = true;
    }
  }
}

void setLayerVisible(RenderLayer::Layer layer, bool visible) {
  if (layer >= RenderLayer::COUNT)
    return;
  if (layers[layer].created) {
    bool wasVisible = layers[layer].visible;
    layers[layer].visible = visible;
    if (visible && !wasVisible) {
      layers[layer].dirty = true; // Mark dirty when made visible
    }
  }
}

TFT_eSprite *getSprite(RenderLayer::Layer layer) {
  if (layer >= RenderLayer::COUNT)
    return nullptr;
  if (!layers[layer].created)
    return nullptr;
  return layers[layer].sprite;
}

void render() {
  if (!initialized)
    return;

  uint32_t renderStart = millis();
  int dirtyCount = 0;
  int visibleCount = 0;

  // Render layers in order (background to foreground)
  for (int i = 0; i < RenderLayer::COUNT; i++) {
    LayerSprite &ls = layers[i];

    // Count visible layers
    if (ls.created && ls.visible) {
      visibleCount++;
    }

    // Skip if not created, not visible, or not dirty
    if (!ls.created || !ls.visible || !ls.dirty)
      continue;

    // Push sprite to screen
    // TFT_eSPI will use DMA if available and enabled
    if (ls.sprite != nullptr) {
      ls.sprite->pushSprite(ls.x, ls.y);
      dirtyCount++;
    }

    // Clear dirty flag
    ls.dirty = false;
  }

  // Update frame timing
  uint32_t now = millis();
  frameTime = now - renderStart;

  // Calculate FPS
  if (now != lastFrameTime) {
    float fps = 1000.0f / (now - lastFrameTime);
    fpsBuffer[fpsIndex] = fps;
    fpsIndex = (fpsIndex + 1) % FPS_SAMPLES;

    // Calculate average
    float sum = 0.0f;
    for (int i = 0; i < FPS_SAMPLES; i++) {
      sum += fpsBuffer[i];
    }
    avgFPS = sum / FPS_SAMPLES;
  }
  lastFrameTime = now;

  // Log rendering stats occasionally (every 10 seconds for less spam)
  static uint32_t lastLog = 0;
  if (now - lastLog > 10000 && dirtyCount > 0) {
    Logger::infof("RenderEngine: FPS=%.1f, dirty=%d/%d visible, frame=%dms", 
                  avgFPS, dirtyCount, visibleCount, frameTime);
    lastLog = now;
  }
}

void clearLayer(RenderLayer::Layer layer, uint16_t color) {
  if (layer >= RenderLayer::COUNT)
    return;
  LayerSprite &ls = layers[layer];
  if (ls.created && ls.sprite != nullptr) {
    ls.sprite->fillSprite(color);
    ls.dirty = true;
  }
}

LayerSprite *getLayerInfo(RenderLayer::Layer layer) {
  if (layer >= RenderLayer::COUNT)
    return nullptr;
  return &layers[layer];
}

void setDMAEnabled(bool enabled) {
  dmaEnabled = enabled;
  Logger::infof("RenderEngine: DMA %s (controlled via TFT_eSPI config)", 
                enabled ? "requested" : "disabled");
  // Note: TFT_eSPI DMA is controlled via library configuration
  // This flag is informational only
}

uint32_t getFrameTime() { return frameTime; }

float getFPS() { return avgFPS; }

void cleanup() {
  for (int i = 0; i < RenderLayer::COUNT; i++) {
    deleteLayer((RenderLayer::Layer)i);
  }
  initialized = false;
  Logger::info("RenderEngine: Cleanup complete");
}

} // namespace RenderEngine

// Layer name helper implementation
namespace RenderLayer {
const char *getLayerName(Layer layer) {
  static const char *names[] = {
      "BACKGROUND", "CAR_BODY", "WHEELS",  "GAUGES", "STEERING",
      "ICONS",      "PEDAL_BAR", "BUTTONS", "OVERLAYS"};
  if (layer < COUNT)
    return names[layer];
  return "UNKNOWN";
}
} // namespace RenderLayer
