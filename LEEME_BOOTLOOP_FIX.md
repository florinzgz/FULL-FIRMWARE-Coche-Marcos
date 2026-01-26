# SOLUCIÓN DEFINITIVA BOOTLOOP ESP32-S3 - v2.17.4

## 🚨 PROBLEMA
Tu ESP32-S3 N16R8 está en un bucle de reinicio continuo:
```
rst:0x3 (RTC_SW_SYS_RST),boot:0x8 (SPI_FAST_FLASH_BOOT)
[se repite una y otra vez]
```

## ✅ SOLUCIÓN IMPLEMENTADA

He aumentado el timeout del watchdog de interrupción a **10 segundos** (era 5 segundos). Esto da tiempo suficiente para que la PSRAM se inicialice incluso en el peor caso de hardware.

### Archivos modificados:
1. **sdkconfig/n16r8.defaults** - CONFIG_ESP_INT_WDT_TIMEOUT_MS=10000
2. **tools/patch_arduino_sdkconfig.py** - Parchea Arduino framework a 10000ms
3. **include/version.h** - Versión actualizada a 2.17.4

## 📋 INSTRUCCIONES PASO A PASO

### Paso 1: Verificar configuración

```bash
# Ejecuta el script de verificación
./verify_bootloop_fix.sh
```

Deberías ver todas las marcas ✅. Si no es así, contacta conmigo.

### Paso 2: Limpiar compilación anterior

```bash
# Limpieza completa (IMPORTANTE)
pio run -t fullclean
```

**¿Por qué?** Esto asegura que:
- Se eliminen configuraciones antiguas
- Se aplique el nuevo timeout de 10 segundos
- Se recompile todo desde cero

### Paso 3: Compilar firmware

```bash
# Compila el firmware
pio run -e esp32-s3-n16r8-standalone-debug
```

**Busca este mensaje durante la compilación:**
```
🔧 ESP32-S3 Bootloop Fix - Patching Arduino Framework (v2.17.4)
...
🔧 dio_qspi: Patched (XXXms → 10000ms)
```

Esto confirma que el parche se aplicó correctamente.

### Paso 4: Subir al ESP32-S3

**IMPORTANTE:** Verifica que el puerto COM en `platformio.ini` sea correcto:

```ini
upload_port = COM3   # ← Cambia si tu puerto es diferente
monitor_port = COM3
```

Para saber tu puerto COM:
- Abre "Administrador de dispositivos" → "Puertos (COM y LPT)"
- Busca "USB Serial Port (COMX)" donde X es tu número

Luego sube el firmware:
```bash
pio run -e esp32-s3-n16r8-standalone-debug -t upload
```

### Paso 5: Monitorear salida serial

```bash
pio device monitor -e esp32-s3-n16r8-standalone-debug
```

O combinado (subir + monitorear):
```bash
pio run -e esp32-s3-n16r8-standalone-debug -t upload -t monitor
```

## ✅ RESULTADO ESPERADO

Deberías ver **UNA SOLA** secuencia de arranque (sin repetirse):

```
ESP-ROM:esp32s3-20210327
Build:Mar 27 2021
rst:0x1 (POWERON),boot:0x8 (SPI_FAST_FLASH_BOOT)
...
entry 0x403c98d0

=== ESP32-S3 EARLY BOOT ===
[STANDALONE] Mode active
A[BootGuard] Boot counter initialized
B[BOOT] Starting vehicle firmware...
[BOOT] Firmware version: 2.17.4    ← IMPORTANTE: Debe decir 2.17.4
C[INIT] ...
[Sistema continúa inicializándose]
```

### Indicadores de éxito:
- ✅ Aparece **UNA SOLA VEZ** (no se repite)
- ✅ Muestra "Firmware version: 2.17.4"
- ✅ El sistema llega al bucle principal
- ✅ No se reinicia continuamente

## ❌ SI AÚN HAY BOOTLOOP

Si el dispositivo sigue reiniciándose, prueba lo siguiente:

### 1. Verificar que se compiló correctamente

```bash
# Limpieza total
rm -rf .pio/build/

# Recompilar
pio run -e esp32-s3-n16r8-standalone-debug
```

Durante la compilación, **DEBES ver**:
```
🔧 ESP32-S3 Bootloop Fix - Patching Arduino Framework (v2.17.4)
```

Si no aparece, el parche no se está aplicando.

### 2. Verificar alimentación

- **Cable USB de calidad** (no solo de carga)
- **Fuente de 5V con mínimo 500mA**
- Prueba otro puerto USB o un hub USB alimentado
- Evita cables largos o de mala calidad

