#include "watchdog.h"
#include "logger.h"
#include "power_mgmt.h"
#include "pins.h"
#include <esp_task_wdt.h>

namespace Watchdog {

// Configuración
constexpr uint32_t WDT_TIMEOUT_SECONDS = 10;  // 10 segundos antes de panic
static bool initialized = false;
static uint32_t lastFeedTime = 0;
static uint32_t feedCount = 0;

void init() {
    // Configurar Watchdog Timer
    // Parámetros: timeout_seconds, panic_on_timeout
    esp_task_wdt_init(WDT_TIMEOUT_SECONDS, true);
    
    // Añadir task actual (loop principal)
    esp_task_wdt_add(NULL);
    
    initialized = true;
    lastFeedTime = millis();
    feedCount = 0;
    
    Logger::infof("Watchdog: WDT inicializado - Timeout: 10s, panic habilitado");
}

void feed() {
    if (!initialized) return;
    
    // Reset watchdog timer
    esp_task_wdt_reset();
    
    uint32_t now = millis();
    uint32_t interval = now - lastFeedTime;
    lastFeedTime = now;
    feedCount++;
    
    // Log cada 100 feeds (~10 segundos si feed se llama cada 100ms)
    if (feedCount % 100 == 0) {
        Logger::infof("WDT feed %lu (interval: %lums)", feedCount, interval);
    }
    
    // Alerta si el intervalo es muy largo (>80% del timeout)
    if (interval > (WDT_TIMEOUT_SECONDS * 800)) {  // 8 segundos
        Logger::warn("Watchdog: Feed interval demasiado largo - Riesgo de reset WDT");
    }
}

void disable() {
    if (!initialized) return;
    
    esp_task_wdt_delete(NULL);
    esp_task_wdt_deinit();
    initialized = false;
    
    Logger::warn("Watchdog: WDT deshabilitado (DEBUG ONLY)");
}

bool isEnabled() {
    return initialized;
}

uint32_t getFeedCount() {
    return feedCount;
}

uint32_t getLastFeedInterval() {
    return millis() - lastFeedTime;
}

// Callback de panic del WDT - ejecutado ANTES del reset
// Intentar apagado seguro de relés
void __attribute__((weak)) esp_task_wdt_isr_user_handler(void) {
    // CRÍTICO: Minimal code aquí - ISR context
    // 🔒 v2.4.1: NO usar delay() en ISR - puede causar corrupción
    
    // Apagar motores y auxiliares INMEDIATAMENTE (pines de pins.h)
    // Acceso directo a registros GPIO para máxima velocidad y seguridad en ISR
    GPIO.out_w1tc = ((1ULL << PIN_RELAY_MAIN) | 
                     (1ULL << PIN_RELAY_TRAC) | 
                     (1ULL << PIN_RELAY_DIR) | 
                     (1ULL << PIN_RELAY_SPARE));
    
    // 🔒 v2.4.1: Usar bucle de CPU para espera mínima en lugar de delay()
    // Espera ~10ms para que los relés se desactiven físicamente
    // A 240MHz, 10ms ≈ 2.4 millones de ciclos
    for (volatile uint32_t i = 0; i < 2400000; i++) {
        __asm__ __volatile__("nop");
    }
    
    // El WDT reset ocurrirá ahora
}

} // namespace Watchdog
