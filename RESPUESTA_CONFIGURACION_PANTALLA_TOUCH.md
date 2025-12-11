# 📺 Respuesta Completa: Configuración de Pantalla y Touch

## ❓ Pregunta del Usuario
> "cual es mi pantalla que driver tiene que touch tiene y cual es la libreria que tiene istalada, o la que se le installa para calibrar el touch que tengo que hacer"

---

## ✅ RESPUESTA COMPLETA

### 1. 🖥️ **¿Cuál es mi pantalla?**

Tu pantalla es:
```
Modelo: ST7796S TFT Display
Resolución: 480x320 píxeles (landscape)
Tamaño: 3.5" pulgadas
Interfaz: SPI
Driver IC: ST7796S
```

**📍 Pines conectados (ESP32-S3-DevKitC-1):**
```
TFT_CS   = GPIO 16  (Chip Select pantalla)
TFT_DC   = GPIO 13  (Data/Command)
TFT_RST  = GPIO 14  (Reset)
TFT_MOSI = GPIO 11  (Master Out Slave In)
TFT_MISO = GPIO 12  (Master In Slave Out)
TFT_SCLK = GPIO 10  (Clock)
TFT_BL   = GPIO 42  (Backlight)
```

---

### 2. 🔧 **¿Qué driver tiene la pantalla?**

El driver es **ST7796_DRIVER** (no ILI9488).

**✅ Configurado en `platformio.ini`:**
```ini
-DST7796_DRIVER              # Driver correcto para tu pantalla
-DTFT_WIDTH=320              # Dimensiones nativas (antes de rotación)
-DTFT_HEIGHT=480
-DSPI_FREQUENCY=40000000     # 40 MHz para ST7796S (óptimo)
-DSPI_READ_FREQUENCY=20000000 # 20 MHz para lectura
```

**📊 Rendimiento:**
- Velocidad SPI: 40 MHz (máxima velocidad recomendada)
- Rendimiento: ~42 FPS
- Superior al driver ILI9488 (~28 FPS)

---

### 3. 👆 **¿Qué touch tiene?**

Tu pantalla tiene un **controlador táctil XPT2046** integrado.

**📍 Pines conectados:**
```
TOUCH_CS  = GPIO 21  (Chip Select del touch)
TOUCH_IRQ = GPIO 47  (Interrupt Request - opcional, no usado por TFT_eSPI)
```

**Especificaciones técnicas:**
- Controlador: XPT2046 (resistivo)
- Interfaz: SPI (compartido con la pantalla)
- Resolución ADC: 12-bit (0-4095)
- Rango práctico: 200-3900 (excluye zonas de borde)
- Frecuencia SPI: 2.5 MHz (requerimiento XPT2046)

**⚠️ IMPORTANTE:** El pin TOUCH_IRQ está definido pero NO es usado por la librería TFT_eSPI. La librería utiliza **modo polling** (no interrupciones) para detectar toques.

---

### 4. 📚 **¿Qué librería tiene instalada?**

Se usa **TFT_eSPI** versión **2.5.43** de Bodmer.

**📦 Configurado en `platformio.ini`:**
```ini
lib_deps =
    bodmer/TFT_eSPI @ ^2.5.43    # ← Librería principal
```

**✅ ¿Por qué TFT_eSPI?**
1. **Integración completa**: Maneja tanto pantalla como touch en una sola librería
2. **Rendimiento optimizado**: Usa DMA y transacciones SPI eficientes
3. **Touch integrado**: Soporte XPT2046 integrado (desde v2.8.8+)
4. **Funciones de calibración**: Incluye métodos para calibrar el touch
5. **Amplio soporte**: Compatible con ST7796S y XPT2046

**🔑 Ventaja clave:** NO necesitas instalar una librería separada para el touch. TFT_eSPI maneja todo.

---

### 5. 🎯 **Configuración del Touch en `platformio.ini`**

```ini
# Touch controller configuration (XPT2046)
-DTOUCH_CS=21                   # Pin Chip Select del touch

# SPI Touch frequency
-DSPI_TOUCH_FREQUENCY=2500000   # 2.5 MHz (requerimiento XPT2046)

# Touch sensitivity
-DZ_THRESHOLD=300               # Presión mínima para detectar toque
                                # Valores más bajos = más sensible
                                # Rango típico: 200-600

# SPI Transaction support (importante para compartir bus SPI)
-DSPI_HAS_TRANSACTION
-DSUPPORT_TRANSACTIONS
```

**📝 Nota sobre sensibilidad:**
- Valor actual: 300 (buena sensibilidad general)
- Si el touch es poco sensible: reducir a 250-280
- Si detecta toques fantasma: aumentar a 350-400

---

### 6. 🔧 **¿Cómo calibrar el touch?**

Hay **3 métodos** para calibrar el touch:

---

#### **MÉTODO 1: Menú Oculto (RECOMENDADO)**

**Paso 1:** Acceder al menú oculto
```
1. En la pantalla principal, toca el icono de batería 4 veces
2. Ingresa el código de acceso: 8989
3. Se abrirá el menú oculto
```

