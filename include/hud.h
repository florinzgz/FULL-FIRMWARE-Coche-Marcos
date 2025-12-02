#pragma once
namespace HUD {
    void init();
    void showLogo();
    void showReady();
    void showError();
    void update();
    void drawPedalBar(float pedalPercent);  // Barra de pedal en parte inferior
    
    // Control de giro sobre eje (axis rotation)
    void toggleAxisRotation();
    bool isAxisRotationEnabled();
}