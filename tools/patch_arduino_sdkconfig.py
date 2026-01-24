"""
ESP32-S3 Arduino Framework sdkconfig Patcher
v2.17.3 - Bootloop Fix

OVERVIEW:
This PlatformIO pre-build script patches the Arduino framework's pre-compiled
sdkconfig.h files to fix bootloop issues on ESP32-S3 N16R8 hardware.

PROBLEM:
Arduino ESP32 framework ships with CONFIG_ESP_INT_WDT_TIMEOUT_MS=300ms, which
is too short for PSRAM initialization on some hardware batches, causing
continuous resets during boot (rst:0x3 RTC_SW_SYS_RST).

SOLUTION:
This script increases the watchdog timeout to 5000ms in Arduino's pre-compiled
SDK configuration headers, providing enough time for PSRAM initialization.

IMPORTANT DESIGN CONSTRAINTS:
1. NO ESP-IDF HEADERS: This script uses only Python stdlib (os, re) and
   PlatformIO's SCons environment. It does NOT include any ESP-IDF headers.

2. NO RUNTIME OPTIONS: This script modifies COMPILE-TIME configuration files
   (sdkconfig.h) during the build phase. It does NOT activate IDF runtime
   options or call ESP-IDF APIs.

3. FUTURE-PROOF: Script uses version detection and graceful degradation:
   - Only patches if CONFIG_ESP_INT_WDT_TIMEOUT_MS exists
   - Skips already-patched files (idempotent)
   - Handles both old and new SDK versions
   - Fails gracefully if SDK structure changes

4. MINIMAL CHANGES: Only modifies watchdog timeout values, nothing else.

COMPATIBILITY:
- Works with Arduino ESP32 core versions 2.x and 3.x
- Compatible with ESP-IDF 4.4, 5.0, 5.1+ (underlying framework)
- Tested on ESP32-S3 N16R8 hardware

USAGE:
Automatically executed by PlatformIO via platformio.ini:
  extra_scripts = pre:tools/patch_arduino_sdkconfig.py

MAINTENANCE:
- If Arduino core fixes default timeout, this patch becomes no-op
- Re-run after Arduino framework package updates
- Monitor Arduino-ESP32 core releases for permanent fix

Author: Florin Zgureanu
Date: 2026-01-23
License: Same as project
"""

# =============================================================================
# IMPORTANT: This script uses ONLY Python stdlib and PlatformIO SCons API
# NO ESP-IDF headers or runtime APIs are used - only file manipulation
# =============================================================================
Import("env")
import os
import re

# Version-specific constants
SCRIPT_VERSION = "2.17.3"
TARGET_TIMEOUT_MS = 5000  # Target watchdog timeout
MIN_SAFE_TIMEOUT_MS = 3000  # Don't patch if already >= this value

def _should_patch_legacy_timeout(content):
    """
    Check if CONFIG_INT_WDT_TIMEOUT_MS should be patched.
    
    BACKWARD COMPATIBILITY: Older SDK versions define CONFIG_INT_WDT_TIMEOUT_MS
    as a standalone value, while newer versions use it as an alias.
    
    Returns True if CONFIG_INT_WDT_TIMEOUT_MS exists as a standalone define
    (not as an alias to CONFIG_ESP_INT_WDT_TIMEOUT_MS).
    
    Old SDK (needs patching):
        #define CONFIG_INT_WDT_TIMEOUT_MS 300
    
    New SDK (skip patching):
        #define CONFIG_INT_WDT_TIMEOUT_MS CONFIG_ESP_INT_WDT_TIMEOUT_MS
    
    Args:
        content (str): The sdkconfig.h file content
        
    Returns:
        bool: True if legacy standalone define exists, False otherwise
    """
    # Find the line with CONFIG_INT_WDT_TIMEOUT_MS
    for line in content.split('\n'):
        if line.startswith('#define CONFIG_INT_WDT_TIMEOUT_MS '):
            # Check if it's an alias (contains CONFIG_ESP_INT_WDT_TIMEOUT_MS)
            if 'CONFIG_ESP_INT_WDT_TIMEOUT_MS' in line:
                return False  # It's an alias, don't patch
            else:
                return True  # It's a standalone value, patch it
    
    return False

