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

    // ============================================================
    // Constantes de layout (480x320, rotación 1)
    // ============================================================

    // Icono batería (arriba derecha, 50x40 px)
    constexpr int BATTERY_X1 = 420;
    constexpr int BATTERY_Y1 = 0;
    constexpr int BATTERY_X2 = 470;
    constexpr int BATTERY_Y2 = 40;

    // Icono luces (arriba izquierda, 50x40 px)
    constexpr int LIGHTS_X1 = 0;
    constexpr int LIGHTS_Y1 = 0;
    constexpr int LIGHTS_X2 = 50;
    constexpr int LIGHTS_Y2 = 40;

    // Icono multimedia (junto a luces, 50x40 px)
    constexpr int MEDIA_X1 = 60;
    constexpr int MEDIA_Y1 = 0;
    constexpr int MEDIA_X2 = 110;
    constexpr int MEDIA_Y2 = 40;

    // Icono 4x4 (abajo izquierda, 60x40 px)
    constexpr int MODE4X4_X1 = 0;
    constexpr int MODE4X4_Y1 = 280;
    constexpr int MODE4X4_X2 = 60;
    constexpr int MODE4X4_Y2 = 320;

    // Icono warning (arriba centro, 80x40 px)
    constexpr int WARNING_X1 = 200;
    constexpr int WARNING_Y1 = 0;
    constexpr int WARNING_X2 = 280;
    constexpr int WARNING_Y2 = 40;
}