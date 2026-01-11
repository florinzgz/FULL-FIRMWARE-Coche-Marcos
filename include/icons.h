#pragma once
#include "shifter.h"
#include "system.h"
#include <TFT_eSPI.h>
// Opcional: para constrain() si se usa fuera de icons.cpp
#include <Arduino.h>

namespace Icons {
void init(TFT_eSPI *display);

// Phase 6.2: All drawing functions accept optional sprite for compositor mode
void drawSystemState(System::State st, TFT_eSprite *sprite = nullptr);
void drawGear(Shifter::Gear g, TFT_eSprite *sprite = nullptr);

// v2.14.0: Simplified - only 4x4/4x2 mode and regenerative
// Lights and multimedia removed (touch-only, no visual icons)
void drawFeatures(bool mode4x4, bool regenOn, TFT_eSprite *sprite = nullptr);

// batería: voltaje en V (aprox)
void drawBattery(float volts, TFT_eSprite *sprite = nullptr);

// advertencia de errores persistentes
void drawErrorWarning(TFT_eSprite *sprite = nullptr);

/**
 * @brief Dibuja indicador de estado de sensores
 * Muestra LED de estado para cada grupo de sensores:
 * - Verde: Todos OK
 * - Amarillo: Algunos fallos
 * - Rojo: Fallo crítico
 *
 * @param currentOK Sensores de corriente OK (0-6)
 * @param tempOK Sensores de temperatura OK (0-5)
 * @param wheelOK Sensores de rueda OK (0-4)
 * @param currentTotal Total sensores de corriente
 * @param tempTotal Total sensores de temperatura
 * @param wheelTotal Total sensores de rueda
 */
void drawSensorStatus(uint8_t currentOK, uint8_t tempOK, uint8_t wheelOK,
                      uint8_t currentTotal, uint8_t tempTotal,
                      uint8_t wheelTotal, TFT_eSprite *sprite = nullptr);

/**
 * @brief Dibuja indicador de temperatura crítica
 * @param tempWarning true si hay temperatura crítica
 * @param maxTemp Temperatura máxima actual
 * @param sprite Optional sprite for compositor mode
 */
void drawTempWarning(bool tempWarning, float maxTemp,
                     TFT_eSprite *sprite = nullptr);

/**
 * @brief Dibuja temperatura ambiente en esquina superior derecha
 * @param ambientTemp Temperatura ambiente en grados Celsius
 * @param sprite Optional sprite for compositor mode
 */
void drawAmbientTemp(float ambientTemp, TFT_eSprite *sprite = nullptr);

// ============================================================
// Constantes de layout (480x320, rotación 1)
// ============================================================

// Icono batería (arriba derecha, 50x40 px)
constexpr int BATTERY_X1 = 420;
constexpr int BATTERY_Y1 = 0;
constexpr int BATTERY_X2 = 470;
constexpr int BATTERY_Y2 = 40;

// Temperatura ambiente (arriba derecha, debajo de batería)
constexpr int AMBIENT_TEMP_X = 420;
constexpr int AMBIENT_TEMP_Y = 42;
constexpr int AMBIENT_TEMP_W = 55;
constexpr int AMBIENT_TEMP_H = 20;

// ============================================================
// Iconos de características - v2.14.0: Solo 4x4 mode
// Posicionados encima de la barra de acelerador (y=300)
// Tamaño uniforme: 70x45 px
// ============================================================

// Icono 4x4/4x2 (izquierda, encima de barra acelerador)
constexpr int MODE4X4_X1 = 5;
constexpr int MODE4X4_Y1 = 250;
constexpr int MODE4X4_X2 = 75;
constexpr int MODE4X4_Y2 = 295;

// v2.14.0: LIGHTS and MEDIA icons removed - cleaner HUD
// Icons were at positions 85-155 and 165-235

// Icono warning (arriba centro, 80x40 px)
constexpr int WARNING_X1 = 200;
constexpr int WARNING_Y1 = 0;
constexpr int WARNING_X2 = 280;
constexpr int WARNING_Y2 = 40;

// Indicador de estado de sensores (junto a warning, 120x40 px)
// Nota: El área termina en 410px, dejando 10px de margen con batería (420px)
constexpr int SENSOR_STATUS_X1 = 290;
constexpr int SENSOR_STATUS_Y1 = 0;
constexpr int SENSOR_STATUS_X2 = 410;
constexpr int SENSOR_STATUS_Y2 = 40;

// Indicador de temperatura crítica (derecha, entre REGEN e iconos)
constexpr int TEMP_WARNING_X = 320;
constexpr int TEMP_WARNING_Y = 260;
constexpr int TEMP_WARNING_W = 70;
constexpr int TEMP_WARNING_H = 25;
} // namespace Icons