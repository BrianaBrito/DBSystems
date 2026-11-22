#pragma once
/**
 * @file BackendClient.h
 * @brief Envía eventos en JSON al backend PHP sin bloquear loop().
 *
 * HTTPClient es bloqueante (puede tardar segundos si la laptop no responde), por eso
 * los envíos corren en una tarea de FreeRTOS propia. loop() solo encola el texto JSON.
 *
 * - Cola acotada: si el backend no está, se descartan los eventos más viejos.
 * - Reintentos: ante un fallo espera `retryMs` y vuelve a intentar el mismo evento.
 * - Si la URL está vacía, el cliente queda desactivado.
 */
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <deque>

struct BackendClientConfig {
  const char* baseUrl;  // p. ej. "http://192.168.1.50:8080" (sin barra final)
  const char* token;    // se envía en X-API-Token
  uint8_t queueMax;     // eventos pendientes que se conservan
  uint16_t timeoutMs;   // tiempo máximo de conexión y de respuesta
  uint32_t retryMs;     // espera tras un fallo
};

class BackendClient {
 public:
  explicit BackendClient(const BackendClientConfig& config);

  /** Crea la tarea de envío. Llamar una vez en setup(). */
  void begin();

  /** Encola un cuerpo JSON para POST {baseUrl}{path}. Seguro de llamar desde cualquier tarea. */
  void enqueue(const char* path, const String& json);

  bool enabled() const { return enabled_; }
  uint32_t sentCount() const { return sent_; }
  uint32_t failedCount() const { return failed_; }
  size_t pending() const;

 private:
  struct Item {
    String path;
    String json;
  };

  static void taskEntry(void* self);
  void run();
  bool post(const Item& item);

  BackendClientConfig config_;
  bool enabled_;
  SemaphoreHandle_t mutex_;
  std::deque<Item> queue_;
  volatile uint32_t sent_;
  volatile uint32_t failed_;
};
