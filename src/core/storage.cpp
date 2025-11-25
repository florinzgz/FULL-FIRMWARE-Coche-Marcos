#include "storage.h"
#include <Preferences.h>
#include "settings.h"
#include "logger.h"
#include "system.h"  // 🔒 v2.4.1: Para logError

static Preferences prefs;

// Global config variable
Storage::Config cfg;

static const char *kNamespace = "vehicle";
static const char *kKeyBlob   = "config";

// 🔒 v2.4.1: Magic number para detección de corrupción EEPROM
static const uint32_t MAGIC_NUMBER = 0xDEADBEEF;
static const char *kKeyMagic = "magic";

void Storage::init() {
    if (!prefs.begin(kNamespace, false)) {
        Logger::warn("Storage init: fallo al abrir namespace");
        System::logError(970);  // código: fallo apertura storage
    }
}

void Storage::defaults(Config &cfg) {
    // Pedal
    cfg.pedalMin = 200;           // ejemplo
    cfg.pedalMax = 3800;          // ejemplo
    cfg.pedalCurve = 0;

    // Freno regenerativo
    cfg.regenPercent = REGEN_DEFAULT;

    // INA226 shunts (coeficiente de conversión corriente)
    cfg.shuntCoeff[0] = 0.0010f;  // batería 100A
    cfg.shuntCoeff[1] = 0.0020f;  // rueda FL 50A
    cfg.shuntCoeff[2] = 0.0020f;  // rueda FR 50A
    cfg.shuntCoeff[3] = 0.0020f;  // rueda RL 50A
    cfg.shuntCoeff[4] = 0.0020f;  // rueda RR 50A
    cfg.shuntCoeff[5] = 0.0020f;  // dirección 50A

    // Dirección
    cfg.steerZeroOffset = 0;

    // HUD
    cfg.showTemps  = true;
    cfg.showEffort = true;
    cfg.displayBrightness = 200;  // Brillo por defecto (200 de 255)

    // Módulos
    cfg.audioEnabled      = true;
    cfg.lightsEnabled     = true;
    cfg.multimediaEnabled = true;
    cfg.tractionEnabled   = true;  // Módulo de tracción habilitado por defecto

    // Nuevos flags de tolerancia a fallos
    // ⚙️ Inicialmente deshabilitados para modo standalone (solo pantalla)
    // Habilitar cuando se conecten los sensores reales
    cfg.wheelSensorsEnabled    = false;
    cfg.tempSensorsEnabled     = false;
    cfg.currentSensorsEnabled  = false;
    cfg.steeringEnabled        = false;

    // Errores persistentes
    cfg.errorCount = 0;
    for(int i=0; i<Config::MAX_ERRORS; i++) {
        cfg.errors[i] = {0,0};
    }

    // versión y checksum
    cfg.version = kConfigVersion;
    cfg.checksum = computeChecksum(cfg);
}

