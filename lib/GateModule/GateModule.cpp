#include "GateModule.h"

#include <ApiServer.h>
#include <Logger.h>
#include <Preferences.h>
#include <ScopedLock.h>
#include <SPI.h>

namespace {

const char* const TAG = "gate";
const size_t kMaxCards = 32;
const size_t kMaxLog = 50;
const uint32_t kRfidRetryMs = 5000;
const int kServoStepDeg = 2;
const uint32_t kServoStepMs = 15;

Preferences prefs;

}  // namespace

GateModule::GateModule(const GateModuleConfig& config, Buzzer& buzzer)
    : config_(config),
      buzzer_(buzzer),
      rfid_(config.rfidSs, config.rfidRst),
      sensor_(config.sensor),
      logic_(config.logic),
      mutex_(nullptr),
      seq_(0),
      cmdOpen_(false),
      cmdClose_(false),
      cmdLearn_(false),
      rfidOk_(false),
      learnMode_(false),
      learnStartMs_(0),
      lastPollMs_(0),
      lastRfidRetryMs_(0),
      lastCardMs_(0),
      lastState_(GateState::Closed),
      servoDeg_(config.closedDeg),
      servoTarget_(config.closedDeg),
      lastServoStepMs_(0) {}

// ---------------------------------------------------------------- utilidades

String GateModule::normalizeUid(const String& raw) {
  String out;
  for (size_t i = 0; i < raw.length(); i++) {
    const char c = raw[i];
    if (c == ':' || c == ' ' || c == '-') continue;
    if (!isxdigit(static_cast<unsigned char>(c))) return "";
    out += static_cast<char>(toupper(c));
  }
  if (out.length() < 8 || out.length() > 20 || (out.length() % 2) != 0) return "";
  return out;
}

String GateModule::uidToString(const MFRC522::Uid& uid) {
  String s;
  for (byte i = 0; i < uid.size; i++) {
    if (uid.uidByte[i] < 0x10) s += '0';
    s += String(uid.uidByte[i], HEX);
  }
  s.toUpperCase();
  return s;
}

void GateModule::loadCards() {
  prefs.begin("gate", true);
  String csv = prefs.getString("cards", "");
  prefs.end();

  cards_.clear();
  int start = 0;
  while (start < static_cast<int>(csv.length())) {
    int comma = csv.indexOf(',', start);
    if (comma < 0) comma = csv.length();
    String uid = normalizeUid(csv.substring(start, comma));
    if (uid.length() > 0) cards_.push_back(uid);
    start = comma + 1;
  }
}

// Llamar con mutex_ tomado.
void GateModule::saveCardsLocked() {
  String csv;
  for (size_t i = 0; i < cards_.size(); i++) {
    if (i > 0) csv += ',';
    csv += cards_[i];
  }
  prefs.begin("gate", false);
  prefs.putString("cards", csv);
  prefs.end();
}

bool GateModule::hasCardLocked(const String& uid) const {
  for (size_t i = 0; i < cards_.size(); i++) {
    if (cards_[i] == uid) return true;
  }
  return false;
}

void GateModule::logEvent(uint32_t nowMs, const String& uid, const char* result) {
  AccessEvent ev;
  ev.uptimeS = nowMs / 1000;
  ev.uid = uid;
  ev.result = result;
  {
    ScopedLock lock(mutex_);
    ev.seq = ++seq_;
    if (log_.size() >= kMaxLog) log_.erase(log_.begin());
    log_.push_back(ev);
    if (listener_) {
      if (outbox_.size() >= kMaxLog) outbox_.erase(outbox_.begin());
      outbox_.push_back(ev);
    }
  }
  LOG_I(TAG, "acceso #%u uid=%s resultado=%s", static_cast<unsigned>(ev.seq), uid.c_str(), result);
}

// ------------------------------------------------------------------- ciclo

