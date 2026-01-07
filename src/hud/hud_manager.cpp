#include "hud_manager.h"
#include "hud.h"
#include "pins.h"
#include "logger.h"
#include "system.h"
#include "storage.h"
#include "settings.h"  // For DISPLAY_BRIGHTNESS_DEFAULT
#include "sensors.h"  // Para estado de sensores
#include "pedal.h"    // Para calibración del pedal
#include <Arduino.h>

// Variables estáticas
MenuType HUDManager::currentMenu = MenuType::NONE;
CarData HUDManager::carData = CarData{};
uint32_t HUDManager::lastUpdateMs = 0;
bool HUDManager::needsRedraw = true;
uint8_t HUDManager::brightness = 200;
bool HUDManager::hiddenMenuActive = false;
uint32_t HUDManager::longPressStartMs = 0;
uint8_t HUDManager::longPressButtonId = 0;

// 🔒 v2.5.0: Flag de inicialización
static bool initialized = false;

// ✅ ÚNICA instancia global de TFT_eSPI - compartida con HUD y otros módulos
// 🔒 v2.11.6: BOOTLOOP FIX - Removed () to use default constructor
// Explicit constructor call TFT_eSPI() was running complex initialization
// in global constructor (before main) which could crash on ESP32-S3 OPI mode
TFT_eSPI tft;

// ============================================================================
// Boot Screen Configuration
// ============================================================================
static constexpr uint16_t BOOT_SCREEN_BG_COLOR = TFT_BLUE;    // Background during boot
static constexpr uint16_t BOOT_SCREEN_TEXT_COLOR = TFT_WHITE; // Text during boot

// ============================================================================
// Gear names constant (shared between renderHardwareTest and renderHiddenMenu)
// Gear index mapping: 0=P (Park), 1=D2 (Drive 2), 2=D1 (Drive 1), 3=N (Neutral), 4=R (Reverse)
// ============================================================================
static constexpr const char* GEAR_NAMES[] = {"P", "D2", "D1", "N", "R"};
static constexpr uint8_t GEAR_COUNT = sizeof(GEAR_NAMES) / sizeof(GEAR_NAMES[0]);

