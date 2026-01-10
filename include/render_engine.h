#pragma once

#include "layer_map.h"
#include <TFT_eSPI.h>

// Rendering engine for layered sprite-based HUD
// Manages PSRAM-backed sprites, dirty tracking, and DMA transfers
namespace RenderEngine {

// Layer sprite information
struct LayerSprite {
  TFT_eSprite *sprite; // Sprite buffer (allocated in PSRAM)
  int16_t x;           // Screen position X
  int16_t y;           // Screen position Y
  int16_t w;           // Sprite width
  int16_t h;           // Sprite height
  bool dirty;          // Needs redraw
  bool visible;        // Should be rendered
  bool created;        // Sprite allocated
};

// Initialize rendering engine
// Must be called after TFT_eSPI is initialized
void init(TFT_eSPI *display);

// Create a layer sprite with specified dimensions and position
// Uses PSRAM for buffer allocation
// colorDepth: 1, 8, 16, or 24 bits per pixel
bool createLayer(RenderLayer::Layer layer, int16_t x, int16_t y, int16_t w,
                 int16_t h, uint8_t colorDepth = 16);

// Delete a layer sprite and free PSRAM
void deleteLayer(RenderLayer::Layer layer);

// Mark a layer as dirty (needs redraw)
void markDirty(RenderLayer::Layer layer);

// Mark all layers as dirty
void markAllDirty();

// Set layer visibility
void setLayerVisible(RenderLayer::Layer layer, bool visible);

// Get sprite for drawing (returns nullptr if not created)
TFT_eSprite *getSprite(RenderLayer::Layer layer);

// Render all dirty layers to screen
// Only pushes dirty sprites to minimize bus traffic
// Uses DMA when available for faster transfer
void render();

// Clear a specific layer (fill with transparent or background color)
void clearLayer(RenderLayer::Layer layer, uint16_t color = TFT_BLACK);

// Get layer info for manual positioning/sizing
LayerSprite *getLayerInfo(RenderLayer::Layer layer);

// Enable/disable DMA transfers (if supported by hardware)
void setDMAEnabled(bool enabled);

// Get current frame time in milliseconds
uint32_t getFrameTime();

// Get average FPS
float getFPS();

// Cleanup and free all resources
void cleanup();

} // namespace RenderEngine