void GateModule::begin() {
  mutex_ = xSemaphoreCreateMutex();
  loadCards();
  LOG_I(TAG, "%u tarjetas registradas", static_cast<unsigned>(cards_.size()));

  SPI.begin(config_.rfidSck, config_.rfidMiso, config_.rfidMosi, config_.rfidSs);
  rfid_.PCD_Init();
  const byte version = rfid_.PCD_ReadRegister(MFRC522::VersionReg);
  rfidOk_ = (version != 0x00 && version != 0xFF);
  if (rfidOk_) {
    LOG_I(TAG, "RC522 detectado (version 0x%02X)", version);
  } else {
    LOG_E(TAG, "RC522 sin comunicacion: revisa cableado y 3.3V (reintento cada 5 s)");
  }

  sensor_.begin();

  prefs.begin("gate", true);
  config_.closedDeg = static_cast<uint8_t>(prefs.getUChar("closed", config_.closedDeg));
  config_.openDeg = static_cast<uint8_t>(prefs.getUChar("open", config_.openDeg));
  prefs.end();
  LOG_I(TAG, "servo: cerrado=%u grados, abierto=%u grados", config_.closedDeg, config_.openDeg);

  servo_.setPeriodHertz(50);
  servo_.attach(config_.servoPin, 500, 2400);
  writeServo(config_.closedDeg);
}

void GateModule::update(uint32_t nowMs) {
  sensor_.update(nowMs);

  // Órdenes recibidas por HTTP
  if (cmdOpen_) {
    cmdOpen_ = false;
    if (logic_.state() == GateState::Closed) {
      logic_.requestOpen(nowMs);
      logEvent(nowMs, "", "manual_open");
      buzzer_.play(Buzzer::Accept);
    }
  }
  if (cmdClose_) {
    cmdClose_ = false;
    logic_.requestClose(nowMs);
  }
  if (cmdLearn_) {
    cmdLearn_ = false;
    learnMode_ = true;
    learnStartMs_ = nowMs;
    buzzer_.play(Buzzer::Click);
    LOG_I(TAG, "modo aprendizaje: acerca la tarjeta nueva");
  }
  if (learnMode_ && nowMs - learnStartMs_ >= config_.learnModeMs) {
    learnMode_ = false;
    LOG_I(TAG, "modo aprendizaje terminado");
  }

  pollRfid(nowMs);

  logic_.update(nowMs, sensor_.isPresent());
  if (logic_.state() != lastState_) {
    lastState_ = logic_.state();
    LOG_I(TAG, "estado: %s", GateLogic::stateName(lastState_));
    if (lastState_ == GateState::Closing) buzzer_.play(Buzzer::Click);
  }
  servoTarget_ = logic_.servoShouldBeOpen() ? config_.openDeg : config_.closedDeg;
  stepServo(nowMs);

  // Avisar eventos pendientes fuera del mutex
  if (listener_) {
    std::vector<AccessEvent> pending;
    {
      ScopedLock lock(mutex_);
      pending.swap(outbox_);
    }
    for (size_t i = 0; i < pending.size(); i++) listener_(pending[i]);
  }
}

void GateModule::pollRfid(uint32_t nowMs) {
  if (!rfidOk_) {
    if (nowMs - lastRfidRetryMs_ < kRfidRetryMs) return;
    lastRfidRetryMs_ = nowMs;
    rfid_.PCD_Init();
    const byte v = rfid_.PCD_ReadRegister(MFRC522::VersionReg);
    rfidOk_ = (v != 0x00 && v != 0xFF);
    if (rfidOk_) LOG_I(TAG, "RC522 recuperado");
    return;
  }

  if (nowMs - lastPollMs_ < config_.rfidPollMs) return;
  lastPollMs_ = nowMs;

  if (!rfid_.PICC_IsNewCardPresent() || !rfid_.PICC_ReadCardSerial()) return;

  const String uid = uidToString(rfid_.uid);
  rfid_.PICC_HaltA();
  rfid_.PCD_StopCrypto1();

  // Evita lecturas repetidas de la misma tarjeta
  if (uid == lastCardUid_ && nowMs - lastCardMs_ < config_.rfidCooldownMs) return;
  lastCardUid_ = uid;
  lastCardMs_ = nowMs;

  handleCard(nowMs, uid);
}

