# Build Tools

This directory contains build-time validation and checking tools for the ESP32-S3 firmware.

## Tools

### preflight_validator.py

**Pre-Flight Hardware Validation System** - Prevents building firmware that would crash due to improper hardware initialization order.

**Purpose**: Blocks the build if code uses hardware (TFT, I2C, SPI, GPIO) before it's properly initialized.

**Usage**:
- Runs automatically before every build (configured in `platformio.ini`)
- Can be run manually: `python tools/preflight_validator.py`

**Documentation**: See `docs/HARDWARE_PREFLIGHT_SYSTEM.md` for complete documentation.

**Rules**: Hardware validation rules are in `rules/hardware_rules.json`

**Key Features**:
- ✅ Zero runtime overhead (build-time only)
- ✅ Detects TFT usage before tft.init()
- ✅ Detects I2C usage before Wire.begin()
- ✅ Detects PWM usage before ledcSetup()
- ✅ Prevents the bootloop bug (TFT accessed before initialization)
- ✅ Detailed error messages with file, line, and fix suggestions

**Exit codes**:
- `0`: Validation passed
- `1`: Fatal violations detected, build blocked

## Adding New Tools

Place build scripts in this directory and reference them in `platformio.ini` under `extra_scripts`:

```ini
extra_scripts =
    pre:tools/your_script.py
```

Use `pre:` for scripts that run before compilation, `post:` for post-build scripts.
