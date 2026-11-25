#include "ApiServer.h"

#include <LittleFS.h>
#include <Logger.h>

namespace {
const char* const TAG = "api";
const char* const TOKEN_HEADER = "X-API-Token";

// Comparación en tiempo constante (evita filtrar el token por diferencias de tiempo).
bool secureEquals(const String& received, const char* expected) {
  const size_t expectedLen = strlen(expected);
  if (received.length() != expectedLen) return false;
  uint8_t diff = 0;
  for (size_t i = 0; i < expectedLen; i++) {
    diff |= static_cast<uint8_t>(received[i] ^ expected[i]);
  }
  return diff == 0;
}
}  // namespace

ApiServer::ApiServer(uint16_t port, const char* apiToken, const char* fwName,
                     const char* fwVersion, const WifiManager& wifi)
    : server_(port),
      port_(port),
      token_(apiToken),
      fwName_(fwName),
      fwVersion_(fwVersion),
      wifi_(wifi) {}

void ApiServer::addModule(IModule* module) { modules_.push_back(module); }

void ApiServer::begin() {
  server_.on("/api/status", HTTP_GET,
             [this](AsyncWebServerRequest* request) { handleStatus(request); });

  for (size_t i = 0; i < modules_.size(); i++) {
    modules_[i]->registerRoutes(*this);
  }

  // Panel web: los archivos de la carpeta data/ (se suben con "Upload Filesystem Image").
  // Se registra después de las rutas /api para que estas tengan prioridad.
  if (LittleFS.exists("/index.html")) {
    server_.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
  } else {
    LOG_W(TAG, "No hay /index.html en LittleFS: sube el sistema de archivos para ver el panel");
    server_.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
      request->send(200, "text/plain; charset=utf-8",
                    "Panel no instalado. En PlatformIO: Upload Filesystem Image. La API sigue en /api/status.");
    });
  }

  server_.onNotFound(
      [this](AsyncWebServerRequest* request) { sendError(request, 404, "ruta no encontrada"); });

  server_.begin();
  LOG_I(TAG, "Servidor HTTP escuchando en el puerto %u", (unsigned)port_);
}

bool ApiServer::isAuthorized(AsyncWebServerRequest* request) const {
  const AsyncWebHeader* header = request->getHeader(TOKEN_HEADER);
  return header != nullptr && secureEquals(header->value(), token_);
}

bool ApiServer::requireAuth(AsyncWebServerRequest* request) const {
  if (isAuthorized(request)) return true;
  sendError(request, 401, "token ausente o inválido (cabecera X-API-Token)");
  return false;
}

void ApiServer::sendJson(AsyncWebServerRequest* request, int code, const JsonDocument& doc) const {
  AsyncResponseStream* response = request->beginResponseStream("application/json");
  response->setCode(code);
  serializeJson(doc, *response);
  request->send(response);
}

void ApiServer::sendError(AsyncWebServerRequest* request, int code, const char* message) const {
  JsonDocument doc;
  doc["error"] = message;
  sendJson(request, code, doc);
}

void ApiServer::handleStatus(AsyncWebServerRequest* request) const {
  JsonDocument doc;

  JsonObject firmware = doc["firmware"].to<JsonObject>();
  firmware["name"] = fwName_;
  firmware["version"] = fwVersion_;

  doc["uptime_s"] = millis() / 1000UL;
  doc["heap_free"] = ESP.getFreeHeap();

  JsonObject ap = doc["wifi"]["ap"].to<JsonObject>();
  ap["running"] = wifi_.isApRunning();
  ap["ssid"] = wifi_.apSsid();
  ap["ip"] = wifi_.apIp().toString();
  ap["clients"] = wifi_.apClientCount();

  JsonObject sta = doc["wifi"]["sta"].to<JsonObject>();
  sta["configured"] = wifi_.isStaConfigured();
  sta["connected"] = wifi_.isStaConnected();
  if (wifi_.isStaConnected()) {
    sta["ssid"] = wifi_.staSsid();
    sta["ip"] = wifi_.staIp().toString();
    sta["rssi"] = wifi_.staRssi();
  }
  doc["wifi"]["channel"] = wifi_.channel();

  JsonObject modules = doc["modules"].to<JsonObject>();
  for (size_t i = 0; i < modules_.size(); i++) {
    JsonObject entry = modules[modules_[i]->name()].to<JsonObject>();
    modules_[i]->describe(entry);
  }

  sendJson(request, 200, doc);
}
