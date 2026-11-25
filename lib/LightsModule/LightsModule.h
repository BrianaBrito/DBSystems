#pragma once
/**
 * @file LightsModule.h
 * @brief Módulo "luces": LEDs de 5 recámaras por 74HC595 + sensor PIR (HC-SR501)
 *        que enciende un LED de presencia.
 *
 * El estado vive en un LightBank protegido por mutex: los manejadores HTTP lo
 * modifican y levantan `dirty_`; solo update() (tarea de loop) escribe en el 74HC595.
 */
#include <Arduino.h>
#include <ArduinoJson.h>
#include <IModule.h>
#include <LightsLogic.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

struct LightsModuleConfig {
  uint8_t dataPin;    // SER
  uint8_t clockPin;   // SRCLK
  uint8_t latchPin;   // RCLK
  uint8_t pirPin;
  uint32_t pirWarmupMs;  // el HC-SR501 dispara solo al encenderse
  uint32_t holdMs;       // la presencia se mantiene tras el último movimiento
  uint32_t pirIntervalMs;
};

class LightsModule : public IModule {
 public:
  explicit LightsModule(const LightsModuleConfig& config);

  const char* name() const override { return "lights"; }
  void begin() override;
  void update(uint32_t nowMs) override;
  void describe(JsonObject out) const override;
  void registerRoutes(ApiServer& api) override;

 private:
  void writeRegister(uint8_t value);

  LightsModuleConfig config_;
  SemaphoreHandle_t mutex_;
  LightBank bank_;          // protegido por mutex_
  PresenceHold presence_;
  bool motion_;             // nivel actual del PIR
  bool pirReady_;
  uint32_t lastPirMs_;
  int16_t lastOut_;         // último byte enviado (-1 = ninguno)

  volatile bool presenceEnabled_;
  volatile bool dirty_;
};
