#include "KitchenLogic.h"

KitchenLogic::KitchenLogic(const KitchenLogicConfig& config)
    : config_(config),
      phase_(WarmingUp),
      phaseStartMs_(0),
      baseSum_(0),
      baseCount_(0),
      baseline_(0),
      gasEma_(0),
      gasEmaInit_(false),
      gasHits_(0),
      gasAlert_(false),
      temperature_(0.0f),
      tempSeen_(false),
      tempFailures_(0),
      tempHits_(0),
      tempAlert_(false) {}

void KitchenLogic::reset(uint32_t nowMs) {
  phase_ = WarmingUp;
  phaseStartMs_ = nowMs;
  baseSum_ = 0;
  baseCount_ = 0;
  baseline_ = 0;
  gasEmaInit_ = false;
  gasHits_ = 0;
  gasAlert_ = false;
}

void KitchenLogic::recalibrate(uint32_t nowMs) {
  phase_ = Calibrating;
  phaseStartMs_ = nowMs;
  baseSum_ = 0;
  baseCount_ = 0;
  gasHits_ = 0;
  gasAlert_ = false;
}

void KitchenLogic::setGasDelta(uint16_t delta) {
  config_.gasDelta = delta;
  if (config_.gasHyst >= delta) config_.gasHyst = delta / 2;  // la histéresis debe ser menor
}

const char* KitchenLogic::phaseName(Phase phase) {
  switch (phase) {
    case WarmingUp: return "warming_up";
    case Calibrating: return "calibrating";
    case Ready: return "ready";
  }
  return "unknown";
}

// ---------------------------------------------------------------- temperatura

void KitchenLogic::addTemperature(float celsius) {
  if (celsius != celsius) {  // NaN: lectura fallida
    if (tempFailures_ < 255) tempFailures_++;
    return;
  }
  tempFailures_ = 0;
  temperature_ = celsius;
  tempSeen_ = true;

  if (!tempAlert_) {
    if (celsius >= config_.tempMaxC) {
      if (++tempHits_ >= config_.confirmTemp) {
        tempAlert_ = true;
        tempHits_ = 0;
      }
    } else {
      tempHits_ = 0;
    }
  } else {
    if (celsius < config_.tempMaxC - config_.tempHystC) {
      if (++tempHits_ >= config_.confirmTemp) {
        tempAlert_ = false;
        tempHits_ = 0;
      }
    } else {
      tempHits_ = 0;
    }
  }
}

// ------------------------------------------------------------------------ gas

void KitchenLogic::updatePhase(uint32_t nowMs) {
  const uint32_t elapsed = nowMs - phaseStartMs_;  // seguro ante el desbordamiento de millis()

  if (phase_ == WarmingUp && elapsed >= config_.warmupMs) {
    phase_ = Calibrating;
    phaseStartMs_ = nowMs;
    baseSum_ = 0;
    baseCount_ = 0;
  } else if (phase_ == Calibrating && elapsed >= config_.baselineMs) {
    // Sin muestras (no debería pasar) se usa el valor filtrado actual.
    baseline_ = baseCount_ > 0 ? static_cast<uint16_t>(baseSum_ / baseCount_)
                               : static_cast<uint16_t>(gasEma_);
    phase_ = Ready;
    gasHits_ = 0;
  }
}

void KitchenLogic::addGas(uint32_t nowMs, uint16_t raw) {
  // Filtro exponencial (1/4) para suavizar el ruido del ADC.
  if (!gasEmaInit_) {
    gasEma_ = raw;
    gasEmaInit_ = true;
  } else {
    gasEma_ += (static_cast<int32_t>(raw) - gasEma_) / 4;
  }

  updatePhase(nowMs);

  if (phase_ == Calibrating) {
    baseSum_ += raw;
    baseCount_++;
    return;
  }
  if (phase_ != Ready) return;

  const int32_t threshold = static_cast<int32_t>(baseline_) + config_.gasDelta;
  const int32_t clearAt = threshold - config_.gasHyst;

  if (!gasAlert_) {
    if (gasEma_ >= threshold) {
      if (++gasHits_ >= config_.confirmGas) {
        gasAlert_ = true;
        gasHits_ = 0;
      }
    } else {
      gasHits_ = 0;
    }
  } else {
    if (gasEma_ < clearAt) {
      if (++gasHits_ >= config_.confirmGas) {
        gasAlert_ = false;
        gasHits_ = 0;
      }
    } else {
      gasHits_ = 0;
    }
  }
}
