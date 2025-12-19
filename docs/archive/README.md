# Documentación Archivada

Este directorio contiene documentación de funcionalidades que fueron removidas o están obsoletas.

## ⚠️ Advertencia

**Los documentos en este directorio NO representan la funcionalidad actual del firmware v2.11.0.**

Son mantenidos únicamente con propósitos históricos y de referencia.

---

## Documentos Archivados

### WiFi/OTA (Removido en v2.11.0)

#### CONFIGURACION_WIFI_OTA.md
- **Fecha de archivo:** 2025-12-19
- **Razón:** WiFi/OTA fue completamente removido en v2.11.0 por razones de seguridad
- **Última versión con WiFi/OTA:** v2.10.x
- **Alternativa actual:** Actualizaciones únicamente por USB

### Touch Documentation (Consolidado)

Los siguientes documentos fueron consolidados en las guías principales del directorio `docs/`:
- **TOUCH_CALIBRATION.md, TOUCH_CALIBRATION_GUIDE.md, TOUCH_CALIBRATION_IMPLEMENTATION.md** - Duplicados, ver TOUCH_CALIBRATION_QUICK_GUIDE.md
- **TOUCH_IMPLEMENTATION_CHANGES.md** - Cambios ya integrados en v2.11.0
- **README_TOUCH.md, SOLUCION_TOUCH.md, GUIA_RAPIDA_TOUCH.md** - Supersedidos por SOLUCION_COMPLETA_TOUCH_v2.9.4.md
- **TOUCH_FIX_v2.9.3.md** - Correcciones ya integradas en versiones posteriores
- **Fecha de archivo:** 2025-12-19
- **Documentación actual:** Ver TOUCH_CALIBRATION_QUICK_GUIDE.md, SOLUCION_COMPLETA_TOUCH_v2.9.4.md, TOUCH_TROUBLESHOOTING.md

### Informes y Auditorías Históricas

#### INFORME_AUDITORIA_2025-12-01.md
- **Versión:** v2.8.5
- **Fecha de archivo:** 2025-12-19
- **Razón:** Supersedido por INFORME_AUDITORIA_2025-12-07.md (v2.10.0)
- **Informe actual:** Ver docs/INFORME_AUDITORIA_2025-12-07.md

### Historical Reports (historical-reports/)

Directorio que contiene reportes históricos de desarrollo, fixes, verificaciones y summaries de versiones anteriores (v2.9.x - v2.11.0).

**Contenido:**
- Análisis de código de versiones anteriores
- Changelogs e instrucciones de versiones específicas
- Reportes de fixes (boot loop, IPC stack, pantalla, touch)
- Summaries de implementaciones y verificaciones
- Documentos de diagnóstico y estrategias de depuración
- PR summaries históricos

**Fecha de archivo:** 2025-12-19  
**Razón:** Consolidación - la información relevante está en:
- BUILD_INSTRUCTIONS_v2.11.0.md (root)
- CHANGELOG_v2.11.0.md (root)
- IMPLEMENTATION_SUMMARY_v2.11.1.md (root)
- SYSTEM_INIT_VALIDATIONS_v2.11.2.md (root)

---

## Documentación Actual

Para documentación actualizada del firmware v2.11.0, consultar:
- **[docs/README.md](../README.md)** - Índice principal de documentación
- **Root directory** - Archivos BUILD_INSTRUCTIONS, CHANGELOG, IMPLEMENTATION_SUMMARY más recientes
- **[platformio.ini](../../platformio.ini)** - Configuración de compilación actualizada

---

*Última actualización: 2025-12-19*
