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
  "firmware": { "name": "smart-home-esp32", "version": "0.1.0" },
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

*(Los endpoints de cada módulo se documentan aquí conforme se agregan.)*

### Cocina

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
si el sensor de temperatura no responde.

| Método | Ruta | Token | Descripción |
|---|---|---|---|
| POST | `/api/kitchen/silence` | sí | Silencia el buzzer de la alerta actual (el LED sigue). `202` |
| POST | `/api/kitchen/calibrate` | sí | Vuelve a medir la línea base del gas. `202` |
| POST | `/api/kitchen/config?temp_max=&gas_delta=` | sí | Cambia umbrales (20–100 °C, 50–3000); se guardan en NVS. `202`, `400` si es inválido |
