/**
 * @file main.cpp
 * @brief Punto de entrada: arma los componentes y los mantiene en marcha.
 *
 * Toda la lógica vive en lib/ (un módulo por responsabilidad); aquí solo se
 * conectan. loop() no bloquea: cada componente recibe millis() y decide qué hacer.
 */
#include <Arduino.h>
#include <ESP32Servo.h>
#include <LittleFS.h>

#include <ApiServer.h>
#include <ArduinoJson.h>
#include <BackendClient.h>
#include <Buzzer.h>
#include <GarageModule.h>
#include <GateModule.h>
#include <KitchenModule.h>
#include <LightsModule.h>
#include <Logger.h>
#include <StatusLed.h>
#include <WifiManager.h>

#include "config.h"
#include "secrets.h"

namespace {

const char* const TAG = "main";

WifiManagerConfig makeWifiConfig() {
  WifiManagerConfig c;
  c.apSsid = AP_SSID;
  c.apPassword = AP_PASS;
  c.apChannel = cfg::AP_CHANNEL;
  c.apMaxClients = cfg::AP_MAX_CLIENTS;
  c.staSsid = WIFI_STA_SSID;
  c.staPassword = WIFI_STA_PASS;
  c.reconnectIntervalMs = cfg::STA_RECONNECT_INTERVAL_MS;
  return c;
}

GateModuleConfig makeGateConfig() {
  GateModuleConfig c;
  c.rfidSck = pins::RFID_SCK;
  c.rfidMiso = pins::RFID_MISO;
  c.rfidMosi = pins::RFID_MOSI;
  c.rfidSs = pins::RFID_SS;
  c.rfidRst = pins::RFID_RST;
  c.servoPin = pins::SERVO_GATE;
  c.closedDeg = cfg::GATE_CLOSED_DEG;
  c.openDeg = cfg::GATE_OPEN_DEG;
  c.rfidPollMs = cfg::RFID_POLL_MS;
  c.rfidCooldownMs = cfg::RFID_COOLDOWN_MS;
  c.learnModeMs = cfg::LEARN_MODE_MS;
  c.logic.travelMs = cfg::GATE_TRAVEL_MS;
  c.logic.waitVehicleMs = cfg::GATE_WAIT_VEHICLE_MS;
  c.logic.clearDelayMs = cfg::GATE_CLEAR_DELAY_MS;
  c.sensor.trigPin = pins::GATE_TRIG;
  c.sensor.echoPin = pins::GATE_ECHO;
  c.sensor.maxCm = cfg::SENSOR_MAX_CM;
  c.sensor.detectCm = cfg::GATE_DETECT_CM;
  c.sensor.releaseCm = cfg::GATE_DETECT_CM + cfg::GATE_RELEASE_MARGIN_CM;
  c.sensor.confirmReads = cfg::SENSOR_CONFIRM_READS;
  c.sensor.intervalMs = cfg::SENSOR_INTERVAL_MS;
  return c;
}

KitchenModuleConfig makeKitchenConfig() {
  KitchenModuleConfig c;
  c.dhtPin = pins::DHT_DATA;
  c.gasPin = pins::MQ2_AO;
  c.ledPin = pins::KITCHEN_ALERT_LED;
  c.dhtIntervalMs = cfg::KITCHEN_DHT_MS;
  c.gasIntervalMs = cfg::KITCHEN_GAS_MS;
  c.ledBlinkMs = cfg::KITCHEN_LED_BLINK_MS;
  c.logic.tempMaxC = cfg::KITCHEN_TEMP_MAX_C;
  c.logic.tempHystC = cfg::KITCHEN_TEMP_HYST_C;
  c.logic.gasDelta = cfg::KITCHEN_GAS_DELTA;
  c.logic.gasHyst = cfg::KITCHEN_GAS_HYST;
  c.logic.confirmTemp = cfg::KITCHEN_CONFIRM_TEMP;
  c.logic.confirmGas = cfg::KITCHEN_CONFIRM_GAS;
  c.logic.warmupMs = cfg::KITCHEN_WARMUP_MS;
  c.logic.baselineMs = cfg::KITCHEN_BASELINE_MS;
  return c;
}

GarageModuleConfig makeGarageConfig() {
  GarageModuleConfig c;
  c.rfPin = pins::RF_OPEN;
  c.rfPin2 = pins::RF_CLOSE;
  c.servoPin = pins::SERVO_GARAGE;
  c.closedDeg = cfg::GARAGE_CLOSED_DEG;
  c.openDeg = cfg::GARAGE_OPEN_DEG;
  c.travelMs = cfg::GARAGE_TRAVEL_MS;
  c.debounceMs = cfg::GARAGE_RF_DEBOUNCE_MS;
  c.lockMs = cfg::GARAGE_RF_LOCK_MS;
  c.servoIntervalMs = cfg::GARAGE_SERVO_INTERVAL_MS;
  return c;
}

LightsModuleConfig makeLightsConfig() {
  LightsModuleConfig c;
  c.dataPin = pins::SR_DATA;
  c.clockPin = pins::SR_CLOCK;
  c.latchPin = pins::SR_LATCH;
  c.pirPin = pins::PRESENCE_PIR;
  c.pirWarmupMs = cfg::PIR_WARMUP_MS;
  c.holdMs = cfg::PRESENCE_HOLD_MS;
  c.pirIntervalMs = cfg::PIR_INTERVAL_MS;
  return c;
}

BackendClientConfig makeBackendConfig() {
  BackendClientConfig c;
#ifdef BACKEND_URL
  c.baseUrl = BACKEND_URL;
#else
  c.baseUrl = "";
#endif
  c.token = API_TOKEN;  // mismo secreto compartido que valida el backend
  c.queueMax = cfg::BACKEND_QUEUE_MAX;
  c.timeoutMs = cfg::BACKEND_TIMEOUT_MS;
  c.retryMs = cfg::BACKEND_RETRY_MS;
  return c;
}

// Identifica este arranque: el contador `seq` de la bitácora reinicia en cada reinicio,
// y con boot_id el backend distingue "seq 1 de hoy" de "seq 1 de ayer".
uint32_t bootId = 0;

WifiManager wifi(makeWifiConfig());
ApiServer api(cfg::HTTP_PORT, API_TOKEN, FW_NAME, FW_VERSION, wifi);
StatusLed statusLed(pins::STATUS_LED);
Buzzer buzzer(pins::BUZZER);
GateModule gate(makeGateConfig(), buzzer);
KitchenModule kitchen(makeKitchenConfig(), buzzer);
LightsModule lights(makeLightsConfig());
GarageModule garage(makeGarageConfig());
BackendClient backend(makeBackendConfig());

}  // namespace

