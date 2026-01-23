# Arduino ESP32 API Usage Guidelines

## Overview

This document clarifies the proper use of ESP-IDF and Arduino APIs in this codebase to maintain framework boundaries and ensure future compatibility.

## Framework Architecture

```
┌─────────────────────────────────────┐
│   Application Code (src/)           │
│   - Use Arduino APIs when available │
│   - Use FreeRTOS via Arduino        │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│   Arduino ESP32 Framework           │
│   - Provides Arduino APIs           │
│   - Exposes FreeRTOS primitives     │
│   - Wraps ESP-IDF functionality     │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│   ESP-IDF (Espressif IoT Dev FW)   │
│   - Low-level hardware abstraction  │
│   - FreeRTOS RTOS kernel           │
│   - Should NOT be used directly     │
└─────────────────────────────────────┘
```

## Allowed FreeRTOS Usage

### ✅ ALLOWED: FreeRTOS via Arduino Framework

The Arduino ESP32 framework **officially exposes** FreeRTOS primitives for multithreading. These are **part of the Arduino API** on ESP32:

```cpp
// CORRECT: Include FreeRTOS headers for ESP32 Arduino
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

// These are Arduino ESP32 APIs:
SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
QueueHandle_t queue = xQueueCreate(10, sizeof(int));
xTaskCreate(myTask, "TaskName", 4096, NULL, 1, NULL);
```

**Rationale**: 
- FreeRTOS is the RTOS kernel on ESP32
- Arduino ESP32 provides no alternative to FreeRTOS for multithreading
- These APIs are **stable across Arduino ESP32 versions**
- Arduino ESP32 documentation explicitly covers FreeRTOS usage

### ❌ FORBIDDEN: Direct ESP-IDF APIs

Do NOT bypass Arduino framework to call ESP-IDF APIs directly:

```cpp
// WRONG: Direct ESP-IDF includes
#include <esp_system.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <driver/gpio.h>

// WRONG: ESP-IDF functions
esp_restart();                    // Use ESP.restart() instead
nvs_flash_init();                 // Use Preferences library instead
gpio_set_direction(GPIO_NUM_5);   // Use pinMode() instead
```

**Why forbidden**:
- Breaks encapsulation
- May conflict with Arduino initialization
- Not guaranteed stable across ESP-IDF versions
- Arduino framework may change underlying IDF version

## Memory and PSRAM

### ✅ ALLOWED: Arduino Memory APIs

```cpp
#include <Arduino.h>

// Heap management
uint32_t freeHeap = ESP.getFreeHeap();
uint32_t heapSize = ESP.getHeapSize();

// PSRAM management  
uint32_t freePsram = ESP.getFreePsram();
uint32_t psramSize = ESP.getPsramSize();
bool hasPsram = psramFound();

// Allocate in PSRAM (Arduino way)
void* ptr = ps_malloc(size);  // Allocate in PSRAM if available
```

### ❌ FORBIDDEN: Direct ESP-IDF Memory APIs

```cpp
// WRONG: ESP-IDF heap APIs
#include <esp_heap_caps.h>
heap_caps_get_free_size(MALLOC_CAP_SPIRAM);  // Use ESP.getFreePsram()
```

## Configuration (sdkconfig)

### ✅ ALLOWED: Build-time Configuration

Configuration through PlatformIO build flags:

```ini
# platformio.ini
build_flags =
    -DBOARD_HAS_PSRAM        # Custom app flag
    -DCORE_DEBUG_LEVEL=3     # Arduino framework flag
```

### ✅ ALLOWED: sdkconfig Patching (TEMPORARY)

Our `tools/patch_arduino_sdkconfig.py` script:
- **Only modifies compile-time headers** (sdkconfig.h)
- **Does NOT include ESP-IDF headers** (uses only Python)
- **Does NOT activate runtime options** (no IDF API calls)
- **Temporary workaround** until Arduino framework fixes defaults

### ❌ FORBIDDEN: Runtime IDF Configuration

```cpp
// WRONG: Runtime ESP-IDF config
#include <esp_idf_version.h>
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    // Version-specific code
#endif
```

## PSRAM Configuration

### Current Setup (v2.17.3)