def patch_arduino_sdkconfig(env):
    """
    Patch Arduino framework sdkconfig.h files to increase watchdog timeout.
    
    SAFETY GUARANTEES:
    1. Read-only until validation passes
    2. Atomic writes (write to temp, then rename)
    3. Idempotent (can run multiple times safely)
    4. Version detection (only patches known patterns)
    5. Graceful degradation (continues on errors)
    
    WHAT THIS DOES:
    - Locates Arduino ESP32 framework SDK directory
    - Finds all sdkconfig.h files for ESP32-S3 variants
    - Increases CONFIG_ESP_INT_WDT_TIMEOUT_MS from <3000ms to 5000ms
    - Skips files already at safe timeout values
    
    WHAT THIS DOES NOT DO:
    - Does NOT include ESP-IDF headers
    - Does NOT call ESP-IDF runtime APIs
    - Does NOT modify source code or compilation flags
    - Does NOT activate IDF runtime options
    """
    print("=" * 70)
    print(f"🔧 ESP32-S3 Bootloop Fix - Patching Arduino Framework (v{SCRIPT_VERSION})")
    print("=" * 70)
    
    # Get the platform packages directory
    platform_dir = env.PioPlatform().get_package_dir("framework-arduinoespressif32")
    
    if not platform_dir or not os.path.isdir(platform_dir):
        print("⚠️  Arduino framework not found - skipping patch")
        print("   This is normal if you haven't installed the framework yet.")
        return
    
    # Find all ESP32-S3 sdkconfig.h files
    sdk_dir = os.path.join(platform_dir, "tools", "sdk", "esp32s3")
    
    if not os.path.isdir(sdk_dir):
        print(f"⚠️  ESP32-S3 SDK directory not found at: {sdk_dir}")
        print(f"   Framework structure may have changed in newer versions.")
        print(f"   Patch skipped - check if bootloop issue persists.")
        return
    
    # Pattern to find sdkconfig.h files
    sdkconfig_files = []
    for root, dirs, files in os.walk(sdk_dir):
        for file in files:
            if file == "sdkconfig.h":
                sdkconfig_files.append(os.path.join(root, file))
    
    if not sdkconfig_files:
        print("⚠️  No sdkconfig.h files found")
        print("   Framework structure may have changed. Patch skipped.")
        return
    
    print(f"📁 Found {len(sdkconfig_files)} sdkconfig.h file(s) to patch")
    print("")
    
    # Patch each file
    patched_count = 0
    already_patched_count = 0
    skipped_count = 0
    
    for filepath in sdkconfig_files:
        variant = os.path.basename(os.path.dirname(os.path.dirname(filepath)))
        
        try:
            # Read the file
            with open(filepath, 'r') as f:
                content = f.read()
            
            # Check current value
            match = re.search(r'#define\s+CONFIG_ESP_INT_WDT_TIMEOUT_MS\s+(\d+)', content)
            
            if not match:
                print(f"   ⚠️  {variant}: CONFIG_ESP_INT_WDT_TIMEOUT_MS not found - skipping")
                skipped_count += 1
                continue
            
            # Parse current value with error handling
            try:
                current_value = int(match.group(1))
            except (ValueError, IndexError) as e:
                print(f"   ⚠️  {variant}: Invalid timeout value '{match.group(1)}' - skipping")
                skipped_count += 1
                continue
            
            # Skip if already at safe value (idempotent)
            if current_value >= MIN_SAFE_TIMEOUT_MS:
                print(f"   ✅ {variant}: Already at safe timeout ({current_value}ms >= {MIN_SAFE_TIMEOUT_MS}ms)")
                already_patched_count += 1
                continue
            
            # Patch the content
            new_content = re.sub(
                r'#define\s+CONFIG_ESP_INT_WDT_TIMEOUT_MS\s+\d+',
                f'#define CONFIG_ESP_INT_WDT_TIMEOUT_MS {TARGET_TIMEOUT_MS}',
                content
            )
            
            # Also patch CONFIG_INT_WDT_TIMEOUT_MS if it exists as a standalone define
            # (not as an alias to CONFIG_ESP_INT_WDT_TIMEOUT_MS)
            if _should_patch_legacy_timeout(new_content):
                new_content = re.sub(
                    r'#define\s+CONFIG_INT_WDT_TIMEOUT_MS\s+\d+',
                    f'#define CONFIG_INT_WDT_TIMEOUT_MS {TARGET_TIMEOUT_MS}',
                    new_content
                )
            
            # Write to temporary file first, then rename (atomic write)
            temp_filepath = filepath + '.tmp'
            try:
                with open(temp_filepath, 'w') as f:
                    f.write(new_content)
                # Atomic rename (overwrites destination on Unix/Windows)
                os.replace(temp_filepath, filepath)
            except Exception as write_error:
                # Clean up temp file on error
                if os.path.exists(temp_filepath):
                    os.remove(temp_filepath)
                raise write_error
            
            print(f"   🔧 {variant}: Patched ({current_value}ms → {TARGET_TIMEOUT_MS}ms)")
            patched_count += 1
            
        except Exception as e:
            print(f"   ❌ {variant}: Error - {e}")
            print(f"      Continuing with remaining files...")
            skipped_count += 1
    
    print("")
    print(f"✅ Patching complete:")
    print(f"   • {patched_count} file(s) patched")
    print(f"   • {already_patched_count} file(s) already at safe timeout")
    if skipped_count > 0:
        print(f"   • {skipped_count} file(s) skipped (errors or not applicable)")
    print("")
    print("📝 Note: This patch persists in your PlatformIO installation.")
    print("   Re-run if you update the Arduino framework package.")
    print("   Monitor Arduino-ESP32 GitHub for permanent fix.")
    print("=" * 70)
    print("")

# Run the patch before build
patch_arduino_sdkconfig(env)

print("✅ Bootloop fix script loaded successfully")
print("")
