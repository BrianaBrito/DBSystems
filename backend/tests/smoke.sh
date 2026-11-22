#!/usr/bin/env bash
# Prueba de humo del backend. Requiere el servidor en marcha:
#   php -S 127.0.0.1:8000 -t public
# Uso: TOKEN=tu_token bash tests/smoke.sh   (BASE opcional, por defecto http://127.0.0.1:8000)
set -u
BASE="${BASE:-http://127.0.0.1:8000}/api/events.php"
TOKEN="${TOKEN:?define TOKEN}"
fail=0

check() { # nombre, código esperado, código real
  if [ "$2" = "$3" ]; then echo "ok   $1"; else echo "FALLA $1 (esperado $2, obtuvo $3)"; fail=1; fi
}
post() { curl -s -o /dev/null -w '%{http_code}' -X POST -H "X-API-Token: $TOKEN" -d "$1" "$BASE"; }

B=$RANDOM
check "sin token"        401 "$(curl -s -o /dev/null -w '%{http_code}' "$BASE")"
check "evento valido"    201 "$(post "{\"device\":\"t\",\"boot\":$B,\"seq\":1,\"uptime_s\":1,\"uid\":\"4ED8EF06\",\"result\":\"granted\"}")"
check "duplicado"        200 "$(post "{\"device\":\"t\",\"boot\":$B,\"seq\":1,\"uptime_s\":1,\"uid\":\"4ED8EF06\",\"result\":\"granted\"}")"
check "uid vacio"        201 "$(post "{\"device\":\"t\",\"boot\":$B,\"seq\":2,\"uptime_s\":1,\"uid\":\"\",\"result\":\"manual_open\"}")"
check "result invalido"  422 "$(post "{\"device\":\"t\",\"boot\":$B,\"seq\":3,\"uptime_s\":1,\"uid\":\"\",\"result\":\"x\"}")"
check "uid con SQL"      422 "$(post "{\"device\":\"t\",\"boot\":$B,\"seq\":4,\"uptime_s\":1,\"uid\":\"1' OR '1\",\"result\":\"granted\"}")"
check "JSON roto"        400 "$(post '{no')"
check "consulta"         200 "$(curl -s -o /dev/null -w '%{http_code}' -H "X-API-Token: $TOKEN" "$BASE?limit=5")"
exit $fail
