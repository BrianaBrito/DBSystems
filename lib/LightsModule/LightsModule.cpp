#include "LightsModule.h"

#include <ApiServer.h>
#include <Logger.h>
#include <ScopedLock.h>

namespace {
const char* const TAG = "lights";
}

LightsModule::LightsModule(const LightsModuleConfig& config)
    : config_(config),
      mutex_(nullptr),
      presence_(config.pirWarmupMs, config.holdMs),
      motion_(false),
      pirReady_(false),
      lastPirMs_(0),
      lastOut_(-1),
      presenceEnabled_(true),
      dirty_(true) {}

void LightsModule::begin() {
  mutex_ = xSemaphoreCreateMutex();
  pinMode(config_.dataPin, OUTPUT);
  pinMode(config_.clockPin, OUTPUT);
  pinMode(config_.latchPin, OUTPUT);
  pinMode(config_.pirPin, INPUT);
  writeRegister(0);  // todo apagado al arrancar
  presence_.reset(millis());
  LOG_I(TAG, "luces listas (PIR calentando %lu ms)", static_cast<unsigned long>(config_.pirWarmupMs));
}

void LightsModule::writeRegister(uint8_t value) {
  digitalWrite(config_.latchPin, LOW);
  shiftOut(config_.dataPin, config_.clockPin, MSBFIRST, value);
  digitalWrite(config_.latchPin, HIGH);
  lastOut_ = value;
}

void LightsModule::update(uint32_t nowMs) {
  if (nowMs - lastPirMs_ >= config_.pirIntervalMs) {
    lastPirMs_ = nowMs;
    motion_ = digitalRead(config_.pirPin) == HIGH;
    presence_.update(nowMs, motion_);
    pirReady_ = presence_.warmedUp(nowMs);
  }

  uint8_t out;
  {
    ScopedLock lock(mutex_);
    bank_.setPresence(presenceEnabled_ && presence_.active());
    out = bank_.output();
  }
  if (dirty_ || out != lastOut_) {
    dirty_ = false;
    writeRegister(out);
  }
}

void LightsModule::describe(JsonObject out) const {
  JsonArray rooms = out["rooms"].to<JsonArray>();
  bool presence;
  {
    ScopedLock lock(mutex_);
    for (uint8_t i = 1; i <= LightBank::kRooms; i++) rooms.add(bank_.room(i));
    presence = bank_.presence();
  }
  out["presence"] = presence;
  out["presence_enabled"] = presenceEnabled_;
  out["motion"] = motion_;
  out["pir_ready"] = pirReady_;
}

void LightsModule::registerRoutes(ApiServer& api) {
  AsyncWebServer& srv = api.server();
  ApiServer* a = &api;

  // POST /api/lights/room?n=1..5&state=on|off|toggle
  srv.on("/api/lights/room", HTTP_POST, [this, a](AsyncWebServerRequest* req) {
    if (!a->requireAuth(req)) return;
    if (!req->hasParam("n") || !req->hasParam("state")) {
      return a->sendError(req, 400, "faltan parametros n y state");
    }
    const long n = req->getParam("n")->value().toInt();
    const String state = req->getParam("state")->value();
    if (n < 1 || n > LightBank::kRooms) return a->sendError(req, 400, "n debe estar entre 1 y 5");
    if (state != "on" && state != "off" && state != "toggle") {
      return a->sendError(req, 400, "state debe ser on, off o toggle");
    }
    bool now;
    {
      ScopedLock lock(mutex_);
      if (state == "toggle") bank_.toggleRoom(static_cast<uint8_t>(n));
      else bank_.setRoom(static_cast<uint8_t>(n), state == "on");
      now = bank_.room(static_cast<uint8_t>(n));
    }
    dirty_ = true;
    JsonDocument doc;
    doc["room"] = n;
    doc["on"] = now;
    a->sendJson(req, 200, doc);
  });

  // POST /api/lights/all?state=on|off
  srv.on("/api/lights/all", HTTP_POST, [this, a](AsyncWebServerRequest* req) {
    if (!a->requireAuth(req)) return;
    if (!req->hasParam("state")) return a->sendError(req, 400, "falta el parametro state");
    const String state = req->getParam("state")->value();
    if (state != "on" && state != "off") return a->sendError(req, 400, "state debe ser on u off");
    {
      ScopedLock lock(mutex_);
      bank_.setAll(state == "on");
    }
    dirty_ = true;
    JsonDocument doc;
    doc["accepted"] = true;
    a->sendJson(req, 200, doc);
  });

  // POST /api/lights/presence?mode=auto|off  (activa o desactiva el LED de presencia)
  srv.on("/api/lights/presence", HTTP_POST, [this, a](AsyncWebServerRequest* req) {
    if (!a->requireAuth(req)) return;
    if (!req->hasParam("mode")) return a->sendError(req, 400, "falta el parametro mode");
    const String mode = req->getParam("mode")->value();
    if (mode != "auto" && mode != "off") return a->sendError(req, 400, "mode debe ser auto u off");
    presenceEnabled_ = (mode == "auto");
    dirty_ = true;
    JsonDocument doc;
    doc["presence_enabled"] = presenceEnabled_;
    a->sendJson(req, 200, doc);
  });
}
