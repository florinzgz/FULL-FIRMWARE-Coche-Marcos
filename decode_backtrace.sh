#!/bin/bash
# Script de Decodificación de Backtrace
# Para ESP32-S3 firmware
# Versión: 1.0
# Fecha: 2025-12-15

# Colores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}======================================"
echo "Decodificador de Backtrace ESP32-S3"
echo -e "======================================${NC}"
echo ""

# Verificar parámetros
if [ $# -lt 1 ]; then
    echo -e "${YELLOW}Uso:${NC}"
    echo "  $0 <entorno> [archivo_log]"
    echo ""
    echo "Ejemplos:"
    echo "  $0 esp32-s3-devkitc"
    echo "  $0 esp32-s3-devkitc error.log"
    echo "  $0 esp32-s3-devkitc-release"
    echo ""
    echo "Entornos disponibles:"
    echo "  - esp32-s3-devkitc"
    echo "  - esp32-s3-devkitc-release"
    echo "  - esp32-s3-devkitc-ota"
    echo "  - esp32-s3-devkitc-touch-debug"
    echo "  - esp32-s3-devkitc-predeployment"
    echo "  - esp32-s3-devkitc-no-touch"
    echo ""
    exit 1
fi

ENV=$1
LOG_FILE=$2

# Localizar el archivo .elf
ELF_FILE=$(find .pio/build/$ENV -name "firmware.elf" 2>/dev/null | head -1)

if [ -z "$ELF_FILE" ]; then
    echo -e "${RED}❌ Error: No se encontró firmware.elf para el entorno $ENV${NC}"
    echo ""
    echo "Asegúrate de haber compilado primero:"
    echo "  pio run -e $ENV"
    echo ""
    exit 1
fi

echo -e "${GREEN}✅ Archivo ELF encontrado:${NC} $ELF_FILE"
echo ""

# Verificar si addr2line está disponible
ADDR2LINE="xtensa-esp32s3-elf-addr2line"

if ! command -v $ADDR2LINE &> /dev/null; then
    echo -e "${YELLOW}⚠️  $ADDR2LINE no encontrado en PATH${NC}"
    echo ""
    echo "Opciones:"
    echo "1. Usar PlatformIO monitor con filtro automático:"
    echo "   pio device monitor --port COM4 --filter esp32_exception_decoder"
    echo ""
    echo "2. Instalar ESP32 toolchain manualmente"
    echo ""
    
    # Intentar encontrar en PlatformIO (múltiples ubicaciones posibles)
    PLATFORMIO_HOME="${PLATFORMIO_HOME:-$HOME/.platformio}"
    TOOLCHAIN_PATH=$(find "$PLATFORMIO_HOME/packages" -name "xtensa-esp32s3-elf-addr2line" 2>/dev/null | head -1)
    
    if [ -n "$TOOLCHAIN_PATH" ]; then
        echo -e "${GREEN}✅ Encontrado en PlatformIO:${NC} $TOOLCHAIN_PATH"
        ADDR2LINE="$TOOLCHAIN_PATH"
    else
        echo -e "${RED}❌ No se pudo localizar el toolchain${NC}"
        exit 1
    fi
fi

echo -e "${GREEN}✅ Toolchain encontrado:${NC} $ADDR2LINE"
echo ""

# Función para decodificar una dirección
decode_address() {
    local addr=$1
    echo -e "${BLUE}Decodificando: $addr${NC}"
    
    # Obtener función y línea
    local result=$($ADDR2LINE -e "$ELF_FILE" -f -C "$addr" 2>/dev/null)
    
    if [ $? -eq 0 ]; then
        echo "$result"
    else
        echo -e "${RED}Error decodificando dirección${NC}"
    fi
    echo ""
}

# Si se proporcionó un archivo de log
if [ -n "$LOG_FILE" ]; then
    if [ ! -f "$LOG_FILE" ]; then
        echo -e "${RED}❌ Error: Archivo $LOG_FILE no encontrado${NC}"
        exit 1
    fi
    
    echo -e "${BLUE}Procesando archivo de log: $LOG_FILE${NC}"
    echo ""
    
    # Buscar líneas de backtrace
    # Formato típico: "Backtrace: 0x40379990:0x3fcf0d50 0x0005002d:0xa5a5a5a5"
    
    BACKTRACES=$(grep -oP "Backtrace:\s+\K.*" "$LOG_FILE")
    
    if [ -z "$BACKTRACES" ]; then
        echo -e "${YELLOW}⚠️  No se encontraron backtraces en el archivo${NC}"
        echo ""
        echo "Asegúrate de que el archivo contiene el output del monitor serial"
        echo "con un error de panic (Guru Meditation Error)."
        exit 1
    fi
    
    echo -e "${GREEN}✅ Backtraces encontrados, decodificando...${NC}"
    echo ""
    echo "========================================"
    echo ""
    
    # Extraer direcciones y decodificar
    # Formato: 0x40379990:0x3fcf0d50 -> queremos 0x40379990
    
    while IFS= read -r line; do
        # Extraer todas las direcciones del formato 0xXXXXXXXX:0xXXXXXXXX
        addresses=$(echo "$line" | grep -oP '0x[0-9a-fA-F]+(?=:0x[0-9a-fA-F]+)')
        
        for addr in $addresses; do
            decode_address "$addr"
        done
    done <<< "$BACKTRACES"
    
else
    # Modo interactivo - pedir direcciones al usuario
    echo -e "${YELLOW}Modo interactivo: Introduce direcciones de memoria para decodificar${NC}"
    echo ""
    echo "Formato esperado: 0x40379990"
    echo "Puedes pegar una línea completa de backtrace y se extraerán las direcciones"
    echo "Escribe 'quit' o presiona Ctrl+C para salir"
    echo ""
    echo "Ejemplos de input válido:"
    echo "  0x40379990"
    echo "  Backtrace: 0x40379990:0x3fcf0d50 0x0005002d:0xa5a5a5a5"
    echo "  0x40379990 0x40379abc 0x40379def"
    echo ""
    
    while true; do
        echo -n "Dirección(es) > "
        read input
        
        if [ "$input" = "quit" ] || [ "$input" = "exit" ]; then
            echo "Saliendo..."
            break
        fi
        
        if [ -z "$input" ]; then
            continue
        fi
        
        # Extraer todas las direcciones hexadecimales del input
        addresses=$(echo "$input" | grep -oP '0x[0-9a-fA-F]+')
        
        if [ -z "$addresses" ]; then
            echo -e "${YELLOW}⚠️  No se encontraron direcciones válidas${NC}"
            echo ""
            continue
        fi
        
        echo ""
        for addr in $addresses; do
            decode_address "$addr"
        done
    done
fi

echo ""
echo -e "${BLUE}======================================"
echo "Decodificación completa"
echo -e "======================================${NC}"
