# 📋 CÓDIGOS DE ERROR DEL FIRMWARE

## Versión del Firmware: 2.9.5
## Fecha: 2025-12-05

---

## 🔍 CÓMO VER LOS ERRORES

Para acceder al menú de errores:
1. Accede al **Menú Oculto** (tocar icono batería 4 veces: código 8-9-8-9)
2. Selecciona opción **"8) Ver errores"**
3. Los errores se muestran con su código y descripción

**Método alternativo (sin touch):**
- Mantén presionado el **botón 4X4** durante **5 segundos**

---

## 📚 LISTADO COMPLETO DE CÓDIGOS DE ERROR

### 🎮 ENTRADAS Y CONTROLES (100-199)

#### **100** - Error de Pedal
- **Descripción:** Fallo en la lectura del sensor Hall del pedal
- **Causa:** Sensor desconectado, fuera de rango o con voltaje incorrecto
- **Solución:** 
  - Verificar conexión del sensor A1324LUA-T en GPIO35
  - Verificar divisor de voltaje (5V → 3.3V)
  - Calibrar pedal desde menú oculto

---

### 🎯 SISTEMA DE DIRECCIÓN (200-299)

#### **200** - Error de Inicialización del Encoder
- **Descripción:** El encoder de dirección no responde durante el autotest
- **Causa:** Pines no asignados o encoder no conectado
- **Solución:**
  - Verificar conexiones del encoder E6B2-CWZ6C
  - Verificar pines GPIO37 (A), GPIO38 (B), GPIO39 (Z)
  - Verificar módulo optoacoplador HY-M158

#### **201** - Pines del Encoder No Asignados
- **Descripción:** Los pines del encoder no están configurados correctamente
- **Causa:** Error de configuración en pins.h
- **Solución:** Verificar definiciones PIN_ENCODER_A, PIN_ENCODER_B, PIN_ENCODER_Z

#### **210** - Encoder Sin Centrado
- **Descripción:** El encoder no detecta la señal Z de centrado
- **Causa:** Señal Z no conectada o no funcional
- **Solución:**
  - Verificar cable de señal Z (GPIO39)
  - Verificar que el encoder tenga señal índice (Z)
  - Realizar calibración manual desde menú oculto

#### **211** - Fallo de Centrado por Señal Z
- **Descripción:** El centrado automático del encoder falló
- **Causa:** Señal Z intermitente o ruidosa
- **Solución:** Revisar conexiones y blindaje de cables

#### **212** - Ticks por Vuelta Inválido
- **Descripción:** El cálculo de ticks por vuelta es incorrecto
- **Causa:** Configuración errónea de ENCODER_PPR en constants.h
- **Solución:** Verificar que ENCODER_PPR = 1200 para E6B2-CWZ6C

#### **213** - Timeout Señal Z
- **Descripción:** Tiempo de espera agotado esperando señal Z
- **Causa:** Encoder no girando o señal Z no funcional
- **Solución:** Girar volante completamente y verificar señal Z

#### **250** - PCA9685 Dirección No Responde
- **Descripción:** El módulo PCA9685 del motor de dirección no responde en I2C
- **Causa:** Módulo desconectado, dirección I2C incorrecta o fallo de bus
- **Solución:**
  - Verificar conexión I2C del PCA9685 @ 0x42
  - Verificar cables SDA (GPIO8) y SCL (GPIO9)
  - Verificar pull-ups de I2C (4.7kΩ recomendado)

#### **251** - Sobrecorriente Motor de Dirección
- **Descripción:** Corriente del motor de dirección excede límite seguro
- **Causa:** Motor bloqueado, cortocircuito o carga mecánica excesiva
- **Solución:**
  - Verificar que el motor RS390 gire libremente
  - Revisar driver BTS7960 de dirección
  - Verificar sensor INA226 de dirección

#### **252** - Canal PWM Inválido
- **Descripción:** Se intentó usar un canal PWM fuera de rango (0-15)
- **Causa:** Error de programación o configuración
- **Solución:** Reportar como bug, verificar pwm_channels.h

---

### ⚡ SENSORES DE CORRIENTE (300-399)

#### **300-303** - Fallo Persistente Sensor INA226 (FL/FR/RL/RR)
- **Descripción:** Sensor de corriente de motor no responde
- **Códigos:**
  - **300**: Motor delantero izquierdo (FL)
  - **301**: Motor delantero derecho (FR)
  - **302**: Motor trasero izquierdo (RL)
  - **303**: Motor trasero derecho (RR)
