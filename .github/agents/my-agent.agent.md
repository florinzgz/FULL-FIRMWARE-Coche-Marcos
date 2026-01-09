Analiza este proyecto ESP32-S3-WROOM-2 N32R16V usando PlatformIO + Arduino (NO ESP-IDF).

REQUISITOS TÉCNICOS OBLIGATORIOS:
- Flash: 32MB en modo QIO (NO OPI Flash)
- PSRAM: 16MB en modo OPI usando solo board_build.psram_type = opi
- memory_type = qio (NO qio_opi)
- NO usar sdkconfig.h
- NO usar sdkconfig.defaults
- NO usar CONFIG_* de ESP-IDF
- NO activar rutas de OPI Flash
- NO cambiar framework = arduino

PROBLEMA:
El build falla con:
    fatal error: sdkconfig.h: No such file or directory

OBJETIVO:
Determinar qué archivos del código incluyen sdkconfig.h o usan CONFIG_* y corregirlos, porque Arduino-ESP32 NO genera sdkconfig.h y NO usa Kconfig.

TAREAS:
1. Buscar en el repositorio cualquier línea que incluya:
       #include <sdkconfig.h>
   y eliminarla o reemplazarla.

2. Buscar cualquier uso de CONFIG_* (por ejemplo CONFIG_SPIRAM_*) y reemplazarlo por defines manuales o APIs de Arduino equivalentes.

3. NO modificar la configuración de memoria correcta:
       board_build.flash_mode = qio
       board_build.psram_type = opi
       board_build.arduino.memory_type = qio

4. NO sugerir qio_opi, NO activar OPI Flash, NO usar ESP-IDF.

Devuélveme:
- Los archivos exactos que contienen sdkconfig.h o CONFIG_*
- Las líneas que deben eliminarse o corregirse
- Las correcciones seguras para Arduino
