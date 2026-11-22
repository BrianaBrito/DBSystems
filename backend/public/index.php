<?php
declare(strict_types=1);

require __DIR__ . '/../src/bootstrap.php';

$labels = [
    'granted' => 'Acceso concedido',
    'denied' => 'Acceso denegado',
    'learned' => 'Tarjeta registrada',
    'already_registered' => 'Ya estaba registrada',
    'card_list_full' => 'Lista llena',
    'manual_open' => 'Apertura manual',
];

$rows = db()->query(
    'SELECT id, device_id, uptime_s, uid, result, received_at
     FROM access_events ORDER BY id DESC LIMIT 50'
)->fetchAll();

$totals = db()->query(
    "SELECT SUM(result = 'granted') AS granted, SUM(result = 'denied') AS denied,
            COUNT(*) AS total FROM access_events"
)->fetch();

function e(string $text): string
{
    return htmlspecialchars($text, ENT_QUOTES, 'UTF-8');
}
?>
<!doctype html>
<html lang="es">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta http-equiv="refresh" content="5">
  <title>Bitácora de accesos</title>
  <style>
    body { font-family: system-ui, sans-serif; margin: 2rem auto; max-width: 52rem; padding: 0 1rem; color: #1f2933; }
    h1 { font-size: 1.4rem; }
    .stats { display: flex; gap: 1rem; margin: 1rem 0; }
    .stat { border: 1px solid #d3d9df; border-radius: 8px; padding: .6rem 1rem; }
    .stat b { display: block; font-size: 1.4rem; }
    table { border-collapse: collapse; width: 100%; }
    th, td { text-align: left; padding: .45rem .6rem; border-bottom: 1px solid #e4e8ec; }
    th { background: #f4f6f8; }
    .granted { color: #1a7f37; } .denied { color: #b42318; }
    small { color: #5f6b76; }
  </style>
</head>
<body>
  <h1>Bitácora de accesos de la caseta</h1>
  <div class="stats">
    <div class="stat"><b><?= (int) $totals['total'] ?></b>Eventos</div>
    <div class="stat"><b><?= (int) $totals['granted'] ?></b>Concedidos</div>
    <div class="stat"><b><?= (int) $totals['denied'] ?></b>Denegados</div>
  </div>
  <table>
    <thead><tr><th>Hora</th><th>Tarjeta</th><th>Resultado</th><th>Dispositivo</th></tr></thead>
    <tbody>
    <?php if (!$rows): ?>
      <tr><td colspan="4">Todavía no hay eventos.</td></tr>
    <?php endif; ?>
    <?php foreach ($rows as $row): ?>
      <tr>
        <td><?= e($row['received_at']) ?></td>
        <td><?= $row['uid'] !== '' ? e($row['uid']) : '—' ?></td>
        <td class="<?= e($row['result']) ?>"><?= e($labels[$row['result']] ?? $row['result']) ?></td>
        <td><?= e($row['device_id']) ?></td>
      </tr>
    <?php endforeach; ?>
    </tbody>
  </table>
  <p><small>Se actualiza cada 5 segundos. La hora es la del servidor al recibir el evento.</small></p>
</body>
</html>