void GateModule::handleCard(uint32_t nowMs, const String& uid) {
  if (learnMode_) {
    learnMode_ = false;
    const char* result;
    {
      ScopedLock lock(mutex_);
      if (hasCardLocked(uid)) {
        result = "already_registered";
      } else if (cards_.size() >= kMaxCards) {
        result = "card_list_full";
      } else {
        cards_.push_back(uid);
        saveCardsLocked();
        result = "learned";
      }
    }
    logEvent(nowMs, uid, result);
    buzzer_.play(strcmp(result, "learned") == 0 ? Buzzer::Accept : Buzzer::Deny);
    return;
  }

  bool allowed;
  {
    ScopedLock lock(mutex_);
    allowed = hasCardLocked(uid);
  }

  if (allowed) {
    logic_.requestOpen(nowMs);  // si ya está abierta, extiende la espera
    logEvent(nowMs, uid, "granted");
    buzzer_.play(Buzzer::Accept);
  } else {
    logEvent(nowMs, uid, "denied");
    buzzer_.play(Buzzer::Deny);
  }
}

void GateModule::writeServo(int deg) {
  servoDeg_ = deg;
  servo_.write(deg);
}

// Mueve el servo poco a poco para no sacudir la estructura ni la fuente.
void GateModule::stepServo(uint32_t nowMs) {
  if (servoDeg_ == servoTarget_) return;
  if (nowMs - lastServoStepMs_ < kServoStepMs) return;
  lastServoStepMs_ = nowMs;

  int next = servoDeg_;
  if (servoTarget_ > servoDeg_) {
    next = min(servoDeg_ + kServoStepDeg, servoTarget_);
  } else {
    next = max(servoDeg_ - kServoStepDeg, servoTarget_);
  }
  writeServo(next);
}

// -------------------------------------------------------------------- API

void GateModule::describe(JsonObject out) const {
  out["state"] = GateLogic::stateName(logic_.state());
  out["vehicle_present"] = sensor_.isPresent();
  out["distance_cm"] = sensor_.distanceCm();
  out["last_close_reason"] = GateLogic::reasonName(logic_.lastCloseReason());
  out["servo_closed_deg"] = config_.closedDeg;
  out["servo_open_deg"] = config_.openDeg;
  out["rfid_ok"] = rfidOk_;
  out["learn_mode"] = learnMode_;
  size_t n = 0;
  {
    ScopedLock lock(mutex_);
    n = cards_.size();
  }
  out["cards"] = n;
}

