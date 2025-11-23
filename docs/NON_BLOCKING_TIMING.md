# Non-Blocking Timing Architecture

## Overview

This firmware uses a **non-blocking timing architecture** based on `millis()` to ensure the main loop never blocks. All modules control their own update rates independently, allowing the system to remain responsive to critical inputs while managing different refresh rates for different subsystems.

## Core Principle

**Never use `delay()` in production code.** The `delay()` function blocks the entire processor, preventing:
- Watchdog timer feeding (can cause system reset)
- Emergency input processing (safety critical)
- Sensor data collection
- Real-time control updates

## Main Loop Architecture

```cpp
void loop() {
    // Loop runs at maximum speed (~10,000 Hz theoretical)
    // Each module controls its own timing
    
    Watchdog::feed();              // Every iteration
    
    // High-priority inputs (every iteration)
    Pedal::update();               // Immediate response
    Steering::update();            // Immediate response
    Buttons::update();             // Immediate response
    
    // Sensors (internal timing)
    Sensors::updateCurrent();      // 50ms interval (20 Hz)
    Sensors::updateTemperature();  // 1000ms interval (1 Hz)
    Sensors::updateWheels();       // 100ms interval (10 Hz)
    
    // Control (every iteration with internal safety checks)
    Traction::update();            // Immediate response
    SteeringMotor::update();       // Immediate response
    
    // HUD (30 FPS timing)
    if (now - lastHudUpdate >= 33) {
        HUDManager::update();
        lastHudUpdate = now;
    }
    
    // No delay() - loop repeats immediately
}
```

## Standard Timing Pattern

All modules follow this pattern:

```cpp
void ModuleName::update() {
    static unsigned long lastUpdate = 0;
    const unsigned long UPDATE_INTERVAL = 50; // milliseconds
    
    unsigned long now = millis();
    
    // Guard: only update at specified interval
    if (now - lastUpdate < UPDATE_INTERVAL) {
        return; // Exit early, try again next loop
    }
    lastUpdate = now;
    
    // Guard: respect config flags
    if (!cfg.moduleEnabled) {
        // Reset to safe state
        return;
    }
    
    // Perform module update
    // ... module-specific code ...
}
```

## Module Update Rates

| Module | Rate | Interval | Reason |
|--------|------|----------|--------|
| **Watchdog** | Every loop | 0ms | Critical: prevent system reset |
| **Pedal** | Every loop | 0ms | Safety: immediate throttle response |
| **Steering** | Every loop | 0ms | Safety: immediate steering response |
| **Buttons** | Every loop | 0ms | User interaction responsiveness |
| **Current Sensors** | 20 Hz | 50ms | Balance: sensor accuracy vs. I²C bus load |
| **Temperature** | 1 Hz | 1000ms | Slow-changing, thermal inertia |
| **Wheels** | 10 Hz | 100ms | Adequate for speed calculation |
| **HUD** | 30 Hz | 33ms | Smooth visual updates |
| **LED Controller** | 20 Hz | 50ms | Configurable, smooth animations |
| **WiFi Reconnect** | 0.033 Hz | 30000ms | Network retry backoff |
| **Car Sensors (secondary)** | 2 Hz | 500ms | Non-critical telemetry |

## Special Cases

### 1. I²C Recovery
**File**: `src/core/i2c_recovery.cpp`

Uses `delayMicroseconds()` instead of `delay()`:
- Bus recovery pulses: `delayMicroseconds(100)` (~0.1ms)
- Retry settling: `delayMicroseconds(500)` (~0.5ms)

**Rationale**: Hardware timing requirements are sub-millisecond. Using microseconds keeps total blocking time under 2ms.

### 2. Watchdog Panic Handler
**File**: `src/core/watchdog.cpp`

```cpp
void esp_task_wdt_isr_user_handler(void) {
    digitalWrite(PIN_RELAY_MAIN, LOW);  // Emergency shutdown
    delay(1000);  // ACCEPTABLE: Allow relays to disengage
}
```

**Rationale**: This runs only during catastrophic failure (watchdog timeout). The `delay()` ensures relays physically disengage before system reset. This is the **only acceptable use** of `delay()` in the entire codebase.

### 3. WiFi Connection
**File**: `src/core/wifi_manager.cpp`

Non-blocking connection with timeout:
```cpp
// In init()
WiFi.begin(SSID, PASSWORD);
connectionInProgress = true;
connectionStartTime = millis();

// In update()
if (connectionInProgress) {
    if (WiFi.status() == WL_CONNECTED) {
        // Success
    } else if (millis() - connectionStartTime > 10000) {
        // Timeout after 10 seconds
    }
}
```

