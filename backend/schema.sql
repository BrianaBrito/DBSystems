-- Esquema de la bitácora de accesos (SQLite). Está escrito con SQL estándar para
-- poder migrarlo después a MySQL o PostgreSQL cambiando solo los tipos marcados.
CREATE TABLE IF NOT EXISTS access_events (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,  -- MySQL: AUTO_INCREMENT / PG: SERIAL
    device_id   TEXT    NOT NULL,                   -- qué ESP32 lo envió
    boot_id     INTEGER NOT NULL,                   -- cambia en cada reinicio del ESP32
    seq         INTEGER NOT NULL,                   -- número consecutivo dentro de ese arranque
    uptime_s    INTEGER NOT NULL,                   -- segundos desde el arranque del ESP32
    uid         TEXT    NOT NULL DEFAULT '',        -- tarjeta RFID (vacío en apertura manual)
    result      TEXT    NOT NULL,                   -- granted, denied, learned, ...
    received_at TEXT    NOT NULL,                   -- hora del servidor al recibirlo
    -- Evita duplicados si el ESP32 reintenta un envío que sí llegó.
    UNIQUE (device_id, boot_id, seq)
);

CREATE INDEX IF NOT EXISTS idx_access_events_received ON access_events (received_at);
CREATE INDEX IF NOT EXISTS idx_access_events_uid ON access_events (uid);
