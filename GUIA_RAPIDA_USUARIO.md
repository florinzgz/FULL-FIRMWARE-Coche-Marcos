# 🚀 GUÍA RÁPIDA DE USUARIO - Coche Eléctrico ESP32-S3

**Versión**: 2.17.1 | **Actualización**: Enero 2026

---

## ⚡ INICIO RÁPIDO EN 5 PASOS

### 1️⃣ Conectar Batería
- Conectar batería 24V: **ROJO (+)**, **NEGRO (-)**
- Verificar voltaje: debe estar entre 22-26V

### 2️⃣ Encender
- Pulsar botón de encendido
- Esperar logo y mensaje: *"Bienvenido Marcos..."*
- Verificar dashboard visible

### 3️⃣ Verificar
- ✅ Batería > 30%
- ✅ Temperatura < 40°C
- ✅ Touch responde
- ✅ Sin códigos de error

### 4️⃣ Seleccionar Marcha
- Tocar **[D]** para avanzar
- Tocar **[R]** para retroceso
- Tocar **[P]** para estacionar

### 5️⃣ Conducir
- Presionar pedal suavemente
- ABS y TCS se activan automáticamente
- Monitorear temperatura en pantalla

---

## 🎯 CONTROLES PRINCIPALES

### Pantalla Dashboard

```
┌─────────────────────────────────────┐
│ ⚡24V  🌡️25°C      [DRIVE]  🔊    │
├─────────────────────────────────────┤
│                                     │
│   Velocímetro    |    Batería       │
│      0 km/h      |      85%         │
│                                     │
│  Ruedas FL FR    |  Corriente 2.5A  │
│  Ruedas RL RR    |  Temp Max 28°C   │
│                                     │
│  [P] [R] [D]     ← Cambio marcha    │
│                                     │
│  [💡] [🔊] [⚙️]  ← Funciones       │
└─────────────────────────────────────┘
```

### Botones Rápidos

| Botón | Función |
|-------|---------|
| **💡** | Cambiar luces (OFF → Low → High → DRL) |
| **🔊** | Control volumen audio |
| **⚙️** | Menú configuración |
| **[P] [R] [D]** | Cambiar marcha (solo con vehículo parado) |
| **Batería** × 5 | Menú oculto (calibraciones) |

---

## 🔧 CALIBRACIONES RÁPIDAS

### Touch no funciona
**Mantener botón físico 5 segundos** → Calibración automática

### Pedal no responde bien
1. Menú batería × 5 → "1) Calibrar pedal"
2. Soltar pedal completamente
3. Presionar hasta el fondo
4. Guardar

### Dirección descentrada
1. Menú batería × 5 → "2) Calibrar encoder"
2. Centrar volante físicamente
3. Presionar "Calibrar"
4. Girar izq/der al máximo
5. Volver al centro

---

## 🛡️ SISTEMAS DE SEGURIDAD

| Sistema | Estado | Indicador |
|---------|--------|-----------|
| **ABS** | Auto (>5 km/h) | LED en dashboard |
| **TCS** | Auto (>3 km/h) | Icono parpadeante |
| **Regenerativo** | Nivel 0-3 | Flecha verde batería |

**Activación**: Todos automáticos. Configurar en Menú ⚙️ → Seguridad

---

## 💡 LUCES

### LEDs Frontales (28)
- Tocar 💡 para cambiar modo
- Modos: OFF → Low → High → DRL → Emergencia

### LEDs Traseros (16)
**Automáticos**:
- REVERSE: Blanco intenso
- Frenado: Rojo intenso
- Normal: Rojo suave
- PARK: Rojo intermitente

---

## 🔊 AUDIO

