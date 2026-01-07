#ifndef HUD_MANAGER_H
#define HUD_MANAGER_H

#include "display_types.h"
#include "sensors.h"  // For Sensors::InputDeviceStatus type
#include <TFT_eSPI.h>

/**
 * @brief Gestor unificado de HUD (básico + avanzado)
 * 
 * Centraliza la gestión de ambos sistemas de visualización:
 * - HUD básico: Logo, Ready, Error states
 * - HUD avanzado: Dashboard completo con widgets, gauges, ruedas
 * 
 * Optimizado para 30 FPS con actualización selectiva de áreas.
 */
class HUDManager {
public:
    /**
     * @brief Inicializa el sistema de visualización
     * Configura TFT_eSPI, backlight, touchscreen
     */
    static void init();
    
    /**
     * @brief Actualiza la visualización (llamar en loop principal)
     * Frame rate: 30 FPS (33ms por frame)
     */
    static void update();
    
    /**
     * @brief Actualiza los datos del vehículo
     * @param data Estructura con datos de sensores
     */
    static void updateCarData(const CarData& data);
    
    /**
     * @brief Cambia el menú activo
     * @param menu Tipo de menú a mostrar
     */
    static void showMenu(MenuType menu);
    
    /**
     * @brief Obtiene el menú actual
     * @return MenuType actual
     */
    static MenuType getCurrentMenu();
    
    /**
     * @brief Fuerza redibujado completo en el próximo update
     */
    static void forceRedraw();
    
    /**
     * @brief Muestra logo de inicio
     */
    static void showLogo();
    
    /**
     * @brief Muestra estado "Ready"
     */
    static void showReady();
    
    /**
     * @brief Muestra error
     * @param message Mensaje de error
     */
    static void showError(const char* message);
    
    /**
     * @brief Procesa evento táctil
     * @param x Coordenada X
     * @param y Coordenada Y
     * @param pressed Estado (presionado/liberado)
     */
    static void handleTouch(int16_t x, int16_t y, bool pressed);
    
    /**
     * @brief Establece brillo del backlight
     * @param brightness Brillo (0-255)
     */
    static void setBrightness(uint8_t brightness);
    
    /**
     * @brief Activa/desactiva el menú oculto
     * Requiere combinación de botones o gesto especial
     * @param activate true para activar, false para desactivar
     */
    static void activateHiddenMenu(bool activate);
    
    /**
     * @brief Verifica si el menú oculto está activo
     * @return true si está activo
     */
    static bool isHiddenMenuActive();
    
    /**
     * @brief Procesa pulsación larga de botón (para activar menú oculto)
     * @param buttonId ID del botón presionado
     * @param duration Duración de la pulsación (ms)
     */
    static void handleLongPress(uint8_t buttonId, uint32_t duration);
    
    /**
     * @brief Verifica si el módulo está inicializado correctamente
     * @return true si la inicialización fue exitosa
     */
    static bool initOK();

private:
    static MenuType currentMenu;
    static CarData carData;
    static uint32_t lastUpdateMs;
    static bool needsRedraw;
    static uint8_t brightness;
    
    // Helper methods
    static void clearScreenIfNeeded();
    
    // Color calculation helpers for status display
    static uint16_t getSensorStatusColor(uint8_t okCount, uint8_t totalCount);
    static uint16_t getPedalColor(const Sensors::InputDeviceStatus& status);
    static uint16_t getSteeringColor(const Sensors::InputDeviceStatus& status);
    static uint16_t getShifterColor(const Sensors::InputDeviceStatus& status);
    static const char* getGearName(uint8_t gear);
    
    // Funciones de renderizado
    static void renderDashboard();
    static void renderSettings();
    static void renderCalibration();
    static void renderHardwareTest();
    static void renderWifiConfig();
    static void renderINA226Monitor();
    static void renderStatistics();
    static void renderQuickMenu();
    static void renderHiddenMenu();
    
    // Helpers optimización
    static void updateOnlyChanged();
    static bool dataHasChanged();
    
    // Hidden menu state
    static bool hiddenMenuActive;
    static uint32_t longPressStartMs;
    static uint8_t longPressButtonId;
};

#endif // HUD_MANAGER_H
