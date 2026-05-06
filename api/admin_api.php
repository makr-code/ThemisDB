<?php
/**
 * ThemisDB Telemetry Admin API
 *
 * Provides authenticated setup + CRUD + analytics endpoints for telemetry data.
 */
declare(strict_types=1);

const TELEMETRY_DB_PATH = __DIR__ . '/../data/telemetry/telemetry.sqlite3';
const DEFAULT_ADMIN_TOKEN = 'change-me-before-deploying';

function jsonResponse(int $status, array $payload): never
{
    http_response_code($status);
    header('Content-Type: application/json; charset=utf-8');
    echo json_encode($payload, JSON_UNESCAPED_SLASHES);
    exit;
}

function fail(int $status, string $message): never
{
    jsonResponse($status, ['ok' => false, 'error' => $message]);
}

function adminToken(): string
{
    $token = getenv('THEMIS_SETUP_TOKEN');
    if ($token === false || $token === '') {
        return DEFAULT_ADMIN_TOKEN;
    }
    return $token;
}

function requireAdminToken(): void
{
    $expected = adminToken();
    $provided = $_SERVER['HTTP_X_ADMIN_TOKEN'] ?? ($_GET['token'] ?? '');
    if (!is_string($provided) || $provided === '') {
        fail(403, 'Missing admin token');
    }
    if (!hash_equals($expected, $provided)) {
        fail(403, 'Invalid admin token');
    }
}

function parseJsonBody(): array
{
    $raw = file_get_contents('php://input');
    if ($raw === false || trim($raw) === '') {
        return [];
    }

    try {
        $decoded = json_decode($raw, true, 32, JSON_THROW_ON_ERROR);
    } catch (Throwable) {
        fail(400, 'Invalid JSON body');
    }

    if (!is_array($decoded)) {
        fail(400, 'JSON body must be an object');
    }
    return $decoded;
}

function ensureSqliteDriver(): void
{
    if (!class_exists('PDO')) {
        fail(500, 'PDO extension is not available');
    }
    if (!in_array('sqlite', PDO::getAvailableDrivers(), true)) {
        fail(500, 'PDO SQLite driver is not available');
    }
}

function ensureDbDirectory(): void
{
    $dbDir = dirname(TELEMETRY_DB_PATH);
    if (!is_dir($dbDir) && !mkdir($dbDir, 0750, true)) {
        fail(500, 'Cannot create telemetry directory');
    }
    if (!is_writable($dbDir)) {
        fail(500, 'Telemetry directory is not writable');
    }
}

function db(): PDO
{
    ensureSqliteDriver();
    ensureDbDirectory();

    $pdo = new PDO('sqlite:' . TELEMETRY_DB_PATH);
    $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
    $pdo->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);
    return $pdo;
}

