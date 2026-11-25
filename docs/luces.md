# Módulo Luces (recámaras y presencia)

Controla desde la API los LEDs de 5 recámaras mediante un registro de desplazamiento
74HC595 y enciende un LED de presencia cuando el sensor PIR detecta movimiento.

## Hardware

| Componente | Pines ESP32 | Notas |
|---|---|---|
| 74HC595 | SER 16, SRCLK 17, RCLK 21 | Alimentado a **3.3 V**; OE a GND, SRCLR a 3.3 V, 100 nF entre VCC y GND |
| LEDs | Q0–Q4 = recámaras 1–5, Q5 = presencia | Resistencia de 330 Ω en serie con cada LED |
| PIR HC-SR501 | OUT → GPIO 26 | VCC a **5 V**, su salida ya es de 3.3 V (no necesita divisor) |

Ajuste del HC-SR501 (potenciómetros y puente): tiempo al mínimo, sensibilidad a la
mitad y el puente en **H** (modo repetible).

## Cómo funciona

- Las recámaras se controlan por API (`on`, `off` o `toggle`). Todas arrancan apagadas.
- **Presencia:** el HC-SR501 dispara solo al encenderse, así que durante los
  primeros 30 s se ignora (`pir_ready: false`). Después, el LED se enciende con
  cada movimiento y se mantiene 10 s tras el último, para que no parpadee.
- `POST /api/lights/all?state=off` apaga solo las recámaras; el LED de presencia
  responde únicamente al sensor y se puede desactivar con `mode=off`.
- El estado vive en una clase pura (`lib/LightsLogic`, con pruebas unitarias). Los
  manejadores HTTP solo modifican ese estado bajo un mutex y `update()` escribe el
  74HC595 en el loop.

## Probar

```powershell
curl.exe http://192.168.4.1/api/status
curl.exe -X POST -H "X-API-Token: dbsystems" "http://192.168.4.1/api/lights/room?n=1&state=on"
curl.exe -X POST -H "X-API-Token: dbsystems" "http://192.168.4.1/api/lights/room?n=1&state=toggle"
curl.exe -X POST -H "X-API-Token: dbsystems" "http://192.168.4.1/api/lights/all?state=on"
curl.exe -X POST -H "X-API-Token: dbsystems" "http://192.168.4.1/api/lights/presence?mode=off"
```

Si un LED no enciende, revisa su polaridad (pata larga hacia la resistencia) y que
el 595 esté a 3.3 V. Si el LED de presencia se enciende solo al inicio, espera el
calentamiento del PIR.
