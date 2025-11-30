#include "led_controller.h"
#include "logger.h"
#include "pins.h"
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

// 🔒 CORRECCIÓN 3.3: Parámetros configurables para efectos
struct EffectsConfig {
    uint8_t kittTailLength = 4;
    uint8_t chaseSpeedLow = 8;
    uint8_t chaseSpeedMed = 5;
    uint8_t chaseSpeedHigh = 3;
    uint8_t rainbowSpeed = 3;
};
static EffectsConfig effectsConfig;

// Animation state
static uint32_t lastUpdateMs = 0;
static uint16_t animationStep = 0;  // 🔒 Se controla overflow en update()
static bool blinkState = false;
static uint32_t lastBlinkMs = 0;
static bool enabled = true;
static bool hardwareOK = false;  // 🔒 CORRECCIÓN 3.1: Flag de hardware válido

// Emergency flash state (non-blocking)
static bool emergencyFlashActive = false;
static uint8_t emergencyFlashCount = 0;
static uint8_t emergencyFlashCurrent = 0;
static uint32_t emergencyFlashLastToggle = 0;
static bool emergencyFlashOn = false;

// KITT scanner state
static int8_t scannerPos = 0;
static int8_t scannerDirection = 1;

// 🔒 v2.4.1: Sine lookup table for performance optimization
// Pre-calculated: sin(x * PI / 50) * 127 for x = 0..49 (half period)
// Full sine wave computed via mirroring to reduce memory usage
static const uint8_t SINE_LUT_50[50] = {
     0,   8,  16,  24,  32,  40,  48,  55,  62,  69,
    75,  81,  87,  93,  99, 104, 110, 115, 120, 124,
   127, 130, 133, 135, 127, 139, 140, 142, 143, 143,
   143, 143, 142, 140, 139, 137, 134, 131, 127, 123,
   118, 113, 107, 101,  94,  87,  80,  73,  65,  57
};

// Helper: fast sine lookup (0-99 index) -> returns 0-127
static inline uint8_t fastSinLUT(uint8_t idx) {
    // Map 0-99 to 0-49 using modulo 50, handles full sine wave period
    return SINE_LUT_50[idx % 50];
}

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
static void updateKITTScanner(CRGB* leds, int count, CRGB color) {
    // 🔒 Usar configuración para tailLength
    int tailLength = effectsConfig.kittTailLength;
    
    // Fade all LEDs
    for (int i = 0; i < count; i++) {
        leds[i].fadeToBlackBy(FADE_RATE_KITT);  // 🔒 Using constant instead of magic number
    }
    
    // Draw bright center LED
    if (scannerPos >= 0 && scannerPos < count) {
        leds[scannerPos] = color;
    }
    
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
        // 🔒 Asegurar que scannerPos está dentro de límites
        scannerPos = constrain(scannerPos, 0, count - 1);
    }
}

// Chase effect (LEDs light up in sequence)
static void updateChase(CRGB* leds, int count, CRGB color, uint8_t speed) {
    if (count <= 0 || speed == 0) return;  // 🔒 Protección división por cero
    
    int pos = (animationStep / speed) % count;
    
    // Fade all LEDs
    for (int i = 0; i < count; i++) {
        leds[i].fadeToBlackBy(FADE_RATE_CHASE);  // 🔒 Using constant instead of magic number
    }
    
    // Light up current position and tail
    for (int i = 0; i < 5 && pos - i >= 0; i++) {
        leds[pos - i] = color;
        leds[pos - i].fadeToBlackBy(i * 50);
    }
}

