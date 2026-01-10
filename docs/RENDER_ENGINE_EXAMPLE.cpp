/**
 * Example: Minimal Integration of Render Engine into HUD
 * 
 * This file shows how to start integrating the sprite-based rendering engine
 * into the existing HUD system without breaking current functionality.
 * 
 * Integration approach: Gradual, non-breaking migration
 */

#include "render_engine.h"
#include "layer_map.h"
#include <TFT_eSPI.h>

// Example: Initialize render engine alongside existing HUD init
void exampleInitRenderEngine(TFT_eSPI *tft) {
  // Initialize the render engine
  RenderEngine::init(tft);
  
  // Create layers for static/semi-static content first
  // These are the easiest to migrate and provide immediate benefits
  
  // 1. Car body layer - drawn once, rarely changes
  RenderEngine::createLayer(RenderLayer::CAR_BODY, 
                           165, 90,    // Position on screen
                           150, 170,   // Size to cover car body + margin
                           16);        // 16-bit color depth
  
  // 2. Pedal bar layer - bottom of screen, updates frequently but in isolation
  RenderEngine::createLayer(RenderLayer::PEDAL_BAR,
                           0, 300,     // Bottom of 480x320 screen
                           480, 20,    // Full width, 20px height
                           16);
  
  // 3. Steering wheel layer - overlays car body, moderate update frequency
  RenderEngine::createLayer(RenderLayer::STEERING,
                           215, 150,   // Center area
                           50, 50,     // Small centered sprite
                           16);
}

// Example: Drawing car body into sprite instead of direct to TFT
void exampleDrawCarBodySprite() {
  // Get the sprite for the car body layer
  TFT_eSprite *carSprite = RenderEngine::getSprite(RenderLayer::CAR_BODY);
  if (carSprite == nullptr) {
    // Fallback: sprite not created, skip or draw directly to TFT
    return;
  }
  
  // Clear sprite background
  carSprite->fillSprite(TFT_BLACK);
  
  // Draw car body into sprite using sprite-local coordinates
  // Note: (0,0) is top-left of sprite, not screen
  
  // Example: Simple car outline
  carSprite->fillRect(10, 20, 130, 140, 0x2945);  // Car body color
  carSprite->drawRect(10, 20, 130, 140, 0x6B6D);  // Outline
  
  // Add details (headlights, wheels, etc.)
  carSprite->fillRect(15, 25, 20, 10, 0xFFE0);  // Headlight left
  carSprite->fillRect(115, 25, 20, 10, 0xFFE0); // Headlight right
  
  // Mark layer as dirty so it will be pushed to screen
  RenderEngine::markDirty(RenderLayer::CAR_BODY);
}

// Example: Drawing pedal bar into sprite
void exampleDrawPedalBarSprite(float pedalPercent) {
  TFT_eSprite *pedalSprite = RenderEngine::getSprite(RenderLayer::PEDAL_BAR);
  if (pedalSprite == nullptr) return;
  
  // Clear background
  pedalSprite->fillSprite(0x1082);  // Dark background
  
  // Calculate bar width
  int barWidth = (int)((pedalPercent / 100.0f) * 480.0f);
  
  // Draw progress bar
  if (barWidth > 0) {
    uint16_t barColor = (pedalPercent > 80) ? TFT_RED : TFT_GREEN;
    pedalSprite->fillRect(0, 0, barWidth, 20, barColor);
  }
  
  // Draw border
  pedalSprite->drawRect(0, 0, 480, 20, 0x2104);
  
  // Add text
  pedalSprite->setTextColor(TFT_WHITE, 0x1082);
  pedalSprite->setTextDatum(MC_DATUM);
  char txt[32];
  snprintf(txt, sizeof(txt), "PEDAL %d%%", (int)pedalPercent);
  pedalSprite->drawString(txt, 240, 10, 2);
  
  // Mark dirty
  RenderEngine::markDirty(RenderLayer::PEDAL_BAR);
}

// Example: Main rendering loop integration
void exampleRenderLoop() {
  // This would be called in the main loop
  // It pushes all dirty layers to the screen
  
  RenderEngine::render();
  
  // Optional: Monitor performance
  static uint32_t lastPerfLog = 0;
  if (millis() - lastPerfLog > 5000) {
    float fps = RenderEngine::getFPS();
    uint32_t frameTime = RenderEngine::getFrameTime();
    // Log or display performance metrics
    lastPerfLog = millis();
  }
}

// Example: Hybrid approach - gradual migration
void exampleHybridUpdate(TFT_eSPI *tft, float steerAngle, float pedalPercent) {
  // Static elements: use sprites (drawn once or rarely)
  static bool carBodyInitialized = false;
  if (!carBodyInitialized) {
    exampleDrawCarBodySprite();
    carBodyInitialized = true;
  }
  
  // Semi-dynamic elements: use sprites (update when changed)
  static float lastPedalPercent = -1.0f;
  if (fabs(pedalPercent - lastPedalPercent) > 0.5f) {
    exampleDrawPedalBarSprite(pedalPercent);
    lastPedalPercent = pedalPercent;
  }
  
  // Dynamic elements: keep drawing direct to TFT for now
  // (These can be migrated later in subsequent phases)
  // For example: wheels, gauges, icons can still draw directly
  
  // Push all dirty sprites to screen
  RenderEngine::render();
}

/**
 * Integration Checklist:
 * 
 * 1. In HUD::init():
 *    - Call exampleInitRenderEngine(&tft) after TFT initialization
 * 
 * 2. In HUD::update():
 *    - Migrate static elements to sprites first (car body)
 *    - Keep dynamic elements drawing directly initially
 *    - Call RenderEngine::render() at end of update()
 * 
 * 3. Gradual migration:
 *    - Phase 1: Car body (static)
 *    - Phase 2: Pedal bar (semi-dynamic)
 *    - Phase 3: Steering wheel (dynamic)
 *    - Phase 4: Gauges (dynamic)
 *    - Phase 5: Wheels (dynamic)
 *    - Phase 6: Icons (dynamic)
 * 
 * 4. Performance validation:
 *    - Monitor FPS with RenderEngine::getFPS()
 *    - Verify no flicker
 *    - Confirm PSRAM usage is acceptable
 * 
 * 5. Testing:
 *    - Verify touch still works
 *    - Verify MenuHidden system works
 *    - Test all display modes
 *    - Validate sensor display logic
 */
