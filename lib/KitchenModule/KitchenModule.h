#pragma once
/**
 * @file KitchenModule.h
 * @brief Módulo "cocina": DHT11 en módulo KY-015 (temperatura/humedad) + MQ-2 (gas) -> LED de alerta y buzzer.
 *
 * La decisión de alertar vive en KitchenLogic (pura, con pruebas unitarias).
 * Aquí solo se leen los sensores, se mueven LED y buzzer y se atienden las órdenes
 * HTTP. Los manejadores HTTP únicamente levantan banderas / valores pendientes;
 * update() los aplica en la tarea de loop().
 */
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Buzzer.h>
#include <DHT.h>
#include <IModule.h>
#include <KitchenLogic.h>

struct KitchenModuleConfig {
  uint8_t dhtPin;
  uint8_t gasPin;
  uint8_t ledPin;
  uint32_t dhtIntervalMs;   // el DHT11 admite una lectura por segundo; se usa cada 2 s
  uint32_t gasIntervalMs;
  uint32_t ledBlinkMs;
  KitchenLogicConfig logic;
};

class KitchenModule : public IModule {
 public:
  KitchenModule(const KitchenModuleConfig& config, Buzzer& buzzer);

  const char* name() const override { return "kitchen"; }
  void begin() override;
  void update(uint32_t nowMs) override;
  void describe(JsonObject out) const override;
  void registerRoutes(ApiServer& api) override;

 private:
  void saveConfig();
  void applyPending(uint32_t nowMs);
  void driveOutputs(uint32_t nowMs);

  KitchenModuleConfig config_;
  Buzzer& buzzer_;
  DHT dht_;
  KitchenLogic logic_;

  float humidity_;
  uint16_t gasRaw_;
  bool dhtOk_;
  bool silenced_;
  bool ledOn_;
  uint32_t lastDhtMs_;
  uint32_t lastGasMs_;
  uint32_t lastBlinkMs_;

  // Órdenes puestas por el servidor HTTP (se aplican en update())
  volatile bool cmdSilence_;
  volatile bool cmdCalibrate_;
  volatile bool cmdConfig_;
  volatile float pendingTempMax_;
  volatile int32_t pendingGasDelta_;
};
