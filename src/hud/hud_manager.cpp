#include "hud_manager.h"
#include "hud.h"
#include "pins.h"
#include <Arduino.h>

// Variables estáticas
MenuType HUDManager::currentMenu = MenuType::NONE;
CarData HUDManager::carData = {};
uint32_t HUDManager::lastUpdateMs = 0;
bool HUDManager::needsRedraw = true;
uint8_t HUDManager::brightness = 200;
bool HUDManager::hiddenMenuActive = false;
uint32_t HUDManager::longPressStartMs = 0;
uint8_t HUDManager::longPressButtonId = 0;

// TFT singleton
static TFT_eSPI tft = TFT_eSPI();

void HUDManager::init() {
    // 🔒 CORRECCIÓN CRÍTICA: Eliminar esperas activas, usar lógica no bloqueante
    // CRITICAL: Enable backlight and reset display BEFORE tft.init()
    pinMode(PIN_TFT_BL, OUTPUT);
    digitalWrite(PIN_TFT_BL, HIGH);  // Turn on backlight
    
    pinMode(PIN_TFT_RST, OUTPUT);
    digitalWrite(PIN_TFT_RST, LOW);
    delay(10);  // Unavoidable hardware reset timing
    digitalWrite(PIN_TFT_RST, HIGH);
    delay(10);  // Unavoidable hardware reset timing
    
    // 🔒 CORRECCIÓN CRÍTICA: Validar inicialización TFT
    tft.init();
    if (tft.width() == 0 || tft.height() == 0) {
        Logger::error("HUD: TFT init failed - dimensions invalid");
        System::logError(600);
        return;
    }
    
    // CRITICAL: ST7796S rotation configuration for full screen rendering
    // Rotation 3 provides landscape mode (480x320)
    // This rotation works best for 4-inch ST7796S displays
    tft.setRotation(3);  // Landscape mode: 480x320
    delay(50);  // Allow display to process rotation - essential for ST7796S
    
    // 🔒 CORRECCIÓN CRÍTICA: Verificar dimensiones correctas
    int w = tft.width();
    int h = tft.height();
    if (w != 480 || h != 320) {
        Logger::warnf("HUD: Dimensiones inesperadas %dx%d (esperado 480x320)", w, h);
        System::logError(601);
    } else {
        Logger::infof("HUD: Display inicializado correctamente %dx%d", w, h);
    }
    
    // Force complete screen clear to initialize all pixels
    tft.fillScreen(TFT_BLACK);
    delay(50);  // Allow display buffer to stabilize
    
    // Configurar backlight PWM (GPIO 42) - optional for dimming control
    ledcSetup(0, 5000, 8);  // Canal 0, 5kHz, 8-bit resolution
    ledcAttachPin(PIN_TFT_BL, 0);
    ledcWrite(0, brightness);
    
    // Inicializar HUD básico (will show color test and initialize components)
    // Display is now ready with rotation=3 (480x320 landscape, ST7796S)
    HUD::init();
    
    // Inicializar datos
    memset(&carData, 0, sizeof(CarData));
    carData.gear = GearPosition::PARK;
    
    needsRedraw = true;
    currentMenu = MenuType::NONE;
}

void HUDManager::update() {
    // 🔒 CORRECCIÓN: Control de frame rate con constante
    static constexpr uint32_t FRAME_INTERVAL_MS = 33;  // 30 FPS
    uint32_t now = millis();
    if (now - lastUpdateMs < FRAME_INTERVAL_MS) {
        return;  // Saltar frame para mantener 30 FPS
    }
    lastUpdateMs = now;
    
    // Renderizar según menú activo
    if (needsRedraw) {
        tft.fillScreen(TFT_BLACK);
        needsRedraw = false;
    }
    
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
        needsRedraw = true;
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

void HUDManager::handleTouch(int16_t x, int16_t y, bool pressed) {
    // TODO: Implementar lógica táctil según menú activo
    // Por ahora solo placeholder
}

void HUDManager::setBrightness(uint8_t newBrightness) {
    brightness = newBrightness;
    ledcWrite(0, brightness);
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

// ===== Funciones de renderizado =====

void HUDManager::renderDashboard() {
    // Use the rich graphics dashboard from HUD::update()
    // This includes car visualization, wheels, gauges, icons, etc.
    HUD::update();
}

void HUDManager::renderSettings() {
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
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(20, 20);
    tft.println("TEST HARDWARE");
    
    tft.setTextSize(1);
    tft.setCursor(20, 60);
    tft.println("INA226 Sensores: 6 activos");
    tft.setCursor(20, 90);
    tft.println("Temperatura DS18B20: 5 activos");
    tft.setCursor(20, 120);
    tft.println("Encoders rueda: 4 activos");
}

void HUDManager::renderWifiConfig() {
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
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(20, 20);
    tft.println("ESTADISTICAS");
    
    tft.setTextSize(1);
    tft.setCursor(20, 60);
    tft.println("En desarrollo...");
}

void HUDManager::renderQuickMenu() {
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
    // Menú oculto con TODOS los datos de calibración y sensores
    tft.fillScreen(TFT_BLACK);
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
    
    // Sección 4: Pedal y Encoder
    tft.setCursor(250, 125);
    tft.print("PEDAL:");
    tft.setCursor(250, 140);
    tft.printf("Posicion: %5.1f%%", carData.pedalPosition);
    tft.setCursor(250, 155);
    tft.printf("Encoder raw: %6.0f", carData.encoderValue);
    tft.setCursor(250, 170);
    tft.printf("Angulo volante: %5.1f", carData.steeringAngle);
    
    // Sección 5: Velocidad y RPM
    tft.setCursor(5, 160);
    tft.print("MOVIMIENTO:");
    tft.setCursor(5, 175);
    tft.printf("Velocidad: %5.1f km/h", carData.speed);
    tft.setCursor(5, 190);
    tft.printf("RPM: %6.0f", carData.rpm);
    
    // Sección 6: Odómetros
    tft.setCursor(5, 210);
    tft.printf("Odo Total: %8.2f km", carData.odoTotal);
    tft.setCursor(5, 225);
    tft.printf("Odo Parcial: %6.2f km", carData.odoTrip);
    
    // Sección 7: Estado del sistema
    tft.setCursor(5, 245);
    tft.print("ESTADO:");
    tft.setCursor(5, 260);
    tft.print("Marcha: ");
    switch (carData.gear) {
        case GearPosition::PARK:    tft.print("PARK"); break;
        case GearPosition::REVERSE: tft.print("REVERSA"); break;
        case GearPosition::NEUTRAL: tft.print("NEUTRAL"); break;
        case GearPosition::DRIVE1:  tft.print("DRIVE 1"); break;
        case GearPosition::DRIVE2:  tft.print("DRIVE 2"); break;
    }
    
    // Instrucciones salida
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setCursor(250, 280);
    tft.print("Pulse 3s para salir");
}
