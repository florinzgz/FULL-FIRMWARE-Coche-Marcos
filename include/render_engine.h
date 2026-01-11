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
 *
 * When RENDER_SHADOW_MODE is enabled, a third sprite (STEERING_SHADOW) is
 * added for validation and debugging purposes.
 */
class RenderEngine {
public:
  /**
   * @brief Sprite identifiers
   */
  enum SpriteID {
    CAR_BODY = 0, // Car body layer (drawn first, static background)
    STEERING = 1, // Steering wheel layer (drawn on top, dynamic)
#ifdef RENDER_SHADOW_MODE
    STEERING_SHADOW =
        2 // Shadow sprite for validation (RENDER_SHADOW_MODE only)
#endif
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

#ifdef RENDER_SHADOW_MODE
  /**
   * @brief Shadow mode ignore region structure
   */
  struct ShadowMask {
    uint16_t x, y, w, h;
  };

  /**
   * @brief Compare steering sprite with shadow sprite (validation mode)
   * @return Number of mismatched pixels
   */
  static uint32_t compareShadowSprites();

  /**
   * @brief Get shadow comparison statistics
   * @param outTotalComparisons Total number of comparisons performed
   * @param outTotalMismatches Total number of comparisons with mismatches
   * @param outLastMismatchCount Number of mismatched pixels in last comparison
   */
  static void getShadowStats(uint32_t &outTotalComparisons,
                             uint32_t &outTotalMismatches,
                             uint32_t &outLastMismatchCount);

  /**
   * @brief Get detailed shadow comparison metrics
   * @param outMatchPercentage Percentage of matching pixels in last comparison
   * @param outMaxMismatch Maximum mismatch count across all comparisons
   * @param outAvgMismatch Average mismatch count per comparison
   */
  static void getShadowMetrics(float &outMatchPercentage,
                               uint32_t &outMaxMismatch, float &outAvgMismatch);

  /**
   * @brief Get safety protection statistics
   * @param outClampedRects Number of dirty rects clamped to bounds
   * @param outRejectedRects Number of invalid dirty rects rejected
   * @param outNullSprites Number of null sprite accesses prevented
   * @param outDMABlocks Number of invalid DMA transfers blocked
   */
  static void getSafetyStats(uint32_t &outClampedRects,
                             uint32_t &outRejectedRects,
                             uint32_t &outNullSprites, uint32_t &outDMABlocks);

  /**
   * @brief Clear all shadow ignore regions
   */
  static void clearShadowIgnoreRegions();

  /**
   * @brief Add a region to ignore during shadow comparison
   * @param x X coordinate of the region
   * @param y Y coordinate of the region
   * @param w Width of the region
   * @param h Height of the region
   */
  static void addShadowIgnoreRegion(uint16_t x, uint16_t y, uint16_t w,
                                    uint16_t h);

  /**
   * @brief Check if a pixel should be ignored during shadow comparison
   * @param x X coordinate of the pixel
   * @param y Y coordinate of the pixel
   * @return true if the pixel should be ignored, false otherwise
   */
  static bool isShadowIgnored(uint16_t x, uint16_t y);
#endif

private:
  static TFT_eSPI *tft;
#ifdef RENDER_SHADOW_MODE
  static TFT_eSprite
      *sprites[3]; // CAR_BODY, STEERING, and STEERING_SHADOW sprites
#else
  static TFT_eSprite *sprites[2]; // CAR_BODY and STEERING sprites
#endif
  static bool initialized;

  // Dirty rectangles for each sprite (bounding box)
#ifdef RENDER_SHADOW_MODE
  static int dirtyX[3];
  static int dirtyY[3];
  static int dirtyW[3];
  static int dirtyH[3];
  static bool isDirty[3];

  // Shadow rendering statistics
  static uint32_t shadowComparisonCount;
  static uint32_t shadowMismatchCount;
  static uint32_t shadowLastMismatch;
  static uint32_t shadowMaxMismatch;
  static uint64_t shadowTotalMismatch;

  // Safety protection statistics
  static uint32_t shadowClampedRects;
  static uint32_t shadowRejectedRects;
  static uint32_t shadowNullSprites;
  static uint32_t shadowDMABlocks;
#else
  static int dirtyX[2];
  static int dirtyY[2];
  static int dirtyW[2];
  static int dirtyH[2];
  static bool isDirty[2];
#endif

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
