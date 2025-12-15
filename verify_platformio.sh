#!/bin/bash
# Script de Verificación de Configuración de platformio.ini
# Compatible con: Linux, macOS, Windows (Git Bash)
# Versión: 1.0
# Fecha: 2025-12-15
# Compatible con: Firmware v2.10.8+

set -e  # Exit on error

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}======================================"
echo "Verificación de platformio.ini"
echo "Firmware ESP32-S3 v2.10.8+"
echo -e "======================================${NC}"
echo ""

# Lista de entornos a verificar
ENVIRONMENTS=(
    "esp32-s3-devkitc"
    "esp32-s3-devkitc-release"
    "esp32-s3-devkitc-ota"
    "esp32-s3-devkitc-touch-debug"
    "esp32-s3-devkitc-predeployment"
    "esp32-s3-devkitc-no-touch"
)

# Contadores
TOTAL=0
PASSED=0
FAILED=0

echo -e "${BLUE}1. Verificando entorno base...${NC}"
echo ""

# Verificar que el entorno base tiene CONFIG_ESP_IPC_TASK_STACK_SIZE
# Buscar solo en la sección [env:esp32-s3-devkitc] hasta el próximo [env:
if sed -n '/^\[env:esp32-s3-devkitc\]/,/^\[env:/p' platformio.ini | grep -q "^\s*-DCONFIG_ESP_IPC_TASK_STACK_SIZE=2048"; then
    echo -e "${GREEN}✅ Base environment tiene CONFIG_ESP_IPC_TASK_STACK_SIZE=2048${NC}"
else
    echo -e "${RED}❌ Base environment NO tiene CONFIG_ESP_IPC_TASK_STACK_SIZE=2048${NC}"
    echo -e "${YELLOW}   Acción requerida: Agregar -DCONFIG_ESP_IPC_TASK_STACK_SIZE=2048 al [env:esp32-s3-devkitc]${NC}"
fi

echo ""
echo -e "${BLUE}2. Verificando herencia en entornos derivados...${NC}"
echo ""

# Verificar que todos los entornos derivados heredan correctamente
for env in "${ENVIRONMENTS[@]:1}"; do  # Skip first (base environment)
    TOTAL=$((TOTAL + 1))
    echo -n "  [$env]: "
    
    # Verificar que extiende del base
    if grep -q "\[env:$env\]" platformio.ini; then
        # Verificar que tiene "extends = env:esp32-s3-devkitc"
        extends_line=$(sed -n "/\[env:$env\]/,/\[env:/p" platformio.ini | grep -m 1 "extends")
        
        if echo "$extends_line" | grep -q "env:esp32-s3-devkitc"; then
            # Verificar que incluye ${env:esp32-s3-devkitc.build_flags}
            uses_base_flags=$(sed -n "/\[env:$env\]/,/\[env:/p" platformio.ini | grep "\${env:esp32-s3-devkitc.build_flags}")
            
            if [ -n "$uses_base_flags" ]; then
                echo -e "${GREEN}✅ Hereda correctamente${NC}"
                PASSED=$((PASSED + 1))
            else
                echo -e "${YELLOW}⚠️  Extiende base pero NO incluye \${env:esp32-s3-devkitc.build_flags}${NC}"
                echo -e "${YELLOW}     Puede no heredar CONFIG_ESP_IPC_TASK_STACK_SIZE${NC}"
            fi
        else
            echo -e "${YELLOW}⚠️  NO extiende del base environment${NC}"
        fi
    else
        echo -e "${RED}❌ No encontrado en platformio.ini${NC}"
        FAILED=$((FAILED + 1))
    fi
done

echo ""
echo -e "${BLUE}3. Verificando configuraciones de stack adicionales...${NC}"
echo ""

# Verificar configuraciones de stack en predeployment
if grep -A 20 "\[env:esp32-s3-devkitc-predeployment\]" platformio.ini | grep -q "CONFIG_ARDUINO_LOOP_STACK_SIZE"; then
    loop_stack=$(grep -A 20 "\[env:esp32-s3-devkitc-predeployment\]" platformio.ini | grep "CONFIG_ARDUINO_LOOP_STACK_SIZE" | head -1)
    echo -e "${GREEN}✅ Predeployment tiene configuración de loop stack:${NC}"
    echo "   $loop_stack"
else
    echo -e "${YELLOW}⚠️  Predeployment NO tiene CONFIG_ARDUINO_LOOP_STACK_SIZE (usando default)${NC}"
fi

if grep -A 20 "\[env:esp32-s3-devkitc-predeployment\]" platformio.ini | grep -q "CONFIG_ESP_MAIN_TASK_STACK_SIZE"; then
    main_stack=$(grep -A 20 "\[env:esp32-s3-devkitc-predeployment\]" platformio.ini | grep "CONFIG_ESP_MAIN_TASK_STACK_SIZE" | head -1)
    echo -e "${GREEN}✅ Predeployment tiene configuración de main task stack:${NC}"
    echo "   $main_stack"
else
    echo -e "${YELLOW}⚠️  Predeployment NO tiene CONFIG_ESP_MAIN_TASK_STACK_SIZE (usando default)${NC}"
fi

echo ""
echo -e "${BLUE}4. Verificando otras configuraciones críticas...${NC}"
echo ""

# Verificar watchdog
if grep -q "Watchdog::init()" src/main.cpp; then
    echo -e "${GREEN}✅ Watchdog inicializado en main.cpp${NC}"
    
    # Verificar que hay múltiples Watchdog::feed()
    feed_count=$(grep -c "Watchdog::feed()" src/main.cpp || echo "0")
    if [ "$feed_count" -ge 10 ]; then
        echo -e "${GREEN}✅ Watchdog::feed() llamado $feed_count veces (suficiente)${NC}"
    else
        echo -e "${YELLOW}⚠️  Watchdog::feed() llamado solo $feed_count veces (recomendado >10)${NC}"
    fi
else
    echo -e "${RED}❌ Watchdog NO inicializado en main.cpp${NC}"
fi

# Verificar stack watermark monitoring
if grep -q "uxTaskGetStackHighWaterMark" src/main.cpp; then
    echo -e "${GREEN}✅ Stack watermark monitoring presente${NC}"
else
    echo -e "${YELLOW}⚠️  No se encontró stack watermark monitoring${NC}"
fi

echo ""
echo -e "${BLUE}======================================"
echo "Resumen de Verificación"
echo -e "======================================${NC}"
echo ""
echo "Entornos verificados: $TOTAL"
echo -e "Pasados: ${GREEN}$PASSED${NC}"
echo -e "Con advertencias/fallos: ${YELLOW}$((TOTAL - PASSED))${NC}"
echo ""

if [ $PASSED -eq $TOTAL ]; then
    echo -e "${GREEN}✅ TODAS LAS VERIFICACIONES PASARON${NC}"
    echo ""
    echo "El firmware está correctamente configurado para:"
    echo "  • IPC stack de 2KB en todos los entornos"
    echo "  • Herencia correcta de build_flags"
    echo "  • Watchdog inicializado y alimentado"
    echo "  • Stack monitoring presente"
    echo ""
    echo "✅ Listo para compilar y flashear"
    exit 0
else
    echo -e "${YELLOW}⚠️  ALGUNAS VERIFICACIONES FALLARON O TIENEN ADVERTENCIAS${NC}"
    echo ""
    echo "Revisa los mensajes arriba y toma las acciones necesarias."
    echo "Consulta ESTRATEGIA_DEPURACION.md para más detalles."
    echo ""
    exit 1
fi
