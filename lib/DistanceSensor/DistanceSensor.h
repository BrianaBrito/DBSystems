#pragma once
/**
 * @file DistanceSensor.h
 * @brief Sensor ultrasónico HC-SR04 con filtro de presencia.
 *
 * Mide cada `intervalMs` (sin bloquear más de unos milisegundos) y alimenta un
 * PresenceFilter, de modo que isPresent() no parpadea por lecturas sueltas.
 *
 * Importante: el pin ECHO del HC-SR04 sale a 5 V; el ESP32 solo tolera 3.3 V.
 * Necesita divisor de voltaje (ver docs/pines.md).
 */
#include <Arduino.h>
#include <PresenceFilter.h>

struct DistanceSensorConfig {
  uint8_t trigPin;
  uint8_t echoPin;
  uint16_t maxCm;        // alcance máximo que interesa medir
  uint16_t detectCm;     // distancia a la que se considera "hay algo"
  uint16_t releaseCm;    // distancia a la que se considera "ya no hay nada" (> detectCm)
  uint8_t confirmReads;  // lecturas seguidas para cambiar de estado
  uint32_t intervalMs;   // tiempo entre mediciones
};

class DistanceSensor {
 public:
  explicit DistanceSensor(const DistanceSensorConfig& config);

  void begin();
  void update(uint32_t nowMs);

  /** true si hay un objeto confirmado dentro de detectCm. */
  bool isPresent() const { return filter_.present(); }

  /** Última distancia medida en cm; -1 si no hubo eco. */
  int16_t distanceCm() const { return distanceCm_; }

 private:
  int16_t measureCm();

  DistanceSensorConfig config_;
  PresenceFilter filter_;
  int16_t distanceCm_;
  uint32_t lastReadMs_;
};