### 4. LED Emergency Flash
**File**: `src/lighting/led_controller.cpp`

Non-blocking state machine:
```cpp
void startEmergencyFlash(uint8_t count) {
    emergencyFlashActive = true;
    emergencyFlashCount = count;
    // ... state initialization ...
}

void update() {
    if (emergencyFlashActive) {
        if (millis() - lastToggle >= 100) {
            // Toggle LEDs
            // Increment counter
            // Check if complete
        }
        return; // Skip normal updates during emergency
    }
    // Normal LED updates
}
```

## Configuration Guards

All modules respect configuration flags from `Storage::Config`:

```cpp
void ModuleName::update() {
    if (!cfg.moduleEnabled) {
        // Set safe/neutral state
        return;
    }
    // Normal operation
}
```

**Config Flags**:
- `cfg.currentSensorsEnabled` - INA226 current sensors
- `cfg.tempSensorsEnabled` - DS18B20 temperature sensors
- `cfg.wheelSensorsEnabled` - Wheel speed encoders
- `cfg.steeringEnabled` - Steering encoder and motor
- `cfg.tractionEnabled` - Motor traction control

## Fallback Logic

Modules implement fallback behavior when disabled or failed:

```cpp
if (!sensorOk[i]) {
    lastValue[i] = 0.0f;  // Safe default
    System::logError(error_code);
    continue;  // Skip this sensor
}
```

**Safety principle**: Failed sensors should not block the system. Log errors and continue with safe defaults.

## Timing Overflow Safety

`millis()` overflows after ~49 days. Our timing logic handles this correctly:

```cpp
// CORRECT: handles overflow
if (millis() - lastUpdate >= INTERVAL) { }

// WRONG: breaks on overflow
if (millis() >= lastUpdate + INTERVAL) { }
```

The subtraction method works because of unsigned integer underflow wrapping.

## Performance Characteristics

- **Main loop frequency**: ~10,000 Hz (theoretical, depends on module workload)
- **Watchdog feed frequency**: ~10,000 Hz (every loop iteration)
- **Minimum sensor latency**: <1ms (processed next loop iteration)
- **HUD update latency**: ≤33ms (30 FPS guarantee)
- **I²C recovery time**: <2ms (microsecond delays only)

## Migration Guide: Converting delay() to millis()

### Before (Blocking):
```cpp
void blinkLED() {
    digitalWrite(LED, HIGH);
    delay(500);
    digitalWrite(LED, LOW);
    delay(500);
}
```

### After (Non-blocking):
```cpp
void updateLED() {
    static unsigned long lastToggle = 0;
    static bool ledState = false;
    
    if (millis() - lastToggle >= 500) {
        ledState = !ledState;
        digitalWrite(LED, ledState);
        lastToggle = millis();
    }
}
```

## Best Practices

1. **Always use static for timing variables** - they persist between calls
2. **Use millis() for timing** - never delay()
3. **Guard with config flags** - respect module enable/disable
4. **Implement fallback logic** - failed sensors shouldn't block
5. **Document update rates** - help future maintainers understand timing
6. **Use delayMicroseconds() for sub-ms hardware timing** - if absolutely necessary
7. **Feed watchdog frequently** - at least every 100ms, preferably every loop

## Debugging Timing Issues

### Check Loop Frequency
```cpp
static unsigned long lastDebug = 0;
static unsigned long loopCount = 0;

loopCount++;
if (millis() - lastDebug >= 1000) {
    Serial.printf("Loop frequency: %lu Hz\n", loopCount);
    loopCount = 0;
    lastDebug = millis();
}
```

### Check Module Update Times
```cpp
void ModuleName::update() {
    unsigned long start = micros();
    
    // ... module code ...
    
    unsigned long duration = micros() - start;
    if (duration > 1000) {  // Warn if >1ms
        Logger::warnf("Module took %lu µs", duration);
    }
}
```

## Related Files

- `src/main.cpp` - Main loop architecture
- `src/hud/hud_manager.cpp` - 30 FPS timing example
- `src/lighting/led_controller.cpp` - State machine timing example
- `src/sensors/current.cpp` - Sensor timing with config guards
- `src/core/wifi_manager.cpp` - Async connection example
- `include/settings.h` - System-wide timing constants

## Summary

This non-blocking architecture ensures:
- ✅ System remains responsive at all times
- ✅ Watchdog never times out due to blocking code
- ✅ Safety-critical inputs processed immediately  
- ✅ Different subsystems run at optimal rates
- ✅ Failed sensors don't block the system
- ✅ Configuration changes take effect immediately

**Remember**: The only acceptable `delay()` is in the watchdog panic handler. Everything else must be non-blocking.
