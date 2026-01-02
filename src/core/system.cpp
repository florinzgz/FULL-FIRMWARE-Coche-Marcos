#include "system.h"
#include "dfplayer.h"
#include "current.h"
#include "temperature.h"
#include "wheels.h"
#include "pedal.h"
#include "steering.h"
#include "relays.h"
#include "logger.h"
#include "storage.h"
#include "steering_motor.h"   // 🔒 v2.4.0: Para verificar motor dirección
#include "traction.h"         // 🔒 v2.4.0: Para verificar tracción
#include "eeprom_persistence.h"  // 🔒 v2.11.0: Persistencia de configuración
#include "abs_system.h"          // 🔒 v2.11.0: Sistema ABS
#include "tcs_system.h"          // 🔒 v2.11.0: Sistema TCS
#include "regen_ai.h"            // 🔒 v2.11.0: Freno regenerativo
#include "obstacle_safety.h"     // 🔒 v2.11.0: Seguridad obstáculos
#include "led_controller.h"      // 🔒 v2.11.0: Control LEDs
#include "shifter.h"             // 🔒 v2.11.1: Validación de palanca de cambios
#include "operation_modes.h"     // Sistema de modos de operación con tolerancia a fallos
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

extern Storage::Config cfg;

// ========================================
// Configuración de protección de inicialización
// ========================================
namespace SystemInitConfig {
    constexpr uint32_t MUTEX_TIMEOUT_MS = 5000;        // Timeout para adquirir mutex
    constexpr uint32_t MUTEX_CHECK_TIMEOUT_MS = 100;   // Timeout para check de estado
    constexpr uint32_t MIN_HEAP_FOR_INIT = 50000;      // 50KB heap mínimo
    constexpr uint32_t MIN_HEAP_AFTER_INIT = 25000;    // 25KB después de init
}

static System::State currentState = System::OFF;
static bool systemInitialized = false;  // 🔒 v2.11.2: Guard contra re-inicialización

// 🔒 v2.11.6: Mutex para proteger inicialización thread-safe
static SemaphoreHandle_t initMutex = nullptr;
static bool initMutexCreated = false;

static constexpr float PEDAL_REST_THRESHOLD_PERCENT =
    5.0f; // Tolerancia fija (no configurable) para ruido ADC garantizando pedal en reposo antes de dar potencia

