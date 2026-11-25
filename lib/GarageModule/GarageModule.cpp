#include "GarageModule.h"

#include <ApiServer.h>
#include <Logger.h>
#include <Preferences.h>

namespace {
const char* const TAG = "garage";
Preferences prefs;
}  // namespace

GarageModule::GarageModule(const GarageModuleConfig& config)
    : config_(config),
      logic_(config.travelMs),
      button_(config.debounceMs, config.lockMs),
      button2_(config.debounceMs, config.lockMs),
      deg_(config.closedDeg),
      lastServoMs_(0),
      lastSource_("none"),
      closed_(config.closedDeg),
      open_(config.openDeg),
      cmdOpen_(false),
      cmdClose_(false),
      calibDirty_(false) {}

void GarageModule::begin() {
  pinMode(config_.rfPin, INPUT);    // GPIO 36/39: solo entrada, sin pull-up interno
  pinMode(config_.rfPin2, INPUT);

  prefs.begin("garage", false);  // lectura y escritura: crea el espacio si no existe
  closed_ = prefs.getUChar("closed", closed_);
  open_ = prefs.getUChar("open", open_);
  prefs.end();

  servo_.setPeriodHertz(50);
  servo_.attach(config_.servoPin, 500, 2400);

  logic_.begin(millis());
  writeServo();
  LOG_I(TAG, "cochera lista: cerrado %u, abierto %u", closed_, open_);
}

void GarageModule::writeServo() {
  deg_ = GarageLogic::angleFor(logic_.position(), closed_, open_);
  servo_.write(deg_);
}

void GarageModule::update(uint32_t nowMs) {
  // Control remoto (RX480E en modo anclaje): cada cambio de nivel alterna abrir y cerrar
  const bool pressed1 = button_.update(nowMs, digitalRead(config_.rfPin) == HIGH);
  const bool pressed2 = button2_.update(nowMs, digitalRead(config_.rfPin2) == HIGH);
  if (pressed1 || pressed2) {
    if (logic_.toggle()) {
      lastSource_ = "remote";
      LOG_I(TAG, "control remoto: %s", logic_.state() == GarageLogic::Opening ? "abrir" : "cerrar");
    }
  }

  // Órdenes de la API
  if (cmdOpen_) {
    cmdOpen_ = false;
    if (logic_.requestOpen()) lastSource_ = "api";
  }
  if (cmdClose_) {
    cmdClose_ = false;
    if (logic_.requestClose()) lastSource_ = "api";
  }

  const GarageLogic::State before = logic_.state();
  logic_.update(nowMs);
  if (logic_.state() != before) LOG_I(TAG, "estado: %s", GarageLogic::stateName(logic_.state()));

  if (calibDirty_) {
    calibDirty_ = false;
    prefs.begin("garage", false);
    prefs.putUChar("closed", closed_);
    prefs.putUChar("open", open_);
    prefs.end();
    writeServo();  // aplica el nuevo ángulo sin esperar al siguiente movimiento
  }

  if (logic_.moving() && nowMs - lastServoMs_ >= config_.servoIntervalMs) {
    lastServoMs_ = nowMs;
    writeServo();
  } else if (!logic_.moving() && deg_ != GarageLogic::angleFor(logic_.position(), closed_, open_)) {
    writeServo();  // asegura que quede exactamente en el ángulo final
  }
}

void GarageModule::describe(JsonObject out) const {
  out["state"] = GarageLogic::stateName(logic_.state());
  out["position_pct"] = static_cast<int>(logic_.position() * 100.0f + 0.5f);
  out["deg"] = deg_;
  out["last_command"] = lastSource_;
  JsonObject cal = out["calibration"].to<JsonObject>();
  cal["closed"] = closed_;
  cal["open"] = open_;
}

void GarageModule::registerRoutes(ApiServer& api) {
  AsyncWebServer& srv = api.server();
  ApiServer* a = &api;

  srv.on("/api/garage/open", HTTP_POST, [this, a](AsyncWebServerRequest* req) {
    if (!a->requireAuth(req)) return;
    cmdOpen_ = true;
    JsonDocument doc;
    doc["accepted"] = true;
    a->sendJson(req, 202, doc);
  });

  srv.on("/api/garage/close", HTTP_POST, [this, a](AsyncWebServerRequest* req) {
    if (!a->requireAuth(req)) return;
    cmdClose_ = true;
    JsonDocument doc;
    doc["accepted"] = true;
    a->sendJson(req, 202, doc);
  });

  // POST /api/garage/servo?closed=0&open=90 (0-180, cualquiera de los dos)
  srv.on("/api/garage/servo", HTTP_POST, [this, a](AsyncWebServerRequest* req) {
    if (!a->requireAuth(req)) return;
    long v[2] = {closed_, open_};
    const char* names[2] = {"closed", "open"};
    bool any = false;
    for (int i = 0; i < 2; i++) {
      if (req->hasParam(names[i])) {
        v[i] = req->getParam(names[i])->value().toInt();
        any = true;
      }
      if (v[i] < 0 || v[i] > 180) return a->sendError(req, 400, "los angulos deben estar entre 0 y 180");
    }
    if (!any) return a->sendError(req, 400, "indica closed y/o open");
    closed_ = static_cast<uint8_t>(v[0]);
    open_ = static_cast<uint8_t>(v[1]);
    calibDirty_ = true;
    JsonDocument doc;
    doc["closed"] = closed_;
    doc["open"] = open_;
    a->sendJson(req, 200, doc);
  });
}