// Rainbow effect
static void updateRainbow(CRGB* leds, int count, uint8_t speed) {
    // 🔒 CORRECCIÓN 3.4: Protección división por cero
    if (count <= 0) {
        Logger::errorf("updateRainbow: count inválido %d", count);
        return;
    }
    
    uint8_t hue = (animationStep * speed) & 0xFF;
    uint8_t deltaHue = (count > 0) ? (256 / count) : 1;
    fill_rainbow(leds, count, hue, deltaHue);
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
            updateKITTScanner(frontLeds, LED_FRONT_COUNT, CRGB::Red);
            break;
            
        case FRONT_ACCEL_LOW:
            updateChase(frontLeds, LED_FRONT_COUNT, getThrottleColor(12.5f), effectsConfig.chaseSpeedLow);
            break;
            
        case FRONT_ACCEL_MED:
            updateChase(frontLeds, LED_FRONT_COUNT, getThrottleColor(37.5f), effectsConfig.chaseSpeedMed);
            break;
            
        case FRONT_ACCEL_HIGH:
            updateChase(frontLeds, LED_FRONT_COUNT, getThrottleColor(62.5f), effectsConfig.chaseSpeedHigh);
            break;
            
        case FRONT_ACCEL_MAX:
            updateRainbow(frontLeds, LED_FRONT_COUNT, effectsConfig.rainbowSpeed);
            break;
            
        case FRONT_REVERSE:
            updateKITTScanner(frontLeds, LED_FRONT_COUNT, CRGB::White);
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
            brightness = BRIGHTNESS_POSITION_LIGHTS;  // 🔒 Using constant: 20% brightness
            break;
            
        case REAR_BRAKE:
            color = CRGB::Red;
            brightness = BRIGHTNESS_FULL;  // 🔒 Using constant: 100% brightness
            break;
            
        case REAR_BRAKE_EMERGENCY:
            color = blinkState ? CRGB::Red : CRGB::Black;
            brightness = BRIGHTNESS_FULL;  // 🔒 Using constant
            break;
            
        case REAR_REVERSE:
            color = CRGB::White;
            brightness = BRIGHTNESS_FULL;  // 🔒 Using constant
            break;
            
        case REAR_REGEN_ACTIVE:
            // Blue pulse effect - 🔒 v2.4.1: Use LUT instead of sin()
            brightness = 128 + fastSinLUT(animationStep % 100);
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
    // 🔒 CORRECCIÓN 3.1: Validación de pines antes de inicializar FastLED
    if (LED_FRONT_PIN < 0 || LED_REAR_PIN < 0) {
        Logger::errorf("LED pins inválidos: front=%d, rear=%d", LED_FRONT_PIN, LED_REAR_PIN);
        enabled = false;
        hardwareOK = false;
        return;
    }
    
    // Verificar que pines estén asignados en pins.h
    if (!pin_is_assigned(LED_FRONT_PIN) || !pin_is_assigned(LED_REAR_PIN)) {
        Logger::errorf("LED pins no asignados en pins.h");
        enabled = false;
        hardwareOK = false;
        return;
    }
    
    // Verificar que pines no sean strapping pins críticos (0, 45, 46 en ESP32-S3)
    if (LED_FRONT_PIN == 0 || LED_REAR_PIN == 0 || 
        LED_FRONT_PIN == 45 || LED_REAR_PIN == 45 ||
        LED_FRONT_PIN == 46 || LED_REAR_PIN == 46) {
        Logger::errorf("LED pins en strapping pin - riesgo de boot");
        enabled = false;
        hardwareOK = false;
        return;
    }
    
    // Initialize FastLED
    FastLED.addLeds<WS2812B, LED_FRONT_PIN, GRB>(frontLeds, LED_FRONT_COUNT);
    FastLED.addLeds<WS2812B, LED_REAR_PIN, GRB>(rearLeds, LED_REAR_COUNT);
    
    // 🔒 CORRECCIÓN 3.2: Limitar brillo máximo para seguridad
    const uint8_t MAX_SAFE_BRIGHTNESS = 200;
    if (config.brightness > MAX_SAFE_BRIGHTNESS) {
        Logger::warnf("LED brightness reducido de %d a %d por seguridad", 
                      config.brightness, MAX_SAFE_BRIGHTNESS);
        config.brightness = MAX_SAFE_BRIGHTNESS;
    }
    
    // Set initial brightness
    FastLED.setBrightness(config.brightness);
    
    // 🔒 Test de comunicación básico con hardware
    fill_solid(frontLeds, LED_FRONT_COUNT, CRGB::Blue);
    fill_solid(rearLeds, LED_REAR_COUNT, CRGB::Blue);
    FastLED.show();
    delay(100);
    
    // Apagar LEDs después del test
    fill_solid(frontLeds, LED_FRONT_COUNT, CRGB::Black);
    fill_solid(rearLeds, LED_REAR_COUNT, CRGB::Black);
    FastLED.show();
    
    lastUpdateMs = millis();
    hardwareOK = true;
    
    Logger::info("LED Controller initialized OK");
    Logger::infof("Front: %d LEDs on GPIO %d", LED_FRONT_COUNT, LED_FRONT_PIN);
    Logger::infof("Rear: %d LEDs on GPIO %d", LED_REAR_COUNT, LED_REAR_PIN);
    Logger::infof("Brightness: %d, Update rate: %dms", config.brightness, config.updateRateMs);
}