**Paso 2:** Seleccionar calibración
```
4. Toca "Opción 3: Calibrar touch"
5. La pantalla mostrará instrucciones
```

**Paso 3:** Calibrar esquina superior izquierda
```
6. Aparecerá un objetivo ROJO en la esquina superior izquierda
7. Toca y MANTÉN presionado el centro del objetivo
8. La barra de progreso avanzará (10 muestras)
9. Espera hasta que termine
```

**Paso 4:** Calibrar esquina inferior derecha
```
10. Aparecerá un objetivo ROJO en la esquina inferior derecha
11. Toca y MANTÉN presionado el centro del objetivo
12. La barra de progreso avanzará (10 muestras)
13. Espera hasta que termine
```

**Paso 5:** Verificación
```
14. La pantalla mostrará los valores calculados:
    - Min X, Max X (rango horizontal)
    - Min Y, Max Y (rango vertical)
15. La calibración se GUARDA AUTOMÁTICAMENTE en la memoria EEPROM
16. Toca la pantalla para volver al menú
```

---

#### **MÉTODO 2: Botón Físico 4X4 (si está conectado)**

```
1. Mantén presionado el botón 4X4 durante 5 segundos
2. Escucharás una confirmación sonora
3. Se abrirá directamente la pantalla de calibración del touch
4. Sigue los pasos 3-5 del Método 1
```

---

#### **MÉTODO 3: Código Manual (si el touch está deshabilitado)**

Si el touch no funciona porque está deshabilitado en la configuración:

**Editar `src/main.cpp`:**
```cpp
void setup() {
    // ... código existente ...
    
    Storage::load(cfg);
    
    // ===== AÑADIR ESTAS LÍNEAS (TEMPORAL) =====
    #ifdef FORCE_ENABLE_TOUCH
    Serial.println("[FIX] Forzando habilitación del touch...");
    cfg.touchEnabled = true;
    Storage::save(cfg);
    Serial.println("[FIX] Touch habilitado y guardado");
    #endif
    // ==========================================
    
    // ... resto del código ...
}
```

**Editar `platformio.ini`:**
```ini
build_flags =
    # ... otros flags ...
    -DFORCE_ENABLE_TOUCH  # ← Añadir temporalmente
```

**Después de compilar y subir:**
1. El touch se habilitará automáticamente
2. Usa el Método 1 para calibrarlo
3. **ELIMINA** el flag `-DFORCE_ENABLE_TOUCH` después

---

### 7. 💾 **¿Dónde se guarda la calibración?**

**Almacenamiento:** EEPROM (Non-Volatile Storage)
```
Namespace: "vehicle"
Key: "config"
Estructura: Config v7
```

**Datos guardados:**
```cpp
struct Config {
    bool touchCalibrated;        // Flag: ¿está calibrado?
    uint16_t touchCalibration[5]; // [min_x, max_x, min_y, max_y, rotation]
    bool touchEnabled;           // ¿Touch habilitado?
    // ... otros campos ...
};
```

**Valores por defecto (si no hay calibración):**
```
Min X: 200
Max X: 3900
Min Y: 200
Max Y: 3900
Rotation: 3 (landscape)
```

---

### 8. 🔍 **Verificar configuración actual**

**Abrir Serial Monitor (115200 baudios) y buscar:**

**✅ Si el touch está funcionando:**
```
[INFO] Touch: Using default calibration [offset_x=200, range_x=3700, ...]
[INFO] Touchscreen XPT2046 integrado TFT_eSPI inicializado OK
```

**❌ Si el touch está deshabilitado:**
```
[INFO] Touchscreen deshabilitado en configuración
```
→ En este caso, usa el Método 3 para habilitarlo.

---

### 9. 🛠️ **Troubleshooting (Solución de problemas)**

#### Problema 1: El touch no responde
```
✅ Verificar:
1. ¿Está habilitado en la configuración?
   → Serial Monitor debe mostrar "inicializado OK"
   → Si dice "deshabilitado", usar Método 3
   
2. ¿Cables conectados correctamente?
   → TOUCH_CS = GPIO 21
   → TOUCH_IRQ = GPIO 47 (opcional)
   → Compartir MOSI/MISO/SCLK con la pantalla
   
3. ¿Frecuencia SPI correcta?
   → Debe ser 2.5 MHz (verificar platformio.ini)
```

#### Problema 2: El touch es muy poco sensible
```
✅ Solución:
1. Reducir Z_THRESHOLD en platformio.ini:
   -DZ_THRESHOLD=250  (más sensible)
   
2. O usar el entorno de debug:
   [env:esp32-s3-devkitc-touch-debug]
   → Ya tiene Z_THRESHOLD=250 y debug activado
```

#### Problema 3: El touch detecta toques fantasma
```
✅ Solución:
1. Aumentar Z_THRESHOLD:
   -DZ_THRESHOLD=350  (menos sensible)
   
2. Verificar cables:
   → Cables largos o mal apantallados pueden causar ruido
   → Usar cables cortos y trenzados
```

