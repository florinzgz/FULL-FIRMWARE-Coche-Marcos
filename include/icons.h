#pragma once
#include <TFT_eSPI.h>
#include "system.h"
#include "shifter.h"
// Opcional: para constrain() si se usa fuera de icons.cpp
#include <Arduino.h>

namespace Icons {
    void init(TFT_eSPI *display);

    void drawSystemState(System::State st);
    void drawGear(Shifter::Gear g);

    // luces, multimedia, 4x4/4x2, regenerativo activo
    void drawFeatures(bool lights, bool media, bool mode4x4, bool regenOn);

    // batería: voltaje en V (aprox)
    void drawBattery(float volts);

    // advertencia de errores persistentes
    void drawErrorWarning();
    
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
                         uint8_t currentTotal, uint8_t tempTotal, uint8_t wheelTotal);
    
    /**
     * @brief Dibuja indicador de temperatura crítica
     * @param tempWarning true si hay temperatura crítica
     * @param maxTemp Temperatura máxima actual
     */
    void drawTempWarning(bool tempWarning, float maxTemp);

    // ============================================================
    // Constantes de layout (480x320, rotación 1)
    // ============================================================

    // Icono batería (arriba derecha, 50x40 px)
    constexpr int BATTERY_X1 = 420;
    constexpr int BATTERY_Y1 = 0;
    constexpr int BATTERY_X2 = 470;
    constexpr int BATTERY_Y2 = 40;

    // ============================================================
    // Iconos de características (LUCES, MEDIA, 4x4) - Cuadrados 3D
    // Posicionados encima de la barra de acelerador (y=300)
    // Tamaño uniforme: 70x45 px cada uno
    // ============================================================
    
    // Icono 4x4/4x2 (izquierda, encima de barra acelerador)
    constexpr int MODE4X4_X1 = 5;
    constexpr int MODE4X4_Y1 = 250;
    constexpr int MODE4X4_X2 = 75;
    constexpr int MODE4X4_Y2 = 295;
    
    // Icono LUCES (centro-izquierda, encima de barra acelerador)
    constexpr int LIGHTS_X1 = 85;
    constexpr int LIGHTS_Y1 = 250;
    constexpr int LIGHTS_X2 = 155;
    constexpr int LIGHTS_Y2 = 295;

    // Icono MEDIA (centro-derecha, encima de barra acelerador)
    constexpr int MEDIA_X1 = 165;
    constexpr int MEDIA_Y1 = 250;
    constexpr int MEDIA_X2 = 235;
    constexpr int MEDIA_Y2 = 295;

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
}