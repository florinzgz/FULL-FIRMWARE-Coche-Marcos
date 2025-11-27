#pragma once
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

namespace MenuHidden {

    // --- Inicialización del menú oculto ---
    // Recibe el puntero a la pantalla TFT y carga la configuración desde Storage.
    void init(TFT_eSPI *display);
    
    // --- Inicialización táctil ---
    // Recibe el puntero al controlador táctil para navegación interactiva.
    void initTouch(XPT2046_Touchscreen *touchScreen);

    // --- Actualización del menú ---
    // Se invoca en cada ciclo principal.
    // - batteryIconPressed: true si el usuario mantiene pulsado el icono de batería
    //   (secuencia que permite introducir el código secreto 8989).
    // Internamente gestiona:
    //   * Entrada de código de acceso
    //   * Dibujo del menú oculto
    //   * Navegación táctil (tocar opción para seleccionar y ejecutar)
    //   * Calibración interactiva de pedal y encoder
    //   * Ejecución de opciones (calibraciones, ajustes, ver/borrar errores, etc.)
    void update(bool batteryIconPressed);
    
    // --- Estado del menú ---
    // Devuelve true si el menú oculto está activo.
    bool isActive();

    // --- Notas ---
    // * El código de acceso por defecto es 8989.
    // * El menú permite calibrar pedal, encoder, ajustar regen, activar/desactivar módulos,
    //   guardar configuración, restaurar valores de fábrica y gestionar errores persistentes.
    // * La calibración de pedal guía al usuario con interfaz gráfica:
    //   1. Soltar pedal → captura valor MIN
    //   2. Pisar pedal al máximo → captura valor MAX
    //   3. Guarda en Storage con checksum
    // * La calibración de encoder centra el volante y guarda el offset.
    // * Se apoya en Storage (guardar/restaurar), Audio (confirmaciones sonoras) y System (errores).
}