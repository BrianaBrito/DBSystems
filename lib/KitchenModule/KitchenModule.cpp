#include "KitchenModule.h"

#include <ApiServer.h>
#include <Logger.h>
#include <Preferences.h>
#include <math.h>

namespace {

const char* const TAG = "kitchen";
Preferences prefs;

}  // namespace

KitchenModule::KitchenModule(const KitchenModuleConfig& config, Buzzer& buzzer)
    : config_(config),
      buzzer_(buzzer),
      dht_(config.dhtPin, DHT11),
      logic_(config.logic),
      humidity_(NAN),
      gasRaw_(0),
      dhtOk_(false),
      silenced_(false),
      ledOn_(false),
      lastDhtMs_(0),
      lastGasMs_(0),
      lastBlinkMs_(0),
      cmdSilence_(false),
      cmdCalibrate_(false),
      cmdConfig_(false),
      pendingTempMax_(0.0f),
      pendingGasDelta_(-1) {}

void KitchenModule::begin() {
  pinMode(config_.ledPin, OUTPUT);
  digitalWrite(config_.ledPin, LOW);
  pinMode(config_.gasPin, INPUT);
  analogSetPinAttenuation(config_.gasPin, ADC_11db);  // rango ~0-3.3 V
  dht_.begin();

  // Umbrales calibrados desde la API (si existen)
  prefs.begin("kitchen", true);
  const float tempMax = prefs.getFloat("temp_max", config_.logic.tempMaxC);
  const uint16_t gasDelta = prefs.getUShort("gas_delta", config_.logic.gasDelta);
  prefs.end();
  logic_.setTempMax(tempMax);
  logic_.setGasDelta(gasDelta);

  logic_.reset(millis());
  LOG_I(TAG, "cocina lista (calentando MQ-2 %lu ms)", static_cast<unsigned long>(config_.logic.warmupMs));
}

void KitchenModule::saveConfig() {
  prefs.begin("kitchen", false);
  prefs.putFloat("temp_max", logic_.config().tempMaxC);
  prefs.putUShort("gas_delta", logic_.config().gasDelta);
  prefs.end();
}

void KitchenModule::applyPending(uint32_t nowMs) {
  if (cmdCalibrate_) {
    cmdCalibrate_ = false;
    logic_.recalibrate(nowMs);
    silenced_ = false;
    LOG_I(TAG, "recalibrando linea base de gas");
  }
  if (cmdSilence_) {
    cmdSilence_ = false;
    if (logic_.alerting()) silenced_ = true;
  }
  if (cmdConfig_) {
    cmdConfig_ = false;
    const float t = pendingTempMax_;
    const int32_t g = pendingGasDelta_;
    if (t > 0.0f) logic_.setTempMax(t);
    if (g > 0) logic_.setGasDelta(static_cast<uint16_t>(g));
    pendingTempMax_ = 0.0f;
    pendingGasDelta_ = -1;
    saveConfig();
  }
}

void KitchenModule::driveOutputs(uint32_t nowMs) {
  const bool alerting = logic_.alerting();
  if (!alerting) silenced_ = false;  // la silencio solo vale para la alerta en curso

  // LED: parpadea en alerta (aunque se silencie el buzzer), apagado si no.
  if (alerting) {
    if (nowMs - lastBlinkMs_ >= config_.ledBlinkMs) {
      lastBlinkMs_ = nowMs;
      ledOn_ = !ledOn_;
      digitalWrite(config_.ledPin, ledOn_ ? HIGH : LOW);
    }
  } else if (ledOn_) {
    ledOn_ = false;
    digitalWrite(config_.ledPin, LOW);
  }

  // Buzzer: alarma en bucle. Si un sonido de la caseta la interrumpe, se retoma al terminar.
  const bool wantAlarm = alerting && !silenced_;
  const bool alarmPlaying = buzzer_.isPlaying() && buzzer_.currentPattern() == Buzzer::Alert;
  if (wantAlarm && !buzzer_.isPlaying()) {
    buzzer_.play(Buzzer::Alert, true);
  } else if (!wantAlarm && alarmPlaying) {
    buzzer_.stop();
  }
}

