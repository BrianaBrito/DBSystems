# Panel web

Página única (`data/index.html`, sin librerías externas ni fuentes en línea) que el propio ESP32
sirve desde LittleFS. Funciona sin internet, así que sirve con el AP `CasaInteligente`.

**Estilo:** fondo rosa con tarjeta crema, títulos en serif rojo, tarjetas de colores suaves y
botones en píldora. En la portada hay una casa dibujada en SVG que refleja el estado real:
las ventanas se iluminan con las luces, la puerta de la cochera se abre, la barra de la
caseta sube y aparece un aviso sobre el techo si hay alerta en la cocina. Es responsivo
(se ve bien en celular). Los colores están al inicio del archivo, en `:root`.

## Cómo abrirlo

1. Una vez: PlatformIO → *Upload Filesystem Image* (o `pio run -t uploadfs`), con el monitor
   serie cerrado. Repite solo si cambias algo en `data/`.
2. Conéctate al AP y abre `http://192.168.4.1` (o la IP de la red externa que muestra el monitor serie).
3. Al pulsar el primer botón pide el **token** (el `API_TOKEN` de `secrets.h`). Se guarda
   solo mientras la pestaña esté abierta; el botón *Token* permite cambiarlo.

## Qué muestra y controla

| Tarjeta | Muestra | Botones |
|---|---|---|
| Caseta | Estado de la barra, vehículo, distancia, lector RFID, tarjetas | Abrir, cerrar, registrar tarjeta |
| Cocina | Fase, temperatura, humedad, gas contra umbral, alerta activa | Silenciar, recalibrar gas |
| Luces | Las 5 recámaras, presencia, movimiento, estado del PIR | Cada recámara, todas encendidas o apagadas, LED de presencia automático o apagado |
| Cochera | Estado, posición y ángulo del servo, última orden (remoto o API) | Abrir, cerrar |
| Registro de entradas | Últimos 12 accesos de la caseta: UID, resultado (permitido, denegado, tarjeta registrada, apertura manual) y hace cuánto | Ver bitácora completa |
| Sistema | AP, clientes, red externa, memoria libre | — |

El panel consulta `GET /api/status` cada 1.5 s (se pausa si la pestaña está oculta) y usa
los mismos endpoints POST de [api.md](api.md); no hay una API aparte. Si deja de recibir
respuesta, el indicador pasa a *sin conexión*.

## Registro de entradas

La tarjeta lee `GET /api/gate/log?limit=12` (requiere token) cada 4 s, así que muestra la
bitácora que guarda el ESP32 en memoria: se borra al reiniciarlo y la hora es "hace X"
contando desde el encendido. Si no hay token, muestra un botón para introducirlo.

La bitácora **completa y permanente** está en la base de datos del servidor PHP de la laptop
([backend/README.md](../backend/README.md)). El botón *Ver bitácora completa* abre esa página;
la primera vez pide su dirección (por ejemplo `http://192.168.4.2:8000`) y la recuerda en el
navegador. El enlace *cambiar* permite corregirla.

## Seguridad

- Leer el estado es público; cualquier orden exige el token (`X-API-Token`).
- El token no está en el HTML ni en el firmware servido: lo escribe la persona.
- El panel se sirve por HTTP sin cifrar: úsalo solo en el AP o en una red de confianza.

## Personalizarlo

`data/index.html` es un solo archivo (HTML, CSS y JS). Para un módulo nuevo, agrega una
`<section>` y rellénala en la función `render()` con los campos de su `describe()`.