void System::init() {
    // ========================================
    // PASO 1: Crear mutex en primera llamada
    // ========================================
    // Nota: Creación de mutex es thread-safe en ESP32 (usa atomic operations)
    // ========================================
    // PASO 1: Crear mutex en primera llamada (thread-safe)
    // ========================================
    // Use portENTER_CRITICAL/portEXIT_CRITICAL for atomic check-and-set
    static portMUX_TYPE initMutexSpinlock = portMUX_INITIALIZER_UNLOCKED;
    
    portENTER_CRITICAL(&initMutexSpinlock);
    bool needsCreate = !initMutexCreated;
    if (needsCreate) {
        initMutexCreated = true;  // Set flag inside critical section
    }
    portEXIT_CRITICAL(&initMutexSpinlock);
    
    if (needsCreate) {
        initMutex = xSemaphoreCreateMutex();
        if (initMutex == nullptr) {
            // CRÍTICO: No se pudo crear mutex
            Logger::error("System init: CRITICAL - Failed to create init mutex");
            Serial.println("[CRITICAL] System::init() - mutex creation failed");
            // Reset flag on failure
            portENTER_CRITICAL(&initMutexSpinlock);
            initMutexCreated = false;
            portEXIT_CRITICAL(&initMutexSpinlock);
            // Continuar sin protección (menos seguro pero permite boot)
        } else {
            Logger::info("System init: Init mutex created");
        }
    }
    
    // ========================================
    // PASO 2: Tomar mutex ANTES de cualquier check
    // ========================================
    const TickType_t MUTEX_TIMEOUT = pdMS_TO_TICKS(SystemInitConfig::MUTEX_TIMEOUT_MS);  // 5 segundos timeout
    
    if (initMutex != nullptr) {
        if (xSemaphoreTake(initMutex, MUTEX_TIMEOUT) != pdTRUE) {
            Logger::error("System init: Failed to acquire init mutex (timeout)");
            Serial.println("[ERROR] System::init() - mutex timeout");
            return;  // Abortar si no se puede tomar mutex
        }
        Logger::debug("System init: Mutex acquired");
    } else {
        Logger::warn("System init: Running without mutex protection");
    }
    
    // ========================================
    // PASO 3: Check de inicialización (ahora thread-safe)
    // ========================================
    if (systemInitialized) {
        Logger::warn("System init: Sistema ya inicializado, ignorando llamada duplicada");
        if (initMutex != nullptr) {
            xSemaphoreGive(initMutex);
        }
        return;
    }
    
    // ========================================
    // PASO 4: Inicialización normal
    // ========================================
    // NOTA: El flag systemInitialized se establece al FINAL de init()
    // después de que toda la inicialización sea exitosa
    // Inicializar sistema de modos de operación
    SystemMode::init();
    
    Logger::info("System init: entrando en PRECHECK");
    currentState = PRECHECK;
    
    // VALIDACIÓN: Verificar heap disponible
    uint32_t freeHeap = ESP.getFreeHeap();
    Logger::infof("System init: Free heap: %u bytes", freeHeap);
    
    if (freeHeap < SystemInitConfig::MIN_HEAP_FOR_INIT) {
        Logger::errorf("System init: CRÍTICO - Heap insuficiente (%u bytes < %u bytes requeridos)", 
                      freeHeap, SystemInitConfig::MIN_HEAP_FOR_INIT);
        Logger::error("System init: Abortando inicialización - memoria insuficiente");
        currentState = ERROR;
        
        // Resetear flag de inicialización para permitir retry
        systemInitialized = false;
        
        // Liberar mutex antes de salir
        if (initMutex != nullptr) {
            xSemaphoreGive(initMutex);
        }
        return;
    }
    
    // Enhanced diagnostic information
    Logger::infof("System init: Estado inicial OK");
    
    #ifdef ARDUINO_ESP32S3_DEV
    Logger::info("System init: Platform ESP32-S3 detected");
    #endif
    
    // 🔒 v2.11.2: VALIDACIÓN 3 - Cargar y validar configuración persistente
    Logger::info("System init: Cargando configuración persistente");
    if (!EEPROMPersistence::init()) {
        Logger::warn("System init: EEPROM persistence init failed, using defaults");
        // 🔒 No es crítico - continuamos con valores por defecto
    }
    
    // 🔒 v2.11.2: VALIDACIÓN 4 - Cargar configuración general con validación
    GeneralSettings settings;
    
    if (EEPROMPersistence::loadGeneralSettings(settings)) {
        Logger::info("System init: Configuración general cargada exitosamente");
        
        // Aplicar toggles de módulos según configuración cargada
        ABSSystem::setEnabled(settings.absEnabled);
        Logger::infof("System init: ABS %s", settings.absEnabled ? "enabled" : "disabled");
        
        TCSSystem::setEnabled(settings.tcsEnabled);
        Logger::infof("System init: TCS %s", settings.tcsEnabled ? "enabled" : "disabled");
        
        RegenAI::setEnabled(settings.regenEnabled);
        Logger::infof("System init: Regen %s", settings.regenEnabled ? "enabled" : "disabled");
    } else {
        Logger::warn("System init: No se pudo cargar configuración general, usando defaults");
        // 🔒 Aplicar configuración segura por defecto
        ABSSystem::setEnabled(false);  // Deshabilitado por seguridad
        TCSSystem::setEnabled(false);  // Deshabilitado por seguridad
        RegenAI::setEnabled(false);    // Deshabilitado por seguridad
        Logger::info("System init: Módulos avanzados deshabilitados (modo seguro)");
    }
    
    // 🔒 v2.11.2: VALIDACIÓN 5 - Cargar y aplicar configuración de LEDs con validación
    LEDConfig ledConfig;
    
    if (EEPROMPersistence::loadLEDConfig(ledConfig)) {
        Logger::info("System init: Configuración LED cargada exitosamente");
        
        // 🔒 Validar valores de configuración antes de aplicar
        if (ledConfig.brightness > 255) {
            Logger::warnf("System init: Brillo LED inválido (%d), usando default (128)", ledConfig.brightness);
            ledConfig.brightness = 128;
        }
        
        LEDController::setEnabled(ledConfig.enabled);
        LEDController::setBrightness(ledConfig.brightness);
        
        if (LEDController::initOK()) {
            auto &cfgLed = LEDController::getConfig();
            cfgLed.updateRateMs = 50; // Default update rate
        } else {
            Logger::warn("System init: LEDController not initialized, skipping config");
        }
        
        Logger::infof("System init: LEDs %s, brightness %d", 
                      ledConfig.enabled ? "enabled" : "disabled", 
                      ledConfig.brightness);
    } else {
        Logger::warn("System init: No se pudo cargar configuración LED, usando defaults");
        // 🔒 Aplicar configuración segura por defecto
        LEDController::setEnabled(false);  // Deshabilitado por defecto si no hay config
        LEDController::setBrightness(128); // Brillo medio
        Logger::info("System init: LEDs en modo seguro (deshabilitados)");
    }
    
    // Habilitar características de seguridad de obstáculos
    // Usar configuración por defecto ya que no hay persistencia específica para esto
    ObstacleSafety::enableParkingAssist(true);
    ObstacleSafety::enableCollisionAvoidance(true);
    ObstacleSafety::enableBlindSpot(true);
    Logger::info("System init: Seguridad de obstáculos habilitada");
    
    // 🔒 v2.11.2: VALIDACIÓN 6 - Verificar heap después de inicialización
    uint32_t finalHeap = ESP.getFreeHeap();
    uint32_t heapUsed = freeHeap - finalHeap;
    Logger::infof("System init: Heap usado en init: %u bytes, restante: %u bytes", heapUsed, finalHeap);
    
    if (finalHeap < SystemInitConfig::MIN_HEAP_AFTER_INIT) {
        Logger::warnf("System init: ADVERTENCIA - Heap bajo después de init (%u bytes)", finalHeap);
    }
    
    // ========================================
    // PASO 6: Marcar inicialización exitosa
    // ========================================
    systemInitialized = true;
    Logger::info("System init: Marked as initialized (successful completion)");
    
    // ========================================
    // PASO 7: Liberar mutex al finalizar
    // ========================================
    if (initMutex != nullptr) {
        xSemaphoreGive(initMutex);
        Logger::debug("System init: Mutex released");
    }
    
    Logger::info("System init: Completed successfully");
}