void setup() {
  Serial.begin(cfg::SERIAL_BAUD);
  statusLed.begin();
  statusLed.setPattern(StatusLed::BlinkFast);

  Serial.println();
  LOG_I(TAG, "%s v%s iniciando", FW_NAME, FW_VERSION);

  if (strcmp(API_TOKEN, "cambia_este_token") == 0) {
    LOG_W(TAG, "API_TOKEN sigue con el valor de ejemplo: cámbialo en secrets.h");
  }

  if (!LittleFS.begin(true)) {
    LOG_E(TAG, "No se pudo montar LittleFS");
  }

  // Temporizadores PWM para los servos (el buzzer usa el canal 14, aparte).
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);

  buzzer.begin();

  bootId = esp_random();
  backend.begin();
  gate.setAccessListener([](const AccessEvent& e) {
    JsonDocument doc;
    doc["device"] = cfg::DEVICE_ID;
    doc["boot"] = bootId;
    doc["seq"] = e.seq;
    doc["uptime_s"] = e.uptimeS;
    doc["uid"] = e.uid;
    doc["result"] = e.result;
    String body;
    serializeJson(doc, body);
    backend.enqueue("/api/events.php", body);
  });

  // Los módulos se registran y arrancan ANTES de api.begin().
  api.addModule(&gate);
  gate.begin();
  api.addModule(&kitchen);
  kitchen.begin();
  api.addModule(&lights);
  lights.begin();
  api.addModule(&garage);
  garage.begin();

  wifi.begin();
  api.begin();
}

void loop() {
  const uint32_t now = millis();

  wifi.update(now);
  buzzer.update(now);
  gate.update(now);
  kitchen.update(now);
  lights.update(now);
  garage.update(now);

  if (!wifi.isStaConfigured()) {
    statusLed.setPattern(StatusLed::BlinkSlow);  // solo AP
  } else {
    statusLed.setPattern(wifi.isStaConnected() ? StatusLed::On : StatusLed::BlinkFast);
  }
  statusLed.update(now);
}
