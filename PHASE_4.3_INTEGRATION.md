# PHASE 4.3 — LIMP MODE DIAGNOSTICS HUD
## Integration Guide

This phase adds diagnostic display for limp mode, showing the driver WHY the car is limited.

### Files Created
- `include/hud_limp_diagnostics.h`
- `src/hud/hud_limp_diagnostics.cpp`

### How to Integrate

#### 1. Add include to your HUD system
```cpp
#include "hud_limp_diagnostics.h"
```

#### 2. Initialize during HUD setup
Where you have:
```cpp
TFT_eSPI tft = TFT_eSPI();
// ... TFT initialization ...
HudLimpIndicator::init(&tft);
```

Add:
```cpp
HudLimpDiagnostics::init(&tft);
```

#### 3. Update in main loop
Where you have:
```cpp
LimpMode::update();
HudLimpIndicator::draw();
```

Add:
```cpp
HudLimpDiagnostics::draw();
```

#### 4. Clear before menu
Before opening menu:
```cpp
HudLimpIndicator::clear();
HudLimpDiagnostics::clear();  // Add this
```

#### 5. Restore after menu
After closing menu:
```cpp
HudLimpIndicator::forceRedraw();
HudLimpDiagnostics::forceRedraw();  // Add this
```

### Display Layout
```
┌───────────────────────────────────────┐
│  LIMP MODE                            │
│                                       │
│  Pedal:     OK / FAIL                 │
│  Steering:  OK / FAIL                 │
│  Battery:   OK / LOW                  │
│  Temp:      OK / WARN                 │
│  Errors:    <number>                  │
│                                       │
│  Power:     <percent> %               │
│  Speed:     <percent> %               │
└───────────────────────────────────────┘

Position: X=260, Y=60
Size: 210x180 pixels
```

### Behavior
- **NORMAL state**: Nothing displayed (area cleared)
- **DEGRADED/LIMP/CRITICAL**: Diagnostics shown
- **Auto-refresh**: Only redraws when diagnostics change
- **Data source**: `LimpMode::getDiagnostics()` (single source of truth)

### Color Coding
- **Green**: OK, all good
- **Yellow**: FAIL, LOW, WARN (problems detected)
- **Red border**: CRITICAL state (severe issues)
- **White border**: DEGRADED/LIMP (non-critical issues)

### Performance
- Efficient caching prevents unnecessary redraws
- No rendering overhead in NORMAL state
- No blocking, delay, or malloc
- Safe for 30 FPS operation

### Safety
This module is **read-only**:
- Does NOT modify LimpMode
- Does NOT read sensors directly
- Does NOT duplicate logic
- Pure mirror of the safety engine
