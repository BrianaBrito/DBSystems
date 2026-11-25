#pragma once
/**
 * @file LightsLogic.h
 * @brief Lógica pura (sin hardware) de las luces: estado de las recámaras y
 *        LED de presencia con tiempo de retención. Se prueba en la PC.
 *
 * Salidas del 74HC595: Q0-Q4 = recámaras 1-5, Q5 = LED de presencia.
 */
#include <stdint.h>

/** Estado de las recámaras y byte que se envía al 74HC595. */
class LightBank {
 public:
  static const uint8_t kRooms = 5;
  static const uint8_t kPresenceBit = 5;

  LightBank() : rooms_(0), presence_(false) {}

  /** Recámara 1..kRooms. Devuelve false si el número no es válido. */
  bool setRoom(uint8_t room, bool on) {
    if (room < 1 || room > kRooms) return false;
    const uint8_t bit = static_cast<uint8_t>(1u << (room - 1));
    if (on) rooms_ |= bit;
    else rooms_ &= static_cast<uint8_t>(~bit);
    return true;
  }

  bool toggleRoom(uint8_t room) {
    if (room < 1 || room > kRooms) return false;
    rooms_ ^= static_cast<uint8_t>(1u << (room - 1));
    return true;
  }

  void setAll(bool on) { rooms_ = on ? kAllMask : 0; }

  bool room(uint8_t room) const {
    if (room < 1 || room > kRooms) return false;
    return (rooms_ >> (room - 1)) & 1u;
  }

  void setPresence(bool on) { presence_ = on; }
  bool presence() const { return presence_; }
  uint8_t roomsMask() const { return rooms_; }

  /** Byte para el registro de desplazamiento. */
  uint8_t output() const {
    return static_cast<uint8_t>(rooms_ | (presence_ ? (1u << kPresenceBit) : 0u));
  }

 private:
  static const uint8_t kAllMask = 0x1F;
  uint8_t rooms_;
  bool presence_;
};

/**
 * Convierte la salida del PIR en "hay presencia": ignora el sensor mientras
 * calienta (el HC-SR501 dispara solo al encenderse) y mantiene la presencia
 * `holdMs` después del último movimiento para que la luz no parpadee.
 */
class PresenceHold {
 public:
  PresenceHold(uint32_t warmupMs, uint32_t holdMs)
      : warmupMs_(warmupMs), holdMs_(holdMs), startMs_(0), lastMotionMs_(0), active_(false) {}

  void reset(uint32_t nowMs) {
    startMs_ = nowMs;
    active_ = false;
  }

  /** `motion`: nivel actual de la salida del PIR. */
  void update(uint32_t nowMs, bool motion) {
    if (!warmedUp(nowMs)) {
      active_ = false;
      return;
    }
    if (motion) {
      lastMotionMs_ = nowMs;
      active_ = true;
    } else if (active_ && nowMs - lastMotionMs_ >= holdMs_) {  // seguro ante el desbordamiento
      active_ = false;
    }
  }

  bool active() const { return active_; }
  bool warmedUp(uint32_t nowMs) const { return nowMs - startMs_ >= warmupMs_; }

 private:
  uint32_t warmupMs_;
  uint32_t holdMs_;
  uint32_t startMs_;
  uint32_t lastMotionMs_;
  bool active_;
};
