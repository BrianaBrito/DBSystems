#pragma once
/**
 * @file Logger.h
 * @brief Macros de registro por el puerto serie con niveles.
 *
 * Nivel configurable en tiempo de compilación con -DLOG_LEVEL=n en
 * platformio.ini (0 = apagado, 1 = errores, 2 = avisos, 3 = info, 4 = depuración).
 *
 * Uso: LOG_I("wifi", "Conectado a %s", ssid);
 */
#include <Arduino.h>

#ifndef LOG_LEVEL
#define LOG_LEVEL 3
#endif

#define LOG_E(tag, fmt, ...)                                              \
  do {                                                                    \
    if (LOG_LEVEL >= 1) Serial.printf("[E][%s] " fmt "\n", tag, ##__VA_ARGS__); \
  } while (0)

#define LOG_W(tag, fmt, ...)                                              \
  do {                                                                    \
    if (LOG_LEVEL >= 2) Serial.printf("[W][%s] " fmt "\n", tag, ##__VA_ARGS__); \
  } while (0)

#define LOG_I(tag, fmt, ...)                                              \
  do {                                                                    \
    if (LOG_LEVEL >= 3) Serial.printf("[I][%s] " fmt "\n", tag, ##__VA_ARGS__); \
  } while (0)

#define LOG_D(tag, fmt, ...)                                              \
  do {                                                                    \
    if (LOG_LEVEL >= 4) Serial.printf("[D][%s] " fmt "\n", tag, ##__VA_ARGS__); \
  } while (0)
