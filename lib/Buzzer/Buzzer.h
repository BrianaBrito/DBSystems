#pragma once
/**
 * @file Buzzer.h
 * @brief Buzzer pasivo con patrones de sonido no bloqueantes (PWM por LEDC).
 *
 * Usa el canal LEDC 14 para no chocar con los canales que asigna ESP32Servo
 * (que empiezan en 0). Compatible con el core de Arduino 2.x y 3.x.
 */
#include <Arduino.h>

class Buzzer {
 public:
  enum Pattern : uint8_t {
    Click,   // pitido muy corto
    Accept,  // acceso concedido: dos tonos ascendentes
    Deny,    // acceso denegado: dos tonos graves
    Alert    // alarma (se repite con loop = true hasta llamar a stop())
  };

  explicit Buzzer(uint8_t pin);

  void begin();

  /** Reproduce un patrón; interrumpe el que esté sonando. */
  void play(Pattern pattern, bool loop = false);

  void stop();

  /** Avanza el patrón. Llamar en cada loop(). */
  void update(uint32_t nowMs);

  bool isPlaying() const { return playing_; }

  /** Patrón en curso (solo tiene sentido si isPlaying()). */
  Pattern currentPattern() const { return pattern_; }

  struct Note {
    uint16_t freqHz;  // 0 = silencio
    uint16_t ms;
  };

 private:

  void startNote(uint32_t nowMs);
  void setTone(uint32_t freqHz);

  uint8_t pin_;
  Pattern pattern_;
  const Note* notes_;
  uint8_t noteCount_;
  uint8_t index_;
  bool loop_;
  bool playing_;
  uint32_t noteStartMs_;
};
