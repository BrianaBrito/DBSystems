#include "BackendClient.h"

#include <HTTPClient.h>
#include <Logger.h>
#include <ScopedLock.h>
#include <WiFi.h>

namespace {
const char* const TAG = "backend";
}

BackendClient::BackendClient(const BackendClientConfig& config)
    : config_(config), enabled_(false), mutex_(nullptr), sent_(0), failed_(0) {}

void BackendClient::begin() {
  mutex_ = xSemaphoreCreateMutex();
  enabled_ = (config_.baseUrl != nullptr && config_.baseUrl[0] != '\0');
  if (!enabled_) {
    LOG_W(TAG, "BACKEND_URL vacio: no se enviaran eventos al backend");
    return;
  }
  LOG_I(TAG, "enviando eventos a %s", config_.baseUrl);
  xTaskCreatePinnedToCore(taskEntry, "backend", 6144, this, 1, nullptr, 0);
}

void BackendClient::enqueue(const char* path, const String& json) {
  if (!enabled_) return;
  ScopedLock lock(mutex_);
  if (queue_.size() >= config_.queueMax) {
    queue_.pop_front();  // descarta el más viejo
    LOG_W(TAG, "cola llena: se descarto el evento mas antiguo");
  }
  Item item;
  item.path = path;
  item.json = json;
  queue_.push_back(item);
}

size_t BackendClient::pending() const {
  ScopedLock lock(mutex_);
  return queue_.size();
}

void BackendClient::taskEntry(void* self) {
  static_cast<BackendClient*>(self)->run();
}

void BackendClient::run() {
  for (;;) {
    Item item;
    bool have = false;
    {
      ScopedLock lock(mutex_);
      if (!queue_.empty()) {
        item = queue_.front();  // se queda en la cola hasta confirmar el envío
        have = true;
      }
    }

    if (!have) {
      vTaskDelay(pdMS_TO_TICKS(200));
      continue;
    }

    if (post(item)) {
      {
        ScopedLock lock(mutex_);
        if (!queue_.empty()) queue_.pop_front();
      }
      sent_++;
    } else {
      failed_++;
      vTaskDelay(pdMS_TO_TICKS(config_.retryMs));
    }
  }
}

bool BackendClient::post(const Item& item) {
  WiFiClient client;
  HTTPClient http;
  http.setConnectTimeout(config_.timeoutMs);
  http.setTimeout(config_.timeoutMs);

  const String url = String(config_.baseUrl) + item.path;
  if (!http.begin(client, url)) {
    LOG_W(TAG, "URL invalida: %s", url.c_str());
    return false;
  }
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-API-Token", config_.token);

  const int code = http.POST(item.json);
  http.end();

  // 201 = guardado, 200 = duplicado (ya estaba): en ambos casos el evento está a salvo.
  if (code == 200 || code == 201) return true;

  // Un 4xx significa que el backend rechazó el contenido; reintentar no lo arreglaría,
  // así que se descarta para no bloquear la cola.
  if (code >= 400 && code < 500) {
    LOG_E(TAG, "backend rechazo el evento (HTTP %d): se descarta", code);
    return true;
  }

  LOG_W(TAG, "envio fallido (codigo %d), reintento en %lu ms", code,
        static_cast<unsigned long>(config_.retryMs));
  return false;
}
