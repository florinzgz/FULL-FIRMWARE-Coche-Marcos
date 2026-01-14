# TFT_eSPI Thread Safety Guidelines

## ⚠️ CRITICAL: TFT_eSPI is NOT Thread-Safe

**TFT_eSPI and TFT_eSprite are NOT thread-safe libraries.**

Concurrent access to TFT functions from multiple FreeRTOS tasks, ISRs, or callbacks will:
- Corrupt internal heap structures
- Corrupt DMA buffers
- Trigger "Stack canary watchpoint (ipc0)" crashes
- Cause system instability

## Root Cause of ipc0 Crashes

The "Stack canary watchpoint triggered (ipc0)" crash was caused by:

1. **HUDManager::showError()** calling `tft.fillScreen()`, `tft.setTextColor()`, `tft.print()` directly
2. **handleCriticalError()** in main.cpp calling showError from initialization failures
3. **HUDManager::update()** running the normal render pipeline concurrently
4. Multiple execution contexts accessing TFT simultaneously

This corrupted TFT_eSPI's internal heap and DMA structures, killing the ipc0 core.

## Solution: Single-Threaded Render Model

### Architecture

All TFT access MUST happen from a single context:

```
┌─────────────────────────────────────────────────────┐
│                  Main Loop (Core 1)                  │
│  ┌───────────────────────────────────────────────┐  │
│  │        HUDManager::update()                    │  │
│  │  - Process render event queue                  │  │
│  │  - ALL TFT drawing happens here                │  │
│  │  - renderDashboard()                           │  │
│  │  - renderErrorScreen()                         │  │
│  │  - renderMenus()                               │  │
│  └───────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
              ▲
              │ FreeRTOS Queue
              │ (RenderEvent)
              │
┌─────────────┴───────────────┐
│  Any Context:                │
│  - SafetyManager             │
│  - PowerManager              │
│  - handleCriticalError()     │
│  - Watchdog                  │
│  - Telemetry                 │
│  - ISRs / Timers             │
│                              │
│  Call:                       │
│  HUDManager::showError(msg)  │
│  -> Queues event (non-block) │
└──────────────────────────────┘
```

### Render Event Queue System

#### Event Types

```cpp
enum class RenderEvent::Type {
  NONE = 0,          // No event
  SHOW_ERROR,        // Display error screen
  CLEAR_ERROR,       // Clear error and return to normal
  FORCE_REDRAW,      // Force complete screen redraw
  UPDATE_BRIGHTNESS  // Update backlight brightness
};
```

#### Queue Flow

1. **Any context** wants to display something:
   ```cpp
   HUDManager::showError("Motor failure");
   ```

2. **showError()** creates and queues event (non-blocking):
   ```cpp
   RenderEvent::Event event;
   event.type = RenderEvent::Type::SHOW_ERROR;
   strncpy(event.errorMessage, message, MAX_ERROR_MSG_LEN);
   xQueueSend(renderEventQueue, &event, 0); // Never blocks!
   ```

3. **HUDManager::update()** processes queue:
   ```cpp
   void HUDManager::update() {
     processRenderEvents();  // Process ALL pending events
     
     if (errorActive) {
       renderErrorScreen();  // ONLY place TFT is called
       return;
     }
     
     // Normal rendering...
   }
   ```

4. **renderErrorScreen()** performs actual TFT drawing:
   ```cpp
   void HUDManager::renderErrorScreen() {
     tft.fillScreen(TFT_BLACK);          // ✅ SAFE: Called from update()
     tft.setTextColor(TFT_RED);          // ✅ SAFE: Single thread
     tft.print(errorMessage);            // ✅ SAFE: No race conditions
   }
   ```

## Rules for Developers

### ✅ DO

1. **Queue render requests** from any context:
   ```cpp
   HUDManager::showError("Error message");
   HUDManager::queueRenderEvent(event);
   ```

2. **Call TFT functions ONLY in**:
   - `HUDManager::update()`
   - Functions called FROM `update()` (renderDashboard, renderErrorScreen, etc.)
   - Menu render functions (menu_*.cpp)
   - HUD render functions (hud.cpp, hud_compositor.cpp, render_engine.cpp)

3. **Check queue** regularly in update():
   ```cpp
   processRenderEvents();  // First thing in update()
   ```

### ❌ DON'T

1. **NEVER call TFT from**:
   - Manager classes (SafetyManager, PowerManager, etc.)
   - Error handlers (handleCriticalError)
   - Watchdog callbacks
   - Timer ISRs
   - FreeRTOS task callbacks
   - Any context outside HUDManager::update()

2. **NEVER block** the render queue:
   ```cpp
   // ❌ WRONG: Blocking send
   xQueueSend(queue, &event, portMAX_DELAY);
   
   // ✅ CORRECT: Non-blocking send
   xQueueSend(queue, &event, 0);
   ```

3. **NEVER access** global `tft` object directly:
   ```cpp
   // ❌ WRONG: Direct TFT access from manager
   extern TFT_eSPI tft;
   void SafetyManager::emergencyStop() {
     tft.fillScreen(TFT_RED);  // CRASH!
   }
   
   // ✅ CORRECT: Queue render event
   void SafetyManager::emergencyStop() {
     HUDManager::showError("Emergency stop");
   }
   ```

## Queue Configuration

- **Queue Size**: 10 events
- **Item Size**: `sizeof(RenderEvent::Event)` (~132 bytes)
- **Timeout**: 0 ticks (non-blocking send/receive)
- **Created**: In `HUDManager::init()`
- **Processed**: In `HUDManager::update()` every frame (30 FPS)

## Benefits

1. **Thread Safety**: No concurrent TFT access
2. **Stability**: No ipc0 crashes
3. **Simplicity**: Clear separation of concerns
4. **Performance**: Non-blocking queue operations
5. **Flexibility**: Easy to add new render event types

## Migration Guide

### Before (Broken)

```cpp
void handleCriticalError(const char *msg) {
  tft.fillScreen(TFT_BLACK);     // ❌ Concurrent access!
  tft.setTextColor(TFT_RED);     // ❌ Race condition!
  tft.print("ERROR: ");           // ❌ Heap corruption!
  tft.println(msg);               // ❌ ipc0 crash!
  while(1) { yield(); }
}
```

### After (Fixed)

```cpp
void handleCriticalError(const char *msg) {
  HUDManager::showError(msg);    // ✅ Queues event safely
  while(1) {
    Watchdog::feed();            // Continue feeding watchdog
    yield();                     // Let render thread display error
  }
}
```

## Testing

To verify thread safety:

1. Trigger multiple errors simultaneously
2. Check for queue overflow (logged as warnings)
3. Monitor for ipc0 crashes (should not occur)
4. Verify error messages display correctly

## References

- Implementation: `include/render_event.h`
- Queue creation: `src/hud/hud_manager.cpp::init()`
- Queue processing: `src/hud/hud_manager.cpp::processRenderEvents()`
- Error rendering: `src/hud/hud_manager.cpp::renderErrorScreen()`
- API: `include/hud_manager.h`

---

**Last Updated**: 2026-01-13
**Author**: GitHub Copilot
**Version**: 1.0
