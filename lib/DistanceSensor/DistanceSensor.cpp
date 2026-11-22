#include "DistanceSensor.h"

DistanceSensor::DistanceSensor(const DistanceSensorConfig& config)
    : config_(config),
      filter_(config.detectCm, config.releaseCm, config.confirmReads),
      distanceCm_(-1),
      lastReadMs_(0) {}

void DistanceSensor::begin() {
  pinMode(config_.trigPin, OUTPUT);
  digitalWrite(config_.trigPin, LOW);
  pinMode(config_.echoPin, INPUT);
}

void DistanceSensor::update(uint32_t nowMs) {
  if (nowMs - lastReadMs_ < config_.intervalMs) return;
  lastReadMs_ = nowMs;

  distanceCm_ = measureCm();
  filter_.addReading(distanceCm_);
}

int16_t DistanceSensor::measureCm() {
  // Pulso de disparo de 10 µs.
  digitalWrite(config_.trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(config_.trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(config_.trigPin, LOW);

  // El sonido recorre 1 cm de ida y vuelta en ~58 µs. El tiempo límite acota
  // cuánto puede bloquear la lectura cuando no hay nada enfrente.
  const unsigned long timeoutUs = static_cast<unsigned long>(config_.maxCm) * 58UL + 200UL;
  const unsigned long echoUs = pulseIn(config_.echoPin, HIGH, timeoutUs);
  if (echoUs == 0) return -1;  // sin eco

  return static_cast<int16_t>(echoUs / 58UL);
}
