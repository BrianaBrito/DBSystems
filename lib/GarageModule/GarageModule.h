#pragma once
/**
 * @file GarageModule.h
 * @brief Módulo "cochera": portón de una hoja con un servo en la bisagra, controlado
 *        por un control remoto 433 MHz (receptor RX480E) y por la API.
 *
 * El estado y el tiempo de recorrido viven en GarageLogic (pura, con pruebas).
 * Los ángulos de cerrado y abierto se calibran por API y se guardan en NVS.
 * Los manejadores HTTP solo levantan banderas; update() las ejecuta.
 */
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>
#include <GarageLogic.h>
#include <IModule.h>

struct GarageModuleConfig {
  uint8_t rfPin;         // RX480E D0 (botón A del control): alterna abrir/cerrar
  uint8_t rfPin2;        // RX480E D1 (botón B del control): también alterna
  uint8_t servoPin;
  uint8_t closedDeg, openDeg;
  uint32_t travelMs;     // tiempo que tarda en abrir o cerrar
  uint32_t debounceMs;   // un cambio de nivel del receptor debe mantenerse este tiempo
  uint32_t lockMs;       // tiempo muerto entre pulsaciones del control
  uint32_t servoIntervalMs;
};

class GarageModule : public IModule {
 public:
  explicit GarageModule(const GarageModuleConfig& config);

  const char* name() const override { return "garage"; }
  void begin() override;
  void update(uint32_t nowMs) override;
  void describe(JsonObject out) const override;
  void registerRoutes(ApiServer& api) override;

 private:
  void writeServo();

  GarageModuleConfig config_;
  Servo servo_;
  GarageLogic logic_;
  RemoteToggle button_;
  RemoteToggle button2_;

  int deg_;
  uint32_t lastServoMs_;
  const char* lastSource_;  // "remote", "api" o "none"

  // Calibración (la modifica el servidor HTTP; update() la guarda y la aplica)
  volatile uint8_t closed_, open_;
  volatile bool cmdOpen_;
  volatile bool cmdClose_;
  volatile bool calibDirty_;
};