uint32_t Storage::computeChecksum(const Config &cfg) {
    // Simple FNV-1a sobre campos (excluye checksum)
    const uint32_t FNV_OFFSET = 2166136261u;
    const uint32_t FNV_PRIME  = 16777619u;
    uint32_t h = FNV_OFFSET;

    auto mix = [&](const uint8_t *data, size_t len) {
        for(size_t i=0; i<len; ++i) { h ^= data[i]; h *= FNV_PRIME; }
    };

    mix((uint8_t*)&cfg.pedalMin, sizeof(cfg.pedalMin));
    mix((uint8_t*)&cfg.pedalMax, sizeof(cfg.pedalMax));
    mix((uint8_t*)&cfg.pedalCurve, sizeof(cfg.pedalCurve));
    mix((uint8_t*)&cfg.regenPercent, sizeof(cfg.regenPercent));
    mix((uint8_t*)&cfg.shuntCoeff[0], sizeof(cfg.shuntCoeff));
    mix((uint8_t*)&cfg.steerZeroOffset, sizeof(cfg.steerZeroOffset));
    mix((uint8_t*)&cfg.showTemps, sizeof(cfg.showTemps));
    mix((uint8_t*)&cfg.showEffort, sizeof(cfg.showEffort));
    mix((uint8_t*)&cfg.displayBrightness, sizeof(cfg.displayBrightness));
    mix((uint8_t*)&cfg.audioEnabled, sizeof(cfg.audioEnabled));
    mix((uint8_t*)&cfg.lightsEnabled, sizeof(cfg.lightsEnabled));
    mix((uint8_t*)&cfg.multimediaEnabled, sizeof(cfg.multimediaEnabled));
    mix((uint8_t*)&cfg.tractionEnabled, sizeof(cfg.tractionEnabled));

    // Nuevos flags
    mix((uint8_t*)&cfg.wheelSensorsEnabled, sizeof(cfg.wheelSensorsEnabled));
    mix((uint8_t*)&cfg.tempSensorsEnabled, sizeof(cfg.tempSensorsEnabled));
    mix((uint8_t*)&cfg.currentSensorsEnabled, sizeof(cfg.currentSensorsEnabled));
    mix((uint8_t*)&cfg.steeringEnabled, sizeof(cfg.steeringEnabled));

    // Errores persistentes
    mix((uint8_t*)&cfg.errorCount, sizeof(cfg.errorCount));
    mix((uint8_t*)&cfg.errors[0], sizeof(cfg.errors));

    mix((uint8_t*)&cfg.version, sizeof(cfg.version));
    return h;
}

void Storage::load(Config &cfg) {
    // 🔒 v2.4.1: Verificar magic number primero
    uint32_t magic = prefs.getUInt(kKeyMagic, 0);
    if (magic != MAGIC_NUMBER) {
        Logger::error("Storage load: magic number inválido - EEPROM corrupta o no inicializada");
        System::logError(971);  // código: EEPROM corrupta
        defaults(cfg);
        return;
    }
    
    size_t len = prefs.getBytesLength(kKeyBlob);
    if(len != sizeof(Config)) {
        Logger::warnf("Storage load: tamaño inválido (%u vs %u), usando defaults", len, sizeof(Config));
        System::logError(972);  // código: tamaño config inválido
        defaults(cfg);
        return;
    }
    prefs.getBytes(kKeyBlob, &cfg, sizeof(Config));

    // validar versión y checksum
    if(cfg.version != kConfigVersion) {
        Logger::warnf("Storage load: versión inválida (%u vs %u), usando defaults", cfg.version, kConfigVersion);
        System::logError(973);  // código: versión config inválida
        defaults(cfg);
        return;
    }
    uint32_t chk = computeChecksum(cfg);
    if(chk != cfg.checksum) {
        Logger::errorf("Storage load: checksum inválido (0x%08X vs 0x%08X) - datos corruptos", chk, cfg.checksum);
        System::logError(974);  // código: checksum inválido
        defaults(cfg);
        return;
    }
    
    Logger::infof("Storage: Config cargada OK (v%u, checksum 0x%08X)", cfg.version, cfg.checksum);
}

bool Storage::save(const Config &cfgIn) {
    Config tmp = cfgIn;
    tmp.version = kConfigVersion;
    tmp.checksum = computeChecksum(tmp);
    
    // 🔒 v2.4.1: Guardar magic number primero
    if (!prefs.putUInt(kKeyMagic, MAGIC_NUMBER)) {
        Logger::error("Storage save: fallo al escribir magic number");
        System::logError(980);  // código: fallo escritura magic
        return false;
    }
    
    size_t written = prefs.putBytes(kKeyBlob, &tmp, sizeof(Config));
    if(written != sizeof(Config)) {
        Logger::errorf("Storage save: fallo al escribir (%u bytes vs %u esperados)", written, sizeof(Config));
        System::logError(981);  // código: fallo escritura config
        return false;
    }
    
    Logger::infof("Storage: Config guardada OK (v%u, checksum 0x%08X)", tmp.version, tmp.checksum);
    return true;
}

void Storage::resetToFactory() {
    prefs.clear();
    Logger::warn("Storage: reset a valores de fábrica");
    System::logError(985);  // código: reset a fábrica (info)
}