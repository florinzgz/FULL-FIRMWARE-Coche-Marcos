# Sprite-Based Rendering Engine Documentation

This directory contains comprehensive documentation for the sprite-based rendering engine implementation.

## Quick Start

**Start here**: Read documents in this order:

1. **IMPLEMENTATION_SUMMARY.md** - Overview of what was implemented and why
2. **RENDER_ENGINE.md** - Detailed API reference and usage guide
3. **MIGRATION_GUIDE.md** - Step-by-step integration instructions
4. **RENDER_ENGINE_EXAMPLE.cpp** - Working code examples

## Document Guide

### IMPLEMENTATION_SUMMARY.md
**Purpose**: Executive overview  
**Audience**: Project managers, reviewers, developers  
**Contents**:
- What was implemented
- Key features and benefits
- Performance targets
- Architecture overview
- File structure
- Testing checklist
- Next actions

**Read this if**: You want a high-level understanding of the project

---

### RENDER_ENGINE.md
**Purpose**: Technical API reference  
**Audience**: Developers integrating the engine  
**Contents**:
- Complete API documentation
- Usage examples
- Configuration instructions
- Performance optimization tips
- Troubleshooting guide
- Memory usage analysis

**Read this if**: You need to use the rendering engine API

---

### MIGRATION_GUIDE.md
**Purpose**: Integration instructions  
**Audience**: Developers modifying existing code  
**Contents**:
- 5-phase migration strategy
- Step-by-step instructions for each phase
- Code modification examples
- Coordinate translation guide
- Testing checklist
- Rollback procedures

**Read this if**: You're integrating the engine into existing HUD code

---

### RENDER_ENGINE_EXAMPLE.cpp
**Purpose**: Working code samples  
**Audience**: Developers learning the API  
**Contents**:
- Complete function examples
- Integration patterns
- Best practices
- Common use cases
- Commented explanations

**Read this if**: You learn best from code examples

---

## Quick Reference

### Initialize Engine
```cpp
#include "render_engine.h"
#include "layer_map.h"

RenderEngine::init(&tft);
RenderEngine::createLayer(RenderLayer::CAR_BODY, x, y, w, h, 16);
```

### Draw into Sprite
```cpp
TFT_eSprite *sprite = RenderEngine::getSprite(RenderLayer::CAR_BODY);
sprite->fillSprite(TFT_BLACK);
sprite->drawRect(10, 10, 50, 50, TFT_WHITE);
RenderEngine::markDirty(RenderLayer::CAR_BODY);
```

### Render Frame
```cpp
RenderEngine::render();  // In main loop
```

## Architecture

```
┌─────────────────────────────────────────┐
│        Application Code (HUD)           │
└────────────┬────────────────────────────┘
             │
             ▼
┌─────────────────────────────────────────┐
│       Render Engine (Sprites)           │
│  - Layer management                     │
│  - Dirty tracking                       │
│  - PSRAM allocation                     │
└────────────┬────────────────────────────┘
             │
             ▼
┌─────────────────────────────────────────┐
│      TFT_eSPI (Display Driver)          │
│  - SPI communication                    │
│  - DMA transfers                        │
└────────────┬────────────────────────────┘
             │
             ▼
┌─────────────────────────────────────────┐
│    ST7796S Display (480x320)            │
└─────────────────────────────────────────┘
```

## Layers (Back to Front)

0. **BACKGROUND** - Static background
1. **CAR_BODY** - 3D car visualization
2. **WHEELS** - Four wheel widgets
3. **GAUGES** - Speed and RPM
4. **STEERING** - Steering wheel
5. **ICONS** - System icons
6. **PEDAL_BAR** - Pedal position
7. **BUTTONS** - Touch buttons
8. **OVERLAYS** - Menus and warnings

## Performance Targets

| Metric | Target |
|--------|--------|
| **FPS** | 50-60 stable |
| **Flicker** | None |
| **SPI Traffic** | 70%+ reduction |
| **Memory** | PSRAM (not main RAM) |

## Migration Phases

1. ✓ **Phase 1**: Infrastructure (COMPLETE)
2. **Phase 2**: Initialize engine (next)
3. **Phase 3**: Migrate static elements
4. **Phase 4**: Migrate semi-dynamic elements
5. **Phase 5**: Migrate dynamic elements

## Key Files

### Implementation
- `include/render_engine.h` - Public API
- `include/layer_map.h` - Layer definitions
- `src/hud/render_engine.cpp` - Implementation

### Documentation
- `docs/IMPLEMENTATION_SUMMARY.md` - This overview
- `docs/RENDER_ENGINE.md` - API reference
- `docs/MIGRATION_GUIDE.md` - Integration guide
- `docs/RENDER_ENGINE_EXAMPLE.cpp` - Code examples

### Configuration
- `platformio.ini` - Build flags for PSRAM/DMA

## FAQ

**Q: Will this break existing code?**  
A: No. The engine is designed to be non-breaking with fallback paths.

**Q: How much PSRAM is needed?**  
A: ~262KB for all layers. ESP32-S3 has 16MB available.

**Q: Can I migrate incrementally?**  
A: Yes. Each component can be migrated independently.

**Q: What if sprites fail to allocate?**  
A: Code falls back to direct TFT drawing automatically.

**Q: Do I need to change touch handling?**  
A: No. Touch handling is independent of rendering.

**Q: What about the MenuHidden system?**  
A: It continues to work unchanged. Can use OVERLAYS layer if needed.

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Won't compile | Check TFT_eSPI is installed |
| Sprites don't show | Mark layer dirty, call render() |
| Memory errors | Check PSRAM enabled, reduce sprite sizes |
| Low FPS | Reduce dirty marks, optimize sprite sizes |

## Getting Help

1. Check this README
2. Read IMPLEMENTATION_SUMMARY.md for overview
3. Consult RENDER_ENGINE.md for API details
4. Follow MIGRATION_GUIDE.md for integration
5. Review RENDER_ENGINE_EXAMPLE.cpp for code patterns

## Status

- **Phase 1**: ✓ Complete
- **Compilation**: Not yet tested
- **Integration**: Pending Phase 2
- **Testing**: Pending hardware validation

## License

Part of the FULL-FIRMWARE-Coche-Marcos project.

---

**Last Updated**: 2026-01-10  
**Version**: 1.0  
**Status**: Phase 1 Complete
