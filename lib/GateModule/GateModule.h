#pragma once
/**
 * @file GateModule.h
 * @brief Módulo "caseta": lector RFID RC522 + servo + HC-SR04 + bitácora de accesos.
 *
 * Une el hardware con la máquina de estados pura (GateLogic). Los manejadores
 * HTTP solo levantan banderas; update() las ejecuta en la tarea de loop().
 * Las listas compartidas (tarjetas, bitácora) se protegen con un mutex.
 */
#include <Arduino.h>
#include <ArduinoJson.h>
#include <DistanceSensor.h>
#include <ESP32Servo.h>
#include <GateLogic.h>
#include <IModule.h>
#include <MFRC522.h>
#include <Buzzer.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <functional>
#include <string>
#include <vector>

struct GateModuleConfig {
  uint8_t rfidSck, rfidMiso, rfidMosi, rfidSs, rfidRst;
  uint8_t servoPin;
  uint8_t closedDeg;
  uint8_t openDeg;
  uint32_t rfidPollMs;
  uint32_t rfidCooldownMs;
  uint32_t learnModeMs;
  GateLogicConfig logic;
  DistanceSensorConfig sensor;
};

struct AccessEvent {
  uint32_t seq;      // número consecutivo (sirve para que el backend no duplique)
  uint32_t uptimeS;  // segundos desde el arranque (el backend asigna la hora real)
  String uid;
  String result;  // granted, denied, learned, already_registered, card_list_full, manual_open
};

class GateModule : public IModule {
 public:
  typedef std::function<void(const AccessEvent&)> AccessListener;

  GateModule(const GateModuleConfig& config, Buzzer& buzzer);

  const char* name() const override { return "gate"; }
  void begin() override;
  void update(uint32_t nowMs) override;
  void describe(JsonObject out) const override;
  void registerRoutes(ApiServer& api) override;

  /** Se invoca (en loop) por cada evento de acceso; úsalo para enviarlo al backend. */
  void setAccessListener(AccessListener listener) { listener_ = listener; }

 private:
  static String normalizeUid(const String& raw);  // "" si no es válido
  static String uidToString(const MFRC522::Uid& uid);

  void loadCards();
  void saveCardsLocked();
  bool hasCardLocked(const String& uid) const;
  void logEvent(uint32_t nowMs, const String& uid, const char* result);
  void pollRfid(uint32_t nowMs);
  void handleCard(uint32_t nowMs, const String& uid);
  void stepServo(uint32_t nowMs);
  void writeServo(int deg);

  GateModuleConfig config_;
  Buzzer& buzzer_;
  MFRC522 rfid_;
  Servo servo_;
  DistanceSensor sensor_;
  GateLogic logic_;
  AccessListener listener_;

  SemaphoreHandle_t mutex_;
  std::vector<String> cards_;      // protegido por mutex_
  std::vector<AccessEvent> log_;   // ring buffer, protegido por mutex_
  std::vector<AccessEvent> outbox_;  // eventos pendientes de avisar al listener
  uint32_t seq_;

  // Órdenes pendientes puestas por el servidor HTTP
  volatile bool cmdOpen_;
  volatile bool cmdClose_;
  volatile bool cmdLearn_;

  bool rfidOk_;
  bool learnMode_;
  uint32_t learnStartMs_;
  uint32_t lastPollMs_;
  uint32_t lastRfidRetryMs_;
  uint32_t lastCardMs_;
  String lastCardUid_;
  GateState lastState_;

  int servoDeg_;
  int servoTarget_;
  uint32_t lastServoStepMs_;
};
