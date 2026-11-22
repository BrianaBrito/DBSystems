#pragma once
/**
 * @file GateLogic.h
 * @brief Máquina de estados de la caseta, sin dependencias de hardware.
 *
 * Está separada del hardware a propósito: así se puede probar en la PC con
 * pruebas unitarias (ver test/test_logic) sin necesidad del ESP32.
 *
 *   Closed ──requestOpen──► Opening ──(travelMs)──► WaitingVehicle
 *   WaitingVehicle ──vehículo──► VehiclePassing
 *   WaitingVehicle ──(waitVehicleMs sin vehículo)──► Closing   [Timeout]
 *   VehiclePassing ──(sensor libre clearDelayMs)──► Closing    [VehiclePassed]
 *   Closing ──vehículo detectado──► Opening                    [seguridad]
 *   Closing ──(travelMs)──► Closed
 */
#include <stdint.h>

enum class GateState : uint8_t { Closed, Opening, WaitingVehicle, VehiclePassing, Closing };

enum class GateCloseReason : uint8_t { None, VehiclePassed, Timeout, Manual };

struct GateLogicConfig {
  uint32_t travelMs;      // tiempo que tarda la pluma en subir o bajar
  uint32_t waitVehicleMs; // espera máxima al vehículo después de abrir
  uint32_t clearDelayMs;  // tiempo con el sensor libre para dar por pasado el vehículo
};

class GateLogic {
 public:
  explicit GateLogic(const GateLogicConfig& config);

  /** Tarjeta autorizada o apertura manual. Si ya estaba esperando, reinicia la espera. */
  void requestOpen(uint32_t nowMs);

  /** Cierre manual. Solo se acepta mientras espera al vehículo: nunca se cierra sobre uno. */
  void requestClose(uint32_t nowMs);

  /** Avanza la máquina. `vehiclePresent` ya debe venir filtrado (sin rebotes). */
  void update(uint32_t nowMs, bool vehiclePresent);

  GateState state() const { return state_; }
  GateCloseReason lastCloseReason() const { return closeReason_; }
  uint32_t obstacleReopens() const { return obstacleReopens_; }

  /** true si la pluma debe estar arriba (o subiendo). */
  bool servoShouldBeOpen() const;

  static const char* stateName(GateState state);
  static const char* reasonName(GateCloseReason reason);

 private:
  void enter(GateState next, uint32_t nowMs);

  GateLogicConfig config_;
  GateState state_;
  GateCloseReason closeReason_;
  uint32_t stateSinceMs_;
  uint32_t clearSinceMs_;
  bool clearing_;
  uint32_t obstacleReopens_;
};