function applyTelemetrySchema(PDO $pdo): array
{
    $pdo->exec('PRAGMA journal_mode = WAL');
    $pdo->exec('PRAGMA foreign_keys = ON');

    $pdo->exec(
        <<<'SQL'
        CREATE TABLE IF NOT EXISTS hardware_telemetry (
            id             INTEGER PRIMARY KEY AUTOINCREMENT,
            received_at    INTEGER NOT NULL,
            instance_id    TEXT    NOT NULL,
            themis_version TEXT    NOT NULL,
            timestamp_utc  INTEGER NOT NULL,
            cpu_model      TEXT,
            cpu_cores      INTEGER,
            total_ram_mb   INTEGER,
            os_family      TEXT,
            cpu_arch       TEXT,
            build_channel  TEXT    NOT NULL DEFAULT 'community',
            build_id       TEXT    NOT NULL DEFAULT 'unknown',
            build_verified INTEGER NOT NULL DEFAULT 0,
            perf_avg_query_latency_us      INTEGER,
            perf_p99_query_latency_us      INTEGER,
            perf_queries_per_second_bucket INTEGER,
            perf_cache_hit_rate_pct        INTEGER,
            perf_process_rss_mb_bucket     INTEGER,
            perf_uptime_seconds            INTEGER,
            perf_active_connections_bucket INTEGER,
            perf_db_size_mb_bucket         INTEGER
        )
        SQL
    );

    $columns = [
        'build_channel'                  => "TEXT    NOT NULL DEFAULT 'community'",
        'build_id'                       => "TEXT    NOT NULL DEFAULT 'unknown'",
        'build_verified'                 => 'INTEGER NOT NULL DEFAULT 0',
        'perf_avg_query_latency_us'      => 'INTEGER',
        'perf_p99_query_latency_us'      => 'INTEGER',
        'perf_queries_per_second_bucket' => 'INTEGER',
        'perf_cache_hit_rate_pct'        => 'INTEGER',
        'perf_process_rss_mb_bucket'     => 'INTEGER',
        'perf_uptime_seconds'            => 'INTEGER',
        'perf_active_connections_bucket' => 'INTEGER',
        'perf_db_size_mb_bucket'         => 'INTEGER',
    ];

    $existingColumns = [];
    foreach ($pdo->query('PRAGMA table_info(hardware_telemetry)') as $col) {
        $existingColumns[] = (string)$col['name'];
    }

    $migrations = [];
    foreach ($columns as $name => $type) {
        if (!in_array($name, $existingColumns, true)) {
            $pdo->exec("ALTER TABLE hardware_telemetry ADD COLUMN {$name} {$type}");
            $migrations[] = $name;
        }
    }

    $pdo->exec('CREATE INDEX IF NOT EXISTS idx_ht_received_at ON hardware_telemetry (received_at)');
    $pdo->exec('CREATE INDEX IF NOT EXISTS idx_ht_themis_version ON hardware_telemetry (themis_version)');
    $pdo->exec('CREATE INDEX IF NOT EXISTS idx_ht_os_arch ON hardware_telemetry (os_family, cpu_arch)');
    $pdo->exec('CREATE INDEX IF NOT EXISTS idx_ht_build_channel ON hardware_telemetry (build_channel, build_verified)');

    return ['migrated_columns' => $migrations];
}

function normalizePayload(array $data): array
{
    $now = time();
    return [
        'received_at' => isset($data['received_at']) ? (int)$data['received_at'] : $now,
        'instance_id' => substr((string)($data['instance_id'] ?? ''), 0, 128),
        'themis_version' => substr((string)($data['themis_version'] ?? ''), 0, 64),
        'timestamp_utc' => isset($data['timestamp_utc']) ? (int)$data['timestamp_utc'] : $now,
        'cpu_model' => isset($data['cpu_model']) ? substr((string)$data['cpu_model'], 0, 255) : null,
        'cpu_cores' => isset($data['cpu_cores']) ? (int)$data['cpu_cores'] : null,
        'total_ram_mb' => isset($data['total_ram_mb']) ? (int)$data['total_ram_mb'] : null,
        'os_family' => isset($data['os_family']) ? substr((string)$data['os_family'], 0, 64) : null,
        'cpu_arch' => isset($data['cpu_arch']) ? substr((string)$data['cpu_arch'], 0, 64) : null,
        'build_channel' => (isset($data['build_channel']) && $data['build_channel'] === 'official') ? 'official' : 'community',
        'build_id' => isset($data['build_id']) ? substr((string)$data['build_id'], 0, 64) : 'unknown',
        'build_verified' => !empty($data['build_verified']) ? 1 : 0,
        'perf_avg_query_latency_us' => isset($data['perf_avg_query_latency_us']) ? (int)$data['perf_avg_query_latency_us'] : null,
        'perf_p99_query_latency_us' => isset($data['perf_p99_query_latency_us']) ? (int)$data['perf_p99_query_latency_us'] : null,
        'perf_queries_per_second_bucket' => isset($data['perf_queries_per_second_bucket']) ? (int)$data['perf_queries_per_second_bucket'] : null,
        'perf_cache_hit_rate_pct' => isset($data['perf_cache_hit_rate_pct']) ? (int)$data['perf_cache_hit_rate_pct'] : null,
        'perf_process_rss_mb_bucket' => isset($data['perf_process_rss_mb_bucket']) ? (int)$data['perf_process_rss_mb_bucket'] : null,
        'perf_uptime_seconds' => isset($data['perf_uptime_seconds']) ? (int)$data['perf_uptime_seconds'] : null,
        'perf_active_connections_bucket' => isset($data['perf_active_connections_bucket']) ? (int)$data['perf_active_connections_bucket'] : null,
        'perf_db_size_mb_bucket' => isset($data['perf_db_size_mb_bucket']) ? (int)$data['perf_db_size_mb_bucket'] : null,
    ];
}

