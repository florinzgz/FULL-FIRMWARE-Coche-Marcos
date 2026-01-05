#ifndef BUTTONS_H
#define BUTTONS_H

#include <Arduino.h>

/**
 * @brief Gestión de botones físicos del tablero.
 * 
 * v2.14.0: Simplificado - solo botón de luces físico.
 * Multimedia y 4x4 ahora son controlados por touch screen solamente.
 * 
 * Soporta:
 * - Debounce seguro por botón
 * - Detección de toggle (on/off) por botón
 * - Long press y very long press individual
 * - Protección de inicialización
 *
 * El orden (índices) de los botones es seguro por enum:
 *   BTN_LIGHTS = 0
 * 
 * Para expansión, solo crece el enum y los arrays.
 */

class Buttons {
public:
    struct State {
        bool lights;
        // multimedia y mode4x4 eliminados - ahora controlados por touch
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
     * @brief Verifica si fue inicializado correctamente.
     */
    static bool initOK();
};

#endif // BUTTONS_H