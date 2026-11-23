#pragma once
/**
 * @file KitchenLogic.h
 * @brief Lógica pura (sin hardware) de las alertas de la cocina: temperatura alta y gas.
 *
 * Separada de KitchenModule para poder probarla en la PC (pio test -e native).
 * No incluye Arduino.h: recibe el tiempo y las lecturas como parámetros.
 *
 * Gas (MQ-2): el valor del sensor cambia con la temperatura, la humedad y el tiempo
 * encendido, así que no se usa un umbral fijo. Tras un calentamiento se mide una
 * *línea base* en aire normal y se alerta cuando la lectura filtrada supera
 * `base + gasDelta`. Se despeja al bajar de `base + gasDelta - gasHyst` (histéresis).
 *
 * Temperatura (DHT11 / KY-015): alerta al superar `tempMaxC`; se despeja al bajar de
 * `tempMaxC - tempHystC`. Una lectura fallida (NaN) no cambia el estado.
 *
 * En ambos casos hacen falta varias lecturas seguidas para cambiar de estado
 * (antirrebote), de modo que un pico aislado no dispare la alarma.
 */
#include <stdint.h>

struct KitchenLogicConfig {
  float tempMaxC;        // alerta al superar esta temperatura
  float tempHystC;       // histéresis de temperatura (> 0)
  uint16_t gasDelta;     // cuentas ADC sobre la línea base para alertar
  uint16_t gasHyst;      // histéresis de gas (< gasDelta)
  uint8_t confirmTemp;   // lecturas de temperatura seguidas para cambiar de estado
  uint8_t confirmGas;    // lecturas de gas seguidas para cambiar de estado
  uint32_t warmupMs;     // calentamiento: las lecturas se ignoran
  uint32_t baselineMs;   // ventana en la que se promedia la línea base
};

class KitchenLogic {
 public:
  enum Phase : uint8_t { WarmingUp, Calibrating, Ready };

  explicit KitchenLogic(const KitchenLogicConfig& config);

  /** Reinicia todo: vuelve a calentar y a medir la línea base. */
  void reset(uint32_t nowMs);

  /** Vuelve a medir la línea base (sin repetir el calentamiento). */
  void recalibrate(uint32_t nowMs);

  /** Lectura de temperatura en °C; NaN si el sensor falló. */
  void addTemperature(float celsius);

  /** Lectura cruda del ADC (0-4095). */
  void addGas(uint32_t nowMs, uint16_t raw);

  void setTempMax(float celsius) { config_.tempMaxC = celsius; }
  void setGasDelta(uint16_t delta);

  Phase phase() const { return phase_; }
  bool tempAlert() const { return tempAlert_; }
  bool gasAlert() const { return gasAlert_; }
  bool alerting() const { return tempAlert_ || gasAlert_; }

  /** false tras varias lecturas de temperatura fallidas seguidas. */
  bool tempValid() const { return tempFailures_ < kMaxTempFailures; }

  float temperature() const { return temperature_; }
  uint16_t gasFiltered() const { return static_cast<uint16_t>(gasEma_); }
  uint16_t gasBaseline() const { return baseline_; }
  uint16_t gasThreshold() const { return static_cast<uint16_t>(baseline_ + config_.gasDelta); }
  const KitchenLogicConfig& config() const { return config_; }

  static const char* phaseName(Phase phase);

 private:
  static const uint8_t kMaxTempFailures = 5;

  void updatePhase(uint32_t nowMs);

  KitchenLogicConfig config_;
  Phase phase_;
  uint32_t phaseStartMs_;   // inicio de la fase actual
  uint32_t baseSum_;
  uint32_t baseCount_;
  uint16_t baseline_;
  int32_t gasEma_;
  bool gasEmaInit_;
  uint8_t gasHits_;
  bool gasAlert_;

  float temperature_;
  bool tempSeen_;
  uint8_t tempFailures_;
  uint8_t tempHits_;
  bool tempAlert_;
};
