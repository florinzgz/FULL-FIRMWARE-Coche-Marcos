#include "hud.h"
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>   // Librería táctil

#include "gauges.h"
#include "wheels_display.h"
#include "icons.h"
#include "menu_hidden.h"
#include "touch_map.h"         // 👈 añadido
#include "display_types.h"     // For GearPosition enum
#include "sensors.h"           // Para SystemStatus de sensores

#include "pedal.h"
#include "steering.h"
#include "buttons.h"
#include "shifter.h"
#include "current.h"
#include "temperature.h"
#include "wheels.h"
#include "traction.h"
#include "system.h"
#include "logger.h"
#include "settings.h"
#include "storage.h"
#include "dfplayer.h"
#include "alerts.h"
#include "pins.h"

static TFT_eSPI tft;
static XPT2046_Touchscreen touch(PIN_TOUCH_CS, PIN_TOUCH_IRQ);

// Touch calibration constants (matching menu_hidden.cpp)
static const int TOUCH_MIN_RAW = 200;   // Minimum raw touch value
static const int TOUCH_MAX_RAW = 3900;  // Maximum raw touch value

// Demo mode button detection for STANDALONE_DISPLAY
#ifdef STANDALONE_DISPLAY
static uint32_t demoButtonPressStart = 0;
static bool demoButtonWasPressed = false;
static const uint32_t DEMO_BUTTON_LONG_PRESS_MS = 1500;  // 1.5 seconds for long press

// Demo button position (bottom right corner, visible area)
static const int DEMO_BTN_X1 = 400;
static const int DEMO_BTN_Y1 = 260;
static const int DEMO_BTN_X2 = 475;
static const int DEMO_BTN_Y2 = 295;
#endif

// Layout 480x320 (rotación 3 → 480x320 landscape)
// Note: Rotation 3 used for ST7796S 4-inch display in horizontal orientation
static const int X_SPEED = 70;   // Left gauge - moved left for car visualization
static const int Y_SPEED = 175;
static const int X_RPM   = 410;  // Right gauge - moved right for car visualization
static const int Y_RPM   = 175;

// Zonas ruedas - centered between gauges
// Car body center: X=240, Y=175
static const int X_FL = 195;     // Front left wheel - inner from center
static const int Y_FL = 115;     // Front wheels - top
static const int X_FR = 285;     // Front right wheel - inner from center
static const int Y_FR = 115;
static const int X_RL = 195;     // Rear left wheel
static const int Y_RL = 235;     // Rear wheels - bottom
static const int X_RR = 285;     // Rear right wheel
static const int Y_RR = 235;

// Car body dimensions (centered at 240, 175)
static const int CAR_BODY_X = 175;      // Car body left edge
static const int CAR_BODY_Y = 100;      // Car body top edge
static const int CAR_BODY_W = 130;      // Car body width
static const int CAR_BODY_H = 150;      // Car body height

// Car body detail dimensions (used in drawCarBody)
static const int CAR_HOOD_OFFSET = 15;           // Hood/trunk X offset from body edge
static const int CAR_HOOD_WIDTH_REDUCTION = 30;  // Hood/trunk width reduction (2x offset)
static const int CAR_HOOD_HEIGHT = 20;           // Hood and trunk area height
static const int CAR_CENTER_LINE_MARGIN = 25;    // Margin from body edge for center line

// Wheel placeholder radius (used in init)
static const int WHEEL_PLACEHOLDER_RADIUS = 10;

static bool carBodyDrawn = false;       // Track if car body needs redraw

extern Storage::Config cfg;   // acceso a flags

