#ifndef BUTTONS_H
#define BUTTONS_H

#include <Arduino.h>

/**
 * @brief Gestión de botones físicos del tablero.
 * 
 * Soporta:
 * - Debounce seguro por botón
 * - Detección de toggle (on/off) por botón
 * - Long press y very long press individual
 * - Protección de inicialización
 *
 * El orden (índices) de los botones es seguro por enum:
 *   BTN_LIGHTS = 0, BTN_MEDIA = 1, BTN_4X4 = 2
 * 
 * Para expansión, solo crece el enum y los arrays.
 */

class Buttons {
public:
    struct State {
        bool lights;
        bool multimedia;
        bool mode4x4;
        bool reserved;  // Futuro o expansión
    };

    /**
     * @brief Inicializa pines, arrays y estado.
     */
    static void init();

    /**
     * @brief Llama una vez por loop. Debe llamarse frecuentemente.
     */
    static void update();

    /**
     * @brief Devuelve el estado actual completo.
     */
    static const State& get();

    /**
     * @brief Devuelve y limpia el flag de toggle para el botón de luces.
     */
    static bool toggledLights();

    /**
     * @brief Devuelve y limpia el flag de toggle para multimedia.
     */
    static bool toggledMultimedia();

    /**
     * @brief Devuelve y limpia el flag de toggle para modo 4x4.
     */
    static bool toggled4x4();

    /**
     * @brief Verifica si fue inicializado correctamente.
     */
    static bool initOK();
};

#endif // BUTTONS_H