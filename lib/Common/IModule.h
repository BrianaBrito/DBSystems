#pragma once
/**
 * @file IModule.h
 * @brief Interfaz común de los módulos del sistema (caseta, cocina, luces...).
 *
 * Cada funcionalidad de la casa es un módulo que implementa esta interfaz.
 * Así agregar un sensor nuevo es crear una clase y registrarla en main.cpp,
 * sin tocar el resto del código.
 *
 * Reglas de diseño:
 *  - update() nunca bloquea (nada de delay()); trabaja con el tiempo recibido.
 *  - Los manejadores HTTP corren en OTRA tarea de FreeRTOS que loop(). Por eso
 *    un manejador no debe mover hardware ni modificar estructuras complejas:
 *    solo registra una orden (variable simple) que update() ejecuta después.
 */
#include <Arduino.h>
#include <ArduinoJson.h>

class ApiServer;

class IModule {
 public:
  virtual ~IModule() {}

  /** Nombre corto y estable; se usa como clave en GET /api/status. */
  virtual const char* name() const = 0;

  /** Configura pines y hardware. Se llama una vez en setup(). */
  virtual void begin() = 0;

  /** Tarea periódica no bloqueante. Se llama en cada vuelta de loop(). */
  virtual void update(uint32_t nowMs) = 0;

  /** Agrega el estado del módulo al JSON de GET /api/status. */
  virtual void describe(JsonObject out) const { (void)out; }

  /** Registra las rutas HTTP propias del módulo. */
  virtual void registerRoutes(ApiServer& api) { (void)api; }
};