- **Causa:** Sensor desconectado o fallo del multiplexor TCA9548A
- **Solución:**
  - Verificar TCA9548A @ 0x70 (I2C)
  - Verificar sensor INA226 @ 0x40 en canal correspondiente
  - Verificar resistencias shunt CG FL-2C 50A/75mV

#### **310-313** - Error Configuración INA226 (FL/FR/RL/RR)
- **Descripción:** No se pudo configurar el sensor de corriente
- **Causa:** Sensor defectuoso o comunicación I2C inestable
- **Solución:** Reiniciar ESP32, verificar voltaje de alimentación 3.3V

#### **320-323** - Error Lectura Voltaje INA226 (FL/FR/RL/RR)
- **Descripción:** Fallo en lectura de voltaje del sensor
- **Causa:** Sensor en mal estado o fallo temporal de I2C
- **Solución:** Verificar conexiones de bus shunt

#### **330-333** - Error Lectura Corriente INA226 (FL/FR/RL/RR)
- **Descripción:** Fallo en lectura de corriente del sensor
- **Causa:** Shunt desconectado o sensor defectuoso
- **Solución:** Verificar resistencia shunt y conexiones

#### **340-343** - Error Lectura Potencia INA226 (FL/FR/RL/RR)
- **Descripción:** Fallo en lectura de potencia del sensor
- **Causa:** Problema con calibración del sensor
- **Solución:** Recalibrar sensor o reemplazar

#### **399** - Error General Inicialización Sensores de Corriente
- **Descripción:** Fallo durante inicialización del sistema de monitoreo de corriente
- **Causa:** Múltiples sensores no responden o TCA9548A no funcional
- **Solución:**
  - Verificar alimentación del TCA9548A
  - Verificar bus I2C principal (GPIO8/GPIO9)
  - Verificar pull-ups I2C

---

### 🌡️ SENSORES DE TEMPERATURA (400-499)

#### **400-403** - Sensor DS18B20 No Encontrado (FL/FR/RL/RR)
- **Descripción:** Sensor de temperatura de motor no detectado
- **Códigos:**
  - **400**: Motor delantero izquierdo (FL)
  - **401**: Motor delantero derecho (FR)
  - **402**: Motor trasero izquierdo (RL)
  - **403**: Motor trasero derecho (RR)
- **Causa:** Sensor desconectado, cortocircuito o dirección ROM no vinculada
- **Solución:**
  - Verificar conexiones en bus OneWire (GPIO20)
  - Verificar resistencia pull-up 4.7kΩ (OBLIGATORIA)
  - Verificar alimentación 3.3V de sensores
  - Re-escanear ROMs desde menú de diagnóstico

#### **450** - Timeout Conversión de Temperatura
- **Descripción:** La conversión asíncrona de temperatura tardó más de 1 segundo
- **Causa:** Bus OneWire ruidoso, múltiples sensores fallando o cortocircuito
- **Solución:**
  - Verificar calidad de cables (máx 3 metros recomendado)
  - Verificar que pull-up sea de 4.7kΩ (NO mayor ni menor)
  - Reducir número de sensores en bus si es muy largo

---

### 🎡 SENSORES DE RUEDA (500-599)

#### **500-503** - Sensor de Rueda Sin Pulsos (FL/FR/RL/RR)
- **Descripción:** Sensor inductivo de rueda no genera pulsos
- **Códigos:**
  - **500**: Rueda delantera izquierda (FL)
  - **501**: Rueda delantera derecha (FR)
  - **502**: Rueda trasera izquierda (RL)
  - **503**: Rueda trasera derecha (RR)
- **Causa:** 
  - Sensor LJ12A3-4-Z/BX desconectado
  - Módulo HY-M158 sin alimentación
  - Rueda no girando o target metálico ausente
- **Solución:**
  - Verificar alimentación 12V del sensor inductivo
  - Verificar módulo optoacoplador HY-M158 (12V → 3.3V)
  - Verificar pines GPIO: FL=GPIO3, FR=GPIO36, RL=GPIO17, RR=GPIO15
  - Verificar distancia sensor-target (2-4mm óptimo)

---

### 🔌 SISTEMA DE RELÉS Y POTENCIA (600-699)