System::Health System::selfTest() {
    Health h{true,true,true,true,true};
    OperationMode mode = OperationMode::MODE_FULL;
    
    // 🔒 v2.11.2: VALIDACIÓN - Verificar que System::init() fue llamado
    if (!systemInitialized) {
        Logger::error("SelfTest: Sistema no inicializado - llamar System::init() primero");
        h = Health{false,false,false,false,false};
        SystemMode::setMode(OperationMode::MODE_SAFE);
        return h;
    }

    // Actualizar entradas críticas antes de validar estados
    Pedal::update();
    Shifter::update();
    Steering::update();

    // ========================================================================
    // SENSORES OPCIONALES (NO bloquean arranque - modo degradado)
    // ========================================================================
    
    // Corriente (opcional)
    if(cfg.currentSensorsEnabled) {
        if(!Sensors::currentInitOK()) {
            Logger::warn("SelfTest: Sensores corriente no disponibles - modo degradado");
            mode = OperationMode::MODE_DEGRADED;
            h.currentOK = false;
            // NO marcar h.ok = false - continuar operación
        }
    }

    // Temperatura (opcional)
    if(cfg.tempSensorsEnabled) {
        if(!Sensors::temperatureInitOK()) {
            Logger::warn("SelfTest: Sensores temperatura no disponibles - modo degradado");
            mode = OperationMode::MODE_DEGRADED;
            h.tempsOK = false;
            // NO marcar h.ok = false - continuar operación
        }
    }

    // Ruedas (opcional)
    if(cfg.wheelSensorsEnabled) {
        if(!Sensors::wheelsInitOK()) {
            Logger::warn("SelfTest: Sensores rueda limitados - modo degradado");
            mode = OperationMode::MODE_DEGRADED;
            h.wheelsOK = false;
            // NO marcar h.ok = false - continuar operación
        }
    }

    // ========================================================================
    // COMPONENTES CRÍTICOS (bloquean arranque si fallan)
    // ========================================================================

    // Pedal (crítico)
    if(!Pedal::initOK()) {
        System::logError(100);
        Logger::error("SelfTest: CRÍTICO - pedal no responde");
        h.ok = false;
        mode = OperationMode::MODE_SAFE;
    } else {
        const auto &pedalState = Pedal::get();
        if(pedalState.percent > PEDAL_REST_THRESHOLD_PERCENT) {
            Logger::errorf("SelfTest: CRÍTICO - pedal no está en reposo (%.1f%%)", pedalState.percent);
            h.ok = false;
            mode = OperationMode::MODE_SAFE;
        }
    }

    // Dirección (encoder) - crítico
    if(cfg.steeringEnabled) {
        if(!Steering::initOK()) {
            System::logError(200);
            Logger::error("SelfTest: CRÍTICO - encoder dirección no responde");
            h.steeringOK = false;
            h.ok = false;
            mode = OperationMode::MODE_SAFE;
        }
        
        // Motor dirección - advertencia pero no crítico
        if(!SteeringMotor::initOK()) {
            Logger::warn("SelfTest: motor dirección no responde (no crítico en arranque)");
            h.steeringOK = false;
            if (mode == OperationMode::MODE_FULL) {
                mode = OperationMode::MODE_DEGRADED;
            }
        }
    }

    // Palanca de cambios (crítico para arranque seguro)
    if(!Shifter::initOK()) {
        System::logError(650);
        Logger::error("SelfTest: CRÍTICO - palanca de cambios no inicializada");
        h.ok = false;
        mode = OperationMode::MODE_SAFE;
    } else {
        auto gear = Shifter::get().gear;
        
        // Validate gear is in valid range
        if(gear < Shifter::P || gear > Shifter::R) {
            System::logError(652);
            Logger::error("SelfTest: CRÍTICO - palanca en estado inválido");
            h.ok = false;
            mode = OperationMode::MODE_SAFE;
        } else if(gear != Shifter::P) {
            System::logError(651);
            Logger::errorf("SelfTest: CRÍTICO - palanca debe estar en PARK (gear=%d)", static_cast<int>(gear));
            h.ok = false;
            mode = OperationMode::MODE_SAFE;
        }
    }

    // Relés (crítico)
    if(!Relays::initOK()) {
        System::logError(600);
        Logger::error("SelfTest: CRÍTICO - Relés no responden - modo seguro");
        h.ok = false;
        mode = OperationMode::MODE_SAFE;
    }
    
    // ========================================================================
    // COMPONENTES NO CRÍTICOS (solo advertencias)
    // ========================================================================
    
    // Tracción (no crítico)
    if(cfg.tractionEnabled) {
        if(!Traction::initOK()) {
            Logger::warn("SelfTest: módulo tracción no inicializado (no crítico)");
            if (mode == OperationMode::MODE_FULL) {
                mode = OperationMode::MODE_DEGRADED;
            }
        }
    }

    // DFPlayer (no crítico)
    if(!Audio::initOK()) {
        Logger::warn("SelfTest: DFPlayer no inicializado (no crítico)");
        if (mode == OperationMode::MODE_FULL) {
            mode = OperationMode::MODE_DEGRADED;
        }
    }

    // Establecer modo de operación según resultados
    SystemMode::setMode(mode);
    
    return h;
}

