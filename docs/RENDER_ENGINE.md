# Sprite-Based Rendering Engine

## Overview

This rendering engine implements layered sprite-based rendering with:
- PSRAM-backed sprite buffers for reduced main RAM usage
- Dirty rectangle tracking to minimize SPI bus traffic
- DMA support for faster transfers (when configured in TFT_eSPI)
- Layer-based composition for organized UI rendering

## Architecture

### Layers (from back to front)
1. **BACKGROUND** - Static background, rarely changes
2. **CAR_BODY** - 3D car body visualization with axles
3. **WHEELS** - Four wheel widgets with steering
4. **GAUGES** - Speed and RPM gauges
5. **STEERING** - Steering wheel indicator
6. **ICONS** - System icons, battery, temperature
7. **PEDAL_BAR** - Pedal position bar
8. **BUTTONS** - Touch buttons (axis rotation, etc.)
9. **OVERLAYS** - Menus, warnings, mode indicators

### Key Features

#### PSRAM Allocation
- Sprites automatically use PSRAM when available
- ESP32-S3 with OPI PSRAM provides ~16MB for sprite buffers
- Significantly reduces main RAM pressure

#### Dirty Tracking
- Only dirty layers are pushed to screen
- Reduces SPI bus saturation
- Improves frame rate and reduces flicker

#### DMA Transfer
- When enabled in TFT_eSPI, uses DMA for non-blocking transfers
- Allows CPU to continue processing while display updates
- Requires proper SPI configuration

## Usage Example

```cpp
#include "render_engine.h"
#include "layer_map.h"
#include <TFT_eSPI.h>

extern TFT_eSPI tft;

void setup() {
  // Initialize TFT first
  tft.init();
  tft.setRotation(3);  // 480x320 landscape
  
  // Initialize render engine
  RenderEngine::init(&tft);
  
  // Create a layer for car body (example)
  RenderEngine::createLayer(RenderLayer::CAR_BODY, 
                           175, 100,  // x, y position
                           130, 150,  // width, height
                           16);       // color depth (16-bit RGB565)
  
  // Get sprite to draw into
  TFT_eSprite *carSprite = RenderEngine::getSprite(RenderLayer::CAR_BODY);
  if (carSprite != nullptr) {
    // Draw into sprite (local coordinates)
    carSprite->fillSprite(TFT_BLACK);
    carSprite->fillRect(10, 10, 110, 130, TFT_DARKGREY);
    carSprite->drawRect(10, 10, 110, 130, TFT_WHITE);
    
    // Mark as dirty so it will be rendered
    RenderEngine::markDirty(RenderLayer::CAR_BODY);
  }
}

void loop() {
  // Update your sprites as needed
  // Only mark layers dirty when they actually change
  
  // Render all dirty layers to screen
  RenderEngine::render();
  
  // Get performance metrics
  float fps = RenderEngine::getFPS();
  uint32_t frameTime = RenderEngine::getFrameTime();
}
```

## Configuration

### PlatformIO Build Flags

Add to `platformio.ini`:

```ini
build_flags =
    ; Enable PSRAM for sprite buffers
    -DUSE_PSRAM_SPRITES=1
    ; Enable DMA transfers
    -DUSE_DMA_TO_TFT=1
    ; Support SPI transactions
    -DSUPPORT_TRANSACTIONS=1
```

### TFT_eSPI Configuration

The engine uses TFT_eSPI's built-in sprite support. Ensure your configuration has:
- PSRAM enabled on ESP32-S3
- SPI frequency optimized (40MHz for ST7796S)
- DMA enabled if supported

## Migration Strategy

### Phase 1: Add Engine (No Breaking Changes)
- Add render_engine.h, render_engine.cpp, layer_map.h
- Engine exists but is not used yet
- Current code continues to work unchanged

### Phase 2: Convert Static Elements
- Migrate car body to sprite (rarely changes)
- Migrate background elements to sprites
- Current dynamic elements still draw directly

### Phase 3: Convert Dynamic Elements
- Migrate gauges to sprite-based
- Migrate wheels to sprite-based
- Migrate icons to sprite-based

### Phase 4: Full Integration
- All drawing goes through sprites
- Remove direct TFT drawing
- Optimize layer sizes and positions

## Performance Benefits

### Before (Direct Drawing)
- Every frame redraws everything
- SPI bus saturated with redundant data
- Frame rate: 15-25 FPS
- Visible flicker on updates

### After (Sprite-Based)
- Only dirty regions pushed to screen
- Minimal SPI traffic
- Frame rate: 50-60 FPS target
- Smooth, flicker-free rendering

## Memory Usage

### Example Sprite Sizes

| Layer | Size | Depth | Memory |
|-------|------|-------|--------|
| CAR_BODY | 150x170 | 16-bit | 51KB |
| STEERING | 110x70 | 16-bit | 15.4KB |
| GAUGES | 400x100 | 16-bit | 80KB |
| PEDAL_BAR | 480x20 | 16-bit | 19.2KB |
| ICONS | 480x100 | 16-bit | 96KB |
| **Total** | | | **~262KB** |

With 16MB PSRAM, this is easily manageable.

## API Reference

### Initialization
```cpp
void RenderEngine::init(TFT_eSPI *display);
```

### Layer Management
```cpp
bool createLayer(RenderLayer::Layer layer, int16_t x, int16_t y, 
                 int16_t w, int16_t h, uint8_t colorDepth = 16);
void deleteLayer(RenderLayer::Layer layer);
void setLayerVisible(RenderLayer::Layer layer, bool visible);
```

### Drawing
```cpp
TFT_eSprite *getSprite(RenderLayer::Layer layer);
void clearLayer(RenderLayer::Layer layer, uint16_t color = TFT_BLACK);
```

### Dirty Tracking
```cpp
void markDirty(RenderLayer::Layer layer);
void markAllDirty();
```

### Rendering
```cpp
void render();  // Push all dirty layers to screen
```

### Performance
```cpp
float getFPS();
uint32_t getFrameTime();
```

### Cleanup
```cpp
void cleanup();  // Free all sprites and resources
```

## Notes

- Sprite coordinates are in local space (0,0 = top-left of sprite)
- Screen push uses absolute coordinates (layer x, y position)
- Color depth can be 1, 8, 16, or 24 bits
- 16-bit (RGB565) is recommended for best performance/quality balance
- Always check if getSprite() returns nullptr before drawing

## Troubleshooting

### Sprites not appearing
- Check layer is marked dirty: `RenderEngine::markDirty(layer)`
- Verify render() is called in loop
- Ensure layer is visible: `setLayerVisible(layer, true)`

### Memory allocation failures
- Reduce sprite sizes or color depth
- Check PSRAM is enabled: `ESP.getPsramSize()`
- Monitor free PSRAM: `ESP.getFreePsram()`

### Low frame rate
- Reduce number of layers marked dirty each frame
- Optimize sprite sizes to only cover necessary areas
- Use lower color depth (8-bit) for non-critical layers