#### **600** - Fallo General del Sistema de Relés
- **Descripción:** Error crítico en el sistema de relés de potencia
- **Causa:** Múltiples relés no responden o secuencia de encendido falló
- **Solución:**
  - Verificar alimentación 5V de bobinas de relés
  - Verificar pines GPIO 4, 5, 6, 7
  - Verificar módulos SRD-05VDC

#### **601** - Error Secuencia de Apagado
- **Descripción:** Fallo al ejecutar secuencia segura de apagado de relés
- **Causa:** Estado inconsistente de relés
- **Solución:** Reiniciar sistema, verificar hardware de relés

#### **602** - Error Secuencia de Encendido
- **Descripción:** Fallo al ejecutar secuencia segura de encendido de relés
- **Causa:** Relés no cambian de estado correctamente
- **Solución:**
  - Verificar drivers de relé
  - Medir voltaje de bobinas (debe ser ~5V activo)

#### **603-606** - Fallo Relé Individual (MAIN/TRAC/DIR/SPARE)
- **Descripción:** Un relé específico no responde
- **Códigos:**
  - **603**: RELAY_MAIN (GPIO4) - Retención de potencia
  - **604**: RELAY_TRAC (GPIO5) - Tracción 24V
  - **605**: RELAY_DIR (GPIO6) - Dirección 12V
  - **606**: RELAY_SPARE (GPIO7) - Reserva
- **Solución:** Verificar relé individual y su driver

#### **607** - Timeout Estado de Relé
- **Descripción:** Relé no alcanzó estado deseado en tiempo límite
- **Causa:** Relé pegado o driver defectuoso
- **Solución:** Reemplazar relé o driver

#### **608** - Estado Inconsistente de Relé
- **Descripción:** Estado leído no coincide con estado esperado
- **Causa:** Feedback incorrecto o relé intermitente
- **Solución:** Verificar conexiones de feedback

#### **650** - Fallo Detección de Errores de Relé
- **Descripción:** Sistema de detección de errores falló
- **Causa:** Error de software o hardware
- **Solución:** Reportar como bug

#### **699** - Error No Especificado del Sistema de Relés
- **Descripción:** Error desconocido en sistema de relés
- **Causa:** Condición no catalogada
- **Solución:** Revisar logs serie para más detalles

---

### 🔊 SISTEMA DE AUDIO (700-799)

#### **700** - Fallo Inicialización DFPlayer
- **Descripción:** El módulo DFPlayer Mini no responde
- **Causa:** 
  - Módulo desconectado
  - Tarjeta SD ausente o corrupta
  - Comunicación UART fallida
- **Solución:**
  - Verificar conexiones TX (GPIO43) y RX (GPIO44)
  - Verificar tarjeta MicroSD con archivos MP3
  - Verificar baudrate 9600 en DFPlayer

#### **701** - Error Comunicación DFPlayer
- **Descripción:** Pérdida de comunicación con DFPlayer durante operación
- **Causa:** Cable UART desconectado o ruido eléctrico
- **Solución:**
  - Verificar cables UART
  - Añadir capacitores de desacople en VCC (0.1μF + 10μF)

#### **702+** - Código de Error DFPlayer
- **Descripción:** Error reportado por el módulo DFPlayer
- **Códigos:** 702 + (código interno DFPlayer)
- **Solución:** Consultar datasheet DFPlayer Mini para código específico

#### **720** - Sistema de Alertas Sin Inicializar
- **Descripción:** Se intentó reproducir alerta antes de inicializar sistema
- **Causa:** Error de secuencia de arranque
- **Solución:** Reportar como bug de firmware

#### **721** - Track de Alerta Inválido
- **Descripción:** Se intentó reproducir un track inexistente
- **Causa:** ID de track fuera de rango o archivo MP3 faltante
- **Solución:**
  - Verificar archivos en tarjeta SD
  - Verificar constantes en audio_tracks.h

#### **722** - Cola de Alertas Llena
- **Descripción:** No hay espacio en cola de reproducción
- **Causa:** Demasiadas alertas encoladas simultáneamente
- **Solución:** Esperar a que se procesen alertas anteriores

#### **730** - Track de Cola Inválido
- **Descripción:** Track en cola de reproducción es inválido
- **Causa:** ID de track corrupto o fuera de rango
- **Solución:** Verificar sistema de encolado

#### **731** - Cola de Reproducción Llena
- **Descripción:** Cola de tracks generales está llena
- **Causa:** Exceso de comandos de reproducción
- **Solución:** Esperar procesamiento de cola

