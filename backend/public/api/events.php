<?php
declare(strict_types=1);

require __DIR__ . '/../../src/bootstrap.php';

const MAX_BODY_BYTES = 4096;
const ALLOWED_RESULTS = [
    'granted', 'denied', 'learned', 'already_registered', 'card_list_full', 'manual_open',
];

requireToken();

try {
    switch ($_SERVER['REQUEST_METHOD']) {
        case 'POST':
            handlePost();
            // no continúa: handlePost responde y termina
        case 'GET':
            handleGet();
        default:
            jsonResponse(405, ['error' => 'metodo no permitido']);
    }
} catch (PDOException $e) {
    error_log('events.php: ' . $e->getMessage());
    jsonResponse(500, ['error' => 'error de base de datos']);
}

function handlePost(): never
{
    $raw = file_get_contents('php://input', false, null, 0, MAX_BODY_BYTES + 1);
    if ($raw === false || $raw === '') {
        jsonResponse(400, ['error' => 'cuerpo vacio']);
    }
    if (strlen($raw) > MAX_BODY_BYTES) {
        jsonResponse(413, ['error' => 'cuerpo demasiado grande']);
    }
    $data = json_decode($raw, true);
    if (!is_array($data)) {
        jsonResponse(400, ['error' => 'JSON invalido']);
    }

    $device = $data['device'] ?? null;
    $boot = $data['boot'] ?? null;
    $seq = $data['seq'] ?? null;
    $uptime = $data['uptime_s'] ?? null;
    $uid = $data['uid'] ?? '';
    $result = $data['result'] ?? null;

    if (!is_string($device) || !preg_match('/^[A-Za-z0-9_.-]{1,32}$/', $device)) {
        jsonResponse(422, ['error' => 'device invalido']);
    }
    foreach (['boot' => $boot, 'seq' => $seq, 'uptime_s' => $uptime] as $name => $value) {
        if (!is_int($value) || $value < 0) {
            jsonResponse(422, ['error' => "$name debe ser un entero >= 0"]);
        }
    }
    if (!is_string($uid) || ($uid !== '' && !preg_match('/^[0-9A-F]{8,20}$/', $uid))) {
        jsonResponse(422, ['error' => 'uid invalido']);
    }
    if (!is_string($result) || !in_array($result, ALLOWED_RESULTS, true)) {
        jsonResponse(422, ['error' => 'result invalido']);
    }

    $stmt = db()->prepare(
        'INSERT OR IGNORE INTO access_events
            (device_id, boot_id, seq, uptime_s, uid, result, received_at)
         VALUES (:device, :boot, :seq, :uptime, :uid, :result, :received)'
    );
    $stmt->execute([
        ':device' => $device,
        ':boot' => $boot,
        ':seq' => $seq,
        ':uptime' => $uptime,
        ':uid' => $uid,
        ':result' => $result,
        ':received' => date('Y-m-d H:i:s'),
    ]);

    if ($stmt->rowCount() === 0) {
        jsonResponse(200, ['stored' => false, 'duplicate' => true]);
    }
    jsonResponse(201, ['stored' => true]);
}

function handleGet(): never
{
    $limit = isset($_GET['limit']) ? (int) $_GET['limit'] : 50;
    $limit = max(1, min(200, $limit));

    $where = [];
    $params = [];
    if (isset($_GET['result'])) {
        if (!in_array($_GET['result'], ALLOWED_RESULTS, true)) {
            jsonResponse(422, ['error' => 'result invalido']);
        }
        $where[] = 'result = :result';
        $params[':result'] = $_GET['result'];
    }
    if (isset($_GET['uid'])) {
        $uid = strtoupper((string) $_GET['uid']);
        if (!preg_match('/^[0-9A-F]{8,20}$/', $uid)) {
            jsonResponse(422, ['error' => 'uid invalido']);
        }
        $where[] = 'uid = :uid';
        $params[':uid'] = $uid;
    }

    $sql = 'SELECT id, device_id, boot_id, seq, uptime_s, uid, result, received_at
            FROM access_events'
        . ($where ? ' WHERE ' . implode(' AND ', $where) : '')
        . ' ORDER BY id DESC LIMIT ' . $limit;  // $limit ya es un entero acotado

    $stmt = db()->prepare($sql);
    $stmt->execute($params);
    jsonResponse(200, ['events' => $stmt->fetchAll()]);
}
