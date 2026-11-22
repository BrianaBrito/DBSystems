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
#define FW_VERSION "0.1.0"

namespace cfg {
constexpr uint32_t SERIAL_BAUD = 115200;
constexpr uint16_t HTTP_PORT = 80;

// WiFi (modo dual AP+STA)
constexpr uint8_t AP_CHANNEL = 1;  // el AP se moverá al canal del router al conectar la STA
constexpr uint8_t AP_MAX_CLIENTS = 4;
constexpr uint32_t STA_RECONNECT_INTERVAL_MS = 15000;
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

// --- HC-SR04 (5 V). El ECHO necesita divisor de voltaje a 3.3 V ---
constexpr uint8_t GATE_TRIG = 25;  // detección del vehículo en la caseta
constexpr uint8_t GATE_ECHO = 34;  // solo entrada
constexpr uint8_t PRESENCE_TRIG = 26;
constexpr uint8_t PRESENCE_ECHO = 35;  // solo entrada

// --- Cocina ---
constexpr uint8_t DHT22 = 4;
constexpr uint8_t MQ2_AO = 32;  // ADC1 (el ADC2 no funciona con WiFi activo); con divisor
constexpr uint8_t KITCHEN_ALERT_LED = 22;

// --- RX480E (433 MHz), salidas a 5 V: divisor de voltaje ---
constexpr uint8_t RF_OPEN = 36;   // D0, solo entrada
constexpr uint8_t RF_CLOSE = 39;  // D1, solo entrada

// --- Registro de desplazamiento 74HC595 (alimentado a 3.3 V) ---
constexpr uint8_t SR_DATA = 16;   // SER
constexpr uint8_t SR_CLOCK = 17;  // SRCLK
constexpr uint8_t SR_LATCH = 21;  // RCLK

// --- Otros ---
constexpr uint8_t BUZZER = 33;      // buzzer pasivo (PWM)
constexpr uint8_t STATUS_LED = 2;   // LED integrado de la placa
}  // namespace pins