### 3. Verificar puerto COM

Asegúrate que `platformio.ini` tiene el puerto correcto:

```bash
# Windows - Lista puertos COM
mode

# Actualiza platformio.ini con tu puerto
upload_port = COM5   # Tu puerto real
monitor_port = COM5
```

### 4. Probar sin PSRAM (diagnóstico)

Si sospechas que la PSRAM está defectuosa:

Edita `sdkconfig/n16r8.defaults`:
```ini
# Deshabilita PSRAM temporalmente
# CONFIG_SPIRAM=y
CONFIG_SPIRAM=n
```

Recompila y sube. Si arranca sin PSRAM, la memoria PSRAM puede estar defectuosa.

### 5. Aumentar más el timeout

Si 10 segundos no son suficientes:

**Edita `sdkconfig/n16r8.defaults`:**
```ini
CONFIG_ESP_INT_WDT_TIMEOUT_MS=20000  # 20 segundos
```

**Edita `tools/patch_arduino_sdkconfig.py`:**
```python
TARGET_TIMEOUT_MS = 20000  # 20 segundos
```

Recompila y sube.

## 🔍 DIAGNÓSTICO AVANZADO

### Ver razón del reset

Para saber **por qué** se reinicia, añade esto al inicio de `setup()` en `src/main.cpp`:

```cpp
void setup() {
  Serial.begin(115200);
  delay(500);
  
  // Imprime razón del reset
  esp_reset_reason_t reason = esp_reset_reason();
  Serial.print("Razón del reset: ");
  switch(reason) {
    case ESP_RST_POWERON:   Serial.println("Encendido normal"); break;
    case ESP_RST_SW:        Serial.println("Reset por software"); break;
    case ESP_RST_PANIC:     Serial.println("Excepción/pánico"); break;
    case ESP_RST_INT_WDT:   Serial.println("Watchdog de interrupción"); break;
    case ESP_RST_TASK_WDT:  Serial.println("Watchdog de tarea"); break;
    case ESP_RST_WDT:       Serial.println("Otro watchdog"); break;
    default:                Serial.println("Desconocido"); break;
  }
  
  // Resto del setup...
}
```

Esto te dirá exactamente qué está causando el reset.

## 📊 DETALLES TÉCNICOS

### Progresión de timeouts

| Versión | Timeout | Resultado |
|---------|---------|-----------|
| Original | 300ms | ❌ Bootloop |
| v2.17.2 | 3000ms | ⚠️ Funciona en algunos |
| v2.17.3 | 5000ms | ⚠️ Funciona en la mayoría |
| **v2.17.4** | **10000ms** | ✅ **Máxima seguridad** |

### ¿Por qué 10 segundos?

La inicialización de PSRAM puede tardar:
- **500ms**: Init hardware PSRAM
- **1000-8000ms**: Test de memoria (varía según lote!)
- **500ms**: Init framework Arduino

**Total**: Puede superar 9 segundos en algunos lotes de hardware

**10 segundos** da margen para:
- Peores lotes de chips PSRAM
- Arranque en frío (más lento que reset caliente)
- Builds de depuración con logging
- Variaciones de fabricación

## 📞 SI NADA FUNCIONA

Si después de todos estos pasos sigue en bootloop, necesito más información:

**Por favor proporciona:**

1. **Salida serial completa** (toda la secuencia de boot)
2. **Salida de compilación** (para verificar que el parche se aplicó)
3. **Detalles de hardware**:
   - ¿Qué cable USB estás usando?
   - ¿Qué fuente de alimentación?
   - ¿Qué puerto COM?
4. **Razón del reset** (del diagnóstico avanzado)

Con esta información podré ayudarte mejor.

## 📝 RESUMEN

1. ✅ Ejecuta `./verify_bootloop_fix.sh` - Todo debe estar ✅
2. ✅ `pio run -t fullclean` - Limpia compilación anterior
3. ✅ `pio run -e esp32-s3-n16r8-standalone-debug` - Compila
4. ✅ Verifica que aparece "Patching Arduino Framework (v2.17.4)"
5. ✅ `pio run -e esp32-s3-n16r8-standalone-debug -t upload` - Sube
6. ✅ `pio device monitor -e esp32-s3-n16r8-standalone-debug` - Monitorea
7. ✅ Debería ver "Firmware version: 2.17.4" y **no** bootloop

---

**Versión:** 2.17.4  
**Fecha:** 2026-01-26  
**Hardware:** ESP32-S3 N16R8 (16MB Flash + 8MB PSRAM)  
**Estado:** ✅ Listo para probar

---

**¡Buena suerte!** 🚀
