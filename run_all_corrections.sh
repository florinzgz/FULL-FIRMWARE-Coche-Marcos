#!/bin/bash

# =============================================================================
# SCRIPT AUTOMATIZADO PARA APLICAR TODAS LAS CORRECCIONES DEL FIRMWARE
# Ubicación: florinzgz/full-firmware-coche-marcos/run_all_corrections.sh
# =============================================================================

set -e  # Salir si hay algún error

echo "🚀 INICIANDO PROCESO DE CORRECCIÓN COMPLETO..."
echo "================================================"
echo ""

# Verificar que estamos en el directorio correcto
if [ ! -f "platformio.ini" ]; then
    echo "❌ ERROR: No se encuentra platformio.ini"
    echo "   Asegúrate de estar en la raíz del repositorio"
    echo "   cd florinzgz/full-firmware-coche-marcos"
    exit 1
fi

echo "✅ Directorio verificado: $(pwd)"
echo ""

# =============================================================================
# PASO 1: HACER EJECUTABLES TODOS LOS SCRIPTS
# =============================================================================
echo "🔧 PASO 1: HACIENDO EJECUTABLES TODOS LOS SCRIPTS..."
echo "===================================================="

# Verificar que existen los scripts
missing_scripts=()
for i in {1..7}; do
    script="create_script_part${i}.sh"
    if [ ! -f "$script" ]; then
        missing_scripts+=("$script")
    fi
done