void GateModule::registerRoutes(ApiServer& api) {
  AsyncWebServer& srv = api.server();
  ApiServer* a = &api;

  srv.on("/api/gate/open", HTTP_POST, [this, a](AsyncWebServerRequest* req) {
    if (!a->requireAuth(req)) return;
    cmdOpen_ = true;
    JsonDocument doc;
    doc["accepted"] = true;
    a->sendJson(req, 202, doc);
  });

  srv.on("/api/gate/close", HTTP_POST, [this, a](AsyncWebServerRequest* req) {
    if (!a->requireAuth(req)) return;
    cmdClose_ = true;
    JsonDocument doc;
    doc["accepted"] = true;
    a->sendJson(req, 202, doc);
  });

  srv.on("/api/gate/learn", HTTP_POST, [this, a](AsyncWebServerRequest* req) {
    if (!a->requireAuth(req)) return;
    cmdLearn_ = true;
    JsonDocument doc;
    doc["accepted"] = true;
    doc["window_ms"] = config_.learnModeMs;
    a->sendJson(req, 202, doc);
  });

  // Calibración del servo: POST /api/gate/servo?closed=0&open=90 (0-180). Se guarda en NVS.
  srv.on("/api/gate/servo", HTTP_POST, [this, a](AsyncWebServerRequest* req) {
    if (!a->requireAuth(req)) return;
    long closedV = config_.closedDeg;
    long openV = config_.openDeg;
    if (req->hasParam("closed")) closedV = req->getParam("closed")->value().toInt();
    if (req->hasParam("open")) openV = req->getParam("open")->value().toInt();
    if (closedV < 0 || closedV > 180 || openV < 0 || openV > 180) {
      return a->sendError(req, 400, "los angulos deben estar entre 0 y 180");
    }
    config_.closedDeg = static_cast<uint8_t>(closedV);
    config_.openDeg = static_cast<uint8_t>(openV);
    {
      ScopedLock lock(mutex_);
      prefs.begin("gate", false);
      prefs.putUChar("closed", config_.closedDeg);
      prefs.putUChar("open", config_.openDeg);
      prefs.end();
    }
    JsonDocument doc;
    doc["servo_closed_deg"] = config_.closedDeg;
    doc["servo_open_deg"] = config_.openDeg;
    a->sendJson(req, 200, doc);
  });

  srv.on("/api/gate/cards", HTTP_GET, [this, a](AsyncWebServerRequest* req) {
    if (!a->requireAuth(req)) return;
    JsonDocument doc;
    JsonArray arr = doc["cards"].to<JsonArray>();
    {
      ScopedLock lock(mutex_);
      for (size_t i = 0; i < cards_.size(); i++) arr.add(cards_[i]);
    }
    a->sendJson(req, 200, doc);
  });

  srv.on("/api/gate/cards", HTTP_POST, [this, a](AsyncWebServerRequest* req) {
    if (!a->requireAuth(req)) return;
    if (!req->hasParam("uid")) return a->sendError(req, 400, "falta el parametro uid");
    const String uid = normalizeUid(req->getParam("uid")->value());
    if (uid.length() == 0) return a->sendError(req, 400, "uid invalido (hexadecimal, 8 a 20 digitos)");
    int code = 201;
    {
      ScopedLock lock(mutex_);
      if (hasCardLocked(uid)) {
        code = 409;
      } else if (cards_.size() >= kMaxCards) {
        code = 507;
      } else {
        cards_.push_back(uid);
        saveCardsLocked();
      }
    }
    if (code == 409) return a->sendError(req, 409, "la tarjeta ya esta registrada");
    if (code == 507) return a->sendError(req, 507, "lista de tarjetas llena");
    JsonDocument doc;
    doc["uid"] = uid;
    a->sendJson(req, 201, doc);
  });

  srv.on("/api/gate/cards", HTTP_DELETE, [this, a](AsyncWebServerRequest* req) {
    if (!a->requireAuth(req)) return;
    if (!req->hasParam("uid")) return a->sendError(req, 400, "falta el parametro uid");
    const String uid = normalizeUid(req->getParam("uid")->value());
    bool removed = false;
    if (uid.length() > 0) {
      ScopedLock lock(mutex_);
      for (size_t i = 0; i < cards_.size(); i++) {
        if (cards_[i] == uid) {
          cards_.erase(cards_.begin() + i);
          saveCardsLocked();
          removed = true;
          break;
        }
      }
    }
    if (!removed) return a->sendError(req, 404, "tarjeta no encontrada");
    JsonDocument doc;
    doc["removed"] = uid;
    a->sendJson(req, 200, doc);
  });

  srv.on("/api/gate/log", HTTP_GET, [this, a](AsyncWebServerRequest* req) {
    if (!a->requireAuth(req)) return;
    size_t limit = 20;
    if (req->hasParam("limit")) {
      const long v = req->getParam("limit")->value().toInt();
      if (v > 0) limit = static_cast<size_t>(v);
    }
    if (limit > kMaxLog) limit = kMaxLog;

    JsonDocument doc;
    JsonArray arr = doc["events"].to<JsonArray>();
    {
      ScopedLock lock(mutex_);
      size_t count = 0;
      for (size_t i = log_.size(); i > 0 && count < limit; i--, count++) {  // más nuevo primero
        const AccessEvent& e = log_[i - 1];
        JsonObject o = arr.add<JsonObject>();
        o["seq"] = e.seq;
        o["uptime_s"] = e.uptimeS;
        o["uid"] = e.uid;
        o["result"] = e.result;
      }
    }
    a->sendJson(req, 200, doc);
  });
}
