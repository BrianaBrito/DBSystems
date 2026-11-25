# Pruebas

## Unitarias (en la PC)

La lógica sin hardware (caseta, filtro de presencia, cocina, luces y cochera) tiene
pruebas con Unity en `test/test_logic`:

```
pio test -e native
```

Necesita un compilador de C++ en la PC. Son 47 pruebas.

## De hardware (lista para la entrega)

Con todo conectado, el ESP32 encendido y el panel abierto en `http://192.168.4.1`.

| # | Módulo | Qué hacer | Resultado esperado |
|---|---|---|---|
| 1 | Base | Abrir `/api/status` | `modules` trae gate, kitchen, lights y garage |
| 2 | Panel | Abrir `/` | Las 5 tarjetas con datos y *en línea* |
| 3 | Panel | Pulsar un botón con token incorrecto | Aviso *Token incorrecto*, nada se mueve |
| 4 | Caseta | Acercar una tarjeta registrada | Barra abre, el vehículo pasa, cierra; evento en la bitácora |
| 5 | Caseta | Acercar una tarjeta no registrada | Sonido de rechazo, la barra no se mueve |
| 6 | Cocina | Acercar un encendedor sin llama al MQ2 | Alerta en el panel, LED y buzzer |
| 7 | Cocina | Pulsar *Silenciar* | El buzzer calla y el LED sigue |
| 8 | Luces | Pulsar R1 a R5 y *Encender/Apagar todas* | Los LEDs coinciden con el panel |
| 9 | Luces | Mover la mano frente al PIR | *Presencia* se enciende y se apaga ~10 s después |
| 10 | Cochera | *Abrir portón* y *Cerrar portón* | El servo recorre 0° a 90° en ~2 s |
| 11 | Cochera | Pulsar el botón del control remoto | Alterna abrir y cerrar; el panel muestra *control remoto* |
| 12 | Todo | Mover el portón con la API y luego con el control | Cada pulsación del control invierte el estado actual |
| 13 | Panel | Pasar una tarjeta registrada y otra no | En *Registro de entradas* aparecen *Acceso permitido* y *Acceso denegado* con su UID |
| 14 | Panel | Pulsar *Ver bitácora completa* | Pide la dirección del servidor la primera vez y abre la bitácora de la laptop |
| 15 | Backend | Apagar el backend y pasar una tarjeta | El portón funciona igual; revisa si los eventos se reenvían al volver el backend |

Anota aquí cualquier ajuste (ángulos del servo, umbral de gas) para reproducirlo.
