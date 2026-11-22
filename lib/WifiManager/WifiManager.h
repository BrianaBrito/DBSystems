#pragma once
/**
 * @file WifiManager.h
 * @brief Gestión de WiFi en modo dual AP+STA, sin bloqueos.
 *
 *  - AP: red propia del ESP32, siempre disponible (configuración y respaldo).
 *  - STA: conexión al router de la casa, con reintentos automáticos.
 *
 * Nota: en modo dual ambas interfaces comparten el canal de radio; cuando la
 * STA se conecta, el AP se mueve al canal del router.
 */
#include <Arduino.h>
#include <WiFi.h>

struct WifiManagerConfig {
  const char* apSsid;
  const char* apPassword;  // vacío = AP abierto; si no, mínimo 8 caracteres
  uint8_t apChannel;
  uint8_t apMaxClients;
  const char* staSsid;  // vacío = solo modo AP
  const char* staPassword;
  uint32_t reconnectIntervalMs;
};

class WifiManager {
 public:
  explicit WifiManager(const WifiManagerConfig& config);

  /** Inicia el AP y comienza a conectar la STA. Devuelve true si el AP quedó activo. */
  bool begin();

  /** Vigila la conexión y reintenta si se cayó. Llamar en cada loop(). */
  void update(uint32_t nowMs);

  bool isApRunning() const { return apRunning_; }
  bool isStaConfigured() const { return staConfigured_; }
  bool isStaConnected() const;
  IPAddress apIp() const;
  IPAddress staIp() const;
  int32_t staRssi() const;
  uint8_t apClientCount() const;
  uint8_t channel() const;
  const char* apSsid() const { return config_.apSsid; }
  const char* staSsid() const { return config_.staSsid; }

 private:
  void connectSta();

  WifiManagerConfig config_;
  bool apRunning_;
  bool staConfigured_;
  bool staWasConnected_;
  uint32_t lastAttemptMs_;
  uint32_t attempts_;
};
