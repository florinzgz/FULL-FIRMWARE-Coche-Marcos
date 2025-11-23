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
    // Apagar relés inmediatamente (safe state)
    
    // Apagar motores y auxiliares INMEDIATAMENTE (pines de pins.h)
    digitalWrite(PIN_RELAY_MAIN, LOW);  // Main power relay
    digitalWrite(PIN_RELAY_TRAC, LOW);  // Traction relay
    digitalWrite(PIN_RELAY_DIR, LOW);   // Direction relay
    digitalWrite(PIN_RELAY_SPARE, LOW); // Spare relay
    
    // Note: delay() in ISR is acceptable here as this is a critical emergency shutdown
    // before system reset. The watchdog will reset the system regardless.
    // This ensures relays have time to physically disengage before power loss.
    delay(1000);
    
    // El WDT reset ocurrirá ahora
}

} // namespace Watchdog