#### **732** - DFPlayer No Listo
- **Descripción:** DFPlayer no está listo para recibir comandos
- **Causa:** Módulo ocupado o no inicializado
- **Solución:** Esperar o reinicializar DFPlayer

#### **740** - Error Sistema de Botones
- **Descripción:** Fallo en lectura de botones físicos
- **Causa:** Pines no configurados o hardware desconectado
- **Solución:**
  - Verificar BTN_LIGHTS (GPIO2)
  - Verificar BTN_MEDIA (GPIO40)
  - Verificar BTN_4X4 (GPIO41)
  - Verificar módulos HY-M158

---

### 🚗 SISTEMA DE TRACCIÓN (800-899)

#### **800** - Reparto de Tracción Anómalo
- **Descripción:** Distribución de potencia entre motores fuera de límites
- **Causa:** Cálculo Ackermann incorrecto o demanda inconsistente
- **Solución:**
  - Calibrar encoder de dirección
  - Verificar sensores de corriente
  - Revisar lógica de Ackermann

#### **801** - Demanda de Tracción Inválida
- **Descripción:** Valor de demanda (throttle) fuera de rango válido
- **Causa:** Sensor de pedal descalibrado o lecturas NaN
- **Solución:**
  - Calibrar pedal desde menú oculto
  - Verificar sensor Hall A1324LUA-T

#### **802** - Asimetría Extrema de Tracción
- **Descripción:** Diferencia excesiva entre tracción izquierda y derecha
- **Causa:** Motor bloqueado, sensor fallando o superficie irregular
- **Solución:**
  - Verificar que todos los motores giren libremente
  - Verificar sensores de corriente
  - Revisar superficie de conducción

#### **810-813** - Sobrecorriente Motor de Tracción (FL/FR/RL/RR)
- **Descripción:** Corriente de motor excede límite seguro (>50A)
- **Códigos:**
  - **810**: Motor delantero izquierdo (FL)
  - **811**: Motor delantero derecho (FR)
  - **812**: Motor trasero izquierdo (RL)
  - **813**: Motor trasero derecho (RR)
- **Causa:** Motor bloqueado, cortocircuito o fallo de driver BTS7960
- **Solución:**
  - Detener vehículo inmediatamente
  - Verificar motor y driver correspondiente
  - Revisar cableado de potencia

#### **820-823** - PWM Fuera de Límites (FL/FR/RL/RR)
- **Descripción:** Valor PWM calculado fuera de rango válido (0-4095)
- **Códigos:** 820-823 para motores FL, FR, RL, RR
- **Causa:** Error de cálculo o valor NaN/Inf
- **Solución:** Reportar como bug, reiniciar sistema

#### **830** - Fallo PCA9685 Eje Delantero (0x40)
- **Descripción:** Driver PWM del eje delantero no responde en I²C
- **Causa:** Dispositivo desconectado, dirección I²C incorrecta o bus I²C con fallos
- **Solución:**
  - Verificar conexión del PCA9685 @ 0x40
  - Verificar bus I²C (SDA=GPIO8, SCL=GPIO9)
  - Verificar alimentación 5V del PCA9685
  - Realizar test I²C scanner

#### **831** - Fallo PCA9685 Eje Trasero (0x41)
- **Descripción:** Driver PWM del eje trasero no responde en I²C
- **Causa:** Dispositivo desconectado, dirección I²C incorrecta o bus I²C con fallos
- **Solución:**
  - Verificar conexión del PCA9685 @ 0x41
  - Verificar bus I²C (SDA=GPIO8, SCL=GPIO9)
  - Verificar alimentación 5V del PCA9685
  - Realizar test I²C scanner

#### **832** - Fallo MCP23017 Control Motores (0x20)
- **Descripción:** Expansor GPIO para control IN1/IN2 no responde en I²C
- **Causa:** Dispositivo desconectado, dirección I²C incorrecta o bus I²C con fallos
- **Solución:**
  - Verificar conexión del MCP23017 @ 0x20
  - Verificar bus I²C (SDA=GPIO8, SCL=GPIO9)
  - Verificar alimentación 5V del MCP23017
  - Realizar test I²C scanner
- **IMPORTANTE:** Este chip también controla el shifter (GPIOB0-B4)
  - Si falla, TANTO el control de tracción COMO el shifter dejarán de funcionar
  - Los motores de tracción no recibirán señales de dirección (IN1/IN2)
  - El cambio de marchas (P/R/N/D1/D2) no será detectado
  - Ambos sistemas quedarán inoperativos hasta que se restaure el MCP23017

