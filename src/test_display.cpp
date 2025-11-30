// ============================================================================
// test_display.cpp - Isolated TFT Display Test
// ============================================================================
// This file provides a standalone test routine for the TFT display (ST7796S)
// to verify basic functionality without depending on other system components.
//
// Enable by defining TEST_DISPLAY_STANDALONE in platformio.ini build_flags.
// This test runs during boot to confirm display communication works.
// ============================================================================

#ifdef TEST_DISPLAY_STANDALONE

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "pins.h"

// Create a local TFT instance for standalone testing
static TFT_eSPI testTft = TFT_eSPI();

// Test configuration
static constexpr uint32_t TEST_COLOR_DELAY_MS = 500;     // Delay between color screens
static constexpr uint32_t TEST_CIRCLE_COUNT = 10;        // Number of random circles
static constexpr uint32_t TEST_LOOP_DELAY_MS = 100;      // Main loop delay
static constexpr uint32_t WATCHDOG_FEED_INTERVAL_MS = 1000;  // Watchdog feed interval

// Color test palette
static const uint16_t TEST_COLORS[] = {
    TFT_RED, TFT_GREEN, TFT_BLUE, 
    TFT_YELLOW, TFT_CYAN, TFT_MAGENTA,
    TFT_WHITE, TFT_ORANGE, TFT_BLACK
};
static constexpr size_t NUM_TEST_COLORS = sizeof(TEST_COLORS) / sizeof(TEST_COLORS[0]);

// Forward declarations for internal test functions
static void runColorTest();
static void runCircleTest();
static void runTextTest();

/**
 * @brief Initialize and run display tests
 * Call this from setup() when TEST_DISPLAY_STANDALONE is defined
 * Note: This function is only accessible when TEST_DISPLAY_STANDALONE is defined
 */
static void setupDisplayTest() {
    // 1. Initialize Serial for debug output
    Serial.begin(115200);
    yield();  // Non-blocking yield instead of delay
    Serial.println();
    Serial.println("========================================");
    Serial.println("TFT Display Test - ST7796S");
    Serial.println("========================================");
    
    // 2. Configure backlight pin
    Serial.println("[TEST] Configuring backlight...");
    pinMode(PIN_TFT_BL, OUTPUT);
    digitalWrite(PIN_TFT_BL, HIGH);
    Serial.printf("[TEST] Backlight enabled on GPIO %d\n", PIN_TFT_BL);
    
    // 3. Hardware reset the display
    Serial.println("[TEST] Performing hardware reset...");
    pinMode(PIN_TFT_RST, OUTPUT);
    digitalWrite(PIN_TFT_RST, LOW);
    digitalWrite(PIN_TFT_RST, LOW);
    uint32_t resetStart = millis();
    while (millis() - resetStart < 10) yield();  // 10ms reset pulse
    digitalWrite(PIN_TFT_RST, HIGH);
    resetStart = millis();
    while (millis() - resetStart < 50) yield();  // 50ms recovery time
    Serial.println("[TEST] Reset complete");
    
    // 4. Initialize TFT
    Serial.println("[TEST] Initializing TFT_eSPI...");
    testTft.init();
    testTft.setRotation(3);  // Landscape 480x320
    Serial.printf("[TEST] Display dimensions: %dx%d\n", testTft.width(), testTft.height());
    
    // 5. Clear screen
    testTft.fillScreen(TFT_BLACK);
    yield();
    
    // 6. Run test sequence
    Serial.println("[TEST] Running color test...");
    runColorTest();
    
    Serial.println("[TEST] Running circle test...");
    runCircleTest();
    
    Serial.println("[TEST] Running text test...");
    runTextTest();
    
    Serial.println("[TEST] Display test complete!");
    Serial.println("========================================");
}

/**
 * @brief Main loop for display test mode
 * Note: This function is only accessible when TEST_DISPLAY_STANDALONE is defined
 */
static void loopDisplayTest() {
    static uint32_t lastWatchdogFeed = 0;
    static uint32_t loopCount = 0;
    uint32_t now = millis();
    
    // Feed watchdog periodically
    if (now - lastWatchdogFeed >= WATCHDOG_FEED_INTERVAL_MS) {
        lastWatchdogFeed = now;
        // ESP32 watchdog is fed automatically in Arduino framework
        yield();
    }
    
    // Update loop counter on display
    loopCount++;
    if (loopCount % 100 == 0) {
        testTft.fillRect(10, 290, 200, 20, TFT_BLACK);
        testTft.setTextColor(TFT_WHITE, TFT_BLACK);
        testTft.setCursor(10, 290);
        testTft.printf("Loop: %lu  Uptime: %lus", loopCount, now / 1000);
    }
    
    delay(TEST_LOOP_DELAY_MS);
}

/**
 * @brief Run color fill test on the display
 */
static void runColorTest() {
    for (size_t i = 0; i < NUM_TEST_COLORS; i++) {
        testTft.fillScreen(TEST_COLORS[i]);
        delay(TEST_COLOR_DELAY_MS);
        yield();  // Keep watchdog happy
    }
    testTft.fillScreen(TFT_BLACK);
}

/**
 * @brief Draw random circles on the display
 */
static void runCircleTest() {
    testTft.fillScreen(TFT_BLACK);
    
    for (uint32_t i = 0; i < TEST_CIRCLE_COUNT; i++) {
        int16_t x = random(20, testTft.width() - 20);
        int16_t y = random(20, testTft.height() - 20);
        int16_t r = random(10, 50);
        uint16_t color = TEST_COLORS[random(0, NUM_TEST_COLORS)];  // random(min, max) returns min to max-1
        
        testTft.fillCircle(x, y, r, color);
        delay(100);
        yield();
    }
    
    delay(1000);
    testTft.fillScreen(TFT_BLACK);
}

/**
 * @brief Display test text on the screen
 */
static void runTextTest() {
    testTft.fillScreen(TFT_BLACK);
    
    // Title
    testTft.setTextDatum(MC_DATUM);
    testTft.setTextColor(TFT_CYAN, TFT_BLACK);
    testTft.drawString("TFT Display Test", 240, 40, 4);
    
    // Test info
    testTft.setTextDatum(TL_DATUM);
    testTft.setTextColor(TFT_WHITE, TFT_BLACK);
    testTft.setCursor(10, 80);
    testTft.println("Display: ST7796S 480x320");
    testTft.setCursor(10, 100);
    testTft.println("Controller: ESP32-S3");
    testTft.setCursor(10, 120);
    testTft.printf("Rotation: %d (Landscape)", testTft.getRotation());
    
    // Pin info
    testTft.setTextColor(TFT_YELLOW, TFT_BLACK);
    testTft.setCursor(10, 160);
    testTft.println("Pin Configuration:");
    testTft.setTextColor(TFT_GREEN, TFT_BLACK);
    testTft.setCursor(20, 180);
    testTft.printf("CS: GPIO %d", PIN_TFT_CS);
    testTft.setCursor(20, 200);
    testTft.printf("DC: GPIO %d", PIN_TFT_DC);
    testTft.setCursor(20, 220);
    testTft.printf("RST: GPIO %d", PIN_TFT_RST);
    testTft.setCursor(20, 240);
    testTft.printf("BL: GPIO %d", PIN_TFT_BL);
    
    // Status
    testTft.setTextDatum(MC_DATUM);
    testTft.setTextColor(TFT_GREEN, TFT_BLACK);
    testTft.drawString("TEST PASSED", 240, 280, 4);
}

#endif // TEST_DISPLAY_STANDALONE
