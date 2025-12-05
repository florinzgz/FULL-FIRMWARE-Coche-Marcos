# 🎯 SOLUCIONES RÁPIDAS - TOUCH NO FUNCIONA

## 1️⃣ CALIBRAR CON BOTÓN (MÁS RÁPIDO)

```
1. Mantén botón 4X4 presionado 5 segundos
2. Escucha confirmación sonora
3. Sigue instrucciones en pantalla
```

📖 Guía: `docs/CALIBRACION_TOUCH_SIN_PANTALLA.md`

---

## 2️⃣ AJUSTAR SENSIBILIDAD

Edita `platformio.ini`:

```ini
-DZ_THRESHOLD=200              ; Más sensible (era 300)
-DSPI_TOUCH_FREQUENCY=1000000  ; Más lento (era 2500000)
```

Luego:
```bash
platformio run -t upload
```

---

## 3️⃣ USAR ENTORNO DEBUG

```bash
platformio run -e esp32-s3-devkitc-touch-debug --target upload
```

Ya incluye:
- ✅ Touch más lento (1MHz)
- ✅ Más sensible (Z=250)
- ✅ Logs activados

---

## 4️⃣ VERIFICAR HARDWARE

Con multímetro, verifica continuidad:

- T_CS → GPIO 21 ✅
- T_CLK → GPIO 10 ✅
- T_DIN → GPIO 11 ✅
- T_DO → GPIO 12 ✅

**IMPORTANTE:** T_CS debe ser GPIO 21, NO 16

---

## 5️⃣ VER LOGS

Serial Monitor (115200 baud), debe decir:

```
Touchscreen XPT2046 integrado TFT_eSPI inicializado OK
```

Si dice "not responding" → Problema hardware

---

## 📋 PRUEBAS COMPLETAS

Ver: `PRUEBAS_TOUCH_DIAGNOSTICO.md`

Incluye:
- 8 pruebas de diagnóstico paso a paso
- Tabla de síntomas y soluciones
- Checklist de verificación completa
- Plan de acción de 3 días

---

**Actualizado:** 2025-12-05