void HUDManager::init() {
    // 🔒 v2.8.1: Hardware reset y backlight ahora se hacen en main.cpp setup()
    // para asegurar que el display tiene luz incluso si la inicialización falla.
    // Aquí solo verificamos que ya están configurados y procedemos con TFT init.
    
    Serial.println("[HUD] Starting HUDManager initialization...");
    
    // 🔒 v2.8.1: Asegurar que backlight está habilitado (ya configurado en main.cpp)
    // La configuración de OUTPUT/HIGH se realiza únicamente en main.cpp.
    
    // 🔒 CORRECCIÓN CRÍTICA: Validar inicialización TFT con protección ante fallos
    Serial.println("[HUD] Initializing TFT_eSPI...");
    
    // 🔒 v2.11.5: FAULT TOLERANCE - Proteger inicialización del display
    // Si el display falla, el coche debe poder seguir funcionando
    // NOTE: Catching general exception because TFT_eSPI may throw various types
    // We want to ensure vehicle operation continues regardless of display failure
    try {
        tft.init();
        // Set initialized flag immediately after successful tft.init()
        // This ensures the flag reflects TFT initialization state accurately
        initialized = true;
    } catch (const std::exception& e) {
        Logger::errorf("HUD: TFT init exception: %s - continuing in degraded mode", e.what());
        System::logError(602);
        initialized = false;
        Serial.printf("[HUD] CRITICAL: Display init failed: %s, vehicle will operate without UI\n", e.what());
        return;  // Salir sin bloquear el sistema
    } catch (...) {
        Logger::error("HUD: TFT init unknown exception - continuing in degraded mode");
        System::logError(602);
        initialized = false;
        Serial.println("[HUD] CRITICAL: Display init failed, vehicle will operate without UI");
        return;  // Salir sin bloquear el sistema
    }
    
    // 🔒 v2.8.2: CRITICAL FIX - Set rotation IMMEDIATELY after tft.init()
    // Before v2.8.2, the boot screen was displayed before rotation was set,
    // causing the screen to appear vertically inverted (half blue/half white).
    // Rotation 3 provides landscape mode (480x320) for ST7796S display.
    tft.setRotation(3);  // Landscape mode: 480x320
    
    // 🔒 v2.8.3: Eliminada pantalla azul de boot - directo a dashboard
    // El boot screen anterior era innecesario y causaba parpadeo visual
    tft.fillScreen(TFT_BLACK);
    Serial.println("[HUD] Display cleared, skipping boot screen");
    
    // 🔒 v2.8.2: REMOVED early return - continue initialization even if dimensions seem wrong
    // The early return was blocking full HUD initialization, causing the display to stay stuck
    // on the boot screen without ever showing the dashboard with car data.
    if (tft.width() == 0 || tft.height() == 0) {
        Logger::error("HUD: TFT init reported 0 dimensions - continuing anyway");
        System::logError(600);
        Serial.println("[HUD] WARNING: TFT dimensions are 0, but continuing initialization...");
        // DO NOT return here - let the initialization continue
    }
    
    // 🔒 CORRECCIÓN CRÍTICA: Verificar dimensiones correctas
    int w = tft.width();
    int h = tft.height();
    Serial.printf("[HUD] Display dimensions: %dx%d\n", w, h);
    
    if (w != 480 || h != 320) {
        Logger::warnf("HUD: Dimensiones inesperadas %dx%d (esperado 480x320)", w, h);
        System::logError(601);
    } else {
        Logger::infof("HUD: Display inicializado correctamente %dx%d", w, h);
    }
    
    // Force complete screen clear to initialize all pixels
    tft.fillScreen(TFT_BLACK);
    // 🔒 v2.4.2: Eliminado delay(50) - fillScreen es síncrono, no requiere espera
    
    // 🔒 CRITICAL FIX v2.9.9: Enhanced brightness loading with validation
    // Print diagnostic info to help debug display issues
    Serial.printf("[HUD] Config brightness value: %d\n", cfg.displayBrightness);
    
    // Validate and load brightness from configuration
    if (cfg.displayBrightness > 0 && cfg.displayBrightness <= 255) {
        brightness = cfg.displayBrightness;
        Serial.printf("[HUD] Using config brightness: %d\n", brightness);
        Logger::infof("HUD: Brightness cargado de config: %d", brightness);
    } else {
        // 🔒 CRITICAL FIX: Ensure brightness is never 0 (would turn off screen)
        brightness = DISPLAY_BRIGHTNESS_DEFAULT;  // Use safe default if config is invalid
        Serial.printf("[HUD] Config brightness invalid (%d), using default: %d\n", cfg.displayBrightness, DISPLAY_BRIGHTNESS_DEFAULT);
        Logger::warnf("HUD: Valor de brightness inválido (%d), usando predeterminado: %d", cfg.displayBrightness, DISPLAY_BRIGHTNESS_DEFAULT);
    }
    
    // 🔒 CRITICAL: Double-check brightness is valid before setting PWM
    // This is a failsafe in case of memory corruption or race conditions
    if (brightness == 0) {
        Serial.println("[HUD] CRITICAL: Brightness is 0! Forcing to default value.");
        brightness = DISPLAY_BRIGHTNESS_DEFAULT;
    }
    
    Serial.printf("[HUD] Final brightness value: %d (validated)\n", brightness);
    
    // 🔒 v2.8.1: Configurar backlight PWM para control de brillo
    // Usamos LEDC PWM en lugar de digital GPIO para permitir dimming
    // Esto sobrescribe la configuración digital anterior con PWM
    // CRITICAL: Set up PWM and write brightness value to prevent any flicker
    ledcSetup(0, 5000, 8);  // Canal 0, 5kHz, 8-bit resolution
    ledcAttachPin(PIN_TFT_BL, 0);
    // Write brightness value twice to ensure it's definitely applied
    // This prevents any race conditions or timing issues that could cause the backlight to turn off
    ledcWrite(0, brightness);
    delayMicroseconds(100);  // Brief delay to ensure first write completes
    ledcWrite(0, brightness);  // Write again to be absolutely certain
    Serial.printf("[HUD] Backlight PWM configured, brightness: %d\n", brightness);
    
    // 🔒 v2.9.9: Brief delay to ensure PWM signal stabilizes
    // Without this, there may be a momentary flicker or the backlight may not turn on
    delay(10);
    Serial.println("[HUD] Backlight PWM stabilized");
    
    // Inicializar HUD básico (will show color test and initialize components)
    // Display is now ready with rotation=3 (480x320 landscape, ST7796S)
    Serial.println("[HUD] Initializing HUD components...");
    HUD::init();
    
    // Inicializar datos
    memset(&carData, 0, sizeof(CarData));
    carData.gear = GearPosition::PARK;
    
    needsRedraw = true;
    currentMenu = MenuType::NONE;
    
    // Note: initialized flag already set to true after successful tft.init() (line 54)
    Logger::info("HUDManager: Inicialización completada");
    Serial.println("[HUD] HUDManager initialization complete!");
}

