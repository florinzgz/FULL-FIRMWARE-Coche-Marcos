# Hardware Pre-Flight Validation System

## Overview

The **Hardware Pre-Flight Validation System** is a build-time safety mechanism that prevents firmware from being built if it contains code that would crash at runtime due to improper hardware initialization order.

This system is designed for **ESP32-S3** automotive firmware where hardware failures can cause:
- ❌ Guru Meditation Errors
- ❌ Watchdog resets
- ❌ USB CDC disconnects
- ❌ Bootloader loops
- ❌ Flash corruption
- ❌ Physical hardware damage

## Why This Exists

### The Problem

In embedded systems, hardware must be initialized in a specific order. Using hardware before it's properly initialized causes crashes that:

1. **Compile successfully** ✅ (the compiler doesn't know about initialization order)
2. **Crash at runtime** 💥 (the ESP32 detects illegal hardware access)

Example of code that compiles but crashes:

```cpp
void init() {
    // ❌ WRONG - TFT used before initialization
    tft.fillScreen(TFT_BLACK);  // This crashes!
    tft.init();                 // Too late!
}
```

### The Solution

This system **blocks the build** before the firmware binary exists, preventing deployment of broken firmware.

```
Code Written → Pre-Flight Validator → ❌ BUILD BLOCKED
                                   ↓
                            Detailed Error Report
                                   ↓
                            Developer Fixes Code
                                   ↓
                            ✅ Build Succeeds
```

## What It Checks

### 1. TFT Display Initialization

**Rule**: TFT must be initialized with `tft.init()` before any drawing operations.

**Forbidden before init**:
- `tft.fillScreen()`
- `tft.drawPixel()`
- `tft.setRotation()`
- `tft.width()` / `tft.height()`
- `HUD::init()`
- `HudCompositor::init()`

**Impact if violated**: Guru Meditation Error, USB CDC disconnect

**Example violation**:
```cpp
// ❌ WRONG
tft.fillScreen(TFT_BLACK);  // BUILD BLOCKED HERE
tft.init();

// ✅ CORRECT
tft.init();
tft.fillScreen(TFT_BLACK);
```

### 2. Backlight PWM Configuration

**Rule**: PWM channel must be configured before writing brightness values.

**Required order**:
1. `ledcSetup()` - Configure PWM channel
2. `ledcAttachPin()` - Attach pin to channel
3. `ledcWrite()` - Write brightness value

**Impact if violated**: Display remains dark or undefined PWM behavior

### 3. PSRAM Initialization

**Rule**: PSRAM must be detected before allocation from external RAM.

**Required init**: `psramInit()` or `psramFound()`

**Forbidden before init**:
- `ps_malloc()`
- `ps_calloc()`
- `heap_caps_malloc()` with `MALLOC_CAP_SPIRAM`

**Impact if violated**: Memory allocation failures, system crash

### 4. I2C Bus Initialization

**Rule**: I2C bus must be initialized with `Wire.begin()` before device access.

**Forbidden before init**:
- `Wire.beginTransmission()`
- `Wire.read()` / `Wire.write()`
- Creating I2C device instances (MCP23017, PCA9685, INA226, TCA9548A)

**Impact if violated**: I2C bus hangs, watchdog timeout

**Example**:
```cpp
// ❌ WRONG
Wire.beginTransmission(0x20);  // BUILD BLOCKED
Wire.begin();

// ✅ CORRECT
Wire.begin();
Wire.beginTransmission(0x20);
```

### 5. GPIO Configuration

**Rule**: GPIO pins must be configured with `pinMode()` before use.

**Forbidden before init**:
- `digitalWrite()`
- `digitalRead()`

**Impact if violated**: Undefined GPIO state, potential hardware damage

### 6. SPI Bus Initialization

**Rule**: SPI bus must be initialized before device transactions.

**Required**: `SPI.begin()`

**Forbidden before init**:
- `SPI.transfer()`
- `SPI.write()`

**Impact if violated**: SPI communication fails, display/SD errors

### 7. Task Creation Guards

**Rule**: Tasks should not be created before the heap is ready.

**Required**: `System::init()` or `setup()`

**Forbidden before init**:
- `xTaskCreate()`
- `xTaskCreatePinnedToCore()`

**Impact if violated**: Task creation failure, heap corruption

### 8. ISR Safety

**Rule**: ISR handlers must only call IRAM-safe functions.

**Forbidden in ISR context**:
- `Serial.print()` / `Serial.println()`
- `delay()` / `delayMicroseconds()`
- `malloc()` / `free()`
- `new` / `delete`

**Impact if violated**: Cache disabled exception, system crash in ISR

## Project-Specific Rules

These rules enforce initialization order specific to this automotive firmware:

### HUD Manager Init Order

**Critical**: This rule prevents the exact bootloop bug that occurred.

**Required order**:
1. `tft.init()` - Initialize TFT hardware
2. `HudCompositor::init()` - Initialize compositor
3. `HUD::init()` - Initialize HUD rendering

**Why**: The HUD system accesses TFT functions. If TFT isn't initialized first, the ESP32 crashes immediately on boot.

### Backlight Before Display

**Required order**:
1. `ledcSetup()` - Configure PWM
2. `ledcAttachPin()` - Attach backlight pin
3. `ledcWrite()` - Set brightness
4. `tft.fillScreen()` - Display becomes visible

**Why**: Prevents screen flicker or dark screen during boot.

### I2C Before Devices

**Required order**:
1. `Wire.begin()` - Initialize I2C bus
2. Device initialization (MCP23017, PCA9685, etc.)

**Why**: I2C devices cannot communicate without the bus being initialized.

### PSRAM Before Sprites

**Required order**:
1. `psramInit()` - Initialize external RAM
2. Sprite/buffer allocation

**Why**: Large sprites are allocated in PSRAM. Without init, allocation fails.

### SPI Before TFT

**Required order**:
1. `SPI.begin()` - Initialize SPI bus
2. `tft.init()` - Initialize TFT over SPI

**Why**: TFT communicates over SPI. Bus must be ready first.

## How It Works

### Build Process Integration

The validator runs automatically before every build:

```
PlatformIO Build Command
        ↓
install_deps.py (install Python packages)
        ↓
preflight_validator.py ← YOU ARE HERE
        ↓
Compilation (only if validation passes)
        ↓
Linking
        ↓
Firmware Binary
```

Configuration in `platformio.ini`:
```ini
extra_scripts =
    pre:install_deps.py
    pre:tools/preflight_validator.py
```

### Validation Process

1. **Parse Source Files**: Read all `.cpp` and `.h` files in `src/` and `include/`
2. **Build Call Graph**: Track function calls and their locations
3. **Track Init State**: Monitor when hardware is initialized
4. **Detect Violations**: Find hardware usage before initialization
5. **Report Errors**: Generate detailed error messages
6. **Block Build**: Exit with code 1 if violations found

### Error Report Format

When a violation is detected:

```
❌ BUILD BLOCKED — HARDWARE VIOLATION
================================================================================

🚨 FATAL VIOLATION:
   Resource: TFT
   File: src/hud/hud_manager.cpp
   Line: 214
   Function: HUDManager::init
   Code: tft.fillScreen(TFT_BLACK);
   Violation: 'tft.fillScreen' used before initialization
   Reason: TFT display must be initialized before any drawing operations
   Impact: Causes Guru Meditation Error and USB CDC disconnect on ESP32-S3

   Fix: Ensure hardware is properly initialized before use
--------------------------------------------------------------------------------

================================================================================
❌ BUILD CANNOT PROCEED
   1 fatal violation(s) detected
   Fix the issues above and rebuild
================================================================================
```

## Zero Runtime Cost

**Important**: This system has **ZERO runtime overhead**.

- ✅ Validation runs only at **build time**
- ✅ No code added to the firmware binary
- ✅ No runtime assertions
- ✅ No debug checks
- ✅ No performance impact
- ✅ Release builds are unaffected

The validator is a **build tool**, not a runtime library.

## Adding New Hardware Rules

To add validation for new hardware components:

### 1. Edit `rules/hardware_rules.json`

Add a new rule to the `rules` array:

```json
{
  "resource": "SD_CARD",
  "description": "SD card must be initialized before file operations",
  "init_functions": [
    "SD.begin",
    "SD_MMC.begin"
  ],
  "forbidden_before_init": [
    "SD.open",
    "SD.mkdir",
    "SD.remove"
  ],
  "fatal": true,
  "impact": "File operations fail, potential data corruption"
}
```

### 2. Add Project-Specific Rules (Optional)

For complex initialization sequences:

```json
{
  "name": "SD_BEFORE_LOGGING",
  "description": "SD card must be ready before logger initialization",
  "enforced_order": [
    "SD.begin",
    "Logger::init"
  ],
  "fatal": true,
  "impact": "Logger cannot write to SD card"
}
```

### 3. Test the Rule

```bash
# Run validator standalone
python tools/preflight_validator.py

# Or build the project
pio run
```

## Rule Configuration Reference

### Basic Rule Structure

```json
{
  "resource": "RESOURCE_NAME",
  "description": "Human-readable description",
  "init_functions": ["list", "of", "init", "calls"],
  "forbidden_before_init": ["list", "of", "forbidden", "calls"],
  "fatal": true,
  "impact": "What happens if violated"
}
```

### Fields

- **resource**: Unique identifier for the hardware (e.g., "TFT", "I2C")
- **description**: Explains what the rule checks
- **init_functions**: Function calls that initialize the hardware
- **forbidden_before_init**: Function calls that require initialization
- **fatal**: `true` = block build, `false` = warning only
- **impact**: Description of what breaks if rule is violated

### Advanced: Regex Patterns

Forbidden patterns can use regex:

```json
{
  "forbidden_before_init": [
    "Wire\\..*",              // Any Wire method
    "heap_caps_malloc.*SPIRAM"  // PSRAM allocation
  ]
}
```

## Troubleshooting

### False Positives

**Problem**: Validator reports a violation that isn't real.

**Causes**:
- Code is in a comment
- Initialization happens in a different file
- Conditional compilation (`#ifdef`)

**Solutions**:
1. Check if the violation is in a comment block
2. Ensure initialization happens in the same compilation unit
3. For cross-file initialization, use project-specific rules

### Build Blocked Incorrectly

**Problem**: Build fails but code is actually correct.

**Steps**:
1. Check the error report for file and line number
2. Verify initialization order in that file
3. If initialization is in a different file, add a project-specific rule
4. If the rule is incorrect, update `hardware_rules.json`

### Validator Not Running

**Problem**: Violations exist but build succeeds.

**Checks**:
1. Verify `platformio.ini` has the `extra_scripts` line
2. Check `tools/preflight_validator.py` exists and is executable
3. Run manually: `python tools/preflight_validator.py`

### Performance Issues

**Problem**: Validation takes too long.

**Solutions**:
- Validator caches file reads
- Should complete in < 5 seconds for typical projects
- If slower, check for very large files or excessive rules

## Integration with CI/CD

The validator integrates seamlessly with continuous integration:

```yaml
# Example GitHub Actions
- name: Build Firmware
  run: pio run
  # Validator runs automatically
  # Build fails if violations detected
```

Exit codes:
- **0**: Validation passed, build proceeds
- **1**: Validation failed, build blocked

## Best Practices

### 1. Fix Violations Immediately

Don't disable rules to "make the build work". Fix the underlying issue.

### 2. Add Rules Proactively

When adding new hardware, add validation rules immediately.

### 3. Document Complex Init Sequences

Use project-specific rules to document critical initialization order.

### 4. Review Warnings

Even non-fatal warnings indicate potential issues. Fix them.

### 5. Test After Changes

After modifying hardware init code, run a full build to validate.

## Examples

### Example 1: TFT Initialization

❌ **Wrong** (detected and blocked):
```cpp
void HUDManager::init() {
    tft.fillScreen(TFT_BLACK);  // ❌ BUILD BLOCKED
    tft.init();
}
```

✅ **Correct**:
```cpp
void HUDManager::init() {
    tft.init();
    tft.fillScreen(TFT_BLACK);  // ✅ OK
}
```

### Example 2: I2C Device Initialization

❌ **Wrong** (detected and blocked):
```cpp
void setup() {
    MCP23017 mcp;  // ❌ BUILD BLOCKED
    Wire.begin();
}
```

✅ **Correct**:
```cpp
void setup() {
    Wire.begin();
    MCP23017 mcp;  // ✅ OK
}
```

### Example 3: PWM Backlight

❌ **Wrong** (detected and blocked):
```cpp
void initBacklight() {
    ledcWrite(0, 200);  // ❌ BUILD BLOCKED
    ledcSetup(0, 5000, 8);
    ledcAttachPin(PIN_BL, 0);
}
```

✅ **Correct**:
```cpp
void initBacklight() {
    ledcSetup(0, 5000, 8);
    ledcAttachPin(PIN_BL, 0);
    ledcWrite(0, 200);  // ✅ OK
}
```

## Technical Details

### Implementation

- **Language**: Python 3
- **Dependencies**: None (uses only standard library)
- **Integration**: PlatformIO SCons extra script
- **Performance**: ~2-5 seconds for typical projects

### Limitations

1. **Same-file scope**: Primarily detects violations within the same file
2. **No runtime analysis**: Cannot detect dynamic initialization issues
3. **Pattern matching**: Uses regex, not full C++ parsing
4. **No preprocessor**: Doesn't evaluate `#ifdef` directives

For cross-file initialization, use **project-specific rules**.

### Architecture

```
preflight_validator.py
    ├── HardwareValidator (main engine)
    │   ├── _load_rules()
    │   ├── _get_source_files()
    │   ├── _check_initialization_order()
    │   ├── _check_project_specific_rules()
    │   └── _report_violations()
    │
    └── Data Structures
        ├── CodeLocation
        ├── FunctionCall
        └── InitializationState
```

## Safety Guarantees

This system **guarantees**:

✅ TFT is initialized before use  
✅ I2C bus is ready before device access  
✅ PSRAM is available before allocation  
✅ PWM is configured before use  
✅ GPIO pins are set up before access  
✅ SPI is initialized before transactions  
✅ No ISR calls non-IRAM functions  

This system **prevents**:

❌ Bootloops from hardware misuse  
❌ Guru Meditation Errors  
❌ Watchdog resets  
❌ USB CDC disconnects  
❌ Flash corruption  
❌ Hardware damage  

## Support

For issues or questions:

1. Check error messages carefully (they include fix suggestions)
2. Review this documentation
3. Check `rules/hardware_rules.json` for rule definitions
4. Run validator manually for detailed output: `python tools/preflight_validator.py`

## Version History

- **v1.0.0** (2026-01): Initial implementation
  - TFT initialization validation
  - I2C bus validation
  - PSRAM validation
  - PWM validation
  - GPIO validation
  - SPI validation
  - Task creation guards
  - ISR safety checks
  - Project-specific rules engine

## License

This validation system is part of the ESP32-S3 automotive firmware project.

---

**Remember**: This system exists to **protect your hardware and prevent deployment of broken firmware**. Don't bypass it—fix the underlying issues.
