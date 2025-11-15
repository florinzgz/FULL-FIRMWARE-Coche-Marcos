#pragma once
#include <TFT_eSPI.h>

namespace MenuHidden {

    // --- Inicialización del menú oculto ---
    // Recibe el puntero a la pantalla TFT y carga la configuración desde Storage.
    void init(TFT_eSPI *display);

    // --- Actualización del menú ---
    // Se invoca en cada ciclo principal.
    // - batteryIconPressed: true si el usuario mantiene pulsado el icono de batería
    //   (secuencia que permite introducir el código secreto 8989).
    // Internamente gestiona:
    //   * Entrada de código de acceso
    //   * Dibujo del menú oculto
    //   * Ejecución de opciones (calibraciones, ajustes, ver/borrar errores, etc.)
    void update(bool batteryIconPressed);

    // --- Notas ---
    // * El código de acceso por defecto es 8989.
    // * El menú permite calibrar pedal, encoder, ajustar regen, activar/desactivar módulos,
    //   guardar configuración, restaurar valores de fábrica y gestionar errores persistentes.
    // * Se apoya en Storage (guardar/restaurar), Audio (confirmaciones sonoras) y System (errores).
}