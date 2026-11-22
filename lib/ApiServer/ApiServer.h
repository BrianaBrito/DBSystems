#pragma once
/**
 * @file ApiServer.h
 * @brief Servidor HTTP asíncrono con API JSON, autenticación por token y
 *        registro de módulos.
 *
 * Autenticación: las rutas que cambian el estado de la casa (POST/PUT/DELETE)
 * deben llamar a requireAuth(); el cliente envía la cabecera
 * `X-API-Token: <token>`. Las rutas de lectura (GET /api/status) son públicas.
 */
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <IModule.h>
#include <WifiManager.h>

#include <vector>

class ApiServer {
 public:
  ApiServer(uint16_t port, const char* apiToken, const char* fwName, const char* fwVersion,
            const WifiManager& wifi);

  /** Registra un módulo (debe hacerse antes de begin()). */
  void addModule(IModule* module);

  /** Registra las rutas y arranca el servidor. */
  void begin();

  /** true si la petición trae el token correcto. */
  bool isAuthorized(AsyncWebServerRequest* request) const;

  /** Si falta el token responde 401 y devuelve false. Usar al inicio de rutas de escritura. */
  bool requireAuth(AsyncWebServerRequest* request) const;

  void sendJson(AsyncWebServerRequest* request, int code, const JsonDocument& doc) const;
  void sendError(AsyncWebServerRequest* request, int code, const char* message) const;

  AsyncWebServer& server() { return server_; }

 private:
  void handleStatus(AsyncWebServerRequest* request) const;

  AsyncWebServer server_;
  uint16_t port_;
  const char* token_;
  const char* fwName_;
  const char* fwVersion_;
  const WifiManager& wifi_;
  std::vector<IModule*> modules_;
};