---

### 💾 SISTEMA DE ALMACENAMIENTO (900-999)

#### **970** - Fallo Apertura de Storage
- **Descripción:** No se pudo abrir el sistema de almacenamiento persistente
- **Causa:** EEPROM corrupta o no disponible
- **Solución:**
  - Restaurar configuración de fábrica (opción 7 del menú)
  - Puede requerir reflasheo completo

#### **975** - Restauración Automática de Configuración
- **Descripción:** Configuración corrupta fue restaurada automáticamente
- **Causa:** Magic number incorrecto o CRC fallido
- **Solución:** 
  - Re-calibrar todos los sensores
  - Verificar configuración en menú oculto

#### **980** - Fallo Escritura Magic Number
- **Descripción:** No se pudo escribir número mágico en storage
- **Causa:** EEPROM con fallo de escritura
- **Solución:** Hardware defectuoso, puede requerir reemplazo de ESP32

#### **981** - Fallo Escritura de Configuración
- **Descripción:** No se pudo guardar configuración en storage
- **Causa:** EEPROM llena o defectuosa
- **Solución:**
  - Borrar errores viejos (opción 9 del menú)
  - Restaurar de fábrica si persiste

#### **985** - Reset a Configuración de Fábrica
- **Descripción:** Configuración restaurada a valores predeterminados
- **Causa:** Usuario solicitó restauración o corrupción detectada
- **Solución:** 
  - Este es informativo, no un error
  - Recalibrar sensores después de reset

---

## 🔧 PROCEDIMIENTOS DE DIAGNÓSTICO

### Verificación Rápida del Sistema

1. **Acceder al Menú Oculto:**
   - Tocar icono batería 4 veces (8-9-8-9)
   - O mantener botón 4X4 presionado 5 segundos

2. **Ver Errores Activos:**
   - Seleccionar "8) Ver errores"
   - Anotar todos los códigos mostrados

3. **Consultar Este Documento:**
   - Buscar cada código en este documento
   - Seguir procedimientos de solución

4. **Borrar Errores Resueltos:**
   - Seleccionar "9) Borrar errores"
   - Confirmar borrado
   - Sistema reinicia contador

### Errores Críticos que Requieren Atención Inmediata

- **250-252**: Sistema de dirección comprometido
- **600-699**: Sistema de potencia/relés inestable
- **810-813**: Sobrecorriente en motores (riesgo de daño)

### Errores Informativos (No Críticos)

- **985**: Reset a fábrica (informativo)
- **975**: Restauración automática exitosa

---

## 📞 SOPORTE TÉCNICO

Si un error persiste después de seguir los procedimientos:

1. Anotar código de error exacto
2. Anotar condiciones cuando ocurrió
3. Capturar logs del puerto serie (115200 baud)
4. Verificar versión de firmware (debe ser v2.9.5)
5. Consultar documentación adicional en carpeta `/docs`

### Documentos Relacionados:
- `MANUAL_COMPLETO_CONEXIONES.md` - Conexionado completo
- `HARDWARE_CONFIGURACION_COMPLETA.md` - Especificaciones de hardware
- `PIN_MAPPING_DEVKITC1.md` - Mapa de pines GPIO
- `SENSORES_TEMPERATURA_DS18B20.md` - Guía de sensores de temperatura
- `TOUCH_CALIBRATION_GUIDE.md` - Calibración de pantalla táctil

---

## 📊 RANGOS DE CÓDIGOS (RESUMEN)

| Rango | Subsistema | Ejemplos |
|-------|------------|----------|
| 100-199 | Entradas y controles | Pedal, botones |
| 200-299 | Sistema de dirección | Encoder, motor steering |
| 300-399 | Sensores de corriente | INA226 |
| 400-499 | Sensores de temperatura | DS18B20 |
| 500-599 | Sensores de rueda | Inductivos LJ12A3 |
| 600-699 | Relés y potencia | SRD-05VDC |
| 700-799 | Sistema de audio | DFPlayer, alertas |
| 800-899 | Sistema de tracción | Motors, PWM |
| 900-999 | Almacenamiento | EEPROM, storage |

---

**Versión del documento:** 1.0  
**Fecha de creación:** 2025-12-05  
**Compatible con firmware:** v2.9.5+  
**Autor:** Sistema de documentación automática
