# 🌡️ GUÍA DE SENSORES DE TEMPERATURA DS18B20

## Coche Inteligente Marcos - ESP32-S3

**Versión Firmware:** 2.8.0  
**Fecha:** 2025-11-27

---

## 📋 ÍNDICE

1. [Resumen del Sistema](#1-resumen-del-sistema)
2. [Especificaciones Técnicas](#2-especificaciones-técnicas)
3. [Voltaje de Operación](#3-voltaje-de-operación)
4. [Conexiones Detalladas](#4-conexiones-detalladas)
5. [Componentes Necesarios](#5-componentes-necesarios)
6. [Diagrama de Cableado](#6-diagrama-de-cableado)
7. [Configuración en el Firmware](#7-configuración-en-el-firmware)
8. [Resolución de Problemas](#8-resolución-de-problemas)

---

## 1. RESUMEN DEL SISTEMA

El sistema utiliza **4 sensores de temperatura DS18B20** para monitorizar la temperatura de los motores de tracción:

| Sensor | Ubicación | Índice en Código |
|--------|-----------|------------------|
| DS18B20 #1 | Motor FL (Frontal Izquierdo) | 0 |
| DS18B20 #2 | Motor FR (Frontal Derecho) | 1 |
| DS18B20 #3 | Motor RL (Trasero Izquierdo) | 2 |
| DS18B20 #4 | Motor RR (Trasero Derecho) | 3 |
| (Reservado) | Ambiente | 4 |

Todos los sensores se conectan en **bus paralelo** usando el protocolo **OneWire**.

---

## 2. ESPECIFICACIONES TÉCNICAS

### Sensor DS18B20

| Parámetro | Valor |
|-----------|-------|
| **Modelo** | DS18B20 |
| **Protocolo** | OneWire (1-Wire) |
| **Rango de Temperatura** | -55°C a +125°C |
| **Precisión** | ±0.5°C (de -10°C a +85°C) |
| **Resolución** | 12-bit (0.0625°C) |
| **Tiempo de Conversión** | 750ms (12-bit) |
| **Encapsulado Recomendado** | TO-92 o sonda impermeable |

---

## 3. VOLTAJE DE OPERACIÓN

### ⚡ VOLTAJE RECOMENDADO: 3.3V

Los sensores DS18B20 operan en el siguiente rango:

| Parámetro | Mínimo | Típico | Máximo |
|-----------|--------|--------|--------|
| **Tensión de alimentación (VCC)** | 3.0V | **3.3V** | 5.5V |
| **Corriente en reposo** | - | 1µA | 1.5µA |
| **Corriente en conversión** | - | 1mA | 1.5mA |

### ✅ Configuración en Este Proyecto

En este proyecto usamos:

- **VCC:** 3.3V (desde el pin 3.3V del ESP32-S3)
- **GND:** Tierra común
- **Data:** GPIO 20

**Ventajas de usar 3.3V:**
- ✅ Conexión directa al ESP32-S3 (lógica 3.3V)
- ✅ No requiere convertidor de nivel
- ✅ Menor consumo de energía
- ✅ Señal compatible sin riesgo de daño

> ⚠️ **IMPORTANTE:** Aunque el DS18B20 puede operar a 5V, **se recomienda 3.3V** para conexión directa con el ESP32-S3, ya que los GPIOs del ESP32 no son tolerantes a 5V.

---

## 4. CONEXIONES DETALLADAS

### 4.1 Pin del ESP32-S3

| Pin ESP32 | GPIO | Función |
|-----------|------|---------|
| **ONEWIRE** | GPIO 20 | Bus OneWire para DS18B20 |

Definido en `include/pins.h`:
```cpp
#define PIN_ONEWIRE       20  // GPIO 20 - Bus OneWire (4 sensores en paralelo)
```

### 4.2 Pines del Sensor DS18B20

El DS18B20 en encapsulado TO-92 tiene 3 pines:

```
     ┌───────┐
     │ DS18B20│
     │  ┌─┐   │
     └──┴─┴───┘
        │││
        │││
       GND DQ VCC
        │  │  │
        │  │  └── Pin 3: VCC (Alimentación 3.3V)
        │  └───── Pin 2: DQ (Data - OneWire)
        └──────── Pin 1: GND (Tierra)
```

### 4.3 Tabla de Conexiones

| Pin Sensor | Color Cable | Conexión |
|------------|-------------|----------|
| **GND** (Pin 1) | Negro | GND del ESP32-S3 |
| **DQ** (Pin 2) | Amarillo | GPIO 20 del ESP32-S3 |
| **VCC** (Pin 3) | Rojo | 3.3V del ESP32-S3 |

---

## 5. COMPONENTES NECESARIOS

### Sensores
| Cantidad | Componente | Notas |
|----------|------------|-------|
| 4 | DS18B20 (sonda impermeable) | Preferir modelo con cable para fácil montaje en motores |

### Resistencia Pull-up (OBLIGATORIA)
| Cantidad | Valor | Potencia | Notas |
|----------|-------|----------|-------|
| 1 | **4.7kΩ** | 1/4W | Conectar entre VCC (3.3V) y Data (GPIO 20) |

> ⚠️ **CRÍTICO:** Sin la resistencia pull-up de 4.7kΩ, los sensores NO funcionarán.

### Cables
| Cantidad | Tipo | Calibre |
|----------|------|---------|
| 1 | Cable rojo | 22 AWG |
| 1 | Cable negro | 22 AWG |
| 1 | Cable amarillo | 22 AWG |

---

## 6. DIAGRAMA DE CABLEADO

### 6.1 Diagrama Esquemático

```
                            ┌─────────────────────────────────────────┐
                            │              ESP32-S3                   │
                            │                                         │
  ┌──────────────────┐      │                                         │
  │   DS18B20 #1     │      │                                         │
  │   Motor FL       │      │                                         │
  │  ┌─────────────┐ │      │                                         │
  │  │ GND  DQ VCC │ │      │                                         │
  │  └──┬───┬───┬──┘ │      │                                         │
  │     │   │   │    │      │                                         │
  └─────┼───┼───┼────┘      │                                         │
        │   │   │           │                                         │
  ┌──────────────────┐      │                                         │
  │   DS18B20 #2     │      │                                         │
  │   Motor FR       │      │                                         │
  │  ┌─────────────┐ │      │                                         │
  │  │ GND  DQ VCC │ │      │                                         │
  │  └──┬───┬───┬──┘ │      │                                         │
  │     │   │   │    │      │                                         │
  └─────┼───┼───┼────┘      │                                         │
        │   │   │           │                                         │
  ┌──────────────────┐      │                                         │
  │   DS18B20 #3     │      │    3.3V ●────────┬───────────┐          │
  │   Motor RL       │      │                  │           │          │
  │  ┌─────────────┐ │      │               [4.7kΩ]        │          │
  │  │ GND  DQ VCC │ │      │                  │           │          │
  │  └──┬───┬───┬──┘ │      │   GPIO 20 ●─────┴───────────┼──────┐   │
  │     │   │   │    │      │                             │      │   │
  └─────┼───┼───┼────┘      │                             │      │   │
        │   │   │           │                             │      │   │
  ┌──────────────────┐      │                             │      │   │
  │   DS18B20 #4     │      │                             │      │   │
  │   Motor RR       │      │                             │      │   │
  │  ┌─────────────┐ │      │                             │      │   │
  │  │ GND  DQ VCC │ │      │                             │      │   │
  │  └──┬───┬───┬──┘ │      │                             │      │   │
  │     │   │   │    │      │     GND ●───────────────────┼──┐   │   │
  └─────┼───┼───┼────┘      │                             │  │   │   │
        │   │   │           └─────────────────────────────┼──┼───┼───┘
        │   │   │                                         │  │   │
        │   │   └─────────────────────────────────────────┘  │   │
        │   │         (Todos los VCC conectados juntos)      │   │
        │   │                                                │   │
        │   └────────────────────────────────────────────────┼───┘
        │                (Todos los DQ conectados juntos)    │
        │                                                    │
        └────────────────────────────────────────────────────┘
                      (Todos los GND conectados juntos)
```

### 6.2 Diagrama Simplificado

```
    ESP32-S3                              4x DS18B20
┌──────────────────┐                  ┌──────────────────┐
│                  │                  │    Sensor 1      │
│  3.3V ●──────────┼───┬──[4.7kΩ]──┬──┤ VCC              │
│                  │   │           │  │ DQ               │
│ GPIO 20 ●────────┼───┴───────────┼──┤ GND              │
│                  │               │  └──────────────────┘
│                  │               │  ┌──────────────────┐
│                  │               │  │    Sensor 2      │
│                  │               ├──┤ VCC              │
│                  │               │  │ DQ               │
│                  │               │  │ GND              │
│                  │               │  └──────────────────┘
│                  │               │  ┌──────────────────┐
│                  │               │  │    Sensor 3      │
│                  │               ├──┤ VCC              │
│                  │               │  │ DQ               │
│                  │               │  │ GND              │
│                  │               │  └──────────────────┘
│                  │               │  ┌──────────────────┐
│                  │               │  │    Sensor 4      │
│  GND ●───────────┼───────────────┴──┤ VCC              │
│                  │                  │ DQ               │
└──────────────────┘                  │ GND              │
                                      └──────────────────┘

Notas:
- VCC de todos los sensores → 3.3V (ESP32)
- GND de todos los sensores → GND (ESP32)
- DQ de todos los sensores → GPIO 20 (ESP32)
- Resistencia 4.7kΩ entre 3.3V y GPIO 20 (OBLIGATORIA)
```

### 6.3 Ubicación Física de los Sensores

```
          FRENTE DEL VEHÍCULO
    ┌─────────────────────────────┐
    │                             │
    │   🌡️ DS18B20 #1          🌡️ DS18B20 #2   │
    │     Motor FL              Motor FR     │
    │                             │
    │                             │
    │                             │
    │   🌡️ DS18B20 #3          🌡️ DS18B20 #4   │
    │     Motor RL              Motor RR     │
    │                             │
    └─────────────────────────────┘
          PARTE TRASERA

FL = Front Left (Frontal Izquierdo)
FR = Front Right (Frontal Derecho)
RL = Rear Left (Trasero Izquierdo)
RR = Rear Right (Trasero Derecho)
```

---

## 7. CONFIGURACIÓN EN EL FIRMWARE

### 7.1 Constantes de Configuración

Definidas en `include/temperature.h`:

```cpp
namespace Sensors {
    constexpr int NUM_TEMPS = 5;                    // 4 motores + 1 ambiente
    constexpr float EMA_FILTER_ALPHA = 0.2f;        // Factor de suavizado
    constexpr float TEMP_MIN_CELSIUS = -40.0f;      // Rango mínimo válido
    constexpr float TEMP_MAX_CELSIUS = 150.0f;      // Rango máximo válido
    constexpr uint32_t UPDATE_INTERVAL_MS = 1000;   // Frecuencia de actualización (1 Hz)
    constexpr float TEMP_CRITICAL_CELSIUS = 85.0f;  // Temperatura crítica para motores
}
```

### 7.2 Funciones Disponibles

```cpp
// Inicialización (llamada en setup)
Sensors::initTemperature();

// Actualización (llamada en loop)
Sensors::updateTemperature();

// Obtener temperatura de un motor
float temp = Sensors::getTemperature(0);  // Motor FL
float temp = Sensors::getTemperature(1);  // Motor FR
float temp = Sensors::getTemperature(2);  // Motor RL
float temp = Sensors::getTemperature(3);  // Motor RR
float temp = Sensors::getTemperature(4);  // Ambiente (si está conectado)

// Verificar si un sensor funciona correctamente
bool ok = Sensors::isTemperatureSensorOk(0);

// Verificar inicialización global
bool initialized = Sensors::temperatureInitOK();

// Obtener estado completo
Sensors::TemperatureStatus status = Sensors::getTemperatureStatus();
```

### 7.3 Librerías Requeridas

En `platformio.ini`:
```ini
lib_deps =
    milesburton/DallasTemperature@^4.0.5
    paulstoffregen/OneWire@^2.3.8
```

---

## 8. RESOLUCIÓN DE PROBLEMAS

### 8.1 Los sensores no se detectan

| Problema | Causa | Solución |
|----------|-------|----------|
| 0 sensores detectados | Falta resistencia pull-up | Instalar resistencia **4.7kΩ** entre 3.3V y GPIO 20 |
| 0 sensores detectados | Cables sueltos | Verificar conexiones VCC, GND y DQ |
| 0 sensores detectados | GPIO incorrecto | Confirmar que se usa GPIO 20 |
| Menos de 4 sensores | Sensor defectuoso | Probar cada sensor individualmente |

### 8.2 Lecturas incorrectas

| Problema | Causa | Solución |
|----------|-------|----------|
| -127°C | Sensor desconectado | Verificar conexión del sensor |
| Valores erráticos | Ruido eléctrico | Usar cables apantallados cerca de motores |
| Lecturas lentas | Resolución máxima | Normal: 12-bit = 750ms de conversión |

### 8.3 Mensajes de Log

| Mensaje | Significado | Acción |
|---------|-------------|--------|
| `DS18B20: detectados X, esperados 5` | Menos sensores conectados | Verificar conexiones |
| `DS18B20 init FAIL idx X` | Fallo al inicializar sensor X | Revisar sensor específico |
| `DS18B20: timeout en conversión` | Timeout de lectura | Verificar alimentación y conexiones |

### 8.4 Verificación Rápida

Para verificar el funcionamiento de los sensores:

1. **Abrir monitor serial:** `pio device monitor`
2. **Buscar mensajes de inicialización:**
   ```
   DS18B20 0: ROM=0xXXXXXXXXXXXXXXXX
   DS18B20 1: ROM=0xXXXXXXXXXXXXXXXX
   DS18B20 2: ROM=0xXXXXXXXXXXXXXXXX
   DS18B20 3: ROM=0xXXXXXXXXXXXXXXXX
   Temperature sensors init: 4/5 OK
   ```
3. **Si hay errores, verificar:**
   - Resistencia pull-up instalada
   - Conexiones correctas
   - Alimentación 3.3V presente

---

## 📚 REFERENCIAS

- [Datasheet DS18B20 (Maxim)](https://datasheets.maximintegrated.com/en/ds/DS18B20.pdf)
- [Librería DallasTemperature](https://github.com/milesburton/Arduino-Temperature-Control-Library)
- [Librería OneWire](https://github.com/PaulStoffregen/OneWire)
- [ESP32-S3 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf)

---

## ✅ CHECKLIST DE INSTALACIÓN

- [ ] 4x sensores DS18B20 (sondas impermeables recomendadas)
- [ ] 1x resistencia 4.7kΩ instalada entre 3.3V y GPIO 20
- [ ] Cables VCC (rojo) conectados a 3.3V del ESP32
- [ ] Cables GND (negro) conectados a GND del ESP32
- [ ] Cables DQ (amarillo) conectados a GPIO 20 del ESP32
- [ ] Sensores montados cerca de cada motor de tracción
- [ ] Firmware compilado con librerías OneWire y DallasTemperature
- [ ] Verificación en monitor serial: "Temperature sensors init: 4/5 OK"

---

**Versión:** 1.0  
**Fecha:** 2025-11-27  
**Firmware Compatible:** v2.8.0+
