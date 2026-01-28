#include "shifter.h"
#include "alerts.h"
#include "dfplayer.h"
#include "logger.h"
#include "mcp23017_manager.h"
#include "pins.h"
#include "sensors.h" // 🔒 CRITICAL v2.18.3: For wheel speed validation

static Shifter::State s = {Shifter::P, false};

// 🔒 v2.5.0: Flag de inicialización
static bool initialized = false;

// 🔒 CORRECCIÓN CRÍTICA: Debounce para prevenir lecturas erróneas por rebotes
static constexpr uint32_t DEBOUNCE_MS = 50;
static uint32_t lastChangeMs = 0;
static uint8_t stableReadings = 0;
static Shifter::Gear pendingGear = Shifter::P;

// 🔒 CRITICAL v2.18.3: Reverse safety threshold
// No permitir cambio a R si la velocidad supera 3 km/h (protección mecánica)
static constexpr float MAX_SPEED_FOR_REVERSE = 3.0f;

// 🔒 v2.18.3: Cooldown para prevenir spam de alertas de seguridad
static uint32_t lastSafetyBlockMs = 0;
static constexpr uint32_t SAFETY_BLOCK_COOLDOWN_MS = 2000; // 2 segundos

// ✅ v2.3.0: Todo el shifter ahora vía MCP23017 (GPIOB0-B4, pines 8-12
// consecutivos)
static MCP23017Manager *mcpManager = nullptr;
static bool mcpAvailable = false;

// Lee entrada del MCP23017 con pull-up interno (LOW = activo)
// Los optoacopladores HY-M158 invierten: activo = LED encendido = transistor
// conduce = LOW
static bool readMcpPin(uint8_t pin) {
  if (!mcpAvailable || mcpManager == nullptr) return false;
  return mcpManager->digitalRead(pin) == 0;
}

static void announce(Shifter::Gear g) {
  // 🔒 v2.10.3: Gear-specific audio feedback using existing audio library
  // Different audio tracks distinguish between gears for driver feedback
  switch (g) {
  case Shifter::Gear::P:
    Alerts::play(Audio::AUDIO_MODULO_OK); // Confirmation beep for park
    break;
  case Shifter::Gear::R:
    // Use dedicated reverse gear audio - "Marcha atrás activada"
    Alerts::play(Audio::AUDIO_MARCHA_R);
    break;
  case Shifter::Gear::N:
    Alerts::play(Audio::AUDIO_MODULO_OK); // Neutral confirmation
    break;
  case Shifter::Gear::D1:
  case Shifter::Gear::D2:
    Alerts::play(Audio::AUDIO_MODULO_OK); // Drive mode confirmation
    break;
  }
}

void Shifter::init() {
  // ✅ v2.3.0: Todo el shifter ahora vía MCP23017 (GPIOB0-B4)
  // Pines consecutivos 8-12 para las 5 posiciones: P, R, N, D1, D2

  // Get shared MCP23017 manager instance (initialized by ControlManager)
  mcpManager = &MCP23017Manager::getInstance();

  if (mcpManager->isOK()) {
    // Configurar todos los pines del shifter como entrada con pull-up
    mcpManager->pinMode(MCP_PIN_SHIFTER_P, INPUT_PULLUP);  // GPIOB0: Park
    mcpManager->pinMode(MCP_PIN_SHIFTER_R, INPUT_PULLUP);  // GPIOB1: Reverse
    mcpManager->pinMode(MCP_PIN_SHIFTER_N, INPUT_PULLUP);  // GPIOB2: Neutral
    mcpManager->pinMode(MCP_PIN_SHIFTER_D1, INPUT_PULLUP); // GPIOB3: Drive 1
    mcpManager->pinMode(MCP_PIN_SHIFTER_D2, INPUT_PULLUP); // GPIOB4: Drive 2

    mcpAvailable = true;
    initialized = true; // Success
    Logger::info(
        "Shifter: MCP23017 GPIOB0-B4 configured via manager (pines 8-12)");
  } else {
    mcpAvailable = false;
    initialized = false; // Initialization failed
    Logger::error(700, "Shifter: MCP23017 manager not available!");
  }

  Logger::info("Shifter init completado (via MCP23017Manager)");
}

