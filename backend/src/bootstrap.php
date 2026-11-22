<?php
declare(strict_types=1);

/**
 * Carga la configuración, abre la base de datos y ofrece funciones de ayuda.
 * Lo incluyen todos los puntos de entrada (public/*.php).
 */

function config(): array
{
    static $config = null;
    if ($config === null) {
        $path = __DIR__ . '/../config.php';
        if (!is_file($path)) {
            http_response_code(500);
            header('Content-Type: application/json; charset=utf-8');
            echo json_encode(['error' => 'Falta backend/config.php (copia config.example.php)']);
            exit;
        }
        $config = require $path;
        date_default_timezone_set($config['timezone'] ?? 'UTC');
    }
    return $config;
}

function db(): PDO
{
    static $pdo = null;
    if ($pdo === null) {
        $path = config()['db_path'];
        $dir = dirname($path);
        if (!is_dir($dir)) {
            mkdir($dir, 0775, true);
        }
        $pdo = new PDO('sqlite:' . $path, null, null, [
            PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION,
            PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
        ]);
        $pdo->exec('PRAGMA journal_mode = WAL');
        $pdo->exec(file_get_contents(__DIR__ . '/../schema.sql'));
    }
    return $pdo;
}

function jsonResponse(int $code, array $body): never
{
    http_response_code($code);
    header('Content-Type: application/json; charset=utf-8');
    echo json_encode($body, JSON_UNESCAPED_UNICODE);
    exit;
}

/** Exige la cabecera X-API-Token con el valor configurado (comparación en tiempo constante). */
function requireToken(): void
{
    $sent = $_SERVER['HTTP_X_API_TOKEN'] ?? '';
    if (!is_string($sent) || !hash_equals((string) config()['api_token'], $sent)) {
        jsonResponse(401, ['error' => 'token invalido o ausente']);
    }
}
