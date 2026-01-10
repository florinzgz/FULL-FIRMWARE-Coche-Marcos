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
 * PHASE 2 ADDITION (Shadow Rendering):
 * When RENDER_SHADOW_MODE is defined, an additional STEERING_SHADOW sprite
 * is created for validation purposes. This shadow sprite receives the same
 * drawing commands as the real STEERING sprite to enable pixel-level
 * comparison and verification before migration. The shadow sprite is NEVER
 * rendered to the display.
 */
class RenderEngine {
public:
  /**
   * @brief Sprite identifiers
   */
  enum SpriteID {
    CAR_BODY = 0,        // Car body layer (drawn first, static background)
    STEERING = 1,        // Steering wheel layer (drawn on top, dynamic)
#ifdef RENDER_SHADOW_MODE
    STEERING_SHADOW = 2  // Shadow sprite for validation (never displayed)
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
   * @brief Compare STEERING and STEERING_SHADOW sprites for pixel differences
   * 
   * This function compares the STEERING sprite (production) with the
   * STEERING_SHADOW sprite (validation) to detect rendering mismatches.
   * Results are logged via Logger and statistics are updated.
   * 
   * @return Number of pixel mismatches found (0 = perfect match)
   */
  static uint32_t compareShadowSprites();

  /**
   * @brief Get shadow rendering statistics
   * 
   * @param outTotalComparisons Total number of comparisons performed
   * @param outTotalMismatches Total number of frames with mismatches
   * @param outLastMismatchCount Last frame's mismatch pixel count
   */
  static void getShadowStats(uint32_t &outTotalComparisons,
                             uint32_t &outTotalMismatches,
                             uint32_t &outLastMismatchCount);
  
  /**
   * @brief Get detailed shadow comparison metrics (Phase 4)
   * 
   * @param outMatchPercentage Percentage of matching pixels (0.0 - 100.0)
   * @param outMaxMismatch Maximum mismatch count seen across all frames
   * @param outAvgMismatch Average mismatch count across all frames
   */
  static void getShadowMetrics(float &outMatchPercentage,
                               uint32_t &outMaxMismatch,
                               float &outAvgMismatch);
  
  /**
   * @brief Get safety protection statistics (Phase 5)
   * 
   * @param outClampedRects Number of dirty rectangles clamped to bounds
   * @param outRejectedRects Number of invalid dirty rectangles rejected
   * @param outNullSprites Number of null sprite accesses prevented
   * @param outDMABlocks Number of invalid DMA transfers blocked
   */
  static void getSafetyStats(uint32_t &outClampedRects,
                             uint32_t &outRejectedRects,
                             uint32_t &outNullSprites,
                             uint32_t &outDMABlocks);
  
  /**
   * @brief Shadow ignore region structure (Phase 3.6)
   * 
   * Defines rectangular regions that should be excluded from pixel
   * comparison. Used to filter out non-HUD overlays (menus, dialogs,
   * calibration screens) that are not part of normal HUD rendering.
   */
  struct ShadowMask {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
  };
  
  /**
   * @brief Add a region to exclude from shadow pixel comparison (Phase 3.6)
   * 
   * @param x X coordinate of top-left corner
   * @param y Y coordinate of top-left corner
   * @param w Width of the region
   * @param h Height of the region
   */
  static void addShadowIgnoreRegion(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
  
  /**
   * @brief Clear all shadow ignore regions (Phase 3.6)
   */
  static void clearShadowIgnoreRegions();
  
  /**
   * @brief Check if a pixel should be ignored in shadow comparison (Phase 3.6)
   * 
   * @param x X coordinate of the pixel
   * @param y Y coordinate of the pixel
   * @return true if pixel is within an ignored region, false otherwise
   */
  static bool isShadowIgnored(uint16_t x, uint16_t y);
#endif

private:
  static TFT_eSPI *tft;
#ifdef RENDER_SHADOW_MODE
  static TFT_eSprite *sprites[3]; // CAR_BODY, STEERING, and STEERING_SHADOW
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
  
  // Shadow rendering statistics (Phase 2 & 4)
  static uint32_t shadowComparisonCount;    // Total comparisons performed
  static uint32_t shadowMismatchCount;      // Frames with mismatches
  static uint32_t shadowLastMismatch;       // Last mismatch pixel count
  static uint32_t shadowMaxMismatch;        // Maximum mismatch seen (Phase 4)
  static uint64_t shadowTotalMismatch;      // Sum of all mismatches for average (Phase 4)
  
  // Safety protection statistics (Phase 5)
  static uint32_t shadowClampedRects;       // Dirty rects clamped to bounds
  static uint32_t shadowRejectedRects;      // Invalid dirty rects rejected
  static uint32_t shadowNullSprites;        // Null sprite accesses prevented
  static uint32_t shadowDMABlocks;          // Invalid DMA transfers blocked
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
