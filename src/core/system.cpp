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

extern Storage::Config cfg;

static System::State currentState = System::OFF;

void System::init() {
    Logger::info("System init: entrando en PRECHECK");
    currentState = PRECHECK;
    
    // 🔒 v2.10.8: Enhanced diagnostic information
    Logger::infof("System init: Estado inicial OK");
    Logger::infof("System init: Free heap: %u bytes", ESP.getFreeHeap());
    
    #ifdef ARDUINO_ESP32S3_DEV
    Logger::info("System init: Platform ESP32-S3 detected");
    #endif
    
    // 🔒 v2.11.0: Cargar y aplicar ajustes persistentes en arranque
    Logger::info("System init: Cargando configuración persistente");
    if (!EEPROMPersistence::init()) {
        Logger::warn("System init: EEPROM persistence init failed, using defaults");
    }
    
    // Cargar configuración general
    EEPROMPersistence::GeneralSettings settings;
    if (EEPROMPersistence::loadGeneralSettings(settings)) {
        Logger::info("System init: Configuración general cargada");
        
        // Aplicar toggles de módulos
        ABSSystem::setEnabled(settings.absEnabled);
        Logger::infof("System init: ABS %s", settings.absEnabled ? "enabled" : "disabled");
        
        TCSSystem::setEnabled(settings.tcsEnabled);
        Logger::infof("System init: TCS %s", settings.tcsEnabled ? "enabled" : "disabled");
        
        RegenAI::setEnabled(settings.regenEnabled);
        Logger::infof("System init: Regen %s", settings.regenEnabled ? "enabled" : "disabled");
    } else {
        Logger::warn("System init: No se pudo cargar configuración general, usando defaults");
    }
    
    // Cargar y aplicar configuración de LEDs
    EEPROMPersistence::LEDConfig ledConfig;
    if (EEPROMPersistence::loadLEDConfig(ledConfig)) {
        Logger::info("System init: Configuración LED cargada");
        LEDController::setEnabled(ledConfig.enabled);
        LEDController::setBrightness(ledConfig.brightness);
        
        auto &cfgLed = LEDController::getConfig();
        cfgLed.updateRateMs = 50; // Default update rate
        
        Logger::infof("System init: LEDs %s, brightness %d", 
                      ledConfig.enabled ? "enabled" : "disabled", 
                      ledConfig.brightness);
    } else {
        Logger::warn("System init: No se pudo cargar configuración LED, usando defaults");
    }
    
    // Habilitar características de seguridad de obstáculos
    // Usar configuración por defecto ya que no hay persistencia específica para esto
    ObstacleSafety::enableParkingAssist(true);
    ObstacleSafety::enableCollisionAvoidance(true);
    ObstacleSafety::enableBlindSpot(true);
    Logger::info("System init: Seguridad de obstáculos habilitada");
}

System::Health System::selfTest() {
    Health h{true,true,true,true,true};

    // Actualizar entradas críticas antes de validar estados
    Pedal::update();
    Shifter::update();
    Steering::update();

    // Pedal (crítico)
    if(!Pedal::initOK()) {
        System::logError(100);
        Logger::errorf("SelfTest: pedal no responde");
        h.ok = false;
    } else {
        if(Pedal::get().percent > 5.0f) {
            Logger::warnf("SelfTest: pedal no está en reposo (%.1f%%)", Pedal::get().percent);
            h.ok = false;
        }
    }

    // Dirección (encoder)
    if(cfg.steeringEnabled) {
        if(!Steering::initOK()) {
            System::logError(200);
            Logger::errorf("SelfTest: encoder dirección no responde");
            h.steeringOK = false;
            h.ok = false;
        }
        
        // 🔒 v2.4.0: Verificar motor dirección también
        // 🔒 v2.11.0: Motor dirección NO es crítico - se registra como advertencia
        // NOTA CRÍTICA: El motor de dirección NO es crítico para arranque inicial porque:
        // 1. Puede inicializarse tardíamente una vez que I2C esté estable
        // 2. El vehículo está PARADO durante selfTest (marcha P obligatoria)
        // 3. El sistema de relés cortará potencia si hay fallo grave
        // Sin embargo, se marca h.steeringOK = false para indicar problema parcial
        if(!SteeringMotor::initOK()) {
            Logger::warn("SelfTest: motor dirección no responde (no crítico en arranque)");
            h.steeringOK = false;
            // NO registrar como error crítico ni marcar h.ok = false
            // El vehículo puede arrancar pero con precaución
        }
    }

    // Palanca de cambios (crítico para arranque seguro)
    if(!Shifter::initOK()) {
        System::logError(650);
        Logger::error("SelfTest: palanca de cambios no inicializada");
        h.ok = false;
    } else {
        auto gear = Shifter::get().gear;
        if(gear != Shifter::P) {
            System::logError(651);
            Logger::errorf("SelfTest: palanca debe estar en PARK para arrancar (gear=%d)", static_cast<int>(gear));
            h.ok = false;
        }
    }

    // Corriente
    if(cfg.currentSensorsEnabled) {
        if(!Sensors::currentInitOK()) {
            System::logError(300);
            Logger::errorf("SelfTest: INA226 no responde");
            h.currentOK = false;
            h.ok = false;
        }
    }

    // Temperatura
    if(cfg.tempSensorsEnabled) {
        if(!Sensors::temperatureInitOK()) {
            System::logError(400);
            Logger::errorf("SelfTest: DS18B20 no responde");
            h.tempsOK = false;
            h.ok = false;
        }
    }

    // Ruedas
    if(cfg.wheelSensorsEnabled) {
        if(!Sensors::wheelsInitOK()) {
            System::logError(500);
            Logger::errorf("SelfTest: sensores de rueda no responden");
            h.wheelsOK = false;
            h.ok = false;
        }
    }

    // Relés (crítico)
    if(!Relays::initOK()) {
        System::logError(600);
        Logger::errorf("SelfTest: relés no responden");
        h.ok = false;
    }
    
    // 🔒 v2.4.0: Tracción (no crítico pero loggear)
    // 🔒 v2.11.0: Tracción NO bloquea arranque - solo advertencia
    if(cfg.tractionEnabled) {
        if(!Traction::initOK()) {
            Logger::warn("SelfTest: módulo tracción no inicializado (no crítico)");
            // No marcar como fallo crítico - vehículo puede arrancar
            // El sistema de tracción puede recuperarse después
        }
    }

    // 🔒 v2.11.0: DFPlayer (no crítico) - NO bloquea arranque
    // El audio es importante pero no esencial para operación del vehículo
    if(!Audio::initOK()) {
        Logger::warn("SelfTest: DFPlayer no inicializado (no crítico)");
        // No marcar como fallo crítico - vehículo puede operar sin audio
    }

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
