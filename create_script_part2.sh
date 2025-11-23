#!/bin/bash

cat >> apply_corrections.sh << 'SCRIPT_PART2'

# ============================================================================
# CORRECCIÓN 1: platformio.ini - COMPLETA
# ============================================================================
echo ""
echo "🔧 CORRECCIÓN 1: platformio.ini"
echo "--------------------------------"

cat > platformio.ini << 'EOF'
[env:esp32-s3-devkitc]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino

board_build.mcu = esp32s3
board_build.f_cpu = 240000000L
board_build.flash_size = 16MB
board_build.partitions = default.csv

monitor_speed = 115200
upload_speed = 921600

lib_deps =
    bodmer/TFT_eSPI @ ^2.5.43
    dfrobot/DFRobotDFPlayerMini @ ^1.0.6
    milesburton/DallasTemperature@^4.0.5
    paulstoffregen/OneWire@^2.3.8
    https://github.com/adafruit/Adafruit-PWM-Servo-Driver-Library.git
    RobTillaart/INA226 @ ^0.6.4
    https://github.com/PaulStoffregen/XPT2046_Touchscreen.git
    fastled/FastLED @ 3.6.0
    adafruit/Adafruit MCP23017 Arduino Library @ ^2.3.2

build_flags =
    -DCORE_DEBUG_LEVEL=5
    -DUSER_SETUP_LOADED
    -std=gnu++17
    -DLOAD_GLCD
    -DLOAD_FONT2
    -DLOAD_FONT4
    -DLOAD_FONT6
    -DLOAD_FONT7
    -DLOAD_FONT8
    -DSMOOTH_FONT
    -Iinclude
    
    ; ✅ CORRECCIÓN CRÍTICA: TFT configuration para ST7796S
    -DST7796_DRIVER
    -DTFT_WIDTH=480
    -DTFT_HEIGHT=320
    -DTFT_ROTATION=1
    -DTFT_INVERSION_ON
    -DTFT_RGB_ORDER=TFT_BGR
    -DSPI_FREQUENCY=27000000
    -DSPI_READ_FREQUENCY=20000000
    
    ; ✅ CORRECCIÓN CRÍTICA: Touch CS GPIO válido
    -DTOUCH_CS=0
    
    ; ✅ ARQUITECTURA POR EJES: Cada eje con su propio PCA9685
    -DPCA9685_FRONT_AXIS=0x40
    -DPCA9685_REAR_AXIS=0x41
    -DPCA9685_STEERING=0x42
    -DMCP23017_FRONT_AXIS=0x20
    -DMCP23017_REAR_AXIS=0x21
    
    ; ✅ SISTEMA DE SENSORES
    -DTCA9548A_MULTIPLEXOR=0x70
    -DINA226_BASE_ADDR=0x40
    
    ; ESP32-S3 específico
    -DARDUINO_ESP32S3_DEV
    -DBOARD_HAS_PSRAM
    
    ; Optimizaciones
    -O2
    -ffast-math
    
    ; Suprimir warnings
    -w
    -Wno-unused-variable
    -Wno-unused-function
    -Wno-unknown-pragmas
    -Wno-macro-redefined
EOF

echo "✅ platformio.ini corregido completamente"
SCRIPT_PART2

chmod +x create_script_part2.sh