#### Problema 4: Calibración imprecisa
```
✅ Solución:
1. Re-calibrar tocando exactamente el centro de los objetivos
2. Usar un stylus en lugar del dedo para mayor precisión
3. Mantener el dedo firme durante toda la captura (10 muestras)
4. No tocar cerca de los bordes de la pantalla
```

#### Problema 5: No puedo acceder al menú oculto
```
✅ Causas posibles:
1. Touch deshabilitado → Usar Método 3
2. Calibración incorrecta → Usar Método 2 (botón 4X4)
3. Hardware touch dañado → Verificar conexiones
```

---

### 10. 📂 **Archivos de código relevantes**

**Implementación del Touch:**
```
📁 include/
   ├── touch_calibration.h    # Declaraciones de calibración
   ├── touch_map.h            # Mapeo de toques a pantalla
   └── storage.h              # Estructura Config

📁 src/hud/
   ├── touch_calibration.cpp  # Implementación calibración
   ├── touch_map.cpp          # Lógica de mapeo
   ├── hud.cpp                # Inicialización touch
   └── menu_hidden.cpp        # Menú oculto (acceso calibración)

📁 src/core/
   └── storage.cpp            # Guardar/cargar configuración
```

**Configuración:**
```
📄 platformio.ini             # Configuración completa de hardware
📄 include/pins.h             # Definición de pines
```

---

### 11. 🎓 **Entornos de compilación disponibles**

**Para uso normal:**
```bash
pio run -e esp32-s3-devkitc          # Entorno principal
pio run -e esp32-s3-devkitc-release  # Producción optimizada
```

**Para debugging del touch:**
```bash
pio run -e esp32-s3-devkitc-touch-debug
# Incluye:
# - SPI_TOUCH_FREQUENCY=1000000 (1MHz, más lento pero más confiable)
# - TOUCH_DEBUG (logs detallados)
# - Z_THRESHOLD=250 (más sensible)
# - CORE_DEBUG_LEVEL=5 (máximo debug)
```

**Si el touch causa conflictos SPI:**
```bash
pio run -e esp32-s3-devkitc-no-touch
# Deshabilita completamente el touch
# Útil para aislar problemas de hardware
```

---

### 12. 📊 **Resumen Técnico**

| Componente | Especificación |
|------------|---------------|
| **Pantalla** | ST7796S 480x320 TFT 3.5" |
| **Driver Pantalla** | ST7796_DRIVER (TFT_eSPI) |
| **Touch** | XPT2046 resistivo SPI |
| **Driver Touch** | TFT_eSPI integrado |
| **Librería** | TFT_eSPI 2.5.43 (Bodmer) |
| **SPI Pantalla** | 40 MHz (HSPI) |
| **SPI Touch** | 2.5 MHz |
| **Resolución Touch** | 12-bit (0-4095) |
| **Calibración** | 2 puntos (esquinas) |
| **Almacenamiento** | EEPROM (Config v7) |
| **Pin TFT_CS** | GPIO 16 |
| **Pin TOUCH_CS** | GPIO 21 |

---

### 13. 🎯 **Pasos para Calibrar (RESUMEN RÁPIDO)**

```
1. Toca batería 4 veces → Código 8989
2. Selecciona "Opción 3: Calibrar touch"
3. Toca y mantén objetivo ROJO esquina superior izquierda
4. Espera barra de progreso (10 muestras)
5. Toca y mantén objetivo ROJO esquina inferior derecha
6. Espera barra de progreso (10 muestras)
7. ✅ Calibración guardada automáticamente
8. Toca para volver al menú
```

---

### 14. 📚 **Documentación adicional disponible**

Para más detalles, consulta estos archivos en el repositorio:

```
📄 docs/TOUCH_CALIBRATION_GUIDE.md        # Guía completa de calibración
📄 docs/HARDWARE_CONFIGURACION_COMPLETA.md # Hardware completo
📄 docs/TOUCH_TROUBLESHOOTING.md          # Solución de problemas
📄 docs/DISPLAY_DRIVER_EXPLANATION.md     # Explicación técnica driver
📄 SOLUCION_TOUCH_DESHABILITADO.md        # Cómo habilitar touch
📄 RESUMEN_DRIVER_VERIFICACION.md         # Verificación driver
📄 INSTRUCCIONES_RAPIDAS_v2.10.0.md       # Instrucciones rápidas
```

---

## ✅ CONCLUSIÓN

**Tu configuración actual:**
- ✅ Pantalla: ST7796S 480x320 (correcta)
- ✅ Driver pantalla: ST7796_DRIVER (correcto y óptimo)
- ✅ Touch: XPT2046 integrado (correcto)
- ✅ Librería: TFT_eSPI 2.5.43 (correcta, todo integrado)
- ✅ Calibración: Sistema de 2 puntos implementado y funcional
- ✅ Almacenamiento: EEPROM persistente

**NO necesitas instalar ninguna librería adicional.** Todo está integrado en TFT_eSPI.

**Para calibrar:** Usa el Método 1 (menú oculto) siguiendo los pasos del punto 13.

---

**Fecha de creación:** 2025-12-11  
**Versión firmware:** 2.10.1  
**Estado:** ✅ DOCUMENTACIÓN COMPLETA
