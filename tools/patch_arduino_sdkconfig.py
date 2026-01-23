"""
ESP32-S3 Arduino Framework sdkconfig Patcher
v2.17.3 - Bootloop Fix

This script patches the Arduino framework's pre-compiled sdkconfig.h files
to fix bootloop issues on ESP32-S3 N16R8 hardware.

The issue: Arduino ESP32 framework ships with CONFIG_ESP_INT_WDT_TIMEOUT_MS=300
which is too short for PSRAM initialization on some hardware batches.

The fix: Patch the sdkconfig.h files to increase the timeout to 5000ms.

This is a workaround until the Arduino framework is updated with better defaults.

Author: Florin Zgureanu
Date: 2026-01-23
"""

Import("env")
import os
import re

def _should_patch_legacy_timeout(content):
    """
    Check if CONFIG_INT_WDT_TIMEOUT_MS should be patched.
    
    Returns True if CONFIG_INT_WDT_TIMEOUT_MS exists as a standalone define
    (not as an alias to CONFIG_ESP_INT_WDT_TIMEOUT_MS).
    
    Some older SDK versions have:
        #define CONFIG_INT_WDT_TIMEOUT_MS 300
    
    While newer versions have:
        #define CONFIG_INT_WDT_TIMEOUT_MS CONFIG_ESP_INT_WDT_TIMEOUT_MS
    
    We only want to patch the first case.
    """
    if '#define CONFIG_INT_WDT_TIMEOUT_MS ' not in content:
        return False
    
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
    """
    print("=" * 70)
    print("🔧 ESP32-S3 Bootloop Fix - Patching Arduino Framework (v2.17.3)")
    print("=" * 70)
    
    # Get the platform packages directory
    platform_dir = env.PioPlatform().get_package_dir("framework-arduinoespressif32")
    
    if not platform_dir or not os.path.isdir(platform_dir):
        print("⚠️  Arduino framework not found - skipping patch")
        return
    
    # Find all ESP32-S3 sdkconfig.h files
    sdk_dir = os.path.join(platform_dir, "tools", "sdk", "esp32s3")
    
    if not os.path.isdir(sdk_dir):
        print(f"⚠️  ESP32-S3 SDK directory not found at: {sdk_dir}")
        return
    
    # Pattern to find sdkconfig.h files
    sdkconfig_files = []
    for root, dirs, files in os.walk(sdk_dir):
        for file in files:
            if file == "sdkconfig.h":
                sdkconfig_files.append(os.path.join(root, file))
    
    if not sdkconfig_files:
        print("⚠️  No sdkconfig.h files found")
        return
    
    print(f"📁 Found {len(sdkconfig_files)} sdkconfig.h file(s) to patch")
    print("")
    
    # Patch each file
    patched_count = 0
    already_patched_count = 0
    
    for filepath in sdkconfig_files:
        variant = os.path.basename(os.path.dirname(os.path.dirname(filepath)))
        
        try:
            # Read the file
            with open(filepath, 'r') as f:
                content = f.read()
            
            # Check current value
            match = re.search(r'#define\s+CONFIG_ESP_INT_WDT_TIMEOUT_MS\s+(\d+)', content)
            
            if not match:
                print(f"   ⚠️  {variant}: CONFIG_ESP_INT_WDT_TIMEOUT_MS not found")
                continue
            
            current_value = match.group(1)
            
            if current_value == "5000":
                print(f"   ✅ {variant}: Already patched (5000ms)")
                already_patched_count += 1
                continue
            
            # Patch the content
            new_content = re.sub(
                r'#define\s+CONFIG_ESP_INT_WDT_TIMEOUT_MS\s+\d+',
                '#define CONFIG_ESP_INT_WDT_TIMEOUT_MS 5000',
                content
            )
            
            # Also patch CONFIG_INT_WDT_TIMEOUT_MS if it exists as a standalone define
            # (not as an alias to CONFIG_ESP_INT_WDT_TIMEOUT_MS)
            if _should_patch_legacy_timeout(new_content):
                new_content = re.sub(
                    r'#define\s+CONFIG_INT_WDT_TIMEOUT_MS\s+\d+',
                    '#define CONFIG_INT_WDT_TIMEOUT_MS 5000',
                    new_content
                )
            
            # Write back
            with open(filepath, 'w') as f:
                f.write(new_content)
            
            print(f"   🔧 {variant}: Patched ({current_value}ms → 5000ms)")
            patched_count += 1
            
        except Exception as e:
            print(f"   ❌ {variant}: Error - {e}")
    
    print("")
    print(f"✅ Patching complete:")
    print(f"   • {patched_count} file(s) patched")
    print(f"   • {already_patched_count} file(s) already correct")
    print("")
    print("📝 Note: This patch persists in your PlatformIO installation.")
    print("   Re-run if you update the Arduino framework package.")
    print("=" * 70)
    print("")

# Run the patch before build
patch_arduino_sdkconfig(env)

print("✅ Bootloop fix script loaded successfully")
print("")