void KitchenModule::update(uint32_t nowMs) {
  applyPending(nowMs);

  if (nowMs - lastDhtMs_ >= config_.dhtIntervalMs) {
    lastDhtMs_ = nowMs;
    const float t = dht_.readTemperature();
    const float h = dht_.readHumidity();
    dhtOk_ = !isnan(t);
    if (!isnan(h)) humidity_ = h;
    const bool before = logic_.tempAlert();
    logic_.addTemperature(t);
    if (logic_.tempAlert() != before) LOG_W(TAG, "alerta de temperatura: %s (%.1f C)", logic_.tempAlert() ? "ON" : "OFF", t);
  }

  if (nowMs - lastGasMs_ >= config_.gasIntervalMs) {
    lastGasMs_ = nowMs;
    gasRaw_ = static_cast<uint16_t>(analogRead(config_.gasPin));
    const bool before = logic_.gasAlert();
    logic_.addGas(nowMs, gasRaw_);
    if (logic_.gasAlert() != before) LOG_W(TAG, "alerta de gas: %s (raw %u)", logic_.gasAlert() ? "ON" : "OFF", gasRaw_);
  }

  driveOutputs(nowMs);
}

void KitchenModule::describe(JsonObject out) const {
  out["phase"] = KitchenLogic::phaseName(logic_.phase());
  out["alert"] = logic_.alerting();
  out["temp_alert"] = logic_.tempAlert();
  out["gas_alert"] = logic_.gasAlert();
  out["silenced"] = silenced_;
  out["dht_ok"] = dhtOk_ && logic_.tempValid();
  if (logic_.tempValid()) out["temperature_c"] = logic_.temperature();
  else out["temperature_c"] = nullptr;
  if (!isnan(humidity_)) out["humidity"] = humidity_;
  else out["humidity"] = nullptr;
  out["temp_max_c"] = logic_.config().tempMaxC;
  out["gas_raw"] = gasRaw_;
  out["gas_filtered"] = logic_.gasFiltered();
  out["gas_baseline"] = logic_.gasBaseline();
  out["gas_threshold"] = logic_.gasThreshold();
  out["gas_delta"] = logic_.config().gasDelta;
}

void KitchenModule::registerRoutes(ApiServer& api) {
  AsyncWebServer& srv = api.server();
  ApiServer* a = &api;

  // Silencia el buzzer de la alerta en curso (el LED sigue parpadeando).
  srv.on("/api/kitchen/silence", HTTP_POST, [this, a](AsyncWebServerRequest* req) {
    if (!a->requireAuth(req)) return;
    cmdSilence_ = true;
    JsonDocument doc;
    doc["accepted"] = true;
    a->sendJson(req, 202, doc);
  });

  // Vuelve a medir la línea base del MQ-2 (hacerlo con aire limpio).
  srv.on("/api/kitchen/calibrate", HTTP_POST, [this, a](AsyncWebServerRequest* req) {
    if (!a->requireAuth(req)) return;
    cmdCalibrate_ = true;
    JsonDocument doc;
    doc["accepted"] = true;
    doc["window_ms"] = config_.logic.baselineMs;
    a->sendJson(req, 202, doc);
  });

  // POST /api/kitchen/config?temp_max=40&gas_delta=450 (se guarda en NVS)
  srv.on("/api/kitchen/config", HTTP_POST, [this, a](AsyncWebServerRequest* req) {
    if (!a->requireAuth(req)) return;
    float t = 0.0f;
    long g = -1;
    if (req->hasParam("temp_max")) {
      t = req->getParam("temp_max")->value().toFloat();
      if (t < 20.0f || t > 100.0f) return a->sendError(req, 400, "temp_max debe estar entre 20 y 100");
    }
    if (req->hasParam("gas_delta")) {
      g = req->getParam("gas_delta")->value().toInt();
      if (g < 50 || g > 3000) return a->sendError(req, 400, "gas_delta debe estar entre 50 y 3000");
    }
    if (t <= 0.0f && g < 0) return a->sendError(req, 400, "indica temp_max y/o gas_delta");
    pendingTempMax_ = t;
    pendingGasDelta_ = static_cast<int32_t>(g);
    cmdConfig_ = true;
    JsonDocument doc;
    doc["accepted"] = true;
    a->sendJson(req, 202, doc);
  });
}