void HUDManager::update() {
    // 🔒 v2.11.5: FAULT TOLERANCE - Si display no inicializó, salir silenciosamente
    if (!initialized) {
        return;  // No bloquear el sistema si el display falló
    }
    
    // 🔒 CORRECCIÓN: Control de frame rate con constante
    // 🔒 v2.8.4: No saltar el primer frame para permitir el primer dibujo
    static constexpr uint32_t FRAME_INTERVAL_MS = 33;  // 30 FPS
    uint32_t now = millis();
    if (lastUpdateMs != 0 && (now - lastUpdateMs) < FRAME_INTERVAL_MS) {
        return;  // Saltar frame para mantener 30 FPS (excepto el primero)
    }
    lastUpdateMs = now;
    
    // 🔒 v2.8.4: Diagnóstico visual - confirmar que el bucle de render se ejecuta
#ifdef DEBUG_RENDER
    tft.drawPixel(0, 0, TFT_WHITE);
    tft.drawRect(2, 2, 10, 6, TFT_YELLOW);
#endif
    
    // 🔒 v2.11.5: FLICKER FIX - Solo limpiar pantalla una vez al cambiar de menú
    // El flag needsRedraw se maneja dentro de cada función de renderizado
    // para evitar borrados innecesarios que causan parpadeo
    
    switch (currentMenu) {
        case MenuType::DASHBOARD:
            renderDashboard();
            break;
        case MenuType::SETTINGS:
            renderSettings();
            break;
        case MenuType::CALIBRATION:
            renderCalibration();
            break;
        case MenuType::HARDWARE_TEST:
            renderHardwareTest();
            break;
        case MenuType::WIFI_CONFIG:
            renderWifiConfig();
            break;
        case MenuType::INA226_MONITOR:
            renderINA226Monitor();
            break;
        case MenuType::STATISTICS:
            renderStatistics();
            break;
        case MenuType::QUICK_MENU:
            renderQuickMenu();
            break;
        case MenuType::HIDDEN_MENU:
            renderHiddenMenu();
            break;
        default:
            // Sin menú activo - usar HUD básico
            HUD::update();
            break;
    }
}

void HUDManager::updateCarData(const CarData& data) {
    carData = data;
}

void HUDManager::showMenu(MenuType menu) {
    if (currentMenu != menu) {
        currentMenu = menu;
        needsRedraw = true;  // 🔒 v2.11.5: Asegurar limpieza de pantalla al cambiar menú
        
        // 🔒 v2.11.5: Al entrar en el menú oculto, needsRedraw=true provocará que
        // renderHiddenMenu() resetee firstDraw=true, lo cual asegura limpieza completa
        // de la pantalla y evita solapamiento de imágenes del menú anterior
        if (menu == MenuType::HIDDEN_MENU) {
            Logger::info("HUD: Entering hidden menu; needsRedraw will trigger screen clear");
        }
    }
}

MenuType HUDManager::getCurrentMenu() {
    return currentMenu;
}

void HUDManager::forceRedraw() {
    needsRedraw = true;
}

void HUDManager::showLogo() {
    currentMenu = MenuType::NONE;
    HUD::showLogo();
    // After logo, switch to dashboard and force redraw
    currentMenu = MenuType::DASHBOARD;
    needsRedraw = true;  // Force screen clear before drawing dashboard
}

void HUDManager::showReady() {
    currentMenu = MenuType::NONE;
    HUD::showReady();
}

void HUDManager::showError(const char* message) {
    currentMenu = MenuType::NONE;
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(20, 150);
    tft.print("ERROR: ");
    tft.println(message);
}

void HUDManager::handleTouch([[maybe_unused]] int16_t x, 
                            [[maybe_unused]] int16_t y, 
                            [[maybe_unused]] bool pressed) {
    // 🔒 v2.11.5: FAULT TOLERANCE - Si display no inicializó, ignorar touch
    if (!initialized) {
        return;  // No procesar touch si el display falló
    }
    
    // 🔒 v2.10.3: Touch handling delegated to active menu/screen
    // Each screen (Dashboard, MenuHidden, MenuLEDControl, etc.) handles its own touch
    // This function serves as a placeholder for future global touch gestures
    // (e.g., swipe to change screens, pinch to zoom, etc.)
}

void HUDManager::setBrightness(uint8_t newBrightness) {
    // 🔒 v2.11.5: FAULT TOLERANCE - Si display no inicializó, solo guardar valor
    brightness = newBrightness;
    
    if (initialized) {
        ledcWrite(0, brightness);
    } else {
        // Display no disponible, pero guardamos el valor para cuando se recupere
        Logger::warnf("HUD: Display not available, brightness saved: %d", brightness);
    }
}

void HUDManager::activateHiddenMenu(bool activate) {
    hiddenMenuActive = activate;
    if (activate) {
        currentMenu = MenuType::HIDDEN_MENU;
    } else {
        currentMenu = MenuType::DASHBOARD;
    }
    needsRedraw = true;
}

bool HUDManager::isHiddenMenuActive() {
    return hiddenMenuActive;
}

