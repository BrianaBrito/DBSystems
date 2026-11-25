# Mapa de pines — ESP32 DOIT DevKit V1

Fuente única de verdad del cableado. Los mismos valores están en `include/config.h`
(namespace `pins`); si cambias uno, actualiza ambos.

## Tabla de pines

| Función | Componente | GPIO | Alimentación | Notas |
|---|---|---|---|---|
| SCK | RC522 | 18 | 3.3 V | SPI (VSPI) |
| MISO | RC522 | 19 | 3.3 V | |
| MOSI | RC522 | 23 | 3.3 V | |
| SDA / SS | RC522 | 5 | 3.3 V | |
| RST | RC522 | 27 | 3.3 V | |
| Señal servo caseta | Servo 1 | 13 | 5 V externa | GND común con el ESP32 |
| Señal servo portón | Servo 2 | 14 | 5 V externa | GND común con el ESP32 |
| TRIG | HC-SR04 (caseta) | 25 | 5 V | |
| ECHO | HC-SR04 (caseta) | 34 | 5 V | **Divisor de voltaje**; GPIO solo entrada |
| OUT | PIR HC-SR501 (presencia) | 26 | 5 V | Salida de 3.3 V: sin divisor; ver docs/luces.md |
| Datos (S) | KY-015 (DHT11) | 4 | 3.3 V | El módulo ya trae pull-up; pin central = VCC |
| Salida analógica (AO) | MQ2 | 32 | 5 V | ADC1; **divisor de voltaje** (ver docs/cocina.md) |
| Abrir (D0) | RX480E | 36 (VP) | 5 V | **Divisor de voltaje**; solo entrada |
| Cerrar (D1) | RX480E | 39 (VN) | 5 V | **Divisor de voltaje**; solo entrada |
| SER (datos) | 74HC595 | 16 | 3.3 V | LEDs de recámaras y presencia |
| SRCLK (reloj) | 74HC595 | 17 | 3.3 V | |
| RCLK (latch) | 74HC595 | 21 | 3.3 V | |
| LED alerta cocina | LED (otro color) | 22 | — | Directo al GPIO, con resistencia |
| Buzzer pasivo | Buzzer | 33 | — | PWM (`ledc`) |
| LED de estado | LED integrado | 2 | — | Estado del WiFi |

### Salidas del 74HC595

| Salida | Uso |
|---|---|
| Q0 – Q4 | LEDs de recámaras 1 a 5 |
| Q5 | LED de presencia |
| Q6 – Q7 | Libres |

Conexión: OE a GND, SRCLR a 3.3 V, capacitor de 100 nF entre VCC y GND, y una
resistencia de 330 Ω en serie con cada LED. Se alimenta a **3.3 V** (a 5 V no
reconocería bien el nivel lógico del ESP32). Si hace falta más salidas, se puede
encadenar un segundo 595 con los mismos tres pines.

## Notas eléctricas

- **Divisores de voltaje.** El ESP32 tolera 3.3 V en sus entradas. Los ECHO del
  HC-SR04 y las salidas del RX480E son de 5 V: usa 1 kΩ (arriba) y 2 kΩ (abajo).
  Para la salida analógica del MQ2, 10 kΩ y 20 kΩ.
- **Alimentación.** Los servos y el calentador del MQ2 (~150 mA) se alimentan de
  una fuente de 5 V externa (2 A) con el GND unido al del ESP32. Un capacitor de
  100–470 µF cerca de los servos evita reinicios por caídas de voltaje.
- **ADC.** El ADC2 no funciona con WiFi activo: las lecturas analógicas van en
  ADC1 (GPIO 32–39).
- **Pines evitados a propósito:** 0, 2*, 12 (arranque), 6–11 (flash) y 1 y 3 (puerto
  serie). *El GPIO 2 se usa solo como salida del LED integrado, lo cual es seguro una
  vez arrancada la placa.
- **GPIO 15:** pin de arranque; queda libre a propósito.
- **Solo entrada:** GPIO 34, 35, 36 y 39 no tienen pull-up interno ni pueden ser
  salidas.
