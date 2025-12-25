#pragma once
#include <stdint.h>
#include "storage.h"

namespace System {

    // --- Estados globales del sistema ---
    enum State {
        OFF,        // Sistema apagado
        PRECHECK,   // Autotest inicial
        READY,      // Listo para arrancar
        RUN,        // En ejecución normal
        ERROR       // Error crítico → relés apagados
    };

    // --- Resultado del autotest ---
    struct Health {
        bool ok;          // Estado global
        bool steeringOK;  // Dirección
        bool currentOK;   // Sensores de corriente
        bool tempsOK;     // Sensores de temperatura
        bool wheelsOK;    // Sensores de rueda
    };

    // --- Ciclo de vida del sistema ---
    void init();          // Inicializa y entra en PRECHECK
    Health selfTest();    // Ejecuta autotest de módulos
    void update();        // Avanza máquina de estados
    State getState();     // Devuelve estado actual

    // --- API de diagnóstico persistente ---
    // Registra un error persistente (FIFO, evita duplicados)
    void logError(uint16_t code);

    // Devuelve puntero al buffer de errores
    const Storage::ErrorLog* getErrors();

    // Número de errores almacenados
    int getErrorCount();

    // Limpia todos los errores persistentes
    void clearErrors();

    // Verifica si hay errores activos en el sistema
    bool hasError();
    
    // 🔒 v2.11.6: Verificar si sistema está inicializado (thread-safe)
    bool isInitialized();
}