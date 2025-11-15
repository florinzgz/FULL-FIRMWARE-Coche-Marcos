#include "led_controller.h"
#include "logger.h"
#include <Arduino.h>

namespace LEDController {

// LED Arrays
static CRGB frontLeds[LED_FRONT_COUNT];
static CRGB rearLeds[LED_REAR_COUNT];

// Current state
static FrontMode currentFrontMode = FRONT_OFF;
static RearMode currentRearMode = REAR_OFF;
static TurnSignal currentTurnSignal = TURN_OFF;

// Configuration
static Config config = {
    .brightness = 200,
    .updateRateMs = 50,
    .smoothTransitions = true
};

// Animation state
static uint32_t lastUpdateMs = 0;
static uint16_t animationStep = 0;
static bool blinkState = false;
static uint32_t lastBlinkMs = 0;
static bool enabled = true;

// KITT scanner state
static int8_t scannerPos = 0;
static int8_t scannerDirection = 1;

// Helper: blend two colors
static CRGB blendColor(CRGB c1, CRGB c2, uint8_t amount) {
    return CRGB(
        (c1.r * (255 - amount) + c2.r * amount) / 255,
        (c1.g * (255 - amount) + c2.g * amount) / 255,
        (c1.b * (255 - amount) + c2.b * amount) / 255
    );
}

// Helper: get color from throttle percentage
static CRGB getThrottleColor(float percent) {
    if (percent < 25.0f) {
        // Red → Orange
        return blendColor(CRGB::Red, CRGB::Orange, (uint8_t)(percent * 255 / 25));
    } else if (percent < 50.0f) {
        // Orange → Yellow
        return blendColor(CRGB::Orange, CRGB::Yellow, (uint8_t)((percent - 25) * 255 / 25));
    } else if (percent < 75.0f) {
        // Yellow → Green
        return blendColor(CRGB::Yellow, CRGB::Green, (uint8_t)((percent - 50) * 255 / 25));
    } else {
        // Green → Blue
        return blendColor(CRGB::Green, CRGB::Blue, (uint8_t)((percent - 75) * 255 / 25));
    }
}

// KITT scanner effect
static void updateKITTScanner(CRGB* leds, int count, CRGB color, int tailLength = 4) {
    // Fade all LEDs
    for (int i = 0; i < count; i++) {
        leds[i].fadeToBlackBy(60);
    }
    
    // Draw bright center LED
    leds[scannerPos] = color;
    
    // Draw tail
    for (int i = 1; i <= tailLength; i++) {
        int pos = scannerPos - (i * scannerDirection);
        if (pos >= 0 && pos < count) {
            uint8_t brightness = 255 - (i * 255 / tailLength);
            leds[pos] = color;
            leds[pos].fadeToBlackBy(255 - brightness);
        }
    }
    
    // Update scanner position
    scannerPos += scannerDirection;
    if (scannerPos >= count || scannerPos < 0) {
        scannerDirection = -scannerDirection;
        scannerPos += scannerDirection * 2;
    }
}

// Chase effect (LEDs light up in sequence)
static void updateChase(CRGB* leds, int count, CRGB color, int speed) {
    int pos = (animationStep / speed) % count;
    
    // Fade all LEDs
    for (int i = 0; i < count; i++) {
        leds[i].fadeToBlackBy(30);
    }
    
    // Light up current position and tail
    for (int i = 0; i < 5 && pos - i >= 0; i++) {
        leds[pos - i] = color;
        leds[pos - i].fadeToBlackBy(i * 50);
    }
}

// Rainbow effect
static void updateRainbow(CRGB* leds, int count, uint8_t speed) {
    uint8_t hue = (animationStep * speed) & 0xFF;
    fill_rainbow(leds, count, hue, 256 / count);
}

// Flash effect
static void updateFlash(CRGB* leds, int count, CRGB color1, CRGB color2) {
    CRGB color = blinkState ? color1 : color2;
    fill_solid(leds, count, color);
}

// Update front LEDs based on current mode
static void updateFrontLEDs() {
    switch (currentFrontMode) {
        case FRONT_OFF:
            fill_solid(frontLeds, LED_FRONT_COUNT, CRGB::Black);
            break;
            
        case FRONT_KITT_IDLE:
            updateKITTScanner(frontLeds, LED_FRONT_COUNT, CRGB::Red, 4);
            break;
            
        case FRONT_ACCEL_LOW:
            updateChase(frontLeds, LED_FRONT_COUNT, getThrottleColor(12.5f), 8);
            break;
            
        case FRONT_ACCEL_MED:
            updateChase(frontLeds, LED_FRONT_COUNT, getThrottleColor(37.5f), 5);
            break;
            
        case FRONT_ACCEL_HIGH:
            updateChase(frontLeds, LED_FRONT_COUNT, getThrottleColor(62.5f), 3);
            break;
            
        case FRONT_ACCEL_MAX:
            updateRainbow(frontLeds, LED_FRONT_COUNT, 3);
            break;
            
        case FRONT_REVERSE:
            updateKITTScanner(frontLeds, LED_FRONT_COUNT, CRGB::White, 3);
            break;
            
        case FRONT_ABS_ALERT:
            updateFlash(frontLeds, LED_FRONT_COUNT, CRGB::Red, CRGB::White);
            break;
            
        case FRONT_TCS_ALERT:
            updateFlash(frontLeds, LED_FRONT_COUNT, CRGB::Orange, CRGB::Black);
            break;
    }
}

// Update rear center LEDs (brake/position/reverse)
static void updateRearCenter() {
    CRGB color = CRGB::Black;
    uint8_t brightness = 255;
    
    switch (currentRearMode) {
        case REAR_OFF:
            color = CRGB::Black;
            break;
            
        case REAR_POSITION:
            color = CRGB::Red;
            brightness = 51;  // 20% brightness
            break;
            
        case REAR_BRAKE:
            color = CRGB::Red;
            brightness = 255;  // 100% brightness
            break;
            
        case REAR_BRAKE_EMERGENCY:
            color = blinkState ? CRGB::Red : CRGB::Black;
            brightness = 255;
            break;
            
        case REAR_REVERSE:
            color = CRGB::White;
            brightness = 255;
            break;
            
        case REAR_REGEN_ACTIVE:
            // Blue pulse effect
            brightness = 128 + (uint8_t)(127.0f * sin((animationStep % 100) * 3.14159f / 50.0f));
            color = CRGB::Blue;
            break;
    }
    
    // Apply to center LEDs (indices 3-12)
    for (int i = LED_REAR_CENTER_START; i <= LED_REAR_CENTER_END; i++) {
        rearLeds[i] = color;
        rearLeds[i].fadeToBlackBy(255 - brightness);
    }
}

// Update rear turn signals
static void updateTurnSignals() {
    bool leftActive = (currentTurnSignal == TURN_LEFT || currentTurnSignal == TURN_HAZARD);
    bool rightActive = (currentTurnSignal == TURN_RIGHT || currentTurnSignal == TURN_HAZARD);
    
    // Amber color for turn signals
    CRGB amber = CRGB(255, 100, 0);
    
    // Left turn signal (indices 0-2)
    if (leftActive) {
        // Sequential effect for reverse mode
        if (currentRearMode == REAR_REVERSE && blinkState) {
            int pos = (animationStep / 10) % 3;
            for (int i = 0; i < 3; i++) {
                rearLeds[LED_REAR_LEFT_START + i] = (i == pos) ? amber : CRGB::Black;
            }
        } else {
            fill_solid(&rearLeds[LED_REAR_LEFT_START], 3, blinkState ? amber : CRGB::Black);
        }
    } else {
        fill_solid(&rearLeds[LED_REAR_LEFT_START], 3, CRGB::Black);
    }
    
    // Right turn signal (indices 13-15)
    if (rightActive) {
        // Sequential effect for reverse mode
        if (currentRearMode == REAR_REVERSE && blinkState) {
            int pos = (animationStep / 10) % 3;
            for (int i = 0; i < 3; i++) {
                rearLeds[LED_REAR_RIGHT_START + i] = (i == pos) ? amber : CRGB::Black;
            }
        } else {
            fill_solid(&rearLeds[LED_REAR_RIGHT_START], 3, blinkState ? amber : CRGB::Black);
        }
    } else {
        fill_solid(&rearLeds[LED_REAR_RIGHT_START], 3, CRGB::Black);
    }
}

void init() {
    // Initialize FastLED
    FastLED.addLeds<WS2812B, LED_FRONT_PIN, GRB>(frontLeds, LED_FRONT_COUNT);
    FastLED.addLeds<WS2812B, LED_REAR_PIN, GRB>(rearLeds, LED_REAR_COUNT);
    
    // Set initial brightness
    FastLED.setBrightness(config.brightness);
    
    // Clear all LEDs
    fill_solid(frontLeds, LED_FRONT_COUNT, CRGB::Black);
    fill_solid(rearLeds, LED_REAR_COUNT, CRGB::Black);
    FastLED.show();
    
    lastUpdateMs = millis();
    
    Logger::info("LED Controller initialized");
    Logger::infof("Front: %d LEDs on GPIO %d", LED_FRONT_COUNT, LED_FRONT_PIN);
    Logger::infof("Rear: %d LEDs on GPIO %d", LED_REAR_COUNT, LED_REAR_PIN);
}

void update() {
    if (!enabled) return;
    
    uint32_t now = millis();
    
    // Update animation step
    if (now - lastUpdateMs >= config.updateRateMs) {
        animationStep++;
        lastUpdateMs = now;
        
        // Update blink state (500ms on/off for turn signals)
        if (now - lastBlinkMs >= 500) {
            blinkState = !blinkState;
            lastBlinkMs = now;
        }
        
        // Update LED patterns
        updateFrontLEDs();
        updateRearCenter();
        updateTurnSignals();
        
        // Show LEDs
        FastLED.show();
    }
}

void setFrontMode(FrontMode mode) {
    if (currentFrontMode != mode) {
        currentFrontMode = mode;
        animationStep = 0;
        scannerPos = 0;
        scannerDirection = 1;
    }
}

void setFrontFromThrottle(float throttlePercent) {
    if (throttlePercent < 1.0f) {
        setFrontMode(FRONT_KITT_IDLE);
    } else if (throttlePercent < 25.0f) {
        setFrontMode(FRONT_ACCEL_LOW);
    } else if (throttlePercent < 50.0f) {
        setFrontMode(FRONT_ACCEL_MED);
    } else if (throttlePercent < 75.0f) {
        setFrontMode(FRONT_ACCEL_HIGH);
    } else {
        setFrontMode(FRONT_ACCEL_MAX);
    }
}

void setRearMode(RearMode mode) {
    currentRearMode = mode;
}

void setTurnSignal(TurnSignal signal) {
    currentTurnSignal = signal;
}

void setBrightness(uint8_t brightness) {
    config.brightness = brightness;
    FastLED.setBrightness(brightness);
}

void setEnabled(bool en) {
    enabled = en;
    if (!enabled) {
        fill_solid(frontLeds, LED_FRONT_COUNT, CRGB::Black);
        fill_solid(rearLeds, LED_REAR_COUNT, CRGB::Black);
        FastLED.show();
    }
}

Config& getConfig() {
    return config;
}

void emergencyFlash(uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        fill_solid(frontLeds, LED_FRONT_COUNT, CRGB::Red);
        fill_solid(rearLeds, LED_REAR_COUNT, CRGB::Red);
        FastLED.show();
        delay(100);
        
        fill_solid(frontLeds, LED_FRONT_COUNT, CRGB::Black);
        fill_solid(rearLeds, LED_REAR_COUNT, CRGB::Black);
        FastLED.show();
        delay(100);
    }
}

}  // namespace LEDController
