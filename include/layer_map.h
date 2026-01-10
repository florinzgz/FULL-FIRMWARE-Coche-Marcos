#pragma once

// Layer definitions for the rendering engine
// Layers are rendered from background to foreground (lower values first)
namespace RenderLayer {
enum Layer : uint8_t {
  BACKGROUND = 0,  // Static background, clear screen
  CAR_BODY = 1,    // 3D car body with axles and differentials
  WHEELS = 2,      // Four wheel widgets with steering angle
  GAUGES = 3,      // Speed and RPM gauges
  STEERING = 4,    // Steering wheel indicator
  ICONS = 5,       // System state, gear, features, battery, temp
  PEDAL_BAR = 6,   // Bottom pedal bar
  BUTTONS = 7,     // Touch buttons (axis rotation, demo button)
  OVERLAYS = 8,    // MenuHidden, mode indicator, warnings
  COUNT = 9        // Total number of layers
};

// Layer names for debugging
const char *getLayerName(Layer layer);
} // namespace RenderLayer
