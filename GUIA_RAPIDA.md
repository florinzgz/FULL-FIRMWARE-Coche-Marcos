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

📖 **Ver:** `docs/SOLUCION_COMPLETA_TOUCH_v2.9.4.md`  
📖 **Ver:** `docs/TOUCH_CALIBRATION_QUICK_GUIDE.md`

---

## TROUBLESHOOTING

### ❌ Botón no responde
→ Verifica conexiones físicas  
→ Revisa Serial Monitor (115200 baud)

### ❌ Touch sigue sin funcionar
→ Intenta calibrar de nuevo  
→ Ajusta `Z_THRESHOLD=250` en platformio.ini  
→ Lee `docs/TOUCH_TROUBLESHOOTING.md`

### 🐛 Sistema crashea o entra en bucle
→ Revisa los logs de sistema  
→ Lee `BUILD_INSTRUCTIONS_v2.11.0.md` para compilar y depurar

---

**v2.11.0** | **2025-12-19** | **✅ Firmware standalone y seguro**
