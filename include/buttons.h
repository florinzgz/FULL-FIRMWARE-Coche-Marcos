#pragma once
#include <Arduino.h>

namespace Buttons {

    // --- Estado global de los botones ---
    struct State {
        bool lights;       // botón luces (toggle ON/OFF)
        bool multimedia;   // botón multimedia (toggle ON/OFF)
        bool mode4x4;      // toggle 4x4 / 4x2
        bool batteryIcon;  // acceso menú oculto (combinación especial)
    };

    // --- Inicialización ---
    // Configura los pines definidos en pins.h y resetea el estado.
    void init();

    // --- Actualización ---
    // Debe llamarse periódicamente en el loop principal.
    // Aplica debounce y detecta cambios de estado.
    void update();

    // --- Acceso al estado actual ---
    const State& get();

    // --- Eventos de flanco (toggle) ---
    // Devuelven true una sola vez cuando se detecta un cambio de estado.
    bool toggledLights();
    bool toggledMultimedia();
    bool toggled4x4();
    
    // 🔒 v2.5.0: Estado de inicialización
    bool initOK();
}