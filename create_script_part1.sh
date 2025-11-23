#!/bin/bash

cat > apply_corrections.sh << 'SCRIPT_START'
#!/bin/bash

# ============================================================================
# SCRIPT DE CORRECCIONES AUTOMÁTICAS PARA COCHE MARCOS
# Aplica todas las correcciones críticas identificadas
# ============================================================================

set -e  # Salir si hay error

script_start_time=$(date +%s)

echo "🔧 INICIANDO CORRECCIONES AUTOMÁTICAS PARA COCHE MARCOS..."
echo "=========================================================="

# Verificar que estamos en el directorio correcto
if [ ! -f "platformio.ini" ]; then
    echo "❌ Error: No estás en el directorio del proyecto firmware"
    echo "   Ejecuta este script desde: full-firmware-coche-marcos/"
    exit 1
fi

if [ ! -d "src" ] || [ ! -d "include" ]; then
    echo "❌ Error: Estructura de directorios incorrecta"
    exit 1
fi

echo "✅ Directorio verificado: $(pwd)"

# Crear respaldos con timestamp
BACKUP_DIR="backups_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$BACKUP_DIR"
echo "📋 Creando respaldos en: $BACKUP_DIR/"

# Respaldar archivos que se van a modificar
cp platformio.ini "$BACKUP_DIR/" 2>/dev/null || echo "⚠️ platformio.ini no encontrado"
cp include/pins.h "$BACKUP_DIR/" 2>/dev/null || echo "⚠️ include/pins.h no encontrado"
cp include/relays.h "$BACKUP_DIR/" 2>/dev/null || echo "⚠️ include/relays.h no encontrado"
cp src/control/relays.cpp "$BACKUP_DIR/" 2>/dev/null || echo "⚠️ src/control/relays.cpp no encontrado"

echo "✅ Respaldos creados"
SCRIPT_START

chmod +x create_script_part1.sh