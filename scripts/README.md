# Build System Scripts

This directory contains build verification and automation scripts for the ESP32-S3 Car Control firmware.

## Scripts

### verify_build.py

**Purpose:** Validates build integrity and checks for common C++ issues that could lead to silent compilation failures or linker errors.

**Usage:**
```bash
python3 scripts/verify_build.py [project_root]
```

**Checks Performed:**

1. **Header Guards**: Verifies all header files have proper include guards (`#pragma once` or `#ifndef`/`#define`/`#endif`)
   - Missing guards can cause duplicate definition errors

2. **Static Member Definitions**: Checks that all static member variables declared in headers are properly defined in corresponding .cpp files
   - Common source of linker errors ("undefined reference to...")
   - Validates class name::member name definitions

3. **Extern Declarations**: Documents all extern declarations found in the codebase
   - Helps track global dependencies
   - Currently finds 7 extern TFT_eSPI declarations (shared display instance)

4. **CPP/Header Pairs**: Validates that implementation files have corresponding headers
   - Helps maintain consistent project structure
   - Excludes test files and main.cpp

5. **Build Artifacts**: Verifies that all .cpp files successfully compiled to .o files
   - **KEY CHECK**: Detects silent compilation failures
   - If a .cpp file exists but its .o file is missing, compilation silently failed
   - Most important check for preventing linker errors

**Exit Codes:**
- `0`: All checks passed (or only warnings)
- `1`: Errors found that must be fixed

**Example Output:**
```
======================================================================
ESP32-S3 Car Control - Build Verification Tool v1.0
======================================================================
Project: .

=== Checking Header Guards ===
  ✓ All 72 headers have guards

=== Checking Static Member Definitions ===
  Checked 66 static member declarations

=== Checking Extern Declarations ===
  Found 7 extern declarations

=== Checking CPP/Header Pairs ===
  2 cpp files without headers (may be intentional)

=== Checking Build Artifacts (.o files) ===
  ✓ All .cpp files compiled to .o files

======================================================================
Build Verification Summary
======================================================================

✅ All checks passed!

======================================================================
```

**Integration with CI/CD:**

Add to your GitHub Actions workflow:

```yaml
- name: Verify Build Integrity
  run: python3 scripts/verify_build.py .
```

Add to your pre-commit hook:

```bash
#!/bin/bash
python3 scripts/verify_build.py . || exit 1
```

## Common Issues Detected

### Silent Compilation Errors

**Symptom:** Linker fails with "error: .pio/build/.../file.cpp.o: No such file or directory"

**Root Causes:**
1. **Missing static member definitions**: Class declares `static Type member;` in header but no `Type ClassName::member = value;` in .cpp
2. **Template instantiation errors**: Template code fails to compile but error is hidden
3. **Circular header dependencies**: Headers include each other causing compilation to fail
4. **Missing includes**: Required header not included causing forward declarations to fail

**Detection:** The verify_build.py script checks for missing .o files which indicates silent compilation failure

**Fix:** 
1. Check compiler output carefully for warnings
2. Add all warnings flags: `-Wall -Wextra -Werror`
3. Run verify_build.py after every build
4. Fix static member definitions
5. Break circular dependencies

### Static Member Initialization

**Incorrect (causes linker error):**
```cpp
// header.h
class MyClass {
    static bool initialized;  // Declaration only
};
```

**Correct:**
```cpp
// header.h
class MyClass {
    static bool initialized;
};

// impl.cpp
bool MyClass::initialized = false;  // Definition with initialization
```

### Extern Declarations

**Good Practice:**
```cpp
// Declare in header
// tft_instance.h
#ifndef TFT_INSTANCE_H
#define TFT_INSTANCE_H
#include <TFT_eSPI.h>
extern TFT_eSPI tft;
#endif

// Define once in main or dedicated file
// tft_instance.cpp
#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();

// Use in other files
// some_file.cpp
#include "tft_instance.h"
// Now can use tft
```

**Current Pattern (acceptable but could be improved):**
```cpp
// Multiple files declare extern TFT_eSPI tft;
// Defined once in hud.cpp
```

## Future Enhancements

1. **Circular Dependency Detection**: Add graph-based analysis to detect circular #include chains
2. **Template Validation**: Parse template code and validate instantiations
3. **Symbol Checking**: Use `nm` or `objdump` to verify all symbols are defined
4. **Unused Code Detection**: Find functions/classes that are never called
5. **Code Metrics**: LOC, complexity, duplication analysis

## Troubleshooting

**Q: Script reports missing .o files for test files**
A: This is expected. Test files are excluded from release builds via `build_src_filter = +<*> -<test/>`

**Q: False positives on static members**
A: Check if the member is `constexpr` (defined in header) or uses `inline` initialization

**Q: Extern declarations flagged**
A: This is informational only. Ensure extern declarations have corresponding definitions somewhere in the codebase.
