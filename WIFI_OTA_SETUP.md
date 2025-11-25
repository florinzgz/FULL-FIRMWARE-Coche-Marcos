# WiFi y OTA - Guía de Configuración

## 📡 Conectividad WiFi y Actualización OTA

El sistema ahora incluye conectividad WiFi y capacidad de actualización Over-The-Air (OTA) para actualizar el firmware sin cables.

## 🔧 Configuración Inicial

### 1. Configurar Credenciales WiFi (Recomendado: Build Flags)

**Método preferido:** Usa build flags en `platformio.ini`:

```ini
build_flags =
    -DWIFI_SSID=\"TU_RED_WIFI\"
    -DWIFI_PASSWORD=\"TU_CONTRASEÑA\"
    -DOTA_HOSTNAME=\"coche-inteligente\"
    -DOTA_PASSWORD=\"TU_PASSWORD_SEGURO\"
```

**Método alternativo:** Edita `src/core/wifi_manager.cpp` directamente.

> ⚠️ **NOTA IMPORTANTE:** El sufijo `_CONFIG` (ej: `WIFI_SSID_CONFIG`) se usa **solo internamente** en el código C++ para evitar conflictos.  
> Al definir las credenciales en los build flags de `platformio.ini`, **debes usar los nombres originales de los macros SIN el sufijo** (ej: `-DWIFI_SSID=...`, no `-DWIFI_SSID_CONFIG=...`).

### 2. Compilar y Subir Firmware

```bash
pio run -t upload
```

### 3. Verificar Conexión

Abre el monitor serial:
```bash
pio device monitor
```

Deberías ver:
```
WiFi: Conectado! IP: 192.168.1.XXX
WiFi: mDNS iniciado: coche-inteligente.local
WiFi: OTA habilitado
```

## 📲 Actualización OTA (Sin Cable USB)

### Desde PlatformIO

1. **Primera vez:** Sube el firmware con cable USB
2. **Actualizaciones posteriores:** Usa OTA

Edita `platformio.ini` y añade:

```ini
upload_protocol = espota
upload_port = 192.168.1.XXX  ; O coche-inteligente.local
upload_flags = 
    --auth=admin123
```

Luego actualiza:
```bash
pio run -t upload
```

### Desde Arduino IDE

1. Herramientas → Puerto → coche-inteligente.local (ESP32)
2. Subir normalmente
3. Introducir password OTA cuando se solicite

## 🔍 Características WiFi

### Funcionalidades Actuales

✅ **Conexión WiFi automática** al iniciar
✅ **Reconexión automática** cada 30 segundos si se pierde conexión  
✅ **mDNS** - Acceder como `coche-inteligente.local`
✅ **OTA Updates** - Actualización firmware sin cables
✅ **Alertas de audio** al conectar/desconectar
✅ **Log detallado** de estado WiFi

### Auto-Recuperación

- Si pierde conexión WiFi, reintenta cada 30 segundos
- No afecta funcionamiento principal del coche
- Audio alert al reconectar

## 📊 Monitoreo del Estado WiFi

Accede a información WiFi desde cualquier parte del código:

```cpp
#include "wifi_manager.h"

// Verificar conexión
if (WiFiManager::isConnected()) {
    // WiFi disponible
}

// Obtener IP
String ip = WiFiManager::getIPAddress();  // "192.168.1.100"

// Obtener señal
int rssi = WiFiManager::getRSSI();  // -45 dBm (mejor que -70)
```

## 🛡️ Seguridad

⚠️ **IMPORTANTE:**

1. **Cambia el password OTA** en `wifi_manager.cpp`
2. **No compartas** el password OTA en repositorios públicos
3. Usa una red WiFi segura (WPA2/WPA3)
4. Considera crear un archivo `secrets.h` para credenciales (añádelo a `.gitignore`)

### Ejemplo secrets.h (recomendado)

Crea `include/secrets.h`:
```cpp
#pragma once
#define WIFI_SSID_SECRET "MiRedWiFi"
#define WIFI_PASS_SECRET "MiContraseña"
#define OTA_PASS_SECRET "MiPasswordOTA"
```

Modifica `wifi_manager.cpp`:
```cpp
#include "secrets.h"
const char* WIFI_SSID = WIFI_SSID_SECRET;
const char* WIFI_PASSWORD = WIFI_PASS_SECRET;
const char* OTA_PASSWORD = OTA_PASS_SECRET;
```

Añade a `.gitignore`:
```
include/secrets.h
```

## 🚀 Próximos Pasos

Con WiFi funcionando, puedes implementar:

1. **Telemetría en tiempo real** → Enviar datos a servidor/app
2. **Web Dashboard** → Interfaz web en el ESP32
3. **MQTT** → Comunicación bidireccional
4. **Data Logging** → Guardar datos en SD o nube
5. **Remote Control** → Control remoto desde app móvil

## 🔧 Solución de Problemas

### No se conecta a WiFi

1. Verifica SSID y password en `wifi_manager.cpp`
2. Verifica que la red es 2.4GHz (ESP32 no soporta 5GHz)
3. Verifica señal WiFi donde está el coche
4. Revisa el monitor serial para mensajes de error

### OTA no funciona

1. Verifica que WiFi está conectado
2. Verifica que el puerto OTA es correcto (IP o .local)
3. Verifica que el password OTA coincide
4. Verifica que no hay firewall bloqueando
5. Intenta con IP directa en vez de .local

### Actualización OTA falla a mitad

- El ESP32 tiene suficiente memoria flash
- La conexión WiFi es estable
- Reintenta la actualización
- Como último recurso, usa cable USB

## 📝 Logs Útiles

Monitor serial muestra:
```
WiFi: Iniciando conexión a MiRedWiFi
........
WiFi: Conectado! IP: 192.168.1.100
WiFi: RSSI: -45 dBm
WiFi: mDNS iniciado: coche-inteligente.local
WiFi: OTA habilitado
```

Durante actualización OTA:
```
OTA: Iniciando actualización sketch
OTA: Progreso: 10%
OTA: Progreso: 20%
...
OTA: Progreso: 100%
OTA: Actualización completada
```

## 💡 Tips

- **Señal WiFi:** -30 a -50 dBm = Excelente, -50 a -70 = Buena, < -70 = Mejorar antena
- **Desarrollo:** Usa OTA para actualizar código rápidamente sin desmontar
- **Producción:** Considera deshabilitar OTA o usar password más seguro
- **Batería:** WiFi consume ~80mA, considera deshabilitarlo cuando no se use
