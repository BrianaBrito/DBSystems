#pragma once
/**
 * @file config.h
 * @brief Configuración central del firmware: identidad, pines y parámetros.
 *
 * Los pines están documentados en docs/pines.md. Si cambias uno aquí,
 * actualiza también esa tabla. Las credenciales NO van aquí: están en
 * secrets.h (ignorado por Git).
 */
#include <Arduino.h>

#define FW_NAME "smart-home-esp32"
#define FW_VERSION "0.5.1"

namespace cfg {
constexpr uint32_t SERIAL_BAUD = 115200;
constexpr uint16_t HTTP_PORT = 80;

// WiFi (modo dual AP+STA)
constexpr uint8_t AP_CHANNEL = 1;  // el AP se moverá al canal del router al conectar la STA
constexpr uint8_t AP_MAX_CLIENTS = 4;
constexpr uint32_t STA_RECONNECT_INTERVAL_MS = 15000;

// Backend (PHP en la laptop)
constexpr const char* DEVICE_ID = "casa-esp32";
constexpr uint8_t BACKEND_QUEUE_MAX = 30;       // eventos pendientes que se conservan
constexpr uint16_t BACKEND_TIMEOUT_MS = 2000;
constexpr uint32_t BACKEND_RETRY_MS = 5000;

// Caseta: servo y tiempos
constexpr uint8_t GATE_CLOSED_DEG = 0;
constexpr uint8_t GATE_OPEN_DEG = 90;
constexpr uint32_t GATE_TRAVEL_MS = 900;          // lo que tarda la barra en moverse
constexpr uint32_t GATE_WAIT_VEHICLE_MS = 10000;  // espera máxima a que pase el vehículo
constexpr uint32_t GATE_CLEAR_DELAY_MS = 1500;    // tiempo libre antes de cerrar

// Caseta: sensor ultrasónico (calibrar con docs/caseta.md)
constexpr uint16_t GATE_DETECT_CM = 15;
constexpr uint16_t GATE_RELEASE_MARGIN_CM = 5;  // histéresis: libre al superar detect + margen
constexpr uint16_t SENSOR_MAX_CM = 60;
constexpr uint8_t SENSOR_CONFIRM_READS = 2;
constexpr uint32_t SENSOR_INTERVAL_MS = 100;

// Caseta: RFID
constexpr uint32_t RFID_POLL_MS = 100;
constexpr uint32_t RFID_COOLDOWN_MS = 2000;
constexpr uint32_t LEARN_MODE_MS = 20000;

// Cocina (valores iniciales; se pueden ajustar por API, ver docs/cocina.md)
constexpr float KITCHEN_TEMP_MAX_C = 40.0f;      // alerta al alcanzarla
constexpr float KITCHEN_TEMP_HYST_C = 2.0f;      // se despeja por debajo de max - hist
constexpr uint16_t KITCHEN_GAS_DELTA = 450;      // cuentas ADC sobre la línea base
constexpr uint16_t KITCHEN_GAS_HYST = 100;
constexpr uint8_t KITCHEN_CONFIRM_TEMP = 2;      // lecturas seguidas (una cada 2 s)
constexpr uint8_t KITCHEN_CONFIRM_GAS = 4;       // lecturas seguidas (una cada 200 ms)
constexpr uint32_t KITCHEN_WARMUP_MS = 20000;    // el MQ-2 necesita calentar
constexpr uint32_t KITCHEN_BASELINE_MS = 10000;  // ventana para medir aire "normal"
constexpr uint32_t KITCHEN_DHT_MS = 2000;
constexpr uint32_t KITCHEN_GAS_MS = 200;
constexpr uint32_t KITCHEN_LED_BLINK_MS = 250;

// Luces: sensor PIR de presencia
constexpr uint32_t PIR_WARMUP_MS = 30000;   // el HC-SR501 necesita ~30-60 s al encender
constexpr uint32_t PRESENCE_HOLD_MS = 10000;  // el LED sigue encendido tras el último movimiento
constexpr uint32_t PIR_INTERVAL_MS = 100;

// Cochera: portón de una hoja con un servo en la bisagra
constexpr uint8_t GARAGE_CLOSED_DEG = 180;          // calibrable por API
constexpr uint8_t GARAGE_OPEN_DEG = 90;
constexpr uint32_t GARAGE_TRAVEL_MS = 2000;       // tiempo de apertura o cierre
constexpr uint32_t GARAGE_RF_DEBOUNCE_MS = 50;
constexpr uint32_t GARAGE_RF_LOCK_MS = 400;       // tiempo muerto entre pulsaciones
constexpr uint32_t GARAGE_SERVO_INTERVAL_MS = 20;
}  // namespace cfg

namespace pins {
// --- RC522 (SPI VSPI, 3.3 V) ---
constexpr uint8_t RFID_SCK = 18;
constexpr uint8_t RFID_MISO = 19;
constexpr uint8_t RFID_MOSI = 23;
constexpr uint8_t RFID_SS = 5;
constexpr uint8_t RFID_RST = 27;

// --- Servomotores (alimentación 5 V externa, GND común) ---
constexpr uint8_t SERVO_GATE = 13;    // caseta de acceso
constexpr uint8_t SERVO_GARAGE = 14;  // portón de la cochera

// --- HC-SR04 de la caseta (5 V). El ECHO necesita divisor de voltaje a 3.3 V ---
constexpr uint8_t GATE_TRIG = 25;  // detección del vehículo en la caseta
constexpr uint8_t GATE_ECHO = 34;  // solo entrada
// --- Presencia: PIR HC-SR501 (VCC 5 V, salida a 3.3 V) ---
constexpr uint8_t PRESENCE_PIR = 26;

// --- Cocina ---
constexpr uint8_t DHT_DATA = 4;  // (no llamarlo DHT11 ni DHT22: son macros de la librería DHT)
constexpr uint8_t MQ2_AO = 32;  // ADC1 (el ADC2 no funciona con WiFi activo); con divisor
constexpr uint8_t KITCHEN_ALERT_LED = 22;

// --- RX480E (433 MHz), salidas a 5 V: divisor de voltaje ---
constexpr uint8_t RF_OPEN = 36;   // D0 (botón A): alterna abrir/cerrar; solo entrada
constexpr uint8_t RF_CLOSE = 39;  // D1 (botón B): también alterna; solo entrada

// --- Registro de desplazamiento 74HC595 (alimentado a 3.3 V) ---
constexpr uint8_t SR_DATA = 16;   // SER
constexpr uint8_t SR_CLOCK = 17;  // SRCLK
constexpr uint8_t SR_LATCH = 21;  // RCLK

// --- Otros ---
constexpr uint8_t BUZZER = 33;      // buzzer pasivo (PWM)
constexpr uint8_t STATUS_LED = 2;   // LED integrado de la placa
}  // namespace pins
