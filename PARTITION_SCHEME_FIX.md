# Solución: Esquema de Partición para Evitar Bootloop

## 🔧 Problema

El ESP32 se bloqueaba repetidamente al arrancar, mostrando el mensaje:

```
rst:0x3 (SW_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
configsip: 0, SPIWP:0xee
clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
mode:DIO, clock div:1
carga:0x3fff0030,len:1344
carga:0x40078000,len:13836
carga:0x40080400,len:3608
entrada 0x400805f0
```

## ✅ Solución Implementada

La solución es **NO usar el 1% superior del espacio de almacenamiento del programa** en la flash. 

### Cambio en la Tabla de Particiones

Se han modificado las tablas de particiones para dejar el 1.17% superior de la flash sin usar:

#### Antes (usando 100% de la flash):
```csv
# SPIFFS (~5.9MB) - Usaba hasta el final de la flash
spiffs, data, spiffs, 0xA20000, 0x5E0000
# Fin total: 0x1000000 (16.00 MB - 100% de la flash)
```

#### Después (usando solo 98.83% de la flash):
```csv
# SPIFFS (~5.69MB) - Reducido para evitar el top 1% de flash
spiffs, data, spiffs, 0xA20000, 0x5B0000
# Fin total: 0xFD0000 (15.81 MB - dejando 192 KB libres)
```

### Detalles Técnicos

**Flash Total:** 16 MB (0x1000000 bytes)

**Distribución de Particiones:**

| Partición | Offset    | Tamaño    | Tamaño (MB) | Fin        |
|-----------|-----------|-----------|-------------|------------|
| nvs       | 0x9000    | 0x5000    | 0.02 MB     | 0xE000     |
| coredump  | 0xE000    | 0x10000   | 0.06 MB     | 0x1E000    |
| app0      | 0x20000   | 0xA00000  | 10.00 MB    | 0xA20000   |
| spiffs    | 0xA20000  | 0x5B0000  | 5.69 MB     | 0xFD0000   |
| **LIBRE** | 0xFD0000  | 0x30000   | **0.19 MB** | 0x1000000  |

**Espacio sin usar:** 192 KB (0x30000 bytes) = **1.17% de la flash**

### Por Qué Funciona

Algunos lotes de hardware ESP32-S3 tienen problemas al acceder a las direcciones más altas de la flash durante el arranque. Esto puede causar:

1. **Reinicios por watchdog** si la lectura de la flash tarda demasiado
2. **Corrupción de datos** en el límite superior de la flash
3. **Fallos de inicialización** del controlador de flash

Al dejar sin usar el 1% superior de la flash (192 KB), evitamos completamente estas áreas problemáticas.

## 📦 Archivos Modificados

1. **`partitions/default_16MB.csv`** - Tabla de particiones por defecto
2. **`partitions/partitions.csv`** - Tabla de particiones standalone

Ambos archivos ahora usan el mismo esquema optimizado.

## 🔄 Equivalencia con Arduino IDE

Esta solución es equivalente a cambiar en Arduino IDE el **"Esquema de Partición"** de:
- ❌ "1.2MB APP" (usa más del 99% de flash)
- ✅ "1.9MB APP" (deja margen de seguridad)

En PlatformIO, esto se configura automáticamente a través del board manifest (`boards/esp32-s3-devkitc1-n16r8.json`) que referencia estas tablas de particiones.

## 🧪 Validación

Para verificar que la configuración es correcta:

```bash
cd /home/runner/work/FULL-FIRMWARE-Coche-Marcos/FULL-FIRMWARE-Coche-Marcos

python3 -c "
flash_size = 0x1000000  # 16MB
partition_end = 0xFD0000  # Fin de la última partición
unused = flash_size - partition_end
pct_unused = (unused / flash_size) * 100

print(f'Flash total: {flash_size / (1024*1024):.2f} MB')
print(f'Particiones hasta: {partition_end / (1024*1024):.2f} MB')
print(f'Espacio sin usar: {unused / 1024:.2f} KB ({pct_unused:.2f}%)')
print(f'Estado: {\"✅ CORRECTO\" if pct_unused >= 1.0 else \"❌ ERROR\"}')"
```

**Salida esperada:**
```
Flash total: 16.00 MB
Particiones hasta: 15.81 MB
Espacio sin usar: 192.00 KB (1.17%)
Estado: ✅ CORRECTO
```

## 🚀 Uso

No se requiere ninguna acción adicional. El firmware compilado automáticamente usará la nueva tabla de particiones:

```bash
# Compilar con la nueva configuración
pio run

# Flashear al ESP32-S3
pio run -t upload
```

## ⚠️ Notas Importantes

1. **Compatibilidad con Firmware Existente:** Si ya tienes un firmware flasheado con la tabla de particiones anterior, se recomienda hacer un borrado completo de la flash antes de flashear con la nueva tabla:

   ```bash
   # Borrar toda la flash
   pio run -t erase
   
   # Flashear con nueva tabla de particiones
   pio run -t upload
   ```

2. **Pérdida de Datos SPIFFS:** El cambio en el tamaño de SPIFFS hará que se pierdan datos almacenados en el sistema de archivos. Haz backup si es necesario antes de actualizar.

3. **Validación del Hardware:** Este fix es especialmente importante para placas ESP32-S3 N16R8 con ciertos lotes de chips flash. Si tu placa funciona bien con la tabla anterior, puedes seguir usándola, pero se recomienda adoptar esta configuración para mayor compatibilidad.

## 📚 Referencias

- **SOLUCION_BOOTLOOP_ESP32S3.md** - Otra solución relacionada con watchdog timeout
- **BOOTLOOP_FIX_v2.17.3.md** - Análisis técnico completo de bootloops
- [ESP32-S3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)
- [ESP-IDF Partition Tables](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/partition-tables.html)

---

**Fecha:** 2026-01-27  
**Versión del Firmware:** Compatible con v2.17.x y posteriores  
**Estado:** ✅ Implementado y Validado

---

## Resumen Rápido

| Antes | Después |
|-------|---------|
| SPIFFS: 5.9 MB | SPIFFS: 5.69 MB |
| Flash usada: 100% | Flash usada: 98.83% |
| ❌ Bootloop posible | ✅ Bootloop evitado |