void HUDManager::handleLongPress(uint8_t buttonId, uint32_t duration) {
    // Activar menú oculto con pulsación larga (> 3 segundos) en botón específico
    // Por ejemplo, botón de configuración o combinación de botones
    const uint32_t LONG_PRESS_DURATION = 3000;  // 3 segundos
    
    if (duration >= LONG_PRESS_DURATION) {
        // Activar/desactivar menú oculto
        activateHiddenMenu(!hiddenMenuActive);
    }
}

// 🔒 v2.5.0: Estado de inicialización
bool HUDManager::initOK() {
    return initialized;
}

// ============================================================================
// Helper Methods
// ============================================================================

void HUDManager::clearScreenIfNeeded() {
    if (needsRedraw) {
        tft.fillScreen(TFT_BLACK);
        needsRedraw = false;
    }
}

// ============================================================================
// Color calculation helpers - eliminates code duplication between
// renderHardwareTest() and renderHiddenMenu()
// ============================================================================

/**
 * @brief Calculate color for sensor status based on OK count vs total count
 * @param okCount Number of sensors working correctly
 * @param totalCount Total number of sensors
 * @return TFT_GREEN if all OK, TFT_RED if none OK, TFT_YELLOW if partial
 */
uint16_t HUDManager::getSensorStatusColor(uint8_t okCount, uint8_t totalCount) {
    if (okCount == totalCount) {
        return TFT_GREEN;
    } else if (okCount == 0) {
        return TFT_RED;
    } else {
        return TFT_YELLOW;
    }
}

/**
 * @brief Calculate color for pedal status
 * @param status Input device status structure
 * @return TFT_GREEN if OK and valid, TFT_YELLOW if OK but not valid, TFT_RED if not OK
 */
uint16_t HUDManager::getPedalColor(const Sensors::InputDeviceStatus& status) {
    if (status.pedalOK && status.pedalValid) {
        return TFT_GREEN;
    } else if (status.pedalOK) {
        return TFT_YELLOW;
    } else {
        return TFT_RED;
    }
}

/**
 * @brief Calculate color for steering status
 * @param status Input device status structure
 * @return TFT_GREEN if OK and centered, TFT_YELLOW if OK but not centered, TFT_RED if not OK
 */
uint16_t HUDManager::getSteeringColor(const Sensors::InputDeviceStatus& status) {
    if (status.steeringOK && status.steeringCentered) {
        return TFT_GREEN;
    } else if (status.steeringOK) {
        return TFT_YELLOW;
    } else {
        return TFT_RED;
    }
}

/**
 * @brief Calculate color for shifter status
 * @param status Input device status structure
 * @return TFT_GREEN if OK, TFT_RED if not OK
 */
uint16_t HUDManager::getShifterColor(const Sensors::InputDeviceStatus& status) {
    return status.shifterOK ? TFT_GREEN : TFT_RED;
}

/**
 * @brief Get gear name from gear index
 * @param gear Gear index (0 to GEAR_COUNT-1). Values >= GEAR_COUNT default to 0 (Park).
 * @return Gear name string (P, D2, D1, N, R). Out-of-range values return "P" as safe fallback.
 */
const char* HUDManager::getGearName(uint8_t gear) {
    // Default invalid or out-of-range gear indices to 0 ("P") as a safe fallback
    uint8_t gearIdx = gear < GEAR_COUNT ? gear : 0;
    return GEAR_NAMES[gearIdx];
}

// ===== Funciones de renderizado =====

void HUDManager::renderDashboard() {
    // 🔒 v2.11.5: OVERLAP FIX - Limpiar pantalla al entrar en dashboard desde otro menú
    clearScreenIfNeeded();
    
    // 🔒 v2.8.4: Diagnóstico visual - confirmar que renderDashboard se ejecuta
#ifdef DEBUG_RENDER
    tft.drawRect(5, 5, 20, 12, TFT_GREEN);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(7, 7);
    tft.print("DB");
#endif
    
    // Use the rich graphics dashboard from HUD::update()
    // This includes car visualization, wheels, gauges, icons, etc.
    HUD::update();
}

void HUDManager::renderSettings() {
    // 🔒 v2.11.5: OVERLAP FIX - Limpiar pantalla al entrar en settings
    clearScreenIfNeeded();
    
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(20, 20);
    tft.println("CONFIGURACION");
    
    tft.setTextSize(1);
    tft.setCursor(20, 60);
    tft.println("[ ] Ajustes de pantalla");
    tft.setCursor(20, 90);
    tft.println("[ ] Calibracion sensores");
    tft.setCursor(20, 120);
    tft.println("[ ] WiFi/OTA");
    tft.setCursor(20, 150);
    tft.println("[ ] Test hardware");
}

