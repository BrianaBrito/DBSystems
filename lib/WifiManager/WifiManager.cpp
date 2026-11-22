#include "WifiManager.h"

#include <Logger.h>

namespace {
const char* const TAG = "wifi";
}

WifiManager::WifiManager(const WifiManagerConfig& config)
    : config_(config),
      apRunning_(false),
      staConfigured_(config.staSsid != nullptr && config.staSsid[0] != '\0'),
      staWasConnected_(false),
      lastAttemptMs_(0),
      attempts_(0) {}

bool WifiManager::begin() {
  WiFi.persistent(false);  // no escribir credenciales en la flash en cada arranque
  WiFi.mode(WIFI_AP_STA);
  WiFi.setSleep(false);  // menor latencia para el servidor web

  const size_t passLen = config_.apPassword != nullptr ? strlen(config_.apPassword) : 0;
  if (passLen > 0 && passLen < 8) {
    LOG_E(TAG, "AP_PASS debe tener al menos 8 caracteres: el AP no se inicia");
  } else {
    apRunning_ = WiFi.softAP(config_.apSsid, passLen > 0 ? config_.apPassword : nullptr,
                             config_.apChannel, 0, config_.apMaxClients);
    if (apRunning_) {
      LOG_I(TAG, "AP '%s' activo en %s", config_.apSsid, WiFi.softAPIP().toString().c_str());
    } else {
      LOG_E(TAG, "No se pudo iniciar el AP");
    }
  }

  if (staConfigured_) {
    connectSta();
  } else {
    LOG_W(TAG, "WIFI_STA_SSID vacío: funcionando solo en modo AP");
  }
  return apRunning_;
}

void WifiManager::update(uint32_t nowMs) {
  if (!staConfigured_) return;

  const bool connected = isStaConnected();
  if (connected != staWasConnected_) {
    staWasConnected_ = connected;
    if (connected) {
      attempts_ = 0;
      LOG_I(TAG, "STA conectada: IP %s, %d dBm, canal %u", WiFi.localIP().toString().c_str(),
            (int)WiFi.RSSI(), (unsigned)WiFi.channel());
    } else {
      LOG_W(TAG, "STA desconectada");
      lastAttemptMs_ = nowMs;
    }
  }

  if (!connected && (nowMs - lastAttemptMs_) >= config_.reconnectIntervalMs) {
    lastAttemptMs_ = nowMs;
    connectSta();
  }
}

void WifiManager::connectSta() {
  attempts_++;
  LOG_I(TAG, "Conectando a '%s' (intento %lu)...", config_.staSsid, (unsigned long)attempts_);
  WiFi.disconnect(false);
  WiFi.begin(config_.staSsid, config_.staPassword);
}

bool WifiManager::isStaConnected() const { return WiFi.status() == WL_CONNECTED; }

IPAddress WifiManager::apIp() const { return WiFi.softAPIP(); }

IPAddress WifiManager::staIp() const { return WiFi.localIP(); }

int32_t WifiManager::staRssi() const { return WiFi.RSSI(); }

uint8_t WifiManager::apClientCount() const { return WiFi.softAPgetStationNum(); }

uint8_t WifiManager::channel() const { return WiFi.channel(); }
