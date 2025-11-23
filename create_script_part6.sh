#!/bin/bash

cat >> apply_corrections.sh << 'SCRIPT_PART6'

# ============================================================================
# VERIFICACIÓN Y COMPILACIÓN
# ============================================================================
echo ""
echo "🔍 VERIFICANDO COMPILACIÓN..."
echo "=============================="

# Verificar que PlatformIO esté disponible
if ! command -v pio &> /dev/null; then
    echo "⚠️ AVISO: PlatformIO no está instalado o no está en PATH"
    echo "   Instala con: pip install platformio"
    echo "   O usa: python -m pip install platformio"
    echo ""
    echo "   En Ubuntu/Debian: sudo apt install python3-pip && pip3 install platformio"
    echo "   En Windows: pip install platformio"
    echo "   En macOS: brew install platformio"
else
    echo "✅ PlatformIO encontrado: $(pio --version 2>/dev/null || echo 'version unknown')"
    
    # Limpiar build anterior por si acaso
    echo "🧹 Limpiando build anterior..."
    pio run --target clean > /dev/null 2>&1 || true
    
    # Intentar compilar
    echo "🔄 Compilando firmware con las correcciones aplicadas..."
    
    if pio run > compile.log 2>&1; then
        echo "🎉 ✅ COMPILACIÓN EXITOSA!"
        echo "   Firmware compilado correctamente con todas las correcciones"
        
        # Mostrar información del binario compilado
        if [ -f ".pio/build/esp32-s3-devkitc/firmware.bin" ]; then
            # Obtener tamaño del firmware (compatible con macOS y Linux)
            if stat -f%z ".pio/build/esp32-s3-devkitc/firmware.bin" 2>/dev/null; then
                size=$(stat -f%z ".pio/build/esp32-s3-devkitc/firmware.bin")
            else
                size=$(stat -c%s ".pio/build/esp32-s3-devkitc/firmware.bin" 2>/dev/null || echo "unknown")
            fi
            echo "   📏 Tamaño firmware: $size bytes"
            
            # Calcular porcentaje de flash usado (16MB = 16777216 bytes)
            if [ "$size" != "unknown" ] && [ "$size" -gt 0 ]; then
                flash_total=16777216
                percentage=$((size * 100 / flash_total))
                echo "   📊 Uso de flash: ${percentage}% (${size}/${flash_total} bytes)"
                
                if [ $percentage -gt 80 ]; then
                    echo "   ⚠️ ADVERTENCIA: Uso de flash alto (>80%)"
                elif [ $percentage -gt 90 ]; then
                    echo "   🚨 CRÍTICO: Uso de flash muy alto (>90%)"
                else
                    echo "   ✅ Uso de flash dentro del rango normal"
                fi
            fi
            
            # Mostrar otras métricas útiles
            if [ -f ".pio/build/esp32-s3-devkitc/firmware.elf" ]; then
                echo "   📝 Archivos generados:"
                echo "      • firmware.bin (flash image)"
                echo "      • firmware.elf (debug symbols)"
                echo "      • partitions.bin (partition table)"
            fi
        fi
        
        # Limpiar log de compilación exitosa
        rm -f compile.log
    else
        echo "❌ ERROR EN COMPILACIÓN"
        echo "   ⚠️ Revisa los errores en el archivo compile.log"
        echo ""
        echo "   📋 Últimas 20 líneas del error:"
        echo "   ================================"
        tail -20 compile.log | sed 's/^/   │ /'
        echo ""
        echo "   💡 Para ver el log completo: cat compile.log"
        echo "   💡 Para limpiar y reintentar: rm compile.log && pio run --target clean && pio run"
    fi
fi

# Verificar integridad de archivos críticos
echo ""
echo "🔍 VERIFICANDO INTEGRIDAD DE ARCHIVOS CRÍTICOS..."
echo "=================================================="

critical_files=(
    "include/pins.h"
    "platformio.ini" 
    "include/relays.h"
    "src/control/relays.cpp"
)

for file in "${critical_files[@]}"; do
    if [ -f "$file" ]; then
        echo "✅ $file - OK"
    else
        echo "❌ $file - FALTA"
    fi
done

SCRIPT_PART6

chmod +x create_script_part6.sh