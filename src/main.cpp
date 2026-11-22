/**
 * @file main.cpp
 * @brief Punto de entrada: arma los componentes y los mantiene en marcha.
 *
 * Toda la lógica vive en lib/ (un módulo por responsabilidad); aquí solo se
 * conectan. loop() no bloquea: cada componente recibe millis() y decide qué hacer.
 */
#include <Arduino.h>
#include <LittleFS.h>

#include <ApiServer.h>
#include <Logger.h>
#include <StatusLed.h>
#include <WifiManager.h>

#include "config.h"
#include "secrets.h"

namespace {

const char* const TAG = "main";

WifiManagerConfig makeWifiConfig() {
  WifiManagerConfig c;
  c.apSsid = AP_SSID;
  c.apPassword = AP_PASS;
  c.apChannel = cfg::AP_CHANNEL;
  c.apMaxClients = cfg::AP_MAX_CLIENTS;
  c.staSsid = WIFI_STA_SSID;
  c.staPassword = WIFI_STA_PASS;
  c.reconnectIntervalMs = cfg::STA_RECONNECT_INTERVAL_MS;
  return c;
}

WifiManager wifi(makeWifiConfig());
ApiServer api(cfg::HTTP_PORT, API_TOKEN, FW_NAME, FW_VERSION, wifi);
StatusLed statusLed(pins::STATUS_LED);

}  // namespace

void setup() {
  Serial.begin(cfg::SERIAL_BAUD);
  statusLed.begin();
  statusLed.setPattern(StatusLed::BlinkFast);

  Serial.println();
  LOG_I(TAG, "%s v%s iniciando", FW_NAME, FW_VERSION);

  if (strcmp(API_TOKEN, "cambia_este_token") == 0) {
    LOG_W(TAG, "API_TOKEN sigue con el valor de ejemplo: cámbialo en secrets.h");
  }

  if (!LittleFS.begin(true)) {
    LOG_E(TAG, "No se pudo montar LittleFS");
  }

  wifi.begin();
  // Los módulos (caseta, cocina, luces...) se registran aquí con api.addModule(...)
  api.begin();
}

void loop() {
  const uint32_t now = millis();

  wifi.update(now);

  if (!wifi.isStaConfigured()) {
    statusLed.setPattern(StatusLed::BlinkSlow);  // solo AP
  } else {
    statusLed.setPattern(wifi.isStaConnected() ? StatusLed::On : StatusLed::BlinkFast);
  }
  statusLed.update(now);
}