void Shifter::update() {
  // Si MCP23017 no está disponible, mantener última posición conocida
  if (!initialized) {
    s.changed = false;
    return;
  }

  Shifter::Gear detectedGear = s.gear;

  // 🔒 Lee cada posición del shifter vía MCP23017 (prioridad P > R > N > D1 >
  // D2) Orden de prioridad: Park es más importante que cualquier otra posición
  if (readMcpPin(MCP_PIN_SHIFTER_P))
    detectedGear = Shifter::P;
  else if (readMcpPin(MCP_PIN_SHIFTER_R))
    detectedGear = Shifter::R;
  else if (readMcpPin(MCP_PIN_SHIFTER_N))
    detectedGear = Shifter::N;
  else if (readMcpPin(MCP_PIN_SHIFTER_D1))
    detectedGear = Shifter::D1;
  else if (readMcpPin(MCP_PIN_SHIFTER_D2))
    detectedGear = Shifter::D2;

  uint32_t now = millis();

  // Implementar debounce: requiere lecturas estables durante DEBOUNCE_MS
  if (detectedGear != pendingGear) {
    // Nueva lectura detectada
    pendingGear = detectedGear;
    lastChangeMs = now;
    stableReadings = 1;
    s.changed = false;
  } else if (detectedGear != s.gear) {
    // La lectura coincide con pending pero aún no es el gear actual
    if (now - lastChangeMs >= DEBOUNCE_MS) {
      
      // 🔒 CRITICAL v2.18.3: PROTECCIÓN DE REVERSA (Failsafe mecánico)
      // Evita engranar reversa a alta velocidad para proteger engranajes y drivers BTS7960
      if (detectedGear == Shifter::R) {
        // Calcular velocidad promedio solo de ruedas funcionales
        float avgSpeed = 0.0f;
        int validWheels = 0;
        for (int i = 0; i < 4; i++) {
          if (Sensors::isWheelSensorOk(i)) {
            avgSpeed += Sensors::getWheelSpeed(i);
            validWheels++;
          }
        }
        
        // Requerir al menos 2 sensores válidos para validación confiable
        if (validWheels >= 2) {
          avgSpeed /= (float)validWheels;

          // Validar umbral de seguridad
          if (avgSpeed > MAX_SPEED_FOR_REVERSE) {
            uint32_t now = millis();
            // Solo loggear/alertar si ha pasado el cooldown
            if (now - lastSafetyBlockMs >= SAFETY_BLOCK_COOLDOWN_MS) {
              Logger::errorf("BLOQUEO SEGURIDAD: Intento de R a %.1f km/h (max %.1f km/h)", 
                            avgSpeed, MAX_SPEED_FOR_REVERSE);
              Alerts::play(Audio::AUDIO_ERROR);
              lastSafetyBlockMs = now;
            }
            detectedGear = Shifter::N; // Forzar Neutro por seguridad
            pendingGear = Shifter::N;  // También actualizar pending para evitar re-detección
          }
        } else {
          // Menos de 2 sensores funcionando: seguridad failsafe, bloquear reversa
          Logger::error("BLOQUEO SEGURIDAD: Sensores de rueda insuficientes para reversa");
          Alerts::play(Audio::AUDIO_ERROR);
          detectedGear = Shifter::N;
          pendingGear = Shifter::N;
        }
      }

      // Debounce completado, aceptar cambio
      s.gear = detectedGear;
      s.changed = true;
      announce(detectedGear);
      Logger::infof("Shifter: Cambio de marcha a %d", (int)detectedGear);
      stableReadings = 0;
    } else {
      stableReadings++;
      s.changed = false;
    }
  } else {
    // Lectura estable igual al gear actual
    s.changed = false;
  }
}

Shifter::State Shifter::get() { return s; }
void Shifter::setGear(Shifter::Gear g) {
  s.gear = g;
  s.changed = true;
  announce(g);
}

// 🔒 v2.5.0: Estado de inicialización
bool Shifter::initOK() { return initialized; }