void HUD::init() {
    // NOTE: tft.init() and tft.setRotation(3) are already called by HUDManager::init()
    // Do NOT call tft.init() again - it causes display issues
    
    // 🔒 REFACTORED: Non-blocking visual test using millis() instead of delay()
    // Test visual: verify SPI communication works
    // Display is 480x320 in landscape mode (rotation 3 for ST7796S)
    static const uint16_t TEST_COLORS[] = {TFT_RED, TFT_GREEN, TFT_BLUE, TFT_BLACK};
    static const uint32_t COLOR_DURATION_MS = 150;  // Reduced from 500ms for faster init
    
    uint32_t startMs = millis();
    for (int i = 0; i < 4; i++) {
        tft.fillScreen(TEST_COLORS[i]);
        // Non-blocking wait using millis()
        uint32_t colorStart = millis();
        while (millis() - colorStart < COLOR_DURATION_MS) {
            yield();  // Allow background tasks to run
        }
    }
    
    // Draw test text to confirm rendering (centered for 480x320)
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("ST7796S OK", 240, 160, 4);  // Center of 480x320
    
    // Non-blocking wait for text display (reduced from 1000ms)
    uint32_t textStart = millis();
    while (millis() - textStart < 300) {
        yield();  // Allow background tasks to run
    }
    
    // Clear screen and prepare for dashboard
    tft.fillScreen(TFT_BLACK);
    
    // Log total init time for performance monitoring
    uint32_t initDuration = millis() - startMs;
    Logger::infof("HUD visual test completed in %ums", (unsigned int)initDuration);

    // Initialize dashboard components
    // CRITICAL: These must be called after tft is initialized and rotation is set
    Gauges::init(&tft);
    WheelsDisplay::init(&tft);
    Icons::init(&tft);
    MenuHidden::init(&tft);  // MenuHidden stores tft pointer, must be non-null

    if (touch.begin()) {
        touch.setRotation(3);  // Match TFT rotation for proper touch mapping
        MenuHidden::initTouch(&touch);  // Pasar puntero táctil al menú oculto
        Logger::info("Touchscreen XPT2046 inicializado OK");
    } else {
        Logger::error("Touchscreen XPT2046 no detectado");
        System::logError(760); // código reservado: fallo táctil
    }

    Logger::info("HUD init OK - Display ST7796S ready");
    
    // Draw initial dashboard elements
    // Show title and status - car visualization will appear when update() is called
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    tft.drawString("Mercedes AMG GT", 240, 50, 4);
    
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("Esperando sensores...", 240, 80, 2);
    
    // Draw placeholder car outline to show display is working
    // Using named constants for consistency
    tft.drawRect(CAR_BODY_X, CAR_BODY_Y, CAR_BODY_W, CAR_BODY_H, TFT_DARKGREY);
    tft.drawCircle(X_FL, Y_FL, WHEEL_PLACEHOLDER_RADIUS, TFT_DARKGREY);
    tft.drawCircle(X_FR, Y_FR, WHEEL_PLACEHOLDER_RADIUS, TFT_DARKGREY);
    tft.drawCircle(X_RL, Y_RL, WHEEL_PLACEHOLDER_RADIUS, TFT_DARKGREY);
    tft.drawCircle(X_RR, Y_RR, WHEEL_PLACEHOLDER_RADIUS, TFT_DARKGREY);
    
    // Reset carBodyDrawn flag so it will be redrawn on first update
    carBodyDrawn = false;
}

void HUD::showLogo() {
    tft.fillScreen(TFT_BLACK);
    carBodyDrawn = false;  // Reset flag so car body will be redrawn after logo
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Mercedes AMG", 240, 120, 4);
    tft.drawString("Sistema de arranque", 240, 170, 2);
}

void HUD::showReady() {
    tft.fillScreen(TFT_BLACK);
    carBodyDrawn = false;  // Reset flag so car body will be redrawn
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("READY", 240, 40, 4);
}

void HUD::showError() {
    tft.fillScreen(TFT_BLACK);
    carBodyDrawn = false;  // Reset flag so car body will be redrawn
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("ERROR", 240, 40, 4);
}

#ifdef STANDALONE_DISPLAY
// Draw demo mode button for easy hidden menu access
static void drawDemoButton() {
    // Only draw if hidden menu is not active
    if (MenuHidden::isActive()) return;
    
    // Draw button background
    tft.fillRoundRect(DEMO_BTN_X1, DEMO_BTN_Y1, 
                      DEMO_BTN_X2 - DEMO_BTN_X1, DEMO_BTN_Y2 - DEMO_BTN_Y1, 
                      5, TFT_NAVY);
    tft.drawRoundRect(DEMO_BTN_X1, DEMO_BTN_Y1, 
                      DEMO_BTN_X2 - DEMO_BTN_X1, DEMO_BTN_Y2 - DEMO_BTN_Y1, 
                      5, TFT_CYAN);
    
    // Draw button text
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_CYAN, TFT_NAVY);
    int centerX = (DEMO_BTN_X1 + DEMO_BTN_X2) / 2;
    int centerY = (DEMO_BTN_Y1 + DEMO_BTN_Y2) / 2;
    tft.drawString("DEMO", centerX, centerY - 6, 2);
    tft.setTextColor(TFT_WHITE, TFT_NAVY);
    tft.drawString("MENU", centerX, centerY + 8, 1);
}

