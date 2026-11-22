# Casa inteligente con ESP32

Sistema de domótica construido sobre un **ESP32** que funciona como servidor web en modo
dual **AP+STA** y expone una **API JSON** para monitorear sensores y controlar actuadores
de una casa a escala: caseta de acceso con RFID, cocina con alertas, luces de recámaras,
portón de cochera por radiofrecuencia y registro de entradas en base de datos.

> Proyecto universitario en equipo, desarrollado con PlatformIO y Visual Studio Code.

## Funcionalidades

| Módulo | Qué hace | Hardware | Estado |
|---|---|---|---|
| Base | WiFi AP+STA, API `/api/status`, registro por serie | ESP32 | ☑ |
| Caseta | Abre con tarjeta RFID autorizada y cierra al pasar el vehículo; registra entradas | RC522, servo, HC-SR04 | ☐ |
| Registro de entradas | Historial de accesos en base de datos (backend PHP) | Laptop con PHP + SQLite | ☐ |
| Cocina | Alerta (LED y buzzer) por temperatura alta o gas | DHT22, MQ2, LED, buzzer | ☐ |
| Luces | Control de LEDs de recámaras desde la web; LED de presencia | 74HC595, sensor de presencia | ☐ |
| Cochera | Abre y cierra el portón con control remoto de 433 MHz | RX480E + control Sonoff, servo | ☐ |
| Panel web | Página servida por el ESP32 para controlar y consultar | LittleFS | ☐ |

## Arquitectura

```
                  ┌──────────────────────────── ESP32 ────────────────────────────┐
 Navegador  ──►   │  ApiServer (HTTP/JSON, token)                                  │
 (AP o router)    │      ▲                                                         │
                  │      │ registra rutas y estado                                 │
                  │  IModule  ◄── Caseta · Cocina · Luces · Cochera (update() sin  │
                  │                bloqueos en loop)                               │
                  │  WifiManager (AP + STA con reconexión)                         │
                  └───────────────┬────────────────────────────────────────────────┘
                                  │ HTTP POST (eventos)
                                  ▼
                       Backend PHP + base de datos (laptop)
```

Decisiones de diseño:

- **Un módulo por responsabilidad.** Cada funcionalidad implementa `IModule`
  (`begin`, `update`, `describe`, `registerRoutes`). Agregar un sensor no obliga a
  tocar el resto del código.
- **Sin bloqueos.** Nada de `delay()`; cada componente trabaja con `millis()`, así
  el servidor web nunca se congela.
- **Hilos.** Los manejadores HTTP corren en otra tarea de FreeRTOS que `loop()`:
  solo registran una orden y `update()` la ejecuta, para no tocar hardware desde
  dos hilos.
- **Configuración centralizada.** Pines y parámetros en `include/config.h`; las
  credenciales en `include/secrets.h`, que **no se sube a Git**.
- **Seguridad básica.** Las rutas que cambian el estado de la casa exigen un token
  (`X-API-Token`); el AP tiene contraseña.

## Estructura del repositorio

```
├── include/
│   ├── config.h              pines y parámetros
│   ├── secrets.h.example     plantilla de credenciales (secrets.h está en .gitignore)
├── lib/
│   ├── Common/               IModule, Logger, StatusLed
│   ├── WifiManager/          modo dual AP+STA
│   └── ApiServer/            servidor HTTP/JSON y autenticación
├── src/main.cpp              ensambla los componentes
├── docs/                     pines.md, api.md
└── platformio.ini
```

## Puesta en marcha

Requisitos: Visual Studio Code con la extensión PlatformIO y un ESP32 DOIT DevKit V1.

1. Clona el repositorio y ábrelo en VS Code.
2. Copia `include/secrets.h.example` como `include/secrets.h` y completa tus datos
   (red WiFi, contraseña del AP, token de la API).
3. Compila y sube con los botones de PlatformIO (✓ y →), o con `pio run -t upload`.
4. Abre el monitor serial a 115200 baudios y verifica la IP.
5. Prueba: `curl http://192.168.4.1/api/status` conectada al AP, o con la IP del router.

El cableado está en [`docs/pines.md`](docs/pines.md) y los endpoints en
[`docs/api.md`](docs/api.md).

## Flujo de trabajo

- Una **rama por módulo** (`feature/wifi-manager`, `feature/caseta-rfid`...) y un
  **pull request** hacia `main`.
- **Commits pequeños** con mensajes convencionales: `feat:`, `fix:`, `docs:`,
  `refactor:`, `chore:`.
- Cada módulo actualiza `docs/` y la tabla de funcionalidades de este README.
