#ifndef RENDER_ENGINE_H
#define RENDER_ENGINE_H

#include <TFT_eSPI.h>

/**
 * @brief Sprite-based rendering engine for optimized display updates
 * 
 * This engine manages two full-screen sprites (CAR_BODY and STEERING) to
 * optimize rendering performance. Sprites are created at full screen size
 * (480x320) to avoid coordinate translation - all existing draw calls keep
 * their original X,Y values.
 * 
 * The engine uses dirty rectangle tracking to push only changed regions
 * to the display via DMA, improving performance.
 */
class RenderEngine {
public:
  /**
   * @brief Sprite identifiers
   */
  enum SpriteID {
    CAR_BODY = 0,  // Car body layer (drawn first, static background)
    STEERING = 1   // Steering wheel layer (drawn on top, dynamic)
  };

  /**
   * @brief Initialize the rendering engine
   * @param tftDisplay Pointer to the TFT_eSPI display instance
   */
  static void init(TFT_eSPI *tftDisplay);

  /**
   * @brief Create a sprite with the specified dimensions
   * @param id Sprite identifier (CAR_BODY or STEERING)
   * @param width Sprite width in pixels (should be 480)
   * @param height Sprite height in pixels (should be 320)
   * @return true if sprite was created successfully, false otherwise
   */
  static bool createSprite(SpriteID id, int width, int height);

  /**
   * @brief Get a pointer to the specified sprite for drawing
   * @param id Sprite identifier (CAR_BODY or STEERING)
   * @return Pointer to TFT_eSprite, or nullptr if sprite doesn't exist
   */
  static TFT_eSprite *getSprite(SpriteID id);

  /**
   * @brief Mark a rectangular region as dirty (needs redraw)
   * @param x X coordinate of top-left corner
   * @param y Y coordinate of top-left corner
   * @param w Width of the region
   * @param h Height of the region
   */
  static void markDirtyRect(int x, int y, int w, int h);

  /**
   * @brief Render all dirty sprite regions to the display
   * 
   * This function pushes only the bounding box of dirty regions for each
   * sprite to the display, using DMA for optimal performance.
   */
  static void render();

  /**
   * @brief Clear all sprites and reset dirty regions
   */
  static void clear();

  /**
   * @brief Check if the rendering engine is initialized
   * @return true if initialized, false otherwise
   */
  static bool isInitialized();

private:
  static TFT_eSPI *tft;
  static TFT_eSprite *sprites[2];  // CAR_BODY and STEERING sprites
  static bool initialized;
  
  // Dirty rectangles for each sprite (bounding box)
  static int dirtyX[2];
  static int dirtyY[2];
  static int dirtyW[2];
  static int dirtyH[2];
  static bool isDirty[2];

  /**
   * @brief Update the dirty bounding box with a new rectangle
   * @param id Sprite identifier
   * @param x X coordinate of the new dirty region
   * @param y Y coordinate of the new dirty region
   * @param w Width of the new dirty region
   * @param h Height of the new dirty region
   */
  static void updateDirtyBounds(SpriteID id, int x, int y, int w, int h);
};

#endif // RENDER_ENGINE_H