void System::update() {
    switch(currentState) {
        case PRECHECK: {
            auto h = selfTest();
            if(h.ok) {
                Logger::info("SelfTest OK → READY");
                currentState = READY;
            } else {
                Logger::errorf("SelfTest FAIL → ERROR");
                currentState = ERROR;
            }
        } break;

        case READY:
            Logger::info("System READY → RUN");
            currentState = RUN;
            break;

        case RUN:
            // Aquí se puede añadir lógica de watchdog o monitorización
            break;

        case ERROR:
            Relays::disablePower();
            break;

        case OFF:
        default:
            break;
    }
}

System::State System::getState() {
    return currentState;
}

// --- API de diagnóstico persistente ---
void System::logError(uint16_t code) {
    for(int i=0; i<cfg.errorCount; i++) {
        if(cfg.errors[i].code == code) return;
    }
    if(cfg.errorCount < Storage::Config::MAX_ERRORS) {
        cfg.errors[cfg.errorCount++] = {code, millis()};
    } else {
        for(int i=1; i<Storage::Config::MAX_ERRORS; i++)
            cfg.errors[i-1] = cfg.errors[i];
        cfg.errors[Storage::Config::MAX_ERRORS-1] = {code, millis()};
    }
    Storage::save(cfg);
}

const Storage::ErrorLog* System::getErrors() {
    return cfg.errors;
}

int System::getErrorCount() {
    return cfg.errorCount;
}

void System::clearErrors() {
    cfg.errorCount = 0;
    for(int i=0; i<Storage::Config::MAX_ERRORS; i++) {
        cfg.errors[i] = {0,0};
    }
    Storage::save(cfg);
}

bool System::hasError() {
    return currentState == ERROR || cfg.errorCount > 0;
}

// Diagnóstico de estado de inicialización (thread-safe)
bool System::isInitialized() {
    // Lectura de bool es atómica en ESP32, pero añadimos mutex por consistencia
    if (initMutex != nullptr && xSemaphoreTake(initMutex, pdMS_TO_TICKS(SystemInitConfig::MUTEX_CHECK_TIMEOUT_MS)) == pdTRUE) {
        bool state = systemInitialized;
        xSemaphoreGive(initMutex);
        return state;
    }
    // Fallback sin mutex
    return systemInitialized;
}
