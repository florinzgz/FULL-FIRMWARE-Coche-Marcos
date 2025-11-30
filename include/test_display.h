#pragma once

// ============================================================================
// test_display.h - Isolated TFT Display Test Header
// ============================================================================
// Provides function declarations for standalone TFT display testing.
// Only available when TEST_DISPLAY_STANDALONE is defined.
// ============================================================================

#ifdef TEST_DISPLAY_STANDALONE

/**
 * @brief Initialize and run display tests
 * Call this from setup() in main.cpp when TEST_DISPLAY_STANDALONE is defined
 */
void setupDisplayTest();

/**
 * @brief Main loop for display test mode
 * Call this from loop() in main.cpp when TEST_DISPLAY_STANDALONE is defined
 */
void loopDisplayTest();

#endif // TEST_DISPLAY_STANDALONE