void HUDManager::renderCalibration() {
    // 🔒 v2.11.5: OVERLAP FIX - Limpiar pantalla al entrar en calibration
    clearScreenIfNeeded();
    
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(20, 20);
    tft.println("CALIBRACION");
    
    tft.setTextSize(1);
    tft.setCursor(20, 60);
    tft.print("Angulo volante: ");
    tft.print((int)carData.steeringAngle);
    tft.println(" grados");
    tft.setCursor(20, 90);
    tft.print("Pedal acelerador: ");
    tft.print((int)carData.throttlePercent);
    tft.println("%");
}

void HUDManager::renderHardwareTest() {
    // 🔒 v2.11.5: OVERLAP FIX - Limpiar pantalla al entrar en hardware test
    clearScreenIfNeeded();
    
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(20, 5);
    tft.println("TEST HARDWARE");
    
    // Obtener estado real de sensores y entradas
    Sensors::SystemStatus status = Sensors::getSystemStatus();
    Sensors::InputDeviceStatus inputStatus = Sensors::getInputDeviceStatus();
    
    tft.setTextSize(1);
    
    // ====== COLUMNA IZQUIERDA: SENSORES ======
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(10, 35);
    tft.print("-- SENSORES --");
    
    // INA226 sensores
    uint16_t colorINA = getSensorStatusColor(status.currentSensorsOK, status.currentSensorsTotal);
    tft.setTextColor(colorINA, TFT_BLACK);
    tft.setCursor(10, 50);
    tft.printf("INA226: %d/%d", status.currentSensorsOK, status.currentSensorsTotal);
    
    // DS18B20 sensores
    uint16_t colorTemp = getSensorStatusColor(status.temperatureSensorsOK, status.temperatureSensorsTotal);
    tft.setTextColor(colorTemp, TFT_BLACK);
    tft.setCursor(10, 65);
    tft.printf("DS18B20: %d/%d", status.temperatureSensorsOK, status.temperatureSensorsTotal);
    
    // Ruedas
    uint16_t colorWheel = getSensorStatusColor(status.wheelSensorsOK, status.wheelSensorsTotal);
    tft.setTextColor(colorWheel, TFT_BLACK);
    tft.setCursor(10, 80);
    tft.printf("Ruedas: %d/%d", status.wheelSensorsOK, status.wheelSensorsTotal);
    
    // Batería
    tft.setTextColor(status.batteryMonitorOK ? TFT_GREEN : TFT_RED, TFT_BLACK);
    tft.setCursor(10, 95);
    tft.printf("Bateria: %s", status.batteryMonitorOK ? "OK" : "FAIL");
    
    // Temperatura máxima
    if (status.temperatureWarning) {
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.setCursor(10, 115);
        tft.printf("TEMP CRIT: %.1fC", status.maxTemperature);
    } else if (status.maxTemperature != Sensors::INVALID_TEMPERATURE) {
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.setCursor(10, 115);
        tft.printf("Temp max: %.1fC", status.maxTemperature);
    } else {
        tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        tft.setCursor(10, 115);
        tft.print("Temp max: N/A");
    }
    
    // ====== COLUMNA DERECHA: ENTRADAS ======
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(250, 35);
    tft.print("-- ENTRADAS --");
    
    // Pedal
    uint16_t colorPedal = getPedalColor(inputStatus);
    tft.setTextColor(colorPedal, TFT_BLACK);
    tft.setCursor(250, 50);
    tft.printf("Pedal: %.1f%% [%d]", inputStatus.pedalPercent, inputStatus.pedalRaw);
    
    // Steering
    uint16_t colorSteer = getSteeringColor(inputStatus);
    tft.setTextColor(colorSteer, TFT_BLACK);
    tft.setCursor(250, 65);
    tft.printf("Volante: %.1f deg", inputStatus.steeringAngle);
    tft.setCursor(250, 80);
    tft.printf("Centrado: %s", inputStatus.steeringCentered ? "SI" : "NO");
    
    // Shifter
    uint16_t colorShift = getShifterColor(inputStatus);
    tft.setTextColor(colorShift, TFT_BLACK);
    tft.setCursor(250, 95);
    tft.printf("Marcha: %s", getGearName(inputStatus.shifterGear));
    
    // Botones
    tft.setTextColor(inputStatus.buttonsOK ? TFT_GREEN : TFT_RED, TFT_BLACK);
    tft.setCursor(250, 110);
    tft.printf("Botones: %s", inputStatus.buttonsOK ? "OK" : "FAIL");
    tft.setCursor(250, 125);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.printf("Estado: L:%s M:%s 4:%s", 
               inputStatus.lightsActive ? "ON" : "off",
               inputStatus.multimediaActive ? "ON" : "off",
               inputStatus.mode4x4Active ? "ON" : "off");
    
    // ====== ESTADO GENERAL ======
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(10, 150);
    tft.print("-- ESTADO GENERAL --");
    
    // Sensores
    uint16_t sensorColor = status.allSensorsHealthy ? TFT_GREEN : 
                           status.criticalSensorsOK ? TFT_YELLOW : TFT_RED;
    tft.setTextColor(sensorColor, TFT_BLACK);
    tft.setCursor(10, 165);
    tft.printf("Sensores: %s", status.allSensorsHealthy ? "TODOS OK" : 
                               status.criticalSensorsOK ? "PARCIAL" : "FALLO");
    
    // Entradas
    uint16_t inputColor = inputStatus.allInputsOK ? TFT_GREEN : TFT_RED;
    tft.setTextColor(inputColor, TFT_BLACK);
    tft.setCursor(10, 180);
    tft.printf("Entradas: %s", inputStatus.allInputsOK ? "TODOS OK" : "FALLO");
    
    // Estado del sistema general
    bool allOK = status.allSensorsHealthy && inputStatus.allInputsOK;
    uint16_t sysColor = allOK ? TFT_GREEN : 
                        (status.criticalSensorsOK && inputStatus.pedalOK) ? TFT_YELLOW : TFT_RED;
    tft.setTextColor(sysColor, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 200);
    tft.print(allOK ? "SISTEMA: OK" : 
              (status.criticalSensorsOK && inputStatus.pedalOK) ? "SISTEMA: WARN" : "SISTEMA: FAIL");
    
    // Calibración del pedal
    tft.setTextSize(1);
    int pedalMin, pedalMax;
    uint8_t curve;
    Pedal::getCalibration(pedalMin, pedalMax, curve);
    tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
    tft.setCursor(250, 165);
    tft.printf("Pedal cal: %d-%d", pedalMin, pedalMax);
    tft.setCursor(250, 180);
    tft.printf("Curva: %s", curve == 0 ? "Lineal" : curve == 1 ? "Suave" : "Agresiva");
}

