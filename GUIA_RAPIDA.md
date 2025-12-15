# 🎯 CALIBRAR TOUCH SIN PANTALLA - GUÍA RÁPIDA

## ¿TOUCH ROTO? ¡SOLUCIÓN EN 3 PASOS!

### PASO 1: Flashear Firmware
```bash
cd FULL-FIRMWARE-Coche-Marcos
pio run -t upload
```

### PASO 2: Presionar Botón 4X4
```
⏱️ Mantén presionado 5 SEGUNDOS
🔊 Escucharás confirmación
```

### PASO 3: Calibrar
```
📺 Sigue instrucciones en pantalla
✅ ¡Listo!
```

---

## COMPORTAMIENTO BOTÓN 4X4

| Tiempo | Acción |
|--------|--------|
| 👆 < 2s | Toggle 4X4 normal |
| 👆👆 2-5s | Sonido especial |
| 👆👆👆 **≥ 5s** | **🎯 CALIBRACIÓN** |

---

## DOCUMENTACIÓN COMPLETA

📖 **Ver:** `SOLUCION_v2.9.4.md`  
📖 **Ver:** `docs/SOLUCION_COMPLETA_TOUCH_v2.9.4.md`

---

## TROUBLESHOOTING

### ❌ Botón no responde
→ Verifica conexiones físicas  
→ Revisa Serial Monitor (115200 baud)

### ❌ Touch sigue sin funcionar
→ Intenta calibrar de nuevo  
→ Ajusta `Z_THRESHOLD=250` en platformio.ini  
→ Lee `docs/TOUCH_TROUBLESHOOTING.md`

### 🐛 Sistema crashea o entra en bucle (NUEVO v2.10.9)
→ Usa el build de debug:
```bash
pio run -e esp32-s3-devkitc-debug -t upload --upload-port COM4
```
→ Lee `INSTRUCCIONES_DEBUG_BUILD_v2.10.9.md` para más detalles

---

**v2.10.9** | **2025-12-15** | **✅ Debug build disponible**
