#!/bin/bash
set -euo pipefail
# Quick verification script for ESP32-S3 bootloop fix v2.17.4
# This script verifies that all the bootloop fixes are properly configured

echo "========================================="
echo "ESP32-S3 Bootloop Fix Verification v2.17.4"
echo "========================================="
echo ""

# Check if we're in the right directory
if [ ! -f "platformio.ini" ]; then
    echo "❌ Error: platformio.ini not found"
    echo "   Please run this script from the project root directory"
    exit 1
fi

echo "✅ Found platformio.ini"
echo ""

# Check firmware version
echo "Checking firmware version..."
if grep -q "2.17.4" include/version.h; then
    echo "✅ Firmware version is 2.17.4"
else
    echo "❌ Firmware version is not 2.17.4"
    echo "   Current version:"
    grep "FIRMWARE_VERSION" include/version.h | head -1
fi
echo ""

# Check sdkconfig interrupt watchdog timeout
echo "Checking sdkconfig interrupt watchdog timeout..."
if grep -q "CONFIG_ESP_INT_WDT_TIMEOUT_MS=10000" sdkconfig/n16r8.defaults; then
    echo "✅ Interrupt watchdog timeout is 10000ms (10 seconds)"
else
    echo "❌ Interrupt watchdog timeout is not 10000ms"
    echo "   Current setting:"
    grep "CONFIG_ESP_INT_WDT_TIMEOUT_MS" sdkconfig/n16r8.defaults
fi
echo ""

# Check PSRAM memtest is disabled
echo "Checking PSRAM memtest..."
if grep -q "CONFIG_SPIRAM_MEMTEST=n" sdkconfig/n16r8.defaults; then
    echo "✅ PSRAM memtest is disabled"
else
    echo "⚠️  PSRAM memtest setting:"
    grep "CONFIG_SPIRAM_MEMTEST" sdkconfig/n16r8.defaults
fi
echo ""

# Check Arduino framework patch script
echo "Checking Arduino framework patch script..."
if grep -q "TARGET_TIMEOUT_MS = 10000" tools/patch_arduino_sdkconfig.py; then
    echo "✅ Patch script target timeout is 10000ms"
else
    echo "❌ Patch script target timeout is not 10000ms"
    echo "   Current setting:"
    grep "TARGET_TIMEOUT_MS" tools/patch_arduino_sdkconfig.py
fi
echo ""

# Check if patch script is enabled in platformio.ini
echo "Checking platformio.ini configuration..."
if grep -q "pre:tools/patch_arduino_sdkconfig.py" platformio.ini; then
    echo "✅ Arduino framework patch script is enabled"
else
    echo "❌ Arduino framework patch script is not enabled"
fi
echo ""

# Check sdkconfig reference
if grep -q "board_build.sdkconfig = sdkconfig/n16r8.defaults" platformio.ini; then
    echo "✅ sdkconfig/n16r8.defaults is configured"
else
    echo "❌ sdkconfig reference not found or incorrect"
fi
echo ""

echo "========================================="
echo "Verification Summary"
echo "========================================="
echo ""
echo "If all checks show ✅, the bootloop fix is properly configured."
echo ""
echo "Next steps:"
echo "1. Clean build: pio run -t fullclean"
echo "2. Build:       pio run -e esp32-s3-n16r8-standalone-debug"
echo "3. Upload:      pio run -e esp32-s3-n16r8-standalone-debug -t upload"
echo "4. Monitor:     pio device monitor -e esp32-s3-n16r8-standalone-debug"
echo ""
echo "Expected result: Device boots once, shows 'Firmware version: 2.17.4', no bootloop"
echo ""