if [ ${#missing_scripts[@]} -gt 0 ]; then
    echo "❌ ERROR: Faltan scripts:"
    for script in "${missing_scripts[@]}"; do
        echo "   - $script"
    done
    echo ""
    echo "   Asegúrate de tener todos los scripts create_script_part1.sh hasta create_script_part7.sh"
    exit 1
fi

# Hacer ejecutables todos los scripts
chmod +x create_script_part*.sh
echo "✅ Scripts de creación hechos ejecutables"

# Verificar permisos
echo ""
echo "📋 Permisos verificados:"
ls -la create_script_part*.sh | awk '{print "   " $1 " " $9}'
echo ""

# =============================================================================
# PASO 2: EJECUTAR SCRIPTS DE CREACIÓN EN SECUENCIA
# =============================================================================
echo "📝 PASO 2: EJECUTANDO SCRIPTS DE CREACIÓN..."
echo "============================================"

for i in {1..7}; do
    script="create_script_part${i}.sh"
    echo "   🔄 Ejecutando $script..."
    
    if ./"$script"; then
        echo "   ✅ $script completado"
    else
        echo "   ❌ ERROR en $script"
        exit 1
    fi
done

echo ""
echo "✅ Todos los scripts de creación ejecutados exitosamente"
echo ""

# Verificar que se creó apply_corrections.sh
if [ ! -f "apply_corrections.sh" ]; then
    echo "❌ ERROR: No se generó apply_corrections.sh"
    echo "   Verifica que todos los scripts se ejecutaron correctamente"
    exit 1
fi

# =============================================================================
# PASO 3: EJECUTAR EL SCRIPT DE CORRECCIONES
# =============================================================================
echo "🚀 PASO 3: EJECUTANDO CORRECCIONES DEL FIRMWARE..."
echo "================================================="

# Hacer ejecutable el script principal
chmod +x apply_corrections.sh
echo "✅ apply_corrections.sh hecho ejecutable"
echo ""

# Crear respaldo adicional antes de aplicar cambios
backup_dir="backup_before_corrections_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$backup_dir"

# Respaldar archivos críticos
critical_files=("platformio.ini" "include/pins.h" "include/relays.h" "src/control/relays.cpp")
echo "📋 Creando respaldos de seguridad en: $backup_dir"
for file in "${critical_files[@]}"; do
    if [ -f "$file" ]; then
        cp "$file" "$backup_dir/"
        echo "   ✅ Respaldo: $file"
    fi
done
echo ""

# Ejecutar las correcciones
echo "🔄 Aplicando correcciones..."
echo "----------------------------"
if ./apply_corrections.sh; then
    echo ""
    echo "✅ CORRECCIONES APLICADAS EXITOSAMENTE"
else
    echo ""
    echo "❌ ERROR en las correcciones"
    echo "   Los respaldos están en: $backup_dir"
    exit 1
fi

# =============================================================================
# PASO 4: VERIFICACIÓN FINAL
# =============================================================================
echo ""
echo "🔍 PASO 4: VERIFICACIÓN FINAL..."
echo "================================"

# Verificar que el firmware compila
echo "🔄 Compilando firmware para verificar..."
if pio run > compile_verification.log 2>&1; then
    echo "✅ FIRMWARE COMPILA CORRECTAMENTE"
else
    echo "❌ ERROR DE COMPILACIÓN"
    echo "   Ver detalles en: compile_verification.log"
    echo "   Los respaldos están en: $backup_dir"
    echo ""
    echo "📋 Últimas líneas del log de compilación:"
    tail -10 compile_verification.log
    exit 1
fi

# Verificar estado de Git
echo ""
echo "📋 Estado de Git después de las correcciones:"
git status --short | sed 's/^/   /'

# =============================================================================
# PASO 5: CREAR Y VERIFICAR greptile.yml
# =============================================================================
echo ""
echo "📝 PASO 5: VERIFICANDO CONFIGURACIÓN DE GREPTILE..."
echo "=================================================="

if [ -f "greptile.yml" ]; then
    echo "✅ greptile.yml ya existe"
    echo "   Verificando sintaxis YAML..."
    if python3 -c "import yaml; yaml.safe_load(open('greptile.yml'))" 2>/dev/null; then
        echo "✅ greptile.yml tiene sintaxis YAML válida"
    else
        echo "⚠️ greptile.yml puede tener errores de sintaxis"
    fi
else
    echo "⚠️ greptile.yml no encontrado"
    echo "   Necesitarás crear este archivo manualmente para Greptile"
fi

# =============================================================================
# PASO 6: VERIFICACIÓN DE CI/CD
# =============================================================================
echo ""
echo "📝 PASO 6: VERIFICANDO CONFIGURACIÓN CI/CD..."
echo "============================================="

if [ -f ".github/workflows/build.yml" ]; then
    echo "✅ GitHub Actions workflow encontrado"
    echo "   Archivo: .github/workflows/build.yml"
    echo "   El firmware se compilará automáticamente en GitHub"
else
    echo "⚠️ No se encontró workflow de GitHub Actions"
    echo "   Considera añadir .github/workflows/build.yml"
fi

# =============================================================================
# RESUMEN FINAL
# =============================================================================
echo ""
echo "🎉 PROCESO COMPLETADO EXITOSAMENTE"
echo "=================================="
echo ""
echo "✅ ACCIONES REALIZADAS:"
echo "   • Scripts de creación ejecutados (parte 1-7)"
echo "   • Correcciones aplicadas al firmware"
echo "   • GPIOs corregidos para ESP32-S3-DevKitC-1"
echo "   • Arquitectura por ejes implementada"
echo "   • Control físico de relés añadido"
echo "   • Touch CS corregido a GPIO 0"
echo "   • Firmware compilado exitosamente"
echo ""
echo "📋 ARCHIVOS MODIFICADOS:"
echo "   • include/pins.h - Nueva arquitectura por ejes"
echo "   • platformio.ini - TOUCH_CS corregido a GPIO 0"
echo "   • include/relays.h - APIs de monitoreo añadidas"
echo "   • src/control/relays.cpp - Implementación completa"
echo ""
echo "📁 RESPALDOS CREADOS:"
echo "   • $backup_dir/ (respaldos de seguridad)"
echo "   • backups_*/   (respaldos automáticos del script)"
echo ""
echo "🚀 PRÓXIMOS PASOS RECOMENDADOS:"
echo "   1. Verificar funcionamiento en hardware real"
echo "   2. Probar control de motores por ejes"
echo "   3. Validar sistemas de seguridad (ABS/TCS)"
echo "   4. Configurar Greptile si no está hecho"
echo "   5. git push origin main (para activar CI/CD)"
echo ""
echo "📞 SOPORTE:"
echo "   • Logs de compilación: compile_verification.log"
echo "   • Respaldos disponibles en: $backup_dir"
echo "   • Scripts ejecutados correctamente"
echo ""
echo "🏁 SCRIPT AUTOMATIZADO FINALIZADO CORRECTAMENTE"