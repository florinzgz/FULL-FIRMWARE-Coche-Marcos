#pragma once
#include "queue.h"

namespace Audio {
    // Definición completa de los 68 tracks de audio disponibles
    // Ver docs/AUDIO_TRACKS_GUIDE.md para textos sugeridos e instrucciones de grabación
    enum Track : uint16_t {
        // ============================================
        // TRACKS BÁSICOS (1-38)
        // ============================================
        
        // Sistema principal
        AUDIO_INICIO = 1,                    // Bienvenido Marcos. El sistema está listo para comenzar.
        AUDIO_APAGADO = 2,                   // Cerrando sistemas. Hasta pronto.
        AUDIO_ERROR_GENERAL = 3,             // Atención. Se ha detectado un error general.
        
        // Calibración de pedal
        AUDIO_PEDAL_OK = 4,                  // Calibración del pedal completada correctamente.
        AUDIO_PEDAL_ERROR = 5,               // Error en el sensor del pedal. Revise la conexión.
        
        // Sensores de corriente (INA226)
        AUDIO_INA_OK = 6,                    // Calibración de sensores de corriente finalizada.
        AUDIO_INA_ERROR = 7,                 // Error en sensores de corriente o shunt desconectado.
        
        // Encoder de dirección
        AUDIO_ENCODER_OK = 8,                // Encoder sincronizado correctamente.
        AUDIO_ENCODER_ERROR = 9,             // Error en el sensor de dirección. Compruebe el encoder.
        
        // Temperatura
        AUDIO_TEMP_ALTA = 10,                // Temperatura del motor elevada. Reduzca la velocidad.
        AUDIO_TEMP_NORMAL = 11,              // Temperatura del motor normalizada.
        
        // Batería
        AUDIO_BATERIA_BAJA = 12,             // Nivel de batería bajo. Conecte el cargador, por favor.
        AUDIO_BATERIA_CRITICA = 13,          // Advertencia. Batería en nivel crítico. Desconectando tracción.
        
        // Freno de estacionamiento
        AUDIO_FRENO_ON = 14,                 // Freno de estacionamiento activado.
        AUDIO_FRENO_OFF = 15,                // Freno de estacionamiento desactivado.
        
        // Luces
        AUDIO_LUCES_ON = 16,                 // Luces encendidas.
        AUDIO_LUCES_OFF = 17,                // Luces apagadas.
        
        // Radio/Multimedia
        AUDIO_RADIO_ON = 18,                 // Sistema multimedia activado.
        AUDIO_RADIO_OFF = 19,                // Sistema multimedia desactivado.
        
        // Marchas
        AUDIO_MARCHA_D1 = 20,                // Marcha D uno activada.
        AUDIO_MARCHA_D2 = 21,                // Marcha D dos activada.
        AUDIO_MARCHA_R = 22,                 // Marcha atrás activada.
        AUDIO_MARCHA_N = 23,                 // Punto muerto.
        AUDIO_MARCHA_P = 24,                 // Vehículo en posición de estacionamiento.
        
        // Menú oculto y calibración
        AUDIO_MENU_OCULTO = 25,              // Menú de calibración avanzado activado.
        AUDIO_CAL_PEDAL = 26,                // Iniciando calibración del pedal. Presione lentamente hasta el fondo.
        AUDIO_CAL_INA = 27,                  // Calibrando sensores de corriente. Espere unos segundos.
        AUDIO_CAL_ENCODER = 28,              // Calibrando el punto central del volante. Manténgalo recto.
        
        // Test del sistema
        AUDIO_TEST_SISTEMA = 29,             // Iniciando comprobación completa del sistema.
        AUDIO_TEST_OK = 30,                  // Comprobación finalizada. Todos los módulos operativos.
        
        // Emergencia y seguridad
        AUDIO_EMERGENCIA = 31,               // Modo de emergencia activado. Motor deshabilitado.
        AUDIO_REINICIO_SEGURIDAD = 32,       // Reinicio de seguridad completado.
        
        // Errores de sensores específicos
        AUDIO_SENSOR_TEMP_ERROR = 33,        // Error en sensor de temperatura.
        AUDIO_SENSOR_CORRIENTE_ERROR = 34,   // Anomalía en lectura de corriente.
        AUDIO_SENSOR_VELOCIDAD_ERROR = 35,   // Sin señal de velocidad. Revise sensores de rueda.
        