PSRAM is configured at **compile-time** via `sdkconfig/n16r8.defaults`:

```ini
# PSRAM hardware configuration
CONFIG_SPIRAM=y                    # Enable PSRAM
CONFIG_SPIRAM_MODE_QUAD=y          # Quad SPI mode
CONFIG_SPIRAM_SPEED_80M=y          # 80MHz speed

# PSRAM memory test DISABLED for fast boot
CONFIG_SPIRAM_MEMTEST=n            # Skip comprehensive test at boot
                                   # Trade-off: Detect bad PSRAM at runtime
                                   # instead of boot time

# Memory allocation strategy
CONFIG_SPIRAM_USE_MALLOC=y         # Allow malloc() to use PSRAM
CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=16384   # Keep small allocs in RAM
CONFIG_SPIRAM_MALLOC_RESERVE_INTERNAL=32768 # Reserve RAM for stack/ISR
```

**Why memory test is disabled**:
- Memory test can take >3 seconds on some hardware
- Exceeds watchdog timeout causing bootloop
- PSRAM still fully functional without test
- Bad PSRAM will be detected during runtime usage

### Runtime PSRAM Detection

```cpp
// CORRECT: Arduino API for PSRAM
#include <Arduino.h>

void checkPsram() {
    if (psramFound()) {
        uint32_t psramSize = ESP.getPsramSize();
        uint32_t freePsram = ESP.getFreePsram();
        
        Logger::infof("PSRAM: %u MB total, %u MB free",
                     psramSize / 1024 / 1024,
                     freePsram / 1024 / 1024);
    } else {
        Logger::error("PSRAM not detected");
        // System continues with internal RAM only
    }
}
```

## Watchdog Configuration

### Compile-time Configuration (sdkconfig)

```ini
# Interrupt watchdog (catches ISR hangs)
CONFIG_ESP_INT_WDT=y
CONFIG_ESP_INT_WDT_TIMEOUT_MS=5000  # 5 second timeout

# Task watchdog (catches task hangs)
CONFIG_ESP_TASK_WDT=y
CONFIG_ESP_TASK_WDT_TIMEOUT_S=5
```

### ❌ FORBIDDEN: Runtime Watchdog Control

```cpp
// WRONG: ESP-IDF watchdog APIs
#include <esp_task_wdt.h>
esp_task_wdt_init(10, true);  // Direct IDF call
```

## Migration Guidelines

If you find code violating these guidelines:

1. **Check for Arduino equivalent**: Most ESP-IDF APIs have Arduino wrappers
2. **Use FreeRTOS if needed**: Multithreading via FreeRTOS is OK on ESP32
3. **Document exceptions**: If you MUST use IDF API, document why
4. **Isolate in wrapper**: Create abstraction layer for future compatibility

## Current Status

### ✅ Compliant Areas
- Memory management (uses `ESP.getFreeHeap()`, `ESP.getFreePsram()`)
- PSRAM detection (uses `psramFound()`)
- GPIO control (uses Arduino `pinMode()`, `digitalWrite()`)
- Build system (uses PlatformIO, not IDF build)

### ⚠️ FreeRTOS Usage (ALLOWED)
- `src/core/system.cpp` - Uses FreeRTOS semaphores for thread safety
- `src/hud/hud_manager.cpp` - Uses FreeRTOS queues for event handling

**Status**: These are acceptable uses. FreeRTOS is the Arduino ESP32 way to do multithreading.

### ✅ sdkconfig Patching (TEMPORARY WORKAROUND)
- `tools/patch_arduino_sdkconfig.py` - Pre-build script
  - Only modifies header files
  - No ESP-IDF includes
  - No runtime configuration
  - Will be removed when Arduino framework fixes defaults

## References

- [Arduino ESP32 Documentation](https://docs.espressif.com/projects/arduino-esp32/en/latest/)
- [FreeRTOS on ESP32](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos.html)
- [ESP32 Memory Management](https://docs.espressif.com/projects/arduino-esp32/en/latest/tutorials/memory.html)

## Version History

- **v2.17.3** (2026-01-23): Initial documentation
  - Clarified FreeRTOS usage is allowed
  - Documented PSRAM memory test disabled
  - Explained sdkconfig patching approach
