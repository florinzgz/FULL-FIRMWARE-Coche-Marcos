#include "storage.h"
#include "logger.h"
#include "settings.h"
#include "system.h" // 🔒 v2.4.1: Para logError
#include <Preferences.h>

static Preferences prefs;

// Global config variable
Storage::Config cfg;

static const char *kNamespace = "vehicle";
static const char *kKeyBlob = "config";

// 🔒 v2.4.1: Magic number para detección de corrupción EEPROM
static const uint32_t MAGIC_NUMBER = 0xDEADBEEF;
static const char *kKeyMagic = "magic";

void Storage::init() {
  if (!prefs.begin(kNamespace, false)) {
    Logger::warn("Storage init: fallo al abrir namespace");
    System::logError(970); // código: fallo apertura storage
  } else {
    // 🔒 v2.10.8: Confirm storage initialization
    Logger::info("Storage init: EEPROM namespace abierto correctamente");
    
    // Verificar si hay datos guardados
    if (prefs.isKey(kKeyMagic)) {
      uint32_t magic = prefs.getUInt(kKeyMagic, 0);
      if (magic == MAGIC_NUMBER) {
        Logger::info("Storage init: Datos válidos detectados en EEPROM");
      } else {
        Logger::warn("Storage init: Magic number incorrecto - EEPROM puede estar corrupta");
      }
    } else {
      Logger::info("Storage init: EEPROM vacía o primera inicialización");
    }
  }
}

void Storage::defaults(Config &cfg) {
  // Pedal
  cfg.pedalMin = 200;  // ejemplo
  cfg.pedalMax = 3800; // ejemplo
  cfg.pedalCurve = 0;

  // Freno regenerativo
  cfg.regenPercent = REGEN_DEFAULT;

  // INA226 shunts (coeficiente de conversión corriente)
  cfg.shuntCoeff[0] = 0.0010f; // batería 100A
  cfg.shuntCoeff[1] = 0.0020f; // rueda FL 50A
  cfg.shuntCoeff[2] = 0.0020f; // rueda FR 50A
  cfg.shuntCoeff[3] = 0.0020f; // rueda RL 50A
  cfg.shuntCoeff[4] = 0.0020f; // rueda RR 50A
  cfg.shuntCoeff[5] = 0.0020f; // dirección 50A

  // Dirección
  cfg.steerZeroOffset = 0;

  // HUD
  cfg.showTemps = true;
  cfg.showEffort = true;
  cfg.displayBrightness = DISPLAY_BRIGHTNESS_DEFAULT; // Default brightness

  // Módulos
  cfg.audioEnabled = true;
  cfg.lightsEnabled = true;
  cfg.multimediaEnabled = true;
  cfg.tractionEnabled = true; // Módulo de tracción habilitado por defecto

  // Nuevos flags de tolerancia a fallos
  // ⚙️ Inicialmente deshabilitados para modo standalone (solo pantalla)
  // Habilitar cuando se conecten los sensores reales
  cfg.wheelSensorsEnabled = false;
  cfg.tempSensorsEnabled = false;
  cfg.currentSensorsEnabled = false;
  cfg.steeringEnabled = false;

  // 🔒 v2.8.6: Touch screen configuration
  cfg.touchEnabled = true; // Touch habilitado por defecto

  // 🔒 v2.9.0: Touch calibration defaults (from TouchCalibration namespace)
  cfg.touchCalibration[0] = 200;  // min_x (RAW_MIN)
  cfg.touchCalibration[1] = 3900; // max_x (RAW_MAX)
  cfg.touchCalibration[2] = 200;  // min_y (RAW_MIN)
  cfg.touchCalibration[3] = 3900; // max_y (RAW_MAX)
  cfg.touchCalibration[4] = 3;    // rotation (matches tft.setRotation(3))
  cfg.touchCalibrated = false;    // No calibration done yet (using defaults)

  // 🔒 v2.10.2: Límites de corriente configurables
  cfg.maxBatteryCurrentA = 100.0f; // Default: 100A para batería
  cfg.maxMotorCurrentA = 50.0f;    // Default: 50A por motor

  // 🔒 v2.4.2: Odómetro y mantenimiento
  cfg.odometer.totalKm = 0.0f;
  cfg.odometer.tripKm = 0.0f;
  cfg.odometer.lastServiceKm = 0.0f;
  cfg.odometer.lastServiceDate = 0;
  cfg.odometer.engineHours = 0;
  cfg.maintenanceIntervalKm = 500;   // Mantenimiento cada 500 km
  cfg.maintenanceIntervalDays = 180; // Mantenimiento cada 6 meses (180 días)

  // Errores persistentes
  cfg.errorCount = 0;
  for (int i = 0; i < Config::MAX_ERRORS; i++) {
    cfg.errors[i] = {0, 0};
  }

  // versión y checksum
  cfg.version = kConfigVersion;
  cfg.checksum = computeChecksum(cfg);
}

