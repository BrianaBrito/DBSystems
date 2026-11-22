#pragma once
/**
 * @file StatusLed.h
 * @brief LED de estado con patrones de parpadeo no bloqueantes.
 */
#include <Arduino.h>

class StatusLed {
 public:
  enum Pattern : uint8_t { Off, On, BlinkSlow, BlinkFast };

  explicit StatusLed(uint8_t pin, bool activeHigh = true)
      : pin_(pin), activeHigh_(activeHigh), pattern_(Off), lastToggleMs_(0), level_(false) {}

  void begin() {
    pinMode(pin_, OUTPUT);
    write(false);
  }

  void setPattern(Pattern pattern) { pattern_ = pattern; }

  void update(uint32_t nowMs) {
    switch (pattern_) {
      case Off:
        write(false);
        break;
      case On:
        write(true);
        break;
      case BlinkSlow:
        blink(nowMs, 1000);
        break;
      case BlinkFast:
        blink(nowMs, 200);
        break;
    }
  }

 private:
  void blink(uint32_t nowMs, uint32_t halfPeriodMs) {
    if (nowMs - lastToggleMs_ >= halfPeriodMs) {
      lastToggleMs_ = nowMs;
      write(!level_);
    }
  }

  void write(bool on) {
    level_ = on;
    digitalWrite(pin_, (on == activeHigh_) ? HIGH : LOW);
  }

  uint8_t pin_;
  bool activeHigh_;
  Pattern pattern_;
  uint32_t lastToggleMs_;
  bool level_;
};
