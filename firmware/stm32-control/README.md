# Firmware STM32G474RE (Control y Seguridad)

Este directorio contiene el firmware dedicado al **STM32G474RE** para control crítico y seguridad. El objetivo es aislar el control determinista del HMI y comunicar por **CAN bus**.

## Alcance inicial
- MCU objetivo: **STM32G474RE** con **FDCAN** integrado.
- Transporte: **CAN bus** (contrato en `shared/protocol/can_protocol.h`).
- Build recomendado: **STM32CubeIDE** / **STM32CubeMX** (proyecto generado en este directorio).

## Estructura prevista
```
stm32-control/
├─ Core/               (CubeMX)
├─ Drivers/            (CubeMX)
├─ src/                (código de aplicación)
└─ include/            (interfaces locales)
```

## Próximos pasos
1. Generar proyecto base en CubeMX (STM32G474RE + FDCAN).
2. Integrar driver CAN y framing según `shared/protocol/can_protocol.h`.
3. Migrar control y safety desde `src/control` y `src/safety`.