### Configurar Audio (Primera Vez)
1. Formatear SD en **FAT32**
2. Ir a [TTSMaker.com](https://ttsmaker.com/)
3. Idioma: **Español (España)**
4. Generar MP3 de `docs/AUDIO_TRACKS_GUIDE.md`
5. Copiar archivos 0001.mp3, 0002.mp3... a raíz de SD
6. Insertar SD en DFPlayer Mini

**Audios mínimos**: 0001, 0002, 0012, 0013, 0020, 0022, 0024

---

## ⚠️ ALERTAS IMPORTANTES

| Alerta | Significado | Acción |
|--------|-------------|--------|
| **Batería < 20%** | Nivel bajo | Cargar pronto |
| **Batería < 10%** | Nivel crítico | Volver a casa |
| **Temp > 60°C** | Motor caliente | Reducir velocidad |
| **Temp > 70°C** | Motor muy caliente | Detener y enfriar |
| **E001-E010** | Código error | Ver manual sección 13.2 |

---

## 🆘 SOLUCIÓN RÁPIDA

### ❌ No enciende
1. Verificar batería conectada (>22V)
2. Verificar fusibles
3. Verificar botón encendido

### ❌ Touch no responde
1. **Mantener botón físico 5 seg** → Calibrar
2. Verificar conexión cable touch
3. Reset: Menú → Reset fábrica

### ❌ Motor sin potencia
1. Verificar batería (>23V)
2. Verificar temperatura (<60°C)
3. Calibrar INA226: Menú batería × 5 → Calibrar INA

### ❌ Audio no suena
1. Verificar SD insertada (FAT32)
2. Verificar archivos .mp3 en raíz
3. Ajustar volumen: 🔊 > 0
4. Verificar LED DFPlayer parpadea

### ❌ LEDs no encienden
1. Tocar 💡 y cambiar modo
2. Verificar conexión GPIO 1 (front) / 48 (rear)
3. Verificar alimentación 5V

---

## 📋 CHECKLIST PRE-USO

Antes de cada uso:
- [ ] Batería > 30%
- [ ] Conexiones firmes
- [ ] Pantalla enciende
- [ ] Touch responde
- [ ] Ruedas giran libremente
- [ ] Sin cables sueltos
- [ ] Temperatura < 40°C
- [ ] Luces funcionan
- [ ] Audio OK
- [ ] Sin códigos error

---

## 🔋 CUIDADO BATERÍA

### Carga
- Usar cargador 24V (2-5A)
- Cargar después de cada uso
- No dejar cargando >8 horas
- Lugar ventilado

### Almacenamiento
- Guardar al 50-70%
- Desconectar si no se usa >1 semana
- Temperatura: 0°C a 40°C
- No descargar por debajo del 20%

---

## 🔧 MANTENIMIENTO BÁSICO

### Cada Uso
- Inspección visual
- Limpiar pantalla

### Semanal
- Limpiar sensores rueda
- Verificar conexiones
- Comprobar neumáticos

### Mensual
- Lubricar dirección
- Verificar calibraciones
- Actualizar firmware si hay actualizaciones

---

## 📊 ESPECIFICACIONES CLAVE

| Parámetro | Valor |
|-----------|-------|
| **Batería** | 24V (22-26V) |
| **Velocidad máx** | 8 km/h |
| **Autonomía** | 2-4 horas |
| **Peso máx carga** | 30 kg |
| **Motores tracción** | 4× RS775 24V |
| **Motor dirección** | 1× RS390 12V |
| **Pantalla** | 480×320 touch |
| **LEDs** | 28 frontales + 16 traseros |

---

## 📞 MÁS INFORMACIÓN

- **Manual completo**: `MANUAL_USUARIO.md`
- **Audio guía**: `docs/AUDIO_TRACKS_GUIDE.md`
- **Códigos error**: `docs/CODIGOS_ERROR.md`
- **Solución problemas touch**: `docs/TOUCH_TROUBLESHOOTING.md`
- **GitHub**: [github.com/florinzgz/FULL-FIRMWARE-Coche-Marcos](https://github.com/florinzgz/FULL-FIRMWARE-Coche-Marcos)

---

## 🎯 CONSEJOS DE USO

✅ **DO** (Hacer):
- Supervisión adulta siempre
- Usar en terreno plano
- Cargar después de usar
- Verificar antes de usar
- Realizar mantenimiento regular

❌ **DON'T** (No hacer):
- Usar sin supervisión
- Cambiar marcha en movimiento
- Descargar batería <20%
- Usar con temperatura >70°C
- Modificar firmware sin conocimientos

---

**¡Disfruta tu coche de forma segura!** 🚗💨

---

*Para información detallada, consultar `MANUAL_USUARIO.md` completo*
