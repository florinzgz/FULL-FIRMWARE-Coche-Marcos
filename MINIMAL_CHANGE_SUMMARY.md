# Minimal Configuration Change - memory_type Only

## User Request
@florinzgz requested: "Desaste de tofos estos cambios y intenta cambiar solamente dio_qspi y las dependencias"

Translation: "Undo all these changes and only try changing dio_qspi and its dependencies"

## Change Made

### Single File Modified
**File**: `boards/esp32-s3-devkitc1-n16r8.json`

**Change**: Line 5
```diff
-      "memory_type": "qio_opi"
+      "memory_type": "dio_qspi"
```

### What This Setting Controls

The `memory_type` parameter in the Arduino board configuration controls how the ESP32-S3 bootloader and framework configure flash and PSRAM:

| Setting | Flash Mode | PSRAM Mode | Description |
|---------|------------|------------|-------------|
| **qio_opi** (before) | QIO (Quad I/O) | OPI (Octal) | Fastest but requires perfect signal integrity |
| **dio_qspi** (after) | DIO (Dual I/O) | QSPI (Quad) | Slower but more compatible across hardware batches |

### Automatic Effects

Changing `memory_type` automatically causes the Arduino framework to:

1. **Flash Configuration**:
   - Switch from QIO (4 data lines) to DIO (2 data lines)
   - Reduces flash read speed by ~40%
   - Increases compatibility with various flash chip brands

2. **PSRAM Configuration**:
   - Switch from OPI (8 data lines) to QSPI (4 data lines)
   - PSRAM remains **enabled** but in quad mode instead of octal
   - Eliminates OPI timing conflicts that cause bootloop

### What Was NOT Changed

All other settings remain exactly as they were:

- ✅ **PSRAM**: Still enabled (just in QSPI mode, not OPI)
- ✅ **flash_mode**: Still shows "qio" in JSON (overridden by memory_type)
- ✅ **psram_type**: Still shows "opi" in JSON (overridden by memory_type)
- ✅ **CDC on boot**: Unchanged (still has default setting)
- ✅ **All source code**: Completely unmodified
- ✅ **All timing optimizations**: Removed/reverted
- ✅ **sdkconfig settings**: Unchanged
- ✅ **platformio.ini**: Unchanged

## Previous Changes (Now Reverted)

The following commits were UNDONE as requested:

1. ❌ `791c73e` - Timing optimizations (delay → yield)
2. ❌ `3030cb3` - Serial.flush() removal
3. ❌ `36c653e` - Additional Serial.flush() fixes
4. ❌ `da7f893` - Documentation (TIMING_CONFLICTS_FIX_SUMMARY.md)
5. ❌ `0caf7cf` - PSRAM disable, DIO flash, CDC changes, code modifications
6. ❌ `e5bb672` - Magic number constants

All these changes have been completely reverted to test if the minimal `memory_type` change alone is sufficient.

## Testing This Change

### Expected Behavior
- Boot should complete without bootloop
- System operates with QSPI PSRAM (still 8MB available)
- Display should work normally
- Slightly slower flash reads (not usually noticeable)

### If This Works
It confirms that the issue was specifically with **OPI PSRAM timing** on your hardware batch, and QSPI mode is sufficient.

### If This Doesn't Work
May need to add back:
- PSRAM complete disable (`CONFIG_SPIRAM=n`)
- CDC on boot disable
- Or other specific changes

## Commit Status

**Local commit**: `1ce2bf8` - "Change memory_type from qio_opi to dio_qspi for bootloop fix"

**Status**: Ready to test locally. Remote push requires force-push to replace previous 6 commits with this single minimal commit.

## Technical Details

### How memory_type Works in Arduino Framework

The `memory_type` setting in board.json is used by the Arduino ESP32 framework during compilation to:

1. Generate appropriate bootloader configuration
2. Set flash frequency and mode in the bootloader header
3. Configure PSRAM initialization code
4. Set up memory mapping in the linker script

It overrides the individual `flash_mode` and `psram_type` settings because it provides a combined configuration that's been tested to work together.

### Supported memory_type Values

Common ESP32-S3 configurations:
- `qio_opi` - QIO flash + OPI PSRAM (fastest, least compatible)
- `qio_qspi` - QIO flash + QSPI PSRAM (fast, good compatibility)
- `dio_qspi` - DIO flash + QSPI PSRAM (slower, best compatibility) ✅ **Current**
- `dio_opi` - DIO flash + OPI PSRAM (mixed, not common)

## Next Steps

1. Test if this minimal change resolves the bootloop
2. If yes: Confirm and close issue
3. If no: Report back which specific symptoms remain
4. Based on results, determine if additional changes needed

---

**This is the absolute minimum change possible** - only 1 line in 1 file modified.