uint32_t Storage::computeChecksum(const Config &cfg) {
  // Simple FNV-1a sobre campos (excluye checksum)
  const uint32_t FNV_OFFSET = 2166136261u;
  const uint32_t FNV_PRIME = 16777619u;
  uint32_t h = FNV_OFFSET;

  auto mix = [&](const uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; ++i) {
      h ^= data[i];
      h *= FNV_PRIME;
    }
  };

  mix((uint8_t *)&cfg.pedalMin, sizeof(cfg.pedalMin));
  mix((uint8_t *)&cfg.pedalMax, sizeof(cfg.pedalMax));
  mix((uint8_t *)&cfg.pedalCurve, sizeof(cfg.pedalCurve));
  mix((uint8_t *)&cfg.regenPercent, sizeof(cfg.regenPercent));
  mix((uint8_t *)&cfg.shuntCoeff[0], sizeof(cfg.shuntCoeff));
  mix((uint8_t *)&cfg.steerZeroOffset, sizeof(cfg.steerZeroOffset));
  mix((uint8_t *)&cfg.showTemps, sizeof(cfg.showTemps));
  mix((uint8_t *)&cfg.showEffort, sizeof(cfg.showEffort));
  mix((uint8_t *)&cfg.displayBrightness, sizeof(cfg.displayBrightness));
  mix((uint8_t *)&cfg.audioEnabled, sizeof(cfg.audioEnabled));
  mix((uint8_t *)&cfg.lightsEnabled, sizeof(cfg.lightsEnabled));
  mix((uint8_t *)&cfg.multimediaEnabled, sizeof(cfg.multimediaEnabled));
  mix((uint8_t *)&cfg.tractionEnabled, sizeof(cfg.tractionEnabled));

  // Nuevos flags
  mix((uint8_t *)&cfg.wheelSensorsEnabled, sizeof(cfg.wheelSensorsEnabled));
  mix((uint8_t *)&cfg.tempSensorsEnabled, sizeof(cfg.tempSensorsEnabled));
  mix((uint8_t *)&cfg.currentSensorsEnabled, sizeof(cfg.currentSensorsEnabled));
  mix((uint8_t *)&cfg.steeringEnabled, sizeof(cfg.steeringEnabled));

  // 🔒 v2.8.6: Touch screen configuration
  mix((uint8_t *)&cfg.touchEnabled, sizeof(cfg.touchEnabled));

  // 🔒 v2.9.0: Touch calibration data
  mix((uint8_t *)&cfg.touchCalibration[0], sizeof(cfg.touchCalibration));
  mix((uint8_t *)&cfg.touchCalibrated, sizeof(cfg.touchCalibrated));

  // 🔒 v2.4.2: Odómetro y mantenimiento
  mix((uint8_t *)&cfg.odometer, sizeof(cfg.odometer));
  mix((uint8_t *)&cfg.maintenanceIntervalKm, sizeof(cfg.maintenanceIntervalKm));
  mix((uint8_t *)&cfg.maintenanceIntervalDays,
      sizeof(cfg.maintenanceIntervalDays));

  // Errores persistentes
  mix((uint8_t *)&cfg.errorCount, sizeof(cfg.errorCount));
  mix((uint8_t *)&cfg.errors[0], sizeof(cfg.errors));

  mix((uint8_t *)&cfg.version, sizeof(cfg.version));
  return h;
}

void Storage::load(Config &cfg) {
  // 🔒 v2.4.2: Verificar corrupción antes de cargar
  if (isCorrupted()) {
    Logger::error("Storage: EEPROM corrupta. Restaurando valores por defecto.");
    System::logError(975); // código: restauración automática
    defaults(cfg);
    save(cfg); // Guardar defaults para próximo arranque
    return;
  }

  // Datos verificados - cargar configuración
  prefs.getBytes(kKeyBlob, &cfg, sizeof(Config));

  // 🔒 v2.9.5: MIGRATION FIX - Force enable touch if disabled
  // Older configs may have touchEnabled=false causing circular dependency issue
  // (can't access hidden menu without touch, but can't calibrate touch without
  // menu)
  if (!cfg.touchEnabled) {
    Logger::warn("Storage: Touch was disabled - force enabling for usability");
    cfg.touchEnabled = true;
    save(cfg); // Save the fix permanently
    Logger::info("Storage: Touch enabled and saved to EEPROM");
  }

  Logger::infof("Storage: Config cargada OK (v%u, checksum 0x%08X)",
                cfg.version, cfg.checksum);
}

