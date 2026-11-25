#pragma once
/**
 * @file GarageLogic.h
 * @brief Lógica pura (sin hardware) del portón de una hoja: estados, posición
 *        y antirrebote del control remoto. Se prueba en la PC.
 *
 * La posición va de 0.0 (cerrado) a 1.0 (abierto) y avanza con el tiempo, así el
 * módulo calcula el ángulo de cada servo por interpolación y el movimiento es
 * suave. Si llega una orden contraria a mitad de recorrido, invierte desde donde está.
 */
#include <stdint.h>

class GarageLogic {
 public:
  enum State : uint8_t { Closed, Opening, Open, Closing };

  explicit GarageLogic(uint32_t travelMs)
      : travelMs_(travelMs < 1 ? 1 : travelMs), state_(Closed), pos_(0.0f), lastMs_(0) {}

  void begin(uint32_t nowMs) { lastMs_ = nowMs; }

  /** Devuelve true si la orden cambió algo. */
  bool requestOpen() {
    if (state_ == Closed || state_ == Closing) {
      state_ = Opening;
      return true;
    }
    return false;
  }

  bool requestClose() {
    if (state_ == Open || state_ == Opening) {
      state_ = Closing;
      return true;
    }
    return false;
  }

  /** Un solo botón: abre si está cerrado o cerrando, cierra si está abierto o abriendo. */
  bool toggle() {
    if (state_ == Open || state_ == Opening) return requestClose();
    return requestOpen();
  }

  void update(uint32_t nowMs) {
    const uint32_t dt = nowMs - lastMs_;  // seguro ante el desbordamiento de millis()
    lastMs_ = nowMs;
    const float step = static_cast<float>(dt) / static_cast<float>(travelMs_);
    if (state_ == Opening) {
      pos_ += step;
      if (pos_ >= 1.0f) {
        pos_ = 1.0f;
        state_ = Open;
      }
    } else if (state_ == Closing) {
      pos_ -= step;
      if (pos_ <= 0.0f) {
        pos_ = 0.0f;
        state_ = Closed;
      }
    }
  }

  State state() const { return state_; }
  float position() const { return pos_; }
  bool moving() const { return state_ == Opening || state_ == Closing; }

  /** Ángulo entre `from` (cerrado) y `to` (abierto) según la posición. Sirve en ambos sentidos. */
  static int angleFor(float pos, int from, int to) {
    return from + static_cast<int>((to - from) * pos + (to >= from ? 0.5f : -0.5f));
  }

  static const char* stateName(State s) {
    switch (s) {
      case Closed: return "closed";
      case Opening: return "opening";
      case Open: return "open";
      case Closing: return "closing";
    }
    return "unknown";
  }

 private:
  uint32_t travelMs_;
  State state_;
  float pos_;
  uint32_t lastMs_;
};

/**
 * Botón del control remoto en modo ANCLAJE del RX480E: cada pulsación cambia la
 * salida entre 0 y 1. Aquí se detecta ese CAMBIO (en cualquier sentido), así que
 * funciona igual aunque el portón se haya movido por la API y el nivel ya no
 * coincida con el estado real.
 *
 * El primer nivel leído al arrancar es la referencia (no cuenta como pulsación).
 * Un cambio debe mantenerse `debounceMs` para confirmarse, y entre dos pulsaciones
 * pasan al menos `lockMs`; un cambio que llega antes se aplica al vencer ese tiempo.
 */
class RemoteToggle {
 public:
  RemoteToggle(uint32_t debounceMs, uint32_t lockMs)
      : debounceMs_(debounceMs), lockMs_(lockMs), candidateSinceMs_(0), lastFireMs_(0),
        stable_(false), candidate_(false), init_(false), everFired_(false) {}

  /** Devuelve true una vez por cada pulsación confirmada. */
  bool update(uint32_t nowMs, bool level) {
    if (!init_) {
      init_ = true;
      stable_ = candidate_ = level;
      return false;
    }
    if (level == stable_) {
      candidate_ = stable_;  // volvió al nivel anterior: era ruido
      return false;
    }
    if (level != candidate_) {
      candidate_ = level;
      candidateSinceMs_ = nowMs;
    }
    if (nowMs - candidateSinceMs_ >= debounceMs_ && (!everFired_ || nowMs - lastFireMs_ >= lockMs_)) {
      stable_ = level;
      lastFireMs_ = nowMs;
      everFired_ = true;
      return true;
    }
    return false;
  }

 private:
  uint32_t debounceMs_;
  uint32_t lockMs_;
  uint32_t candidateSinceMs_;
  uint32_t lastFireMs_;
  bool stable_;
  bool candidate_;
  bool init_;
  bool everFired_;
};
