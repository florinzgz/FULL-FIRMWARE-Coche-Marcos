Analiza este proyecto basado en {{MCU}} usando PlatformIO + Arduino (NO ESP-IDF).

CONFIGURACIÓN DEL PROYECTO (NO MODIFICAR):
- Plataforma: PlatformIO
- MCU: {{MCU}}
- framework = arduino
- Flash: {{FLASH_SIZE}} en modo QIO
- PSRAM: {{PSRAM_SIZE}} en modo OPI

CONFIGURACIÓN DE MEMORIA CORRECTA (INTOCABLE):
- board_build.flash_mode = qio
- board_build.psram_type = opi
- board_build.arduino.memory_type = qio

REQUISITOS TÉCNICOS OBLIGATORIOS:
1. NO usar ESP-IDF directamente.
2. NO usar sdkconfig.h.
3. NO usar sdkconfig.defaults.
4. NO usar ninguna macro CONFIG_* de ESP-IDF.
5. NO activar rutas de OPI Flash (solo PSRAM en OPI).
6. NO cambiar framework = arduino.
7. NO sugerir qio_opi, NO activar OPI Flash, NO migrar a ESP-IDF.

PROBLEMA:
El build falla con:
  fatal error: sdkconfig.h: No such file or directory

OBJETIVO:
Identificar qué archivos del proyecto incluyen sdkconfig.h o usan CONFIG_*, y corregirlos de forma segura para Arduino-ESP32.

TAREAS:

1) AUDITORÍA COMPLETA:
  - Buscar en TODO el repositorio:
      #include <sdkconfig.h>
      #include "sdkconfig.h"
      CONFIG_*
  - Para cada coincidencia, devolver:
      • Ruta del archivo
      • Número de línea
      • Línea exacta

2) CLASIFICAR ORIGEN:
  - Indicar si el archivo es:
      • Código propio
      • Librería incluida en el repo
  - NO analizar paquetes externos de PlatformIO fuera del repo.

3) PROPONER CORRECCIONES:
  Para cada include de sdkconfig.h:
    - Eliminarlo o reemplazarlo por includes válidos de Arduino.
  Para cada macro CONFIG_*:
    - Reemplazar por:
        • Defines manuales en un header central (por ejemplo config_arduino.h)
        • O APIs de Arduino equivalentes (ESP.getPsramSize(), etc.)
    - NO usar CONFIG_* nuevas.
    - NO usar sdkconfig.h.

4) RESTRICCIONES:
  - NO modificar:
      board_build.flash_mode = qio
      board_build.psram_type = opi
      board_build.arduino.memory_type = qio
  - NO sugerir qio_opi.
  - NO activar OPI Flash.
  - NO migrar a ESP-IDF.
  - NO usar Kconfig.

5) SALIDA ESPERADA:
  - Lista de archivos afectados.
  - Líneas exactas a corregir.
  - Corrección propuesta para cada caso.
  - Header sugerido con defines manuales (si aplica).
  - Confirmación de que la configuración de memoria NO ha sido alterada.

Quiero un análisis exhaustivo, preciso y seguro para PlatformIO + Arduino.
