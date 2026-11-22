#pragma once
/**
 * @file ScopedLock.h
 * @brief Bloqueo de un mutex de FreeRTOS con liberación automática (RAII).
 *
 * Los manejadores HTTP corren en otra tarea que loop(); todo dato que ambos
 * toquen (listas, historial) se protege con un mutex.
 *
 * Uso:
 *   {
 *     ScopedLock lock(mutex_);
 *     lista_.push_back(x);
 *   }  // se libera al salir del bloque
 */
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

class ScopedLock {
 public:
  explicit ScopedLock(SemaphoreHandle_t mutex) : mutex_(mutex) {
    if (mutex_ != nullptr) xSemaphoreTake(mutex_, portMAX_DELAY);
  }
  ~ScopedLock() {
    if (mutex_ != nullptr) xSemaphoreGive(mutex_);
  }

 private:
  ScopedLock(const ScopedLock&);             // no copiable
  ScopedLock& operator=(const ScopedLock&);  // no copiable
  SemaphoreHandle_t mutex_;
};
