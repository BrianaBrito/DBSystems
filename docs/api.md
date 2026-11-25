# API HTTP del ESP32

Servidor asíncrono en el puerto 80. Accesible en:

- **Modo AP:** `http://192.168.4.1` (red WiFi propia del ESP32).
- **Modo STA:** la IP que el router asigne (se imprime en el monitor serial).

## Convenciones

- Las respuestas de la API son JSON (`Content-Type: application/json`). La ruta `/` sirve el
  panel web ([panel.md](panel.md)).
- Los errores tienen la forma `{"error": "mensaje"}` con el código HTTP correspondiente.
- Las rutas que **cambian el estado** de la casa exigen la cabecera
  `X-API-Token: <API_TOKEN>` (el valor está en `include/secrets.h`). Sin ella
  responden `401`. Las rutas de lectura son públicas.

## Endpoints

### `GET /api/status`

Estado general del sistema. Cada módulo registrado agrega su sección dentro de `modules`.

```json
{
  "firmware": { "name": "smart-home-esp32", "version": "0.4.0" },
  "uptime_s": 125,
  "heap_free": 214532,
  "wifi": {
    "ap":  { "running": true, "ssid": "CasaInteligente", "ip": "192.168.4.1", "clients": 1 },
    "sta": { "configured": true, "connected": true, "ssid": "MiRed", "ip": "192.168.1.60", "rssi": -58 },
    "channel": 6
  },
  "modules": {}
}
```

Prueba rápida desde PowerShell:

```powershell
curl.exe http://192.168.4.1/api/status
```

### Caseta (`/api/gate/*`)

Todas requieren `X-API-Token`. Detalles de uso y calibración en [caseta.md](caseta.md).

| Método | Ruta | Descripción | Respuesta |
|---|---|---|---|
| POST | `/api/gate/open` | Abre la barra (apertura manual) | `202` |
| POST | `/api/gate/close` | Cierra la barra (solo mientras espera al vehículo) | `202` |
| POST | `/api/gate/learn` | Ventana de 20 s para registrar la próxima tarjeta | `202` |
| POST | `/api/gate/servo?closed=0&open=90` | Calibra los ángulos del servo (0-180); se guardan en NVS | `200`, `400` |
| GET | `/api/gate/cards` | Lista de UIDs autorizados | `200` |
| POST | `/api/gate/cards?uid=AABBCCDD` | Registra una tarjeta | `201`, `400`, `409`, `507` |
| DELETE | `/api/gate/cards?uid=AABBCCDD` | Elimina una tarjeta | `200`, `404` |
| GET | `/api/gate/log?limit=20` | Bitácora de accesos, la más reciente primero (máx. 50) | `200` |

Evento de la bitácora: `{"seq":3,"uptime_s":412,"uid":"A1B2C3D4","result":"granted"}`.
`result`: `granted`, `denied`, `learned`, `already_registered`, `card_list_full`, `manual_open`.

En `GET /api/status` el módulo aparece como `modules.gate` con `state`, `vehicle_present`,
`distance_cm`, `last_close_reason`, `rfid_ok`, `learn_mode` y `cards`.

### Cocina (`/api/kitchen/*`)

Sección `modules.kitchen` de `GET /api/status`:

```json
{
  "phase": "ready", "alert": false, "temp_alert": false, "gas_alert": false,
  "silenced": false, "dht_ok": true, "temperature_c": 26.4, "humidity": 48.1,
  "temp_max_c": 40.0, "gas_raw": 1021, "gas_filtered": 1018,
  "gas_baseline": 1005, "gas_threshold": 1455, "gas_delta": 450
}
```

`phase`: `warming_up`, `calibrating` o `ready`. `temperature_c` y `humidity` son `null`
si el sensor de temperatura no responde. Detalles en [cocina.md](cocina.md).

| Método | Ruta | Descripción | Respuesta |
|---|---|---|---|
| POST | `/api/kitchen/silence` | Silencia el buzzer de la alerta actual (el LED sigue) | `202` |
| POST | `/api/kitchen/calibrate` | Vuelve a medir la línea base del gas (con aire limpio) | `202` |
| POST | `/api/kitchen/config?temp_max=&gas_delta=` | Cambia umbrales (20–100 °C, 50–3000); se guardan en NVS | `202`, `400` |

### Luces (`/api/lights/*`)

Sección `modules.lights` de `GET /api/status`:

```json
{ "rooms": [true, false, false, false, false], "presence": false,
  "presence_enabled": true, "motion": false, "pir_ready": true }
```

`rooms` son las recámaras 1 a 5. `pir_ready` es `false` durante el calentamiento del PIR.
Detalles en [luces.md](luces.md). Todas requieren `X-API-Token`.

| Método | Ruta | Descripción | Respuesta |
|---|---|---|---|
| POST | `/api/lights/room?n=1..5&state=on\|off\|toggle` | Controla una recámara; devuelve `{room, on}` | `200`, `400` |
| POST | `/api/lights/all?state=on\|off` | Enciende o apaga las 5 recámaras | `200`, `400` |
| POST | `/api/lights/presence?mode=auto\|off` | Activa o desactiva el LED de presencia | `200`, `400` |

### Cochera (`/api/garage/*`)

Sección `modules.garage` de `GET /api/status`:

```json
{ "state": "closed", "position_pct": 0, "left_deg": 0, "right_deg": 180,
  "last_command": "none",
  "calibration": { "left_closed": 0, "left_open": 90, "right_closed": 180, "right_open": 90 } }
```

`state`: `closed`, `opening`, `open` o `closing`. `last_command`: `remote`, `api` o `none`.
Detalles y calibración en [cochera.md](cochera.md). Todas requieren `X-API-Token`.

| Método | Ruta | Descripción | Respuesta |
|---|---|---|---|
| POST | `/api/garage/open` | Abre el portón (también invierte un cierre en curso) | `202` |
| POST | `/api/garage/close` | Cierra el portón (también invierte una apertura en curso) | `202` |
| POST | `/api/garage/servo?l_closed=&l_open=&r_closed=&r_open=` | Calibra los ángulos de cada hoja (0-180); se guardan en NVS | `200`, `400` |

*(Los endpoints de los demás módulos se documentan aquí conforme se agregan.)*
