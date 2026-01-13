#ifndef HUD_MANAGER_H
#define HUD_MANAGER_H

#include "display_types.h"
#include "render_event.h" // Thread-safe render event system
#include "sensors.h"      // For Sensors::InputDeviceStatus type
#include <TFT_eSPI.h>

/**
 * @brief Gestor unificado de HUD (básico + avanzado)
 *
 * Centraliza la gestión de ambos sistemas de visualización:
 * - HUD básico: Logo, Ready, Error states
 * - HUD avanzado: Dashboard completo con widgets, gauges, ruedas
 *
 * Optimizado para 30 FPS con actualización selectiva de áreas.
 *
 * 🔒 THREAD SAFETY: TFT_eSPI is NOT thread-safe!
 * - ALL TFT drawing operations MUST happen in HUDManager::update()
 * - Other contexts must use queueRenderEvent() to request rendering
 * - Direct tft.* calls from outside update() will cause crashes (ipc0 stack canary)
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
  static void updateCarData(const CarData &data);

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
   *
   * 🔒 THREAD-SAFE: This method queues the error display request.
   * The actual drawing happens in update() to avoid TFT_eSPI race conditions.
   */
  static void showError(const char *message);

  /**
   * @brief Queue a render event for thread-safe processing
   * @param event The render event to queue
   * @return true if event was queued successfully, false if queue is full
   *
   * 🔒 THREAD-SAFE: Safe to call from any context (ISR, task, callback)
   * Events are processed in HUDManager::update() on the main render thread.
   */
  static bool queueRenderEvent(const RenderEvent::Event &event);

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

  /**
   * @brief Toggle shadow mode validation (PHASE 7)
   * @param enabled true to enable, false to disable
   */
  static void setShadowMode(bool enabled);

  /**
   * @brief Check if shadow mode is enabled (PHASE 7)
   * @return true if shadow mode is active
   */
  static bool isShadowModeEnabled();

private:
  static MenuType currentMenu;
  static CarData carData;
  static uint32_t lastUpdateMs;
  static bool needsRedraw;
  static uint8_t brightness;

  // 🔒 THREAD SAFETY: Render event queue
  // FreeRTOS queue handle for thread-safe render requests
  static void *renderEventQueue; // QueueHandle_t (void* to avoid freertos include)

  // Current error state
  static bool errorActive;
  static char errorMessage[RenderEvent::MAX_ERROR_MSG_LEN];

  // Helper methods
  static void clearScreenIfNeeded();
  static void processRenderEvents(); // 🔒 NEW: Process queued render events

  // Color calculation helpers for status display
  static uint16_t getSensorStatusColor(uint8_t okCount, uint8_t totalCount);
  static uint16_t getPedalColor(const Sensors::InputDeviceStatus &status);
  static uint16_t getSteeringColor(const Sensors::InputDeviceStatus &status);
  static uint16_t getShifterColor(const Sensors::InputDeviceStatus &status);
  static const char *getGearName(uint8_t gear);

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
  static void renderErrorScreen(); // 🔒 NEW: Render error screen (called only from update)

  // Helpers optimización
  static void updateOnlyChanged();
  static bool dataHasChanged();

  // Hidden menu state
  static bool hiddenMenuActive;
  static uint32_t longPressStartMs;
  static uint8_t longPressButtonId;
};

#endif // HUD_MANAGER_H