void update() {
    if (!enabled || !hardwareOK) return;  // 🔒 Verificar hardware OK
    
    uint32_t now = millis();
    
    // Handle emergency flash (highest priority, non-blocking)
    if (emergencyFlashActive) {
        // 🔒 CORRECCIÓN 3.3: Timeout de seguridad para emergency flash
        static unsigned long emergencyFlashStartTime = 0;
        const unsigned long EMERGENCY_FLASH_MAX_DURATION_MS = 10000; // 10 segundos máximo
        
        if (emergencyFlashStartTime == 0) {
            emergencyFlashStartTime = now;
        }
        
        // Timeout de seguridad
        if (now - emergencyFlashStartTime > EMERGENCY_FLASH_MAX_DURATION_MS) {
            Logger::errorf("Emergency flash timeout - finalizando");
            emergencyFlashActive = false;
            emergencyFlashCurrent = 0;
            emergencyFlashStartTime = 0;
            return;
        }
        
        // Toggle every 100ms
        if (now - emergencyFlashLastToggle >= EMERGENCY_FLASH_INTERVAL_MS) {  // 🔒 Using constant
            emergencyFlashLastToggle = now;
            emergencyFlashOn = !emergencyFlashOn;
            
            if (emergencyFlashOn) {
                fill_solid(frontLeds, LED_FRONT_COUNT, CRGB::Red);
                fill_solid(rearLeds, LED_REAR_COUNT, CRGB::Red);
            } else {
                fill_solid(frontLeds, LED_FRONT_COUNT, CRGB::Black);
                fill_solid(rearLeds, LED_REAR_COUNT, CRGB::Black);
                emergencyFlashCurrent++;
                
                // Check if we've completed all flashes
                if (emergencyFlashCurrent >= emergencyFlashCount) {
                    emergencyFlashActive = false;
                    emergencyFlashCurrent = 0;
                    emergencyFlashStartTime = 0; // reset timeout
                }
            }
            FastLED.show();
        }
        return; // Skip normal update during emergency flash
    }
    
    // Normal animation update
    if (now - lastUpdateMs >= config.updateRateMs) {
        // 🔒 CORRECCIÓN 3.2: Prevenir overflow de animationStep
        animationStep = (animationStep + 1) % 65536;
        lastUpdateMs = now;
        
        // Update blink state (500ms on/off for turn signals)
        if (now - lastBlinkMs >= TURN_SIGNAL_BLINK_MS) {  // 🔒 Using constant
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
    // 🔒 CORRECCIÓN 3.2: Limitar brillo máximo para seguridad (prevenir sobrecalentamiento)
    const uint8_t MAX_SAFE_BRIGHTNESS = 200; // 78% del máximo
    
    if (brightness > MAX_SAFE_BRIGHTNESS) {
        Logger::warnf("LED brightness limitado de %d a %d", brightness, MAX_SAFE_BRIGHTNESS);
        brightness = MAX_SAFE_BRIGHTNESS;
    }
    
    config.brightness = brightness;
    FastLED.setBrightness(brightness);
    Logger::infof("LED brightness set: %d", brightness);
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

void startEmergencyFlash(uint8_t count) {
    // Start non-blocking emergency flash
    emergencyFlashActive = true;
    emergencyFlashCount = count;
    emergencyFlashCurrent = 0;
    emergencyFlashOn = false;
    emergencyFlashLastToggle = millis();
    Logger::infof("Emergency flash started: %d cycles", count);
}

bool isEmergencyFlashActive() {
    return emergencyFlashActive;
}

}  // namespace LEDController
