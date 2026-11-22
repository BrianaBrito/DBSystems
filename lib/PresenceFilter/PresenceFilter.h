#pragma once
/**
 * @file PresenceFilter.h
 * @brief Filtro de presencia para sensores de distancia: confirma varias
 *        lecturas seguidas y usa histéresis para evitar parpadeos.
 *
 * Sin dependencias de hardware (se prueba en la PC).
 *
 *  - "Presente": `confirmReads` lecturas seguidas a <= detectCm.
 *  - "Ausente":  `confirmReads` lecturas seguidas a > releaseCm o sin eco (-1).
 *  - Entre detectCm y releaseCm se mantiene el estado actual (histéresis).
 */
#include <stdint.h>

class PresenceFilter {
 public:
  PresenceFilter(int16_t detectCm, int16_t releaseCm, uint8_t confirmReads)
      : detectCm_(detectCm),
        releaseCm_(releaseCm),
        confirmReads_(confirmReads < 1 ? 1 : confirmReads),
        present_(false),
        streak_(0) {}

  /** Agrega una lectura en cm; -1 significa "sin eco". */
  void addReading(int16_t cm) {
    const bool near = (cm >= 0 && cm <= detectCm_);
    const bool far = (cm < 0 || cm > releaseCm_);
    // Cuenta solo la lectura que contradice el estado actual.
    const bool contradicts = present_ ? far : near;

    if (!contradicts) {
      streak_ = 0;
      return;
    }
    if (++streak_ >= confirmReads_) {
      present_ = !present_;
      streak_ = 0;
    }
  }

  bool present() const { return present_; }

 private:
  int16_t detectCm_;
  int16_t releaseCm_;
  uint8_t confirmReads_;
  bool present_;
  uint8_t streak_;
};