        // Estado de módulos
        AUDIO_MODULO_OK = 36,                // Módulo verificado correctamente.
        
        // Tracción 4x4/4x2
        AUDIO_TRACCION_4X4 = 37,             // Tracción 4x4 inteligente activada.
        AUDIO_TRACCION_4X2 = 38,             // Tracción 4x2 inteligente activada.
        
        // ============================================
        // TRACKS AVANZADOS (39-68) - v2.8.0
        // ============================================
        
        // Sistemas de seguridad avanzados (39-44)
        AUDIO_ABS_ACTIVADO = 39,             // Sistema antibloqueo de frenos activado.
        AUDIO_ABS_DESACTIVADO = 40,          // Sistema antibloqueo de frenos desactivado.
        AUDIO_TCS_ACTIVADO = 41,             // Control de tracción activado.
        AUDIO_TCS_DESACTIVADO = 42,          // Control de tracción desactivado.
        AUDIO_REGEN_ON = 43,                 // Frenado regenerativo activado.
        AUDIO_REGEN_OFF = 44,                // Frenado regenerativo desactivado.
        
        // WiFi y conectividad (45-48)
        AUDIO_WIFI_CONECTADO = 45,           // Conexión WiFi establecida.
        AUDIO_WIFI_DESCONECTADO = 46,        // Conexión WiFi perdida.
        AUDIO_OTA_INICIADO = 47,             // Actualización remota iniciada. No desconecte el vehículo.
        AUDIO_OTA_COMPLETADO = 48,           // Actualización completada. Reiniciando sistema.
        
        // Bluetooth (49-51)
        AUDIO_BT_CONECTADO = 49,             // Mando Bluetooth conectado.
        AUDIO_BT_DESCONECTADO = 50,          // Mando Bluetooth desconectado.
        AUDIO_BT_EMPAREJANDO = 51,           // Buscando mando Bluetooth. Mantenga pulsado el botón de emparejamiento.
        
        // Estados del vehículo (52-56)
        AUDIO_VELOCIDAD_MAXIMA = 52,         // Velocidad máxima alcanzada.
        AUDIO_SOBRECORRIENTE = 53,           // Advertencia. Corriente excesiva detectada.
        AUDIO_OBSTACULO = 54,                // Atención. Obstáculo detectado.
        AUDIO_ESTACIONANDO = 55,             // Modo asistencia de estacionamiento activado.
        AUDIO_ARRANQUE_SUAVE = 56,           // Iniciando arranque suave de motores.
        
        // Información de telemetría (57-60)
        AUDIO_BATERIA_50 = 57,               // Nivel de batería al 50 por ciento.
        AUDIO_BATERIA_25 = 58,               // Nivel de batería al 25 por ciento. Considere recargar.
        AUDIO_DISTANCIA_1KM = 59,            // Ha recorrido un kilómetro en esta sesión.
        AUDIO_AHORRO_ENERGIA = 60,           // Modo ahorro de energía activado.
        
        // Modos de conducción (61-63)
        AUDIO_MODO_ECO = 61,                 // Modo eco activado. Máxima eficiencia.
        AUDIO_MODO_NORMAL = 62,              // Modo normal activado.
        AUDIO_MODO_SPORT = 63,               // Modo deportivo activado. Máxima potencia.
        
        // Feedback de configuración (64-68)
        AUDIO_CONFIG_GUARDADA = 64,          // Configuración guardada correctamente.
        AUDIO_CONFIG_RESTAURADA = 65,        // Configuración de fábrica restaurada.
        AUDIO_ERRORES_BORRADOS = 66,         // Registro de errores borrado.
        AUDIO_REGEN_AJUSTADO = 67,           // Nivel de regeneración ajustado.
        AUDIO_BEEP = 68,                     // (Sonido corto de confirmación)
    };
}

class Alerts {
public:
    static void init();
    static void play(const Audio::Item &item);
    static void play(Audio::Track t);

    // 🔎 Declaración añadida para que coincida con alerts.cpp
    static bool initOK();
};