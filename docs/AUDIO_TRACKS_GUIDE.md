# 🔊 Guía de Audios para DFPlayer Mini

**Versión:** 2.12.0  
**Última actualización:** 2025-12-19

---

## ⚠️ IMPORTANTE: Cómo Grabar los Audios

**Los archivos MP3 NO están incluidos.** Debes grabarlos tú siguiendo estos pasos:

### 📝 Pasos Rápidos para Grabar

1. **Ir a [TTSMaker.com](https://ttsmaker.com/)**
2. **Seleccionar idioma:** Español (España)
3. **Copiar el texto** de la columna "Texto Sugerido" de las tablas de abajo
4. **Clic en "Convertir a Voz"**
5. **Descargar el MP3**
6. **Renombrar** el archivo a `XXXX.mp3` (ejemplo: `0001.mp3`, `0039.mp3`)
7. **Copiar todos los archivos** a la raíz de la tarjeta SD

### 💾 Requisitos de la Tarjeta SD
- **Formato:** FAT32
- **Capacidad:** 1GB - 32GB  
- **Velocidad:** Clase 4 o superior
- **Archivos:** Copiar directamente en la raíz (no en carpetas)

---

## 📋 Audios Implementados (68 Tracks)

Los archivos de audio deben copiarse a la tarjeta SD del DFPlayer Mini con el formato `XXXX.mp3` (4 dígitos).

### Sistema Principal (Tracks 1-3)
| Track | Archivo | Descripción | Texto Sugerido |
|-------|---------|-------------|----------------|
| 0001 | `0001.mp3` | AUDIO_INICIO | "Bienvenido Marcos. El sistema está listo para comenzar." |
| 0002 | `0002.mp3` | AUDIO_APAGADO | "Cerrando sistemas. Hasta pronto." |
| 0003 | `0003.mp3` | AUDIO_ERROR_GENERAL | "Atención. Se ha detectado un error general." |

### Calibración de Pedal (Tracks 4-5)
| Track | Archivo | Descripción | Texto Sugerido |
|-------|---------|-------------|----------------|
| 0004 | `0004.mp3` | AUDIO_PEDAL_OK | "Calibración del pedal completada correctamente." |
| 0005 | `0005.mp3` | AUDIO_PEDAL_ERROR | "Error en el sensor del pedal. Revise la conexión." |

### Sensores de Corriente (Tracks 6-7)
| Track | Archivo | Descripción | Texto Sugerido |
|-------|---------|-------------|----------------|
| 0006 | `0006.mp3` | AUDIO_INA_OK | "Calibración de sensores de corriente finalizada." |
| 0007 | `0007.mp3` | AUDIO_INA_ERROR | "Error en sensores de corriente o shunt desconectado." |

### Encoder de Dirección (Tracks 8-9)
| Track | Archivo | Descripción | Texto Sugerido |
|-------|---------|-------------|----------------|
| 0008 | `0008.mp3` | AUDIO_ENCODER_OK | "Encoder sincronizado correctamente." |
| 0009 | `0009.mp3` | AUDIO_ENCODER_ERROR | "Error en el sensor de dirección. Compruebe el encoder." |

### Temperatura (Tracks 10-11)
| Track | Archivo | Descripción | Texto Sugerido |
|-------|---------|-------------|----------------|
| 0010 | `0010.mp3` | AUDIO_TEMP_ALTA | "Temperatura del motor elevada. Reduzca la velocidad." |
| 0011 | `0011.mp3` | AUDIO_TEMP_NORMAL | "Temperatura del motor normalizada." |

### Batería (Tracks 12-13)
| Track | Archivo | Descripción | Texto Sugerido |
|-------|---------|-------------|----------------|
| 0012 | `0012.mp3` | AUDIO_BATERIA_BAJA | "Nivel de batería bajo. Conecte el cargador, por favor." |
| 0013 | `0013.mp3` | AUDIO_BATERIA_CRITICA | "Advertencia. Batería en nivel crítico. Desconectando tracción." |

### Freno de Estacionamiento (Tracks 14-15)
| Track | Archivo | Descripción | Texto Sugerido |
|-------|---------|-------------|----------------|
| 0014 | `0014.mp3` | AUDIO_FRENO_ON | "Freno de estacionamiento activado." |
| 0015 | `0015.mp3` | AUDIO_FRENO_OFF | "Freno de estacionamiento desactivado." |

### Luces (Tracks 16-17)
| Track | Archivo | Descripción | Texto Sugerido |
|-------|---------|-------------|----------------|
| 0016 | `0016.mp3` | AUDIO_LUCES_ON | "Luces encendidas." |
| 0017 | `0017.mp3` | AUDIO_LUCES_OFF | "Luces apagadas." |

### Radio/Multimedia (Tracks 18-19)
| Track | Archivo | Descripción | Texto Sugerido |
|-------|---------|-------------|----------------|
| 0018 | `0018.mp3` | AUDIO_RADIO_ON | "Sistema multimedia activado." |
| 0019 | `0019.mp3` | AUDIO_RADIO_OFF | "Sistema multimedia desactivado." |

### Marchas (Tracks 20-24)
| Track | Archivo | Descripción | Texto Sugerido |
|-------|---------|-------------|----------------|
| 0020 | `0020.mp3` | AUDIO_MARCHA_D1 | "Marcha D uno activada." |
| 0021 | `0021.mp3` | AUDIO_MARCHA_D2 | "Marcha D dos activada." |
| 0022 | `0022.mp3` | AUDIO_MARCHA_R | "Marcha atrás activada." |
| 0023 | `0023.mp3` | AUDIO_MARCHA_N | "Punto muerto." |
| 0024 | `0024.mp3` | AUDIO_MARCHA_P | "Vehículo en posición de estacionamiento." |

### Menú Oculto y Calibración (Tracks 25-28)
| Track | Archivo | Descripción | Texto Sugerido |
|-------|---------|-------------|----------------|
| 0025 | `0025.mp3` | AUDIO_MENU_OCULTO | "Menú de calibración avanzado activado." |
| 0026 | `0026.mp3` | AUDIO_CAL_PEDAL | "Iniciando calibración del pedal. Presione lentamente hasta el fondo." |
| 0027 | `0027.mp3` | AUDIO_CAL_INA | "Calibrando sensores de corriente. Espere unos segundos." |
| 0028 | `0028.mp3` | AUDIO_CAL_ENCODER | "Calibrando el punto central del volante. Manténgalo recto." |

### Test del Sistema (Tracks 29-30)
| Track | Archivo | Descripción | Texto Sugerido |
|-------|---------|-------------|----------------|
| 0029 | `0029.mp3` | AUDIO_TEST_SISTEMA | "Iniciando comprobación completa del sistema." |
| 0030 | `0030.mp3` | AUDIO_TEST_OK | "Comprobación finalizada. Todos los módulos operativos." |

### Emergencia y Seguridad (Tracks 31-32)
| Track | Archivo | Descripción | Texto Sugerido |
|-------|---------|-------------|----------------|
| 0031 | `0031.mp3` | AUDIO_EMERGENCIA | "Modo de emergencia activado. Motor deshabilitado." |
| 0032 | `0032.mp3` | AUDIO_REINICIO_SEGURIDAD | "Reinicio de seguridad completado." |

### Errores de Sensores Específicos (Tracks 33-35)
| Track | Archivo | Descripción | Texto Sugerido |
|-------|---------|-------------|----------------|
| 0033 | `0033.mp3` | AUDIO_SENSOR_TEMP_ERROR | "Error en sensor de temperatura." |
| 0034 | `0034.mp3` | AUDIO_SENSOR_CORRIENTE_ERROR | "Anomalía en lectura de corriente." |
| 0035 | `0035.mp3` | AUDIO_SENSOR_VELOCIDAD_ERROR | "Sin señal de velocidad. Revise sensores de rueda." |

### Estado de Módulos (Track 36)
| Track | Archivo | Descripción | Texto Sugerido |
|-------|---------|-------------|----------------|
| 0036 | `0036.mp3` | AUDIO_MODULO_OK | "Módulo verificado correctamente." |

### Tracción 4x4/4x2 (Tracks 37-38)
| Track | Archivo | Descripción | Texto Sugerido |
|-------|---------|-------------|----------------|
| 0037 | `0037.mp3` | AUDIO_TRACCION_4X4 | "Tracción 4x4 inteligente activada." |
| 0038 | `0038.mp3` | AUDIO_TRACCION_4X2 | "Tracción 4x2 inteligente activada." |

---

## 🆕 Audios Avanzados - IMPLEMENTADOS (Tracks 39-68)

> ✅ **Ya implementados en `include/alerts.h`** - Solo necesitas grabar los MP3

### Sistemas de Seguridad Avanzados (Tracks 39-44)
| Track | Archivo | Constante | Texto para Grabar |
|-------|---------|-----------|-------------------|
| 0039 | `0039.mp3` | AUDIO_ABS_ACTIVADO | "Sistema antibloqueo de frenos activado." |
| 0040 | `0040.mp3` | AUDIO_ABS_DESACTIVADO | "Sistema antibloqueo de frenos desactivado." |
| 0041 | `0041.mp3` | AUDIO_TCS_ACTIVADO | "Control de tracción activado." |
| 0042 | `0042.mp3` | AUDIO_TCS_DESACTIVADO | "Control de tracción desactivado." |
| 0043 | `0043.mp3` | AUDIO_REGEN_ON | "Frenado regenerativo activado." |
| 0044 | `0044.mp3` | AUDIO_REGEN_OFF | "Frenado regenerativo desactivado." |

### WiFi y Conectividad (Tracks 45-48)
| Track | Archivo | Constante | Texto para Grabar |
|-------|---------|-----------|-------------------|
| 0045 | `0045.mp3` | AUDIO_WIFI_CONECTADO | "Conexión WiFi establecida." |
| 0046 | `0046.mp3` | AUDIO_WIFI_DESCONECTADO | "Conexión WiFi perdida." |
| 0047 | `0047.mp3` | AUDIO_OTA_INICIADO | "Actualización remota iniciada. No desconecte el vehículo." |
| 0048 | `0048.mp3` | AUDIO_OTA_COMPLETADO | "Actualización completada. Reiniciando sistema." |

### Bluetooth (Tracks 49-51)
| Track | Archivo | Constante | Texto para Grabar |
|-------|---------|-----------|-------------------|
| 0049 | `0049.mp3` | AUDIO_BT_CONECTADO | "Mando Bluetooth conectado." |
| 0050 | `0050.mp3` | AUDIO_BT_DESCONECTADO | "Mando Bluetooth desconectado." |
| 0051 | `0051.mp3` | AUDIO_BT_EMPAREJANDO | "Buscando mando Bluetooth. Mantenga pulsado el botón de emparejamiento." |

### Estados del Vehículo (Tracks 52-56)
| Track | Archivo | Constante | Texto para Grabar |
|-------|---------|-----------|-------------------|
| 0052 | `0052.mp3` | AUDIO_VELOCIDAD_MAXIMA | "Velocidad máxima alcanzada." |
| 0053 | `0053.mp3` | AUDIO_SOBRECORRIENTE | "Advertencia. Corriente excesiva detectada." |
| 0054 | `0054.mp3` | AUDIO_OBSTACULO | "Atención. Obstáculo detectado." |
| 0055 | `0055.mp3` | AUDIO_ESTACIONANDO | "Modo asistencia de estacionamiento activado." |
| 0056 | `0056.mp3` | AUDIO_ARRANQUE_SUAVE | "Iniciando arranque suave de motores." |

### Información de Telemetría (Tracks 57-60)
| Track | Archivo | Constante | Texto para Grabar |
|-------|---------|-----------|-------------------|
| 0057 | `0057.mp3` | AUDIO_BATERIA_50 | "Nivel de batería al 50 por ciento." |
| 0058 | `0058.mp3` | AUDIO_BATERIA_25 | "Nivel de batería al 25 por ciento. Considere recargar." |
| 0059 | `0059.mp3` | AUDIO_DISTANCIA_1KM | "Ha recorrido un kilómetro en esta sesión." |
| 0060 | `0060.mp3` | AUDIO_AHORRO_ENERGIA | "Modo ahorro de energía activado." |

### Modos de Conducción (Tracks 61-63)
| Track | Archivo | Constante | Texto para Grabar |
|-------|---------|-----------|-------------------|
| 0061 | `0061.mp3` | AUDIO_MODO_ECO | "Modo eco activado. Máxima eficiencia." |
| 0062 | `0062.mp3` | AUDIO_MODO_NORMAL | "Modo normal activado." |
| 0063 | `0063.mp3` | AUDIO_MODO_SPORT | "Modo deportivo activado. Máxima potencia." |

### Feedback de Configuración (Tracks 64-68)
| Track | Archivo | Constante | Texto para Grabar |
|-------|---------|-----------|-------------------|
| 0064 | `0064.mp3` | AUDIO_CONFIG_GUARDADA | "Configuración guardada correctamente." |
| 0065 | `0065.mp3` | AUDIO_CONFIG_RESTAURADA | "Configuración de fábrica restaurada." |
| 0066 | `0066.mp3` | AUDIO_ERRORES_BORRADOS | "Registro de errores borrado." |
| 0067 | `0067.mp3` | AUDIO_REGEN_AJUSTADO | "Nivel de regeneración ajustado." |
| 0068 | `0068.mp3` | AUDIO_BEEP | *(Sonido corto de confirmación - buscar "beep sound" en YouTube)* |

---

## 📁 Estructura Final de la Tarjeta SD

```
SD Card (FAT32)
├── 0001.mp3    (AUDIO_INICIO)
├── 0002.mp3    (AUDIO_APAGADO)
├── 0003.mp3    (AUDIO_ERROR_GENERAL)
├── ...
├── 0038.mp3    (AUDIO_TRACCION_4X2)
├── 0039.mp3    (AUDIO_ABS_ACTIVADO)
├── 0040.mp3    (AUDIO_ABS_DESACTIVADO)
├── ...
└── 0068.mp3    (AUDIO_BEEP)
```

**Total: 68 archivos MP3**

---

## 🎤 Métodos para Grabar los Audios

### ✅ Método 1: TTSMaker (RECOMENDADO - Gratis)

1. Abrir **[ttsmaker.com](https://ttsmaker.com/)**
2. Configurar:
   - Idioma: **Spanish (Spain)**
   - Voz: Seleccionar una voz que te guste
3. Pegar el texto de la tabla
4. Clic en **"Start to Convert"**
5. Clic en **"Download MP3"**
6. Renombrar el archivo descargado a `XXXX.mp3`
7. Repetir para cada track

### Método 2: Natural Readers (Gratis)

1. Ir a **[naturalreaders.com](https://www.naturalreaders.com/)**
2. Seleccionar voz española
3. Pegar texto → Descargar

### Método 3: Script Python con gTTS

```python
from gtts import gTTS

# Diccionario completo de textos (tracks 1-68)
textos = {
    # Sistema principal
    "0001": "Bienvenido Marcos. El sistema está listo para comenzar.",
    "0002": "Cerrando sistemas. Hasta pronto.",
    "0003": "Atención. Se ha detectado un error general.",
    
    # Calibración pedal
    "0004": "Calibración del pedal completada correctamente.",
    "0005": "Error en el sensor del pedal. Revise la conexión.",
    
    # Sensores de corriente
    "0006": "Calibración de sensores de corriente finalizada.",
    "0007": "Error en sensores de corriente o shunt desconectado.",
    
    # Encoder dirección
    "0008": "Encoder sincronizado correctamente.",
    "0009": "Error en el sensor de dirección. Compruebe el encoder.",
    
    # Temperatura
    "0010": "Temperatura del motor elevada. Reduzca la velocidad.",
    "0011": "Temperatura del motor normalizada.",
    
    # Batería
    "0012": "Nivel de batería bajo. Conecte el cargador, por favor.",
    "0013": "Advertencia. Batería en nivel crítico. Desconectando tracción.",
    
    # Freno estacionamiento
    "0014": "Freno de estacionamiento activado.",
    "0015": "Freno de estacionamiento desactivado.",
    
    # Luces
    "0016": "Luces encendidas.",
    "0017": "Luces apagadas.",
    
    # Radio/Multimedia
    "0018": "Sistema multimedia activado.",
    "0019": "Sistema multimedia desactivado.",
    
    # Marchas
    "0020": "Marcha D uno activada.",
    "0021": "Marcha D dos activada.",
    "0022": "Marcha atrás activada.",
    "0023": "Punto muerto.",
    "0024": "Vehículo en posición de estacionamiento.",
    
    # Menú oculto
    "0025": "Menú de calibración avanzado activado.",
    "0026": "Iniciando calibración del pedal. Presione lentamente hasta el fondo.",
    "0027": "Calibrando sensores de corriente. Espere unos segundos.",
    "0028": "Calibrando el punto central del volante. Manténgalo recto.",
    
    # Test sistema
    "0029": "Iniciando comprobación completa del sistema.",
    "0030": "Comprobación finalizada. Todos los módulos operativos.",
    
    # Emergencia
    "0031": "Modo de emergencia activado. Motor deshabilitado.",
    "0032": "Reinicio de seguridad completado.",
    
    # Errores sensores
    "0033": "Error en sensor de temperatura.",
    "0034": "Anomalía en lectura de corriente.",
    "0035": "Sin señal de velocidad. Revise sensores de rueda.",
    
    # Estado módulos
    "0036": "Módulo verificado correctamente.",
    
    # Tracción
    "0037": "Tracción 4x4 inteligente activada.",
    "0038": "Tracción 4x2 inteligente activada.",
    
    # === TRACKS AVANZADOS (39-68) ===
    
    # Sistemas seguridad
    "0039": "Sistema antibloqueo de frenos activado.",
    "0040": "Sistema antibloqueo de frenos desactivado.",
    "0041": "Control de tracción activado.",
    "0042": "Control de tracción desactivado.",
    "0043": "Frenado regenerativo activado.",
    "0044": "Frenado regenerativo desactivado.",
    
    # WiFi y conectividad
    "0045": "Conexión WiFi establecida.",
    "0046": "Conexión WiFi perdida.",
    "0047": "Actualización remota iniciada. No desconecte el vehículo.",
    "0048": "Actualización completada. Reiniciando sistema.",
    
    # Bluetooth
    "0049": "Mando Bluetooth conectado.",
    "0050": "Mando Bluetooth desconectado.",
    "0051": "Buscando mando Bluetooth. Mantenga pulsado el botón de emparejamiento.",
    
    # Estados vehículo
    "0052": "Velocidad máxima alcanzada.",
    "0053": "Advertencia. Corriente excesiva detectada.",
    "0054": "Atención. Obstáculo detectado.",
    "0055": "Modo asistencia de estacionamiento activado.",
    "0056": "Iniciando arranque suave de motores.",
    
    # Telemetría
    "0057": "Nivel de batería al 50 por ciento.",
    "0058": "Nivel de batería al 25 por ciento. Considere recargar.",
    "0059": "Ha recorrido un kilómetro en esta sesión.",
    "0060": "Modo ahorro de energía activado.",
    
    # Modos conducción
    "0061": "Modo eco activado. Máxima eficiencia.",
    "0062": "Modo normal activado.",
    "0063": "Modo deportivo activado. Máxima potencia.",
    
    # Config feedback
    "0064": "Configuración guardada correctamente.",
    "0065": "Configuración de fábrica restaurada.",
    "0066": "Registro de errores borrado.",
    "0067": "Nivel de regeneración ajustado.",
    # 0068 es un beep - descargar de internet
}

# Generar todos los MP3
for num, texto in textos.items():
    print(f"Generando {num}.mp3...")
    tts = gTTS(text=texto, lang='es')
    tts.save(f"{num}.mp3")

print("¡Completado! Generados 67 archivos MP3")
print("Nota: 0068.mp3 (beep) debe descargarse por separado")
```

**Para ejecutar el script:**
```bash
pip install gTTS
python generar_audios.py
```

### Método 4: Grabación con Micrófono

Si prefieres grabar tu propia voz:
- Usar micrófono de buena calidad
- Grabar en ambiente silencioso
- Exportar a MP3: mono, 128kbps, 22050Hz
- Normalizar volumen entre archivos

---

## 📊 Resumen de Tracks

| Categoría | Rango | Cantidad |
|-----------|-------|----------|
| Sistema principal | 1-3 | 3 |
| Calibración pedal/sensores | 4-9 | 6 |
| Temperatura/Batería | 10-13 | 4 |
| Freno/Luces/Media | 14-19 | 6 |
| Marchas | 20-24 | 5 |
| Menú oculto | 25-28 | 4 |
| Test sistema | 29-30 | 2 |
| Emergencia | 31-32 | 2 |
| Errores sensores | 33-35 | 3 |
| Módulos/Tracción | 36-38 | 3 |
| **Seguridad (ABS/TCS/Regen)** | 39-44 | 6 |
| **WiFi/OTA** | 45-48 | 4 |
| **Bluetooth** | 49-51 | 3 |
| **Estados vehículo** | 52-56 | 5 |
| **Telemetría** | 57-60 | 4 |
| **Modos conducción** | 61-63 | 3 |
| **Config feedback** | 64-68 | 5 |
| **TOTAL** | **1-68** | **68** |

---

## 🧪 Validación y Pruebas de Audios

### Script de Validación Automática

Se incluye el script `validate_audio_tracks.py` en la raíz del proyecto para facilitar la validación de los 68 tracks de audio.

**Uso del script:**

```bash
# Validar tracks existentes
python3 validate_audio_tracks.py validate

# Generar placeholders para tracks 39-68 (avanzados)
python3 validate_audio_tracks.py generate

# Generar placeholders para todos los tracks (1-68)
python3 validate_audio_tracks.py generate-all
```

**Qué hace el script:**
- ✅ Verifica que todos los 68 archivos MP3 estén presentes
- ⚠️  Identifica archivos placeholder (0 bytes) que necesitan contenido real
- ❌ Lista archivos faltantes
- 📊 Genera un reporte completo de validación

### Pruebas del Sistema de Audio

El firmware incluye un módulo de pruebas automatizadas en `src/test/audio_validation_tests.cpp` que valida:

1. **Definición de Tracks**: Todos los 68 tracks están correctamente definidos en el enum `Audio::Track`
2. **Validación de Rango**: Los tracks fuera de rango (0, >68) son rechazados correctamente
3. **Cola de Audio**: El sistema de cola funciona correctamente con todos los tracks
4. **Gestión de Errores**: Códigos de error apropiados para tracks inválidos, cola llena, etc.

**Para ejecutar las pruebas:**

Las pruebas se ejecutan automáticamente si está habilitado `ENABLE_AUDIO_VALIDATION_TESTS` en `platformio.ini`.

### Códigos de Error de Audio

Los siguientes códigos de error están implementados (ver `docs/CODIGOS_ERROR.md`):

- **700**: Fallo inicialización DFPlayer
- **701**: Error comunicación DFPlayer  
- **702+**: Códigos internos de DFPlayer
- **720**: Sistema de alertas sin inicializar
- **721**: Track de alerta inválido (fuera de rango 1-68)
- **722**: Cola de alertas llena
- **730**: Track de cola inválido
- **731**: Cola de reproducción llena
- **732**: DFPlayer no listo

### Procedimiento de Validación Completa

1. **Generar archivos MP3:**
   ```bash
   # Usar TTSMaker.com o el script Python con gTTS
   python3 validate_audio_tracks.py generate-all
   ```

2. **Reemplazar placeholders con MP3 reales:**
   - Usar TTSMaker.com para generar cada track
   - O usar el script Python incluido en la documentación
   - Copiar archivos generados a la carpeta `audio/`

3. **Validar estructura:**
   ```bash
   python3 validate_audio_tracks.py validate
   ```

4. **Copiar a tarjeta SD:**
   - Formatear tarjeta SD en FAT32
   - Copiar todos los archivos MP3 a la raíz (no en carpetas)
   - Verificar que los nombres sean exactos: `0001.mp3`, `0002.mp3`, ..., `0068.mp3`

5. **Prueba con hardware:**
   - Insertar SD en DFPlayer Mini
   - Compilar y cargar firmware con `ENABLE_AUDIO_VALIDATION_TESTS`
   - Revisar logs serie para resultados de pruebas
   - Probar reproducción de algunos tracks manualmente

---

## ✅ Checklist de Grabación

- [ ] Tracks 1-38 (Básicos)
- [ ] Tracks 39-44 (Seguridad: ABS, TCS, Regen)
- [ ] Tracks 45-48 (WiFi, OTA)
- [ ] Tracks 49-51 (Bluetooth)
- [ ] Tracks 52-56 (Estados vehículo)
- [ ] Tracks 57-60 (Telemetría)
- [ ] Tracks 61-63 (Modos: Eco, Normal, Sport)
- [ ] Tracks 64-68 (Configuración)
- [ ] Copiar todos a tarjeta SD FAT32
- [ ] Probar con DFPlayer Mini

---

*Documento actualizado: 2025-12-19*  
*Constantes implementadas en: `include/alerts.h`*  
*Sistema de validación: `validate_audio_tracks.py`*  
*Pruebas automatizadas: `src/test/audio_validation_tests.cpp`*
