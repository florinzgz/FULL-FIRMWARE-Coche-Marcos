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

// Layout 480x320 (rotación 3 → 480x320 landscape)
// Note: Rotation 3 used for ST7796S 4-inch display in horizontal orientation
static const int X_SPEED = 120;
static const int Y_SPEED = 180;
static const int X_RPM   = 360;
static const int Y_RPM   = 180;

// Zonas ruedas
static const int X_FL = 120;
static const int Y_FL = 90;
static const int X_FR = 360;
static const int Y_FR = 90;
static const int X_RL = 120;
static const int Y_RL = 260;
static const int X_RR = 360;
static const int Y_RR = 260;

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
        // TODO: Implement dynamic touch calibration if default values don't align properly
        // Current hardcoded mapping may need adjustment for different ILI9488 units
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
    tft.drawString("Esperando sensores...", 240, 90, 2);
    
    // Draw placeholder car outline to show display is working
    tft.drawRect(180, 120, 120, 140, TFT_DARKGREY);  // Simple car box
    tft.drawCircle(210, 130, 10, TFT_DARKGREY);  // FL wheel placeholder
    tft.drawCircle(270, 130, 10, TFT_DARKGREY);  // FR wheel placeholder  
    tft.drawCircle(210, 250, 10, TFT_DARKGREY);  // RL wheel placeholder
    tft.drawCircle(270, 250, 10, TFT_DARKGREY);  // RR wheel placeholder
}

void HUD::showLogo() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Mercedes AMG", 240, 120, 4);
    tft.drawString("Sistema de arranque", 240, 170, 2);
}

void HUD::showReady() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("READY", 240, 40, 4);
}

void HUD::showError() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("ERROR", 240, 40, 4);
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
    // STANDALONE MODE: Use simulated sensor values for display testing
    // This allows the dashboard to be fully visible without hardware
    float speedKmh = 12.0f;  // 12 km/h simulated speed
    float rpm = 850.0f;      // 850 RPM simulated idle
    float pedalPercent = 50.0f;  // 50% pedal
    float steerAngleFL = 0.0f;
    float steerAngleFR = 0.0f;
    float wheelTempFL = 42.0f;  // Simulated temps
    float wheelTempFR = 42.0f;
    float wheelTempRL = 42.0f;
    float wheelTempRR = 42.0f;
    float wheelEffortFL = 30.0f;  // Simulated effort %
    float wheelEffortFR = 30.0f;
    float wheelEffortRL = 30.0f;
    float wheelEffortRR = 30.0f;
    
    // Simulated system state (System::State is an enum, so use READY)
    System::State sys = System::State::READY;
    
    // Simulated shifter gear (use Shifter::Gear for icons)
    Shifter::Gear gear = Shifter::Gear::P;
    
    // Simulated button state (needed for MenuHidden)
    Buttons::State btns;
    btns.batteryIcon = false;
    btns.lights = false;
    btns.multimedia = false;
    btns.mode4x4 = false;
    
    bool lights = false;
    bool multimedia = false;
    bool mode4x4 = true;
    bool eco = false;
    
    // Simulated sensor status for standalone mode
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

    // --- Lectura táctil usando touch_map ---
    bool batteryTouch = false;
    if (touch.touched()) {
        TS_Point p = touch.getPoint();
        int x = map(p.x, 200, 3900, 0, 480);
        int y = map(p.y, 200, 3900, 0, 320);

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
    }

    // Menú oculto: botón físico o toque en batería
    MenuHidden::update(btns.batteryIcon || batteryTouch);
}