// Check if touch is in demo button area
static bool isTouchInDemoButton(int x, int y) {
    return (x >= DEMO_BTN_X1 && x <= DEMO_BTN_X2 && 
            y >= DEMO_BTN_Y1 && y <= DEMO_BTN_Y2);
}
#endif

// Draw car body outline connecting the four wheels
// This creates a visual representation of the vehicle in the center
static void drawCarBody() {
    // Only draw once (static background)
    if (carBodyDrawn) return;
    
    // Draw car body (top-down view)
    uint16_t bodyColor = TFT_DARKGREY;
    
    // Car body outline
    tft.drawRect(CAR_BODY_X, CAR_BODY_Y, CAR_BODY_W, CAR_BODY_H, bodyColor);
    
    // Front hood area (narrower) - using named constants
    int hoodX = CAR_BODY_X + CAR_HOOD_OFFSET;
    int hoodW = CAR_BODY_W - CAR_HOOD_WIDTH_REDUCTION;
    tft.drawRect(hoodX, CAR_BODY_Y, hoodW, CAR_HOOD_HEIGHT, bodyColor);
    
    // Rear trunk area (narrower) - using named constants
    tft.drawRect(hoodX, CAR_BODY_Y + CAR_BODY_H - CAR_HOOD_HEIGHT, hoodW, CAR_HOOD_HEIGHT, bodyColor);
    
    // Center line (car axis) - using named constants for margins
    int centerX = CAR_BODY_X + CAR_BODY_W / 2;
    int lineTop = CAR_BODY_Y + CAR_CENTER_LINE_MARGIN;
    int lineBottom = CAR_BODY_Y + CAR_BODY_H - CAR_CENTER_LINE_MARGIN;
    tft.drawLine(centerX, lineTop, centerX, lineBottom, TFT_DARKGREY);
    
    // Wheel connection lines (axles)
    tft.drawLine(X_FL, Y_FL, X_FR, Y_FR, TFT_DARKGREY);  // Front axle
    tft.drawLine(X_RL, Y_RL, X_RR, Y_RR, TFT_DARKGREY);  // Rear axle
    
    // Side rails (connecting front and rear wheels)
    tft.drawLine(X_FL, Y_FL, X_RL, Y_RL, TFT_DARKGREY);
    tft.drawLine(X_FR, Y_FR, X_RR, Y_RR, TFT_DARKGREY);
    
    carBodyDrawn = true;
}

void HUD::drawPedalBar(float pedalPercent) {
    const int y = 300;       // Posición vertical
    const int height = 18;   // Altura de la barra
    const int width = 480;   // Ancho total de pantalla

    if (pedalPercent < 0.0f) {
        // Pedal inválido → barra vacía con "--"
        tft.fillRect(0, y, width, height, TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        tft.drawString("-- PEDAL --", 240, y + height/2, 2);
        return;
    }

    // Mapear porcentaje a ancho de barra (0-100% → 0-480px)
    int barWidth = map((int)pedalPercent, 0, 100, 0, width);
    if (barWidth > width) barWidth = width;

    // Color: verde normal, amarillo >80%, rojo >95%
    uint16_t color;
    if (pedalPercent > 95.0f) {
        color = TFT_RED;
    } else if (pedalPercent > 80.0f) {
        color = TFT_YELLOW;
    } else {
        color = TFT_GREEN;
    }

    // Dibujar barra de progreso
    tft.fillRect(0, y, barWidth, height, color);

    // Limpiar resto de la barra
    if (barWidth < width) {
        tft.fillRect(barWidth, y, width - barWidth, height, TFT_DARKGREY);
    }

    // Texto con porcentaje en el centro
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, color);
    char txt[16];
    snprintf(txt, sizeof(txt), "%d%%", (int)pedalPercent);
    tft.drawString(txt, 240, y + height/2, 2);
}