bool Storage::save(const Config &cfgIn) {
  Config tmp = cfgIn;
  tmp.version = kConfigVersion;
  tmp.checksum = computeChecksum(tmp);

  // 🔒 v2.4.1: Guardar magic number primero
  if (!prefs.putUInt(kKeyMagic, MAGIC_NUMBER)) {
    Logger::error("Storage save: fallo al escribir magic number");
    System::logError(980); // código: fallo escritura magic
    return false;
  }

  size_t written = prefs.putBytes(kKeyBlob, &tmp, sizeof(Config));
  if (written != sizeof(Config)) {
    Logger::errorf("Storage save: fallo al escribir (%u bytes vs %u esperados)",
                   written, sizeof(Config));
    System::logError(981); // código: fallo escritura config
    return false;
  }

  Logger::infof("Storage: Config guardada OK (v%u, checksum 0x%08X)",
                tmp.version, tmp.checksum);
  return true;
}

void Storage::resetToFactory() {
  prefs.clear();
  Logger::warn("Storage: reset a valores de fábrica");
  System::logError(985); // código: reset a fábrica (info)
}

// 🔒 v2.4.2: Función para verificar corrupción de EEPROM
bool Storage::isCorrupted() {
  // Verificar magic number
  uint32_t magic = prefs.getUInt(kKeyMagic, 0);
  if (magic != MAGIC_NUMBER) {
    Logger::warnf("Storage: magic number inválido (0x%08X vs 0x%08X)", magic,
                  MAGIC_NUMBER);
    return true;
  }

  // Verificar tamaño de datos
  size_t len = prefs.getBytesLength(kKeyBlob);
  if (len != sizeof(Config)) {
    Logger::warnf("Storage: tamaño inválido (%u vs %u)", len, sizeof(Config));
    return true;
  }

  // Leer config temporal para verificar checksum
  Config tempCfg;
  prefs.getBytes(kKeyBlob, &tempCfg, sizeof(Config));

  // Verificar versión
  if (tempCfg.version != kConfigVersion) {
    Logger::warnf("Storage: versión inválida (%u vs %u)", tempCfg.version,
                  kConfigVersion);
    return true;
  }

  // Calcular y comparar checksum
  uint32_t storedChecksum = tempCfg.checksum;
  uint32_t currentChecksum = computeChecksum(tempCfg);

  if (storedChecksum != currentChecksum) {
    Logger::warnf("Storage corrupta: checksum esperado=0x%08X, actual=0x%08X",
                  storedChecksum, currentChecksum);
    return true;
  }

  return false; // Datos válidos
}

// ============================================================================
// 🔒 v2.4.2: Funciones de Odómetro y Mantenimiento
// ============================================================================

void Storage::updateOdometer(float distanceKm) {
  if (distanceKm <= 0.0f)
    return;

  cfg.odometer.totalKm += distanceKm;
  cfg.odometer.tripKm += distanceKm;

  // Guardar cada 0.1 km para evitar escrituras excesivas
  static float lastSavedKm = 0.0f;
  if (cfg.odometer.totalKm - lastSavedKm >= 0.1f) {
    save(cfg);
    lastSavedKm = cfg.odometer.totalKm;
  }
}

void Storage::resetTripOdometer() {
  cfg.odometer.tripKm = 0.0f;
  save(cfg);
  Logger::info("Odómetro parcial reseteado");
}

void Storage::recordMaintenance() {
  cfg.odometer.lastServiceKm = cfg.odometer.totalKm;
  cfg.odometer.lastServiceDate =
      millis() / 1000; // Segundos desde arranque (usar RTC si disponible)
  save(cfg);
  Logger::infof("Mantenimiento registrado a %.1f km", cfg.odometer.totalKm);
}

bool Storage::isMaintenanceDue() {
  // Verificar por kilómetros
  float kmSinceService = cfg.odometer.totalKm - cfg.odometer.lastServiceKm;
  if (kmSinceService >= cfg.maintenanceIntervalKm) {
    Logger::warnf("⚠️ MANTENIMIENTO PENDIENTE: %.1f km desde último servicio "
                  "(intervalo: %u km)",
                  kmSinceService, cfg.maintenanceIntervalKm);
    return true;
  }

  // 🔒 v2.10.3: Time-based maintenance check not implemented
  // RTC module not present in current hardware configuration
  // Limitation: Maintenance tracking is currently based only on odometer readings due to lack of a real-time clock (RTC) module. Time-based maintenance is not supported and may be necessary for some components (e.g., battery, brake fluid). See future enhancement below.
  // Future enhancement: add DS3231 RTC module for time-based maintenance alerts

  return false;
}

float Storage::getKmUntilService() {
  float kmSinceService = cfg.odometer.totalKm - cfg.odometer.lastServiceKm;
  float kmRemaining = cfg.maintenanceIntervalKm - kmSinceService;
  return (kmRemaining > 0.0f) ? kmRemaining : 0.0f;
}