void HUDManager::renderWifiConfig() {
    // 🔒 v2.11.5: OVERLAP FIX - Limpiar pantalla al entrar en wifi config
    clearScreenIfNeeded();
    
    tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(20, 20);
    tft.println("WiFi/OTA");
    
    tft.setTextSize(1);
    tft.setCursor(20, 60);
    tft.println("Estado: Desconectado");
    tft.setCursor(20, 90);
    tft.println("[ ] Conectar WiFi");
    tft.setCursor(20, 120);
    tft.println("[ ] Actualizar firmware");
}

void HUDManager::renderINA226Monitor() {
    // 🔒 v2.11.5: OVERLAP FIX - Limpiar pantalla al entrar en INA226 monitor
    clearScreenIfNeeded();
    
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(20, 20);
    tft.println("MONITOR INA226");
    
    tft.setTextSize(1);
    for (int i = 0; i < 4; i++) {
        tft.setCursor(20, 60 + i * 30);
        tft.printf("Motor %d: %5.1fA %5.1fV %5.0fW",
                   i + 1, 
                   carData.motorCurrent[i],
                   carData.batteryVoltage,  // Aproximación
                   carData.motorCurrent[i] * carData.batteryVoltage);
    }
    
    tft.setCursor(20, 220);
    tft.printf("Direccion: %5.1fA", carData.steeringCurrent);
    tft.setCursor(20, 250);
    tft.printf("Bateria: %5.1fA %5.1fV", carData.batteryCurrent, carData.batteryVoltage);
}

void HUDManager::renderStatistics() {
    // 🔒 v2.11.5: OVERLAP FIX - Limpiar pantalla al entrar en statistics
    clearScreenIfNeeded();
    
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(20, 20);
    tft.println("ESTADISTICAS");
    
    tft.setTextSize(1);
    tft.setCursor(20, 60);
    tft.println("En desarrollo...");
}

void HUDManager::renderQuickMenu() {
    // 🔒 v2.11.5: OVERLAP FIX - Limpiar pantalla al entrar en quick menu
    clearScreenIfNeeded();
    
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(20, 20);
    tft.println("MENU RAPIDO");
    
    tft.setTextSize(1);
    tft.setCursor(20, 60);
    tft.println("[ ] Dashboard");
    tft.setCursor(20, 90);
    tft.println("[ ] Configuracion");
    tft.setCursor(20, 120);
    tft.println("[ ] Monitor INA226");
    tft.setCursor(20, 150);
    tft.println("[ ] Estadisticas");
}