void HUD::update() {
#ifdef STANDALONE_DISPLAY
    // STANDALONE MODE: Use static simulated sensor values
    // NOTE: Animation is handled by main.cpp loop() which updates HUDManager with CarData
    // This block provides fallback values when HUD::update() is called directly
    
    float speedKmh = 12.0f;          // Static fallback speed
    float rpm = 850.0f;              // Static fallback RPM
    float pedalPercent = 50.0f;      // Static fallback pedal position
    float steerAngleFL = 0.0f;       // Static fallback steering
    float steerAngleFR = 0.0f;
    Shifter::Gear gear = Shifter::Gear::P;
    System::State sys = System::State::READY;
    
    // Static wheel temperatures
    float wheelTempFL = 42.0f;
    float wheelTempFR = 42.0f;
    float wheelTempRL = 40.0f;
    float wheelTempRR = 40.0f;
    
    // Static motor effort
    float wheelEffortFL = 30.0f;
    float wheelEffortFR = 30.0f;
    float wheelEffortRL = 28.0f;
    float wheelEffortRR = 28.0f;
    
    // Simulated button state (needed for MenuHidden)
    Buttons::State btns;
    btns.batteryIcon = false;
    btns.lights = false;
    btns.multimedia = false;
    btns.mode4x4 = false;
    
    // Static feature states
    bool lights = false;
    bool multimedia = false;
    bool mode4x4 = true;
    bool eco = false;
    
    // Simulated sensor status for standalone mode (all OK)
    uint8_t sensorCurrentOK = 6;
    uint8_t sensorTempOK = 5;
    uint8_t sensorWheelOK = 4;
    bool tempWarning = false;
    float maxTemp = 42.0f;
    
#else
    auto pedal = Pedal::get();
    auto steer = Steering::get();
    auto sh    = Shifter::get();
    auto sys   = System::getState();
    auto tr    = Traction::get();
    auto btns  = Buttons::get();

    // Velocidad global = media ruedas delanteras
    float vFL = cfg.wheelSensorsEnabled ? Sensors::getWheelSpeed(0) : 0.0f;
    float vFR = cfg.wheelSensorsEnabled ? Sensors::getWheelSpeed(1) : 0.0f;
    float speedKmh = (vFL + vFR) * 0.5f;
    if(speedKmh > MAX_SPEED_KMH) speedKmh = MAX_SPEED_KMH;

    // Tacómetro proporcional a velocidad (PLACEHOLDER)
    // TODO: Replace with actual RPM from motor encoder/controller when sensors connected
    // Current implementation: RPM = (speed/maxSpeed) * maxRPM for visualization only
    float rpm = (speedKmh / MAX_SPEED_KMH) * MAX_RPM;
    if(rpm > MAX_RPM) rpm = MAX_RPM;
    
    float pedalPercent = pedal.valid ? pedal.percent : -1.0f;
    float steerAngleFL = cfg.steeringEnabled ? steer.angleFL : 0.0f;
    float steerAngleFR = cfg.steeringEnabled ? steer.angleFR : 0.0f;
    float wheelTempFL = cfg.tempSensorsEnabled ? tr.w[Traction::FL].tempC : -999.0f;
    float wheelTempFR = cfg.tempSensorsEnabled ? tr.w[Traction::FR].tempC : -999.0f;
    float wheelTempRL = cfg.tempSensorsEnabled ? tr.w[Traction::RL].tempC : -999.0f;
    float wheelTempRR = cfg.tempSensorsEnabled ? tr.w[Traction::RR].tempC : -999.0f;
    float wheelEffortFL = cfg.currentSensorsEnabled ? tr.w[Traction::FL].effortPct : -1.0f;
    float wheelEffortFR = cfg.currentSensorsEnabled ? tr.w[Traction::FR].effortPct : -1.0f;
    float wheelEffortRL = cfg.currentSensorsEnabled ? tr.w[Traction::RL].effortPct : -1.0f;
    float wheelEffortRR = cfg.currentSensorsEnabled ? tr.w[Traction::RR].effortPct : -1.0f;
    Shifter::Gear gear = sh.gear;
    bool lights = cfg.lightsEnabled && btns.lights;
    bool multimedia = cfg.multimediaEnabled && btns.multimedia;
    bool mode4x4 = tr.enabled4x4;
    bool eco = pedal.percent > 0 && !btns.mode4x4;
    
    // Obtener estado de sensores para indicadores
    Sensors::SystemStatus sensorStatus = Sensors::getSystemStatus();
    uint8_t sensorCurrentOK = sensorStatus.currentSensorsOK;
    uint8_t sensorTempOK = sensorStatus.temperatureSensorsOK;
    uint8_t sensorWheelOK = sensorStatus.wheelSensorsOK;
    bool tempWarning = sensorStatus.temperatureWarning;
    float maxTemp = sensorStatus.maxTemperature;
#endif

    // Draw car body outline (once, static background)
    drawCarBody();

    // Render gauges (ya optimizados internamente)
    Gauges::drawSpeed(X_SPEED, Y_SPEED, speedKmh, MAX_SPEED_KMH, pedalPercent);
    Gauges::drawRPM(X_RPM, Y_RPM, rpm, MAX_RPM);

    // Ruedas (optimizado: solo redibuja si cambian ángulo/temp/esfuerzo)
    // Usar -999.0f para temp y -1.0f para effort cuando sensores deshabilitados
    WheelsDisplay::drawWheel(X_FL, Y_FL, steerAngleFL, wheelTempFL, wheelEffortFL);
    WheelsDisplay::drawWheel(X_FR, Y_FR, steerAngleFR, wheelTempFR, wheelEffortFR);
    WheelsDisplay::drawWheel(X_RL, Y_RL, 0.0f, wheelTempRL, wheelEffortRL);
    WheelsDisplay::drawWheel(X_RR, Y_RR, 0.0f, wheelTempRR, wheelEffortRR);

    // Iconos y estados
    Icons::drawSystemState(sys);
    Icons::drawGear(gear);
    Icons::drawFeatures(lights, multimedia, mode4x4, eco);
#ifdef STANDALONE_DISPLAY
    Icons::drawBattery(24.5f);  // Simulated battery voltage
#else
    Icons::drawBattery(cfg.currentSensorsEnabled ? Sensors::getVoltage(0) : 0.0f);
#endif

    // Icono de advertencia si hay errores almacenados
    Icons::drawErrorWarning();
    
    // Indicador de estado de sensores (LEDs I/T/W)
    Icons::drawSensorStatus(sensorCurrentOK, sensorTempOK, sensorWheelOK,
                           Sensors::NUM_CURRENTS, Sensors::NUM_TEMPS, Sensors::NUM_WHEELS);
    
    // Advertencia de temperatura crítica
    Icons::drawTempWarning(tempWarning, maxTemp);

    // Barra de pedal en la parte inferior
    drawPedalBar(pedalPercent);
    
#ifdef STANDALONE_DISPLAY
    // Draw demo button for easy hidden menu access
    drawDemoButton();
#endif

    // --- Lectura táctil usando touch_map ---
    bool batteryTouch = false;
#ifdef STANDALONE_DISPLAY
    bool demoButtonTouched = false;
    static bool hiddenMenuJustActivated = false;
#endif

    if (touch.touched()) {
        TS_Point p = touch.getPoint();
        // Map raw touch coordinates to screen coordinates using calibration constants
        int x = map(p.x, TOUCH_MIN_RAW, TOUCH_MAX_RAW, 0, 480);
        int y = map(p.y, TOUCH_MIN_RAW, TOUCH_MAX_RAW, 0, 320);

#ifdef STANDALONE_DISPLAY
        // Check demo button touch with long press detection
        // Reset long press timer if touch moves outside button area
        bool touchInButton = isTouchInDemoButton(x, y) && !MenuHidden::isActive();
        if (touchInButton) {
            uint32_t now = millis();
            if (!demoButtonWasPressed) {
                demoButtonPressStart = now;
                demoButtonWasPressed = true;
                Logger::info("Demo button touched - hold 1.5s for menu");
            } else if (now - demoButtonPressStart >= DEMO_BUTTON_LONG_PRESS_MS) {
                // Long press detected - activate hidden menu directly
                demoButtonTouched = true;
                hiddenMenuJustActivated = true;
                Logger::info("Demo button long press - activating hidden menu");
            }
        } else {
            // Reset if touch moved outside button area
            demoButtonWasPressed = false;
        }
#endif

        TouchAction act = getTouchedZone(x, y);
        switch (act) {
            case TouchAction::Battery:
                batteryTouch = true;
                break;
            case TouchAction::Lights:
                Logger::info("Toque en icono luces");
                break;
            case TouchAction::Multimedia:
                Logger::info("Toque en icono multimedia");
                break;
            case TouchAction::Mode4x4:
                Logger::info("Toque en icono 4x4");
                break;
            case TouchAction::Warning:
                Logger::info("Toque en icono warning");
                break;
            default:
                break;
        }
    } else {
#ifdef STANDALONE_DISPLAY
        demoButtonWasPressed = false;
#endif
    }

#ifdef STANDALONE_DISPLAY
    // In demo mode, activate hidden menu directly on demo button long press
    // This bypasses the code entry mechanism for easier testing
    if (demoButtonTouched && hiddenMenuJustActivated) {
        MenuHidden::activateDirectly();
        hiddenMenuJustActivated = false;
    } else {
        // Normal battery touch handling
        MenuHidden::update(btns.batteryIcon || batteryTouch);
    }
#else
    // Menú oculto: botón físico o toque en batería
    MenuHidden::update(btns.batteryIcon || batteryTouch);
#endif
}