function tableExists(PDO $pdo): bool
{
    $row = $pdo->query("SELECT COUNT(*) AS c FROM sqlite_master WHERE type='table' AND name='hardware_telemetry'")->fetch();
    return (int)($row['c'] ?? 0) > 0;
}

/**
 * Build WHERE clause + bound params for list/export filters.
 *
 * @return array{whereSql:string,params:array<string,string>}
 */
function buildFilterWhere(array $input): array
{
    $where = [];
    $params = [];
    foreach (['themis_version', 'os_family', 'cpu_arch', 'build_channel'] as $f) {
        if (isset($input[$f]) && $input[$f] !== '') {
            $where[] = "{$f} = :{$f}";
            $params[":{$f}"] = (string)$input[$f];
        }
    }

    return [
        'whereSql' => $where ? ('WHERE ' . implode(' AND ', $where)) : '',
        'params' => $params,
    ];
}

$method = strtoupper($_SERVER['REQUEST_METHOD'] ?? 'GET');
$action = (string)($_GET['action'] ?? 'status');

requireAdminToken();

$pdo = db();

try {
    if ($action === 'setup' && $method === 'POST') {
        $result = applyTelemetrySchema($pdo);
        jsonResponse(200, ['ok' => true, 'message' => 'Setup complete', 'result' => $result]);
    }

    if (!tableExists($pdo)) {
        fail(409, 'Schema not initialized. Call action=setup first.');
    }

    if ($action === 'status' && $method === 'GET') {
        $total = (int)$pdo->query('SELECT COUNT(*) AS c FROM hardware_telemetry')->fetch()['c'];
        $latest = $pdo->query('SELECT MAX(received_at) AS ts FROM hardware_telemetry')->fetch()['ts'];
        jsonResponse(200, [
            'ok' => true,
            'db_path' => TELEMETRY_DB_PATH,
            'db_exists' => file_exists(TELEMETRY_DB_PATH),
            'table_exists' => true,
            'total_records' => $total,
            'latest_received_at' => $latest !== null ? (int)$latest : null,
        ]);
    }

    if ($action === 'list' && $method === 'GET') {
        $limit = max(1, min(200, (int)($_GET['limit'] ?? 25)));
        $offset = max(0, (int)($_GET['offset'] ?? 0));

        $filter = buildFilterWhere($_GET);
        $whereSql = $filter['whereSql'];
        $params = $filter['params'];

        $countStmt = $pdo->prepare("SELECT COUNT(*) AS c FROM hardware_telemetry {$whereSql}");
        $countStmt->execute($params);
        $total = (int)$countStmt->fetch()['c'];

        $sql = "SELECT * FROM hardware_telemetry {$whereSql} ORDER BY id DESC LIMIT :limit OFFSET :offset";
        $stmt = $pdo->prepare($sql);
        foreach ($params as $k => $v) {
            $stmt->bindValue($k, $v, PDO::PARAM_STR);
        }
        $stmt->bindValue(':limit', $limit, PDO::PARAM_INT);
        $stmt->bindValue(':offset', $offset, PDO::PARAM_INT);
        $stmt->execute();

        jsonResponse(200, [
            'ok' => true,
            'total' => $total,
            'limit' => $limit,
            'offset' => $offset,
            'items' => $stmt->fetchAll(),
        ]);
    }

    if (($action === 'export_json' || $action === 'export_csv') && $method === 'GET') {
        $limit = max(1, min(10000, (int)($_GET['limit'] ?? 10000)));

        $filter = buildFilterWhere($_GET);
        $whereSql = $filter['whereSql'];
        $params = $filter['params'];

        $stmt = $pdo->prepare("SELECT * FROM hardware_telemetry {$whereSql} ORDER BY id DESC LIMIT :limit");
        foreach ($params as $k => $v) {
            $stmt->bindValue($k, $v, PDO::PARAM_STR);
        }
        $stmt->bindValue(':limit', $limit, PDO::PARAM_INT);
        $stmt->execute();
        $rows = $stmt->fetchAll();

        $stamp = gmdate('Ymd_His');
        if ($action === 'export_json') {
            http_response_code(200);
            header('Content-Type: application/json; charset=utf-8');
            header('Content-Disposition: attachment; filename="telemetry_export_' . $stamp . '.json"');
            echo json_encode([
                'ok' => true,
                'count' => count($rows),
                'items' => $rows,
            ], JSON_UNESCAPED_SLASHES | JSON_UNESCAPED_UNICODE);
            exit;
        }

        http_response_code(200);
        header('Content-Type: text/csv; charset=utf-8');
        header('Content-Disposition: attachment; filename="telemetry_export_' . $stamp . '.csv"');

        $out = fopen('php://output', 'w');
        if ($out === false) {
            fail(500, 'Could not open output stream for CSV export');
        }

        if (!empty($rows)) {
            fputcsv($out, array_keys($rows[0]));
            foreach ($rows as $row) {
                fputcsv($out, $row);
            }
        } else {
            fputcsv($out, ['id', 'message']);
            fputcsv($out, ['', 'no data']);
        }
        fclose($out);
        exit;
    }

    if ($action === 'get' && $method === 'GET') {
        $id = (int)($_GET['id'] ?? 0);
        if ($id <= 0) {
            fail(400, 'id is required');
        }
        $stmt = $pdo->prepare('SELECT * FROM hardware_telemetry WHERE id = :id');
        $stmt->execute([':id' => $id]);
        $row = $stmt->fetch();
        if (!$row) {
            fail(404, 'Record not found');
        }
        jsonResponse(200, ['ok' => true, 'item' => $row]);
    }

    if ($action === 'create' && $method === 'POST') {
        $payload = normalizePayload(parseJsonBody());
        if ($payload['instance_id'] === '' || $payload['themis_version'] === '') {
            fail(422, 'instance_id and themis_version are required');
        }

        $sql = 'INSERT INTO hardware_telemetry (
                    received_at, instance_id, themis_version, timestamp_utc,
                    cpu_model, cpu_cores, total_ram_mb, os_family, cpu_arch,
                    build_channel, build_id, build_verified,
                    perf_avg_query_latency_us, perf_p99_query_latency_us,
                    perf_queries_per_second_bucket, perf_cache_hit_rate_pct,
                    perf_process_rss_mb_bucket, perf_uptime_seconds,
                    perf_active_connections_bucket, perf_db_size_mb_bucket
                ) VALUES (
                    :received_at, :instance_id, :themis_version, :timestamp_utc,
                    :cpu_model, :cpu_cores, :total_ram_mb, :os_family, :cpu_arch,
                    :build_channel, :build_id, :build_verified,
                    :perf_avg_query_latency_us, :perf_p99_query_latency_us,
                    :perf_queries_per_second_bucket, :perf_cache_hit_rate_pct,
                    :perf_process_rss_mb_bucket, :perf_uptime_seconds,
                    :perf_active_connections_bucket, :perf_db_size_mb_bucket
                )';

        $stmt = $pdo->prepare($sql);
        $stmt->execute($payload);
        $id = (int)$pdo->lastInsertId();
        jsonResponse(201, ['ok' => true, 'id' => $id]);
    }

    if ($action === 'update' && $method === 'PATCH') {
        $id = (int)($_GET['id'] ?? 0);
        if ($id <= 0) {
            fail(400, 'id is required');
        }

        $raw = parseJsonBody();
        $allowed = [
            'cpu_model', 'cpu_cores', 'total_ram_mb', 'os_family', 'cpu_arch',
            'build_channel', 'build_id', 'build_verified', 'themis_version',
            'perf_avg_query_latency_us', 'perf_p99_query_latency_us',
            'perf_queries_per_second_bucket', 'perf_cache_hit_rate_pct',
            'perf_process_rss_mb_bucket', 'perf_uptime_seconds',
            'perf_active_connections_bucket', 'perf_db_size_mb_bucket'
        ];

        $fields = [];
        $params = [':id' => $id];

        foreach ($allowed as $key) {
            if (array_key_exists($key, $raw)) {
                $fields[] = "{$key} = :{$key}";
                $params[":{$key}"] = $raw[$key];
            }
        }

        if (!$fields) {
            fail(400, 'No updatable fields provided');
        }

        $stmt = $pdo->prepare('UPDATE hardware_telemetry SET ' . implode(', ', $fields) . ' WHERE id = :id');
        $stmt->execute($params);

        if ($stmt->rowCount() === 0) {
            fail(404, 'Record not found or unchanged');
        }

        jsonResponse(200, ['ok' => true, 'updated' => true]);
    }

    if ($action === 'delete' && $method === 'DELETE') {
        $id = (int)($_GET['id'] ?? 0);
        if ($id <= 0) {
            fail(400, 'id is required');
        }

        $stmt = $pdo->prepare('DELETE FROM hardware_telemetry WHERE id = :id');
        $stmt->execute([':id' => $id]);
        if ($stmt->rowCount() === 0) {
            fail(404, 'Record not found');
        }

        jsonResponse(200, ['ok' => true, 'deleted' => true]);
    }

    if ($action === 'stats' && $method === 'GET') {
        $total = (int)$pdo->query('SELECT COUNT(*) AS c FROM hardware_telemetry')->fetch()['c'];

        $osRows = $pdo->query('SELECT os_family, COUNT(*) AS c FROM hardware_telemetry GROUP BY os_family ORDER BY c DESC')->fetchAll();
        $versionRows = $pdo->query('SELECT themis_version, COUNT(*) AS c FROM hardware_telemetry GROUP BY themis_version ORDER BY c DESC LIMIT 10')->fetchAll();
        $channelRows = $pdo->query('SELECT build_channel, COUNT(*) AS c FROM hardware_telemetry GROUP BY build_channel ORDER BY c DESC')->fetchAll();

        $perfRow = $pdo->query(
            'SELECT AVG(perf_avg_query_latency_us) AS avg_lat, AVG(perf_p99_query_latency_us) AS avg_p99, AVG(perf_cache_hit_rate_pct) AS avg_cache_hit FROM hardware_telemetry'
        )->fetch();

        jsonResponse(200, [
            'ok' => true,
            'total_records' => $total,
            'by_os' => $osRows,
            'by_version' => $versionRows,
            'by_channel' => $channelRows,
            'performance_overview' => [
                'avg_query_latency_us' => isset($perfRow['avg_lat']) ? (float)$perfRow['avg_lat'] : null,
                'avg_p99_latency_us' => isset($perfRow['avg_p99']) ? (float)$perfRow['avg_p99'] : null,
                'avg_cache_hit_rate_pct' => isset($perfRow['avg_cache_hit']) ? (float)$perfRow['avg_cache_hit'] : null,
            ],
        ]);
    }

    fail(404, 'Unknown action or method');
} catch (PDOException $e) {
    fail(500, 'Database error: ' . $e->getMessage());
}
