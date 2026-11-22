#include "GateLogic.h"

GateLogic::GateLogic(const GateLogicConfig& config)
    : config_(config),
      state_(GateState::Closed),
      closeReason_(GateCloseReason::None),
      stateSinceMs_(0),
      clearSinceMs_(0),
      clearing_(false),
      obstacleReopens_(0) {}

void GateLogic::enter(GateState next, uint32_t nowMs) {
  state_ = next;
  stateSinceMs_ = nowMs;
  clearing_ = false;
  if (next == GateState::Opening) {
    closeReason_ = GateCloseReason::None;
  }
}

void GateLogic::requestOpen(uint32_t nowMs) {
  switch (state_) {
    case GateState::Closed:
    case GateState::Closing:  // cerrando: se vuelve a abrir
      enter(GateState::Opening, nowMs);
      break;
    case GateState::WaitingVehicle:
      stateSinceMs_ = nowMs;  // otra tarjeta: reinicia el tiempo de espera
      break;
    case GateState::Opening:
    case GateState::VehiclePassing:
      break;  // ya está abierta o abriéndose
  }
}

void GateLogic::requestClose(uint32_t nowMs) {
  if (state_ == GateState::WaitingVehicle) {
    closeReason_ = GateCloseReason::Manual;
    enter(GateState::Closing, nowMs);
  }
  // En cualquier otro estado se ignora: no se cierra sobre un vehículo ni se interrumpe un movimiento.
}

void GateLogic::update(uint32_t nowMs, bool vehiclePresent) {
  // La resta sin signo es correcta aunque millis() dé la vuelta (cada ~49 días).
  const uint32_t elapsed = nowMs - stateSinceMs_;

  switch (state_) {
    case GateState::Closed:
      break;

    case GateState::Opening:
      if (elapsed >= config_.travelMs) {
        enter(GateState::WaitingVehicle, nowMs);
      }
      break;

    case GateState::WaitingVehicle:
      if (vehiclePresent) {
        enter(GateState::VehiclePassing, nowMs);
      } else if (elapsed >= config_.waitVehicleMs) {
        closeReason_ = GateCloseReason::Timeout;
        enter(GateState::Closing, nowMs);
      }
      break;

    case GateState::VehiclePassing:
      if (vehiclePresent) {
        clearing_ = false;  // el vehículo sigue ahí: se reinicia la cuenta
      } else if (!clearing_) {
        clearing_ = true;
        clearSinceMs_ = nowMs;
      } else if (nowMs - clearSinceMs_ >= config_.clearDelayMs) {
        closeReason_ = GateCloseReason::VehiclePassed;
        enter(GateState::Closing, nowMs);
      }
      break;

    case GateState::Closing:
      if (vehiclePresent) {
        obstacleReopens_++;  // obstáculo mientras cierra: se vuelve a abrir
        enter(GateState::Opening, nowMs);
      } else if (elapsed >= config_.travelMs) {
        enter(GateState::Closed, nowMs);
      }
      break;
  }
}

bool GateLogic::servoShouldBeOpen() const {
  return state_ == GateState::Opening || state_ == GateState::WaitingVehicle ||
         state_ == GateState::VehiclePassing;
}

const char* GateLogic::stateName(GateState state) {
  switch (state) {
    case GateState::Closed:
      return "closed";
    case GateState::Opening:
      return "opening";
    case GateState::WaitingVehicle:
      return "waiting_vehicle";
    case GateState::VehiclePassing:
      return "vehicle_passing";
    case GateState::Closing:
      return "closing";
  }
  return "unknown";
}

const char* GateLogic::reasonName(GateCloseReason reason) {
  switch (reason) {
    case GateCloseReason::None:
      return "none";
    case GateCloseReason::VehiclePassed:
      return "vehicle_passed";
    case GateCloseReason::Timeout:
      return "timeout";
    case GateCloseReason::Manual:
      return "manual";
  }
  return "unknown";
}
