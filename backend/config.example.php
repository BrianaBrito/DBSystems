<?php
/**
 * Configuración del backend. Copia este archivo como config.php y edítalo.
 * config.php NO se sube a Git (está en .gitignore).
 */
return [
    // Debe ser el mismo valor que API_TOKEN en include/secrets.h del ESP32.
    'api_token' => 'cambia_este_token',

    // Archivo de la base de datos SQLite (la carpeta se crea sola).
    'db_path' => __DIR__ . '/data/smart_home.sqlite',

    // Zona horaria con la que se guarda la hora de recepción.
    'timezone' => 'America/Mexico_City',
];
