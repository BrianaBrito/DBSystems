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
