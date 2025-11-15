#pragma once
#include <Arduino.h>

/**
 * @file watchdog.h
 * @brief Sistema Watchdog Timer (WDT) para evitar bloqueos del sistema
 * 
 * CRÍTICO PARA SEGURIDAD:
 * - Reinicia automáticamente la ESP32 si el programa se bloquea
 * - Timeout: 10 segundos sin feed() → panic + safe state + reset
 * - Safe state: Apaga 24V motores + 12V aux antes del reset
 * 
 * USO:
 * 1. Llamar Watchdog::init() en setup()
 * 2. Llamar Watchdog::feed() periódicamente en loop() (<10s)
 * 3. Llamar Watchdog::feed() en puntos críticos (I²C, sensores, UI)
 */

namespace Watchdog {

/**
 * @brief Inicializa el Watchdog Timer
 * 
 * Configura WDT con timeout de 10 segundos y panic habilitado.
 * Debe llamarse una vez en setup().
 */
void init();

/**
 * @brief Feed watchdog - resetea el timer
 * 
 * DEBE llamarse al menos cada 8 segundos para evitar reset.
 * Recomendación: llamar en loop() principal y en puntos críticos.
 */
void feed();

/**
 * @brief Deshabilita el watchdog (DEBUG ONLY)
 * 
 * ⚠️ PELIGROSO: Solo usar durante debugging.
 * Sistema queda sin protección ante bloqueos.
 */
void disable();

/**
 * @brief Verifica si watchdog está habilitado
 * 
 * @return true si WDT activo
 */
bool isEnabled();

/**
 * @brief Obtiene contador de feeds realizados
 * 
 * @return Número total de feeds desde init()
 */
uint32_t getFeedCount();

/**
 * @brief Obtiene tiempo desde último feed
 * 
 * @return Milisegundos desde último feed()
 */
uint32_t getLastFeedInterval();

} // namespace Watchdog
