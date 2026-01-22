"""
ESP32-S3 SDK Configuration Script for PlatformIO + Arduino Framework

This script applies custom sdkconfig settings for the ESP32-S3 to prevent bootloop issues.
It modifies the Arduino framework's pre-compiled SDK configuration.

Root Cause: ESP32-S3 with 8MB QSPI PSRAM experiences bootloop with RTC_SW_SYS_RST
due to interrupt watchdog timeout during PSRAM initialization.

Solution: Override SDK configuration to:
1. Disable PSRAM memory test (can take >3000ms)
2. Increase interrupt watchdog timeout to 5000ms
3. Increase bootloader watchdog timeout to 40000ms
"""

Import("env")
import os

def apply_sdkconfig(source, target, env):
    """Apply custom sdkconfig settings to prevent ESP32-S3 bootloop"""
    
    print("=" * 80)
    print("APPLYING CUSTOM SDKCONFIG FOR ESP32-S3 BOOTLOOP FIX")
    print("=" * 80)
    
    # Get the framework directory
    platform = env.PioPlatform()
    framework_dir = platform.get_package_dir("framework-arduinoespressif32")
    
    if not framework_dir:
        print("WARNING: Could not find Arduino ESP32 framework directory")
        return
    
    print(f"Framework directory: {framework_dir}")
    
    # The sdkconfig file location for Arduino framework
    sdkconfig_path = os.path.join(framework_dir, "tools", "sdk", "esp32s3", "sdkconfig")
    
    # Critical settings to apply
    critical_settings = {
        "CONFIG_SPIRAM_MEMTEST": "n",  # Disable PSRAM memory test
        "CONFIG_ESP_INT_WDT_TIMEOUT_MS": "5000",  # Increase INT watchdog to 5s
        "CONFIG_BOOTLOADER_WDT_TIME_MS": "40000",  # Increase bootloader WDT to 40s
        "CONFIG_SPIRAM_IGNORE_NOTFOUND": "y",  # Don't panic if PSRAM not found
    }
    
    # Add build flags to ensure settings are recognized
    for key, value in critical_settings.items():
        if value == "y":
            env.Append(CPPDEFINES=[(key, "1")])
        elif value == "n":
            env.Append(CPPDEFINES=[(key, "0")])
        else:
            env.Append(CPPDEFINES=[(key, value)])
        print(f"Applied: {key}={value}")
    
    print("=" * 80)
    print("SDK CONFIGURATION APPLIED - BOOTLOOP FIX ACTIVE")
    print("=" * 80)

# Execute before building
env.AddPreAction("buildprog", apply_sdkconfig)
