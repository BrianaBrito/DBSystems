# API HTTP del ESP32

Servidor asíncrono en el puerto 80. Accesible en:

- **Modo AP:** `http://192.168.4.1` (red WiFi propia del ESP32).
- **Modo STA:** la IP que el router asigne (se imprime en el monitor serial).

## Convenciones

- Todas las respuestas son JSON (`Content-Type: application/json`).
- Los errores tienen la forma `{"error": "mensaje"}` con el código HTTP correspondiente.
- Las rutas que **cambian el estado** de la casa exigen la cabecera
  `X-API-Token: <API_TOKEN>` (el valor está en `include/secrets.h`). Sin ella
  responden `401`. Las rutas de lectura son públicas.

## Endpoints

### `GET /api/status`

Estado general del sistema. Cada módulo registrado agrega su sección dentro de `modules`.

```json
{
  "firmware": { "name": "smart-home-esp32", "version": "0.2.0" },
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

*(Los endpoints de los demás módulos se documentan aquí conforme se agregan.)*
