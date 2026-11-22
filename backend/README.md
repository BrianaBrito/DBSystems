# Backend (PHP + SQLite)

Recibe los eventos de acceso del ESP32 y los guarda en una base de datos en la laptop.
Incluye una página para ver la bitácora.

```
ESP32 ──POST /api/events.php (JSON + X-API-Token)──► PHP ──► SQLite (data/smart_home.sqlite)
                                                       └──► index.php (bitácora en el navegador)
```

## Requisitos

- PHP 8.1 o superior con la extensión `pdo_sqlite` (XAMPP la trae activada).
  Comprueba con `php -v` y `php -m` (debe aparecer `pdo_sqlite`).

## Puesta en marcha

1. Copia la configuración y edítala:
   ```powershell
   cd backend
   copy config.example.php config.php
   ```
   En `config.php` pon el mismo token que `API_TOKEN` en `include/secrets.h`.
2. Arranca el servidor escuchando en todas las interfaces (necesario para que el ESP32 llegue):
   ```powershell
   php -S 0.0.0.0:8000 -t public
   ```
   La primera vez Windows pregunta por el firewall: permite el acceso en **redes privadas**.
   Si no apareció el aviso, abre el puerto:
   ```powershell
   New-NetFirewallRule -DisplayName "Smart Home backend" -Direction Inbound -Protocol TCP -LocalPort 8000 -Action Allow -Profile Private
   ```
3. En `include/secrets.h` pon `BACKEND_URL` con la IP de tu laptop, por ejemplo
   `"http://192.168.1.50:8000"` (sin barra final). Obtén la IP con `ipconfig`.
4. Abre `http://localhost:8000/` para ver la bitácora.

### Qué IP poner según cómo conectes todo

| Situación | `BACKEND_URL` |
|---|---|
| Laptop y ESP32 en la misma red (router o hotspot del celular) | IP de la laptop en esa red |
| Laptop conectada al AP `CasaInteligente` del ESP32 | IP de la laptop en la red `192.168.4.x` (normalmente `192.168.4.2`; míralo con `ipconfig`) |

El ESP32 solo puede salir hacia la laptop por su conexión STA (la red a la que se une) o
hacia los equipos conectados a su propio AP.

## API

Todas las rutas exigen la cabecera `X-API-Token`.

| Método | Ruta | Descripción | Respuestas |
|---|---|---|---|
| POST | `/api/events.php` | Guarda un evento | `201` guardado, `200` duplicado, `400/413/422` datos inválidos, `401` token |
| GET | `/api/events.php?limit=50&result=denied&uid=4ED8EF06` | Consulta eventos (más nuevos primero; filtros opcionales) | `200` |

Cuerpo del POST (lo arma el firmware):

```json
{"device":"casa-esp32","boot":123456,"seq":7,"uptime_s":301,"uid":"4ED8EF06","result":"granted"}
```

- `boot` cambia en cada reinicio del ESP32 y `seq` es consecutivo dentro de cada arranque:
  la combinación `(device, boot, seq)` es única, así que reenviar un evento no lo duplica.
- `uid` vacío es válido (apertura manual).
- `result`: `granted`, `denied`, `learned`, `already_registered`, `card_list_full`, `manual_open`.

## Pruebas rápidas

```powershell
$t = "TU_TOKEN"
curl.exe -X POST -H "X-API-Token: $t" -d '{\"device\":\"prueba\",\"boot\":1,\"seq\":1,\"uptime_s\":10,\"uid\":\"4ED8EF06\",\"result\":\"granted\"}' http://localhost:8000/api/events.php
curl.exe -H "X-API-Token: $t" "http://localhost:8000/api/events.php?limit=5"
```

Prueba automática (con el servidor en marcha, desde Git Bash o WSL):

```bash
TOKEN=tu_token BASE=http://127.0.0.1:8000 bash tests/smoke.sh
```

## Decisiones de diseño

- **Consultas preparadas (PDO)**: ningún dato del ESP32 se concatena en SQL.
- **Validación estricta** de cada campo antes de guardar; cuerpo limitado a 4 KB.
- **Token comparado con `hash_equals`** (tiempo constante).
- **`config.php` y `data/` fuera de Git**: ni el token ni la base de datos se suben.
- **`schema.sql` aparte y con SQL estándar**: migrar a MySQL o PostgreSQL solo exige cambiar
  los tipos marcados en comentarios y la cadena de conexión en `src/bootstrap.php`.
- La bitácora del ESP32 usa tiempo desde el arranque; la fecha real la pone el servidor al recibir.

## Límites conocidos

- La página `index.php` no pide contraseña: está pensada para una red local de demostración.
- Si la laptop está apagada, el ESP32 conserva hasta 30 eventos pendientes y descarta los más
  viejos si se llenan. La bitácora local del ESP32 (`GET /api/gate/log`) guarda los últimos 50.