void HUDManager::renderHiddenMenu() {
    // 🔒 v2.11.5: ANTI-FLICKER - Cache de datos para evitar redibujo innecesario
    // Solo redibujar cuando los datos realmente cambien
    static CarData lastCarData = {};
    static Sensors::SystemStatus lastSensorStatus = {};
    static Sensors::InputDeviceStatus lastInputStatus = {};
    static bool firstDraw = true;
    static bool cacheValid = false;  // 🔒 Flag para validez del cache
    
    // Obtener datos actuales
    Sensors::SystemStatus sensorStatus = Sensors::getSystemStatus();
    Sensors::InputDeviceStatus inputStatus = Sensors::getInputDeviceStatus();
    
    // 🔒 v2.11.5: OVERLAP FIX - Limpiar pantalla COMPLETA solo en el primer dibujado
    // para eliminar cualquier resto de gauges/gráficos del dashboard
    // needsRedraw signals menu change - reset firstDraw to ensure screen clear on re-entry
    if (needsRedraw) {
        firstDraw = true;  // Reset for screen clearing on menu re-entry
        needsRedraw = false;
    }
    
    if (firstDraw) {
        tft.fillScreen(TFT_BLACK);
        firstDraw = false;
        cacheValid = false;  // Invalidar cache para forzar redibujado completo
    }
    
    // 🔒 v2.11.5: FLICKER FIX - Solo redibujar secciones que cambiaron
    // Comparar con datos anteriores para minimizar operaciones de display
    // NOTE: Using field-by-field comparison instead of memcmp to avoid issues with
    // struct padding bytes that may contain uninitialized data. This requires manual
    // maintenance if structs change, but ensures reliable change detection.
    // Only comparing critical fields that affect display - not all struct fields.
    bool dataChanged = !cacheValid ||
                       (carData.voltage != lastCarData.voltage) ||
                       (carData.current != lastCarData.current) ||
                       (carData.batteryPercent != lastCarData.batteryPercent) ||
                       (carData.speed != lastCarData.speed) ||
                       (carData.rpm != lastCarData.rpm);
    
    bool sensorChanged = !cacheValid ||
                         (sensorStatus.currentSensorsOK != lastSensorStatus.currentSensorsOK) ||
                         (sensorStatus.temperatureSensorsOK != lastSensorStatus.temperatureSensorsOK) ||
                         (sensorStatus.wheelSensorsOK != lastSensorStatus.wheelSensorsOK) ||
                         (sensorStatus.temperatureWarning != lastSensorStatus.temperatureWarning);
    
    bool inputChanged = !cacheValid ||
                        (inputStatus.pedalOK != lastInputStatus.pedalOK) ||
                        (inputStatus.steeringOK != lastInputStatus.steeringOK) ||
                        (inputStatus.shifterOK != lastInputStatus.shifterOK) ||
                        (inputStatus.allInputsOK != lastInputStatus.allInputsOK);
    
    // Si nada cambió, no redibujar (elimina el parpadeo a 30 FPS)
    if (!dataChanged && !sensorChanged && !inputChanged) {
        return;
    }
    
    // Actualizar cache
    lastCarData = carData;
    lastSensorStatus = sensorStatus;
    lastInputStatus = inputStatus;
    cacheValid = true;
    
    // Menú oculto con TODOS los datos de calibración y sensores
    // 🔒 v2.11.5: Dibujar sobre fondo negro sin borrar todo (reduce parpadeo)
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 5);
    tft.println("=== MENU OCULTO ===");
    
    tft.setTextSize(1);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    
    // Sección 1: Voltaje y Corriente (INA226)
    tft.setCursor(5, 30);
    tft.print("ENERGIA:");
    tft.setCursor(5, 45);
    tft.printf("Voltaje: %6.2fV  (%3.0f%%)", carData.voltage, carData.batteryPercent);
    tft.setCursor(5, 60);
    tft.printf("Corriente: %6.2fA", carData.current);
    tft.setCursor(5, 75);
    tft.printf("Potencia: %7.1fW", carData.batteryPower);
    
    // Sección 2: Corrientes motores (INA226 canales 0-3)
    tft.setCursor(5, 95);
    tft.print("MOTORES:");
    tft.setCursor(5, 110);
    tft.printf("FL:%5.1fA FR:%5.1fA", carData.motorCurrent[0], carData.motorCurrent[1]);
    tft.setCursor(5, 125);
    tft.printf("RL:%5.1fA RR:%5.1fA", carData.motorCurrent[2], carData.motorCurrent[3]);
    tft.setCursor(5, 140);
    tft.printf("Direccion: %5.1fA", carData.steeringCurrent);
    
    // Sección 3: Temperaturas (DS18B20)
    tft.setCursor(250, 30);
    tft.print("TEMPERATURAS:");
    tft.setCursor(250, 45);
    tft.printf("Motor Principal: %4.1fC", carData.temperature);
    tft.setCursor(250, 60);
    tft.printf("M1:%3.0f M2:%3.0f", carData.motorTemp[0], carData.motorTemp[1]);
    tft.setCursor(250, 75);
    tft.printf("M3:%3.0f M4:%3.0f", carData.motorTemp[2], carData.motorTemp[3]);
    tft.setCursor(250, 90);
    tft.printf("Ambiente: %4.1fC", carData.ambientTemp);
    tft.setCursor(250, 105);
    tft.printf("Controlador: %4.1fC", carData.controllerTemp);
    
    // Sección 4: Estado de dispositivos de entrada
    // inputStatus already declared at top of function (line 643)
    
    tft.setCursor(250, 125);
    tft.print("ENTRADAS:");
    
    // Estado del pedal con color
    uint16_t colorPedal = getPedalColor(inputStatus);
    tft.setTextColor(colorPedal, TFT_BLACK);
    tft.setCursor(250, 140);
    tft.printf("Pedal: %5.1f%% [%d]", inputStatus.pedalPercent, inputStatus.pedalRaw);
    
    // Estado del steering con color
    uint16_t colorSteer = getSteeringColor(inputStatus);
    tft.setTextColor(colorSteer, TFT_BLACK);
    tft.setCursor(250, 155);
    tft.printf("Volante: %5.1f deg", inputStatus.steeringAngle);
    
    // Estado del shifter con color
    uint16_t colorShift = getShifterColor(inputStatus);
    tft.setTextColor(colorShift, TFT_BLACK);
    tft.setCursor(250, 170);
    tft.printf("Marcha: %s", getGearName(inputStatus.shifterGear));
    
    // Sección 5: Velocidad y RPM
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(5, 160);
    tft.print("MOVIMIENTO:");
    tft.setCursor(5, 175);
    tft.printf("Velocidad: %5.1f km/h", carData.speed);
    tft.setCursor(5, 190);
    tft.printf("RPM: %6.0f", carData.rpm);
    
    // Sección 6: Estado de Sensores
    // sensorStatus already declared at top of function (line 642)
    tft.setCursor(5, 210);
    tft.print("SENSORES:");
    
    // INA226 (corriente)
    uint16_t colorINA = getSensorStatusColor(sensorStatus.currentSensorsOK, sensorStatus.currentSensorsTotal);
    tft.setTextColor(colorINA, TFT_BLACK);
    tft.setCursor(5, 225);
    tft.printf("INA226: %d/%d", sensorStatus.currentSensorsOK, sensorStatus.currentSensorsTotal);
    
    // DS18B20 (temperatura)
    uint16_t colorTemp = getSensorStatusColor(sensorStatus.temperatureSensorsOK, sensorStatus.temperatureSensorsTotal);
    tft.setTextColor(colorTemp, TFT_BLACK);
    tft.setCursor(80, 225);
    tft.printf("DS18B20: %d/%d", sensorStatus.temperatureSensorsOK, sensorStatus.temperatureSensorsTotal);
    
    // Ruedas
    uint16_t colorWheel = getSensorStatusColor(sensorStatus.wheelSensorsOK, sensorStatus.wheelSensorsTotal);
    tft.setTextColor(colorWheel, TFT_BLACK);
    tft.setCursor(175, 225);
    tft.printf("RUEDAS: %d/%d", sensorStatus.wheelSensorsOK, sensorStatus.wheelSensorsTotal);
    
    // Advertencia de temperatura
    if (sensorStatus.temperatureWarning) {
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.setCursor(5, 240);
        tft.printf("!! TEMP CRITICA: %.1fC !!", sensorStatus.maxTemperature);
    }
    
    // Sección 7: Estado general del sistema
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(250, 200);
    tft.print("SISTEMA:");
    
    // Estado de entradas
    uint16_t colorInputs = inputStatus.allInputsOK ? TFT_GREEN : TFT_RED;
    tft.setTextColor(colorInputs, TFT_BLACK);
    tft.setCursor(250, 215);
    tft.print(inputStatus.allInputsOK ? "Inputs: OK" : "Inputs: FAIL");
    
    // Botones activos
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setCursor(250, 230);
    tft.printf("BTN: %s%s%s", 
               inputStatus.lightsActive ? "L" : "-",
               inputStatus.multimediaActive ? "M" : "-",
               inputStatus.mode4x4Active ? "4" : "-");
    
    // Odómetros
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(250, 245);
    tft.printf("Odo: %.1f/%.1f", carData.odoTotal, carData.odoTrip);
    
    // Estado general del sistema
    bool allOK = sensorStatus.allSensorsHealthy && inputStatus.allInputsOK;
    uint16_t sysColor = allOK ? TFT_GREEN : 
                        (sensorStatus.criticalSensorsOK && inputStatus.pedalOK) ? TFT_YELLOW : TFT_RED;
    tft.setTextColor(sysColor, TFT_BLACK);
    tft.setCursor(250, 260);
    tft.print(allOK ? "SYS: OK" : 
              (sensorStatus.criticalSensorsOK && inputStatus.pedalOK) ? "SYS: WARN" : "SYS: FAIL");
    
    // Instrucciones salida
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setCursor(5, 300);
    tft.print("Pulse 3s bateria para salir");
}
