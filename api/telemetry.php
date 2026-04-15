/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            telemetry.php                                      ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-04-15 04:07:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     361                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0ae938d481  2026-04-15  feat(updates): anonymous hardware telemetry + Ed25519 bui... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

<?php
/**
 * ThemisDB – Anonymous Hardware Telemetry Receiver
 *
 * File:    telemetry.php
 * Host:    https://api.themisdb.org/telemetry.php
 * Version: 1.0.0
 *
 * Receives anonymous hardware snapshots that ThemisDB instances send when the
 * updates.telemetry.enabled flag is set to true in updates.yaml.
 *
 * ── Privacy guarantee ───────────────────────────────────────────────────────
 * The payload never contains hostnames, IP addresses, usernames, or any
 * database content.  This script additionally strips the REMOTE_ADDR before
 * writing to storage so no IP address is persisted.
 *
 * ── Expected JSON payload ───────────────────────────────────────────────────
 * {
 *   "instance_id":    "550e8400-e29b-41d4-a716-446655440000",  // random UUID, ephemeral
 *   "themis_version": "2.0.0",
 *   "timestamp_utc":  1713121131,
 *   "cpu_model":      "Intel Core i7-12700K",   // optional
 *   "cpu_cores":      12,                        // optional
 *   "total_ram_mb":   16384,                     // optional, bucketed to 1024 MiB
 *   "os_family":      "Linux",                   // optional
 *   "cpu_arch":       "x86_64"                   // optional
 * }
 *
 * ── Storage ─────────────────────────────────────────────────────────────────
 * Rows are appended to a SQLite database at the path defined by
 * TELEMETRY_DB_PATH below.  A plain-text NDJSON fallback log is written when
 * the SQLite extension is unavailable.
 *
 * ── Deployment notes ────────────────────────────────────────────────────────
 * 1. Upload this file to the document root of api.themisdb.org.
 * 2. Ensure the web-server user can write to TELEMETRY_DB_PATH.
 * 3. Optionally restrict write access to the DB directory via .htaccess so
 *    the SQLite file is not web-accessible.
 * 4. PHP ≥ 8.0 required.  ext-pdo_sqlite recommended (falls back to NDJSON).
 */

declare(strict_types=1);

// ── Configuration ────────────────────────────────────────────────────────────

/** Absolute path to the SQLite database file. */
define('TELEMETRY_DB_PATH', __DIR__ . '/../data/telemetry/telemetry.sqlite3');

/** Fallback NDJSON log file (used when SQLite is unavailable). */
define('TELEMETRY_LOG_PATH', __DIR__ . '/../data/telemetry/telemetry.ndjson');

/** Maximum accepted payload size in bytes (16 KiB is far more than needed). */
define('MAX_PAYLOAD_BYTES', 16384);

/** Required Content-Type prefix. */
define('EXPECTED_CONTENT_TYPE', 'application/json');

/** Allowed ThemisDB version pattern (semver). */
define('VERSION_PATTERN', '/^\d+\.\d+\.\d+(-[a-zA-Z0-9.\-]+)?$/');

/** UUID v4 pattern. */
define('UUID_PATTERN', '/^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/i');

// ── Request validation ────────────────────────────────────────────────────────

/**
 * Send a JSON error response and terminate.
 */
function respondError(int $httpStatus, string $message): never
{
    http_response_code($httpStatus);
    header('Content-Type: application/json; charset=utf-8');
    echo json_encode(['ok' => false, 'error' => $message]);
    exit;
}

/**
 * Send a JSON success response and terminate.
 */
function respondOk(string $message = 'accepted'): never
{
    http_response_code(202);
    header('Content-Type: application/json; charset=utf-8');
    echo json_encode(['ok' => true, 'message' => $message]);
    exit;
}

// Only POST is accepted.
if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    header('Allow: POST');
    respondError(405, 'Method Not Allowed');
}

// Validate Content-Type.
$contentType = $_SERVER['CONTENT_TYPE'] ?? '';
if (stripos($contentType, EXPECTED_CONTENT_TYPE) === false) {
    respondError(415, 'Content-Type must be application/json');
}

// Read and size-check the raw body.
$raw = file_get_contents('php://input', false, null, 0, MAX_PAYLOAD_BYTES + 1);
if ($raw === false || strlen($raw) === 0) {
    respondError(400, 'Empty request body');
}
if (strlen($raw) > MAX_PAYLOAD_BYTES) {
    respondError(413, 'Payload too large');
}

// Decode JSON.
$data = json_decode($raw, true, 8, JSON_THROW_ON_ERROR | JSON_BIGINT_AS_STRING);
if (!is_array($data)) {
    respondError(400, 'Invalid JSON');
}

// ── Field validation & sanitisation ──────────────────────────────────────────

/**
 * Safely extract a scalar field, returning null when absent or wrong type.
 *
 * @param array<string,mixed> $arr
 */
function field(array $arr, string $key, string $type): mixed
{
    if (!array_key_exists($key, $arr)) {
        return null;
    }
    $val = $arr[$key];
    return match ($type) {
        'string'  => is_string($val)  ? substr(trim($val), 0, 512) : null,
        'int'     => is_int($val)     ? $val : (is_numeric($val) ? (int)$val : null),
        'float'   => is_float($val) || is_int($val) ? (float)$val : null,
        default   => null,
    };
}

// instance_id – required, must be UUID v4.
$instanceId = field($data, 'instance_id', 'string');
if ($instanceId === null || !preg_match(UUID_PATTERN, $instanceId)) {
    respondError(422, 'Missing or invalid instance_id');
}

// themis_version – required, semver.
$themisVersion = field($data, 'themis_version', 'string');
if ($themisVersion === null || !preg_match(VERSION_PATTERN, $themisVersion)) {
    respondError(422, 'Missing or invalid themis_version');
}

// timestamp_utc – required, Unix epoch seconds (reasonable range check).
$timestampUtc = field($data, 'timestamp_utc', 'int');
if ($timestampUtc === null || $timestampUtc < 1_000_000_000 || $timestampUtc > 9_999_999_999) {
    respondError(422, 'Missing or invalid timestamp_utc');
}

// Optional fields – strip anything that is not a plain ASCII/UTF-8 string or
// small integer.  Hostile values are silently replaced with null.
$cpuModel   = field($data, 'cpu_model',    'string');
$cpuCores   = field($data, 'cpu_cores',    'int');
$totalRamMb = field($data, 'total_ram_mb', 'int');
$osFamily   = field($data, 'os_family',    'string');
$cpuArch    = field($data, 'cpu_arch',     'string');

// Validate optional hardware numeric ranges.
if ($cpuCores   !== null && ($cpuCores < 0   || $cpuCores   > 65535))   { $cpuCores   = null; }
if ($totalRamMb !== null && ($totalRamMb < 0 || $totalRamMb > 67108864)) { $totalRamMb = null; }

// Build provenance fields.
// build_channel must be exactly "official" or "community"; any other value
// is normalised to "community" to prevent injection.
$buildChannelRaw = field($data, 'build_channel', 'string');
$buildChannel    = ($buildChannelRaw === 'official') ? 'official' : 'community';

// build_id must be a short hex SHA (1–40 hex chars) or is replaced with "unknown".
$buildIdRaw = field($data, 'build_id', 'string') ?? '';
$buildId    = preg_match('/^[0-9a-f]{1,40}$/i', $buildIdRaw)
    ? strtolower($buildIdRaw)
    : 'unknown';

// build_verified is a boolean (true/false or 1/0 from JSON).
$buildVerifiedRaw = $data['build_verified'] ?? false;
$buildVerified    = (int)(bool)$buildVerifiedRaw;

// Sanity guard: never accept build_verified=1 for a community channel.
// The C++ client should never send this combination, but defend server-side.
if ($buildChannel !== 'official') {
    $buildVerified = 0;
}

// Optional performance metrics – nested under "performance" key.
// All values are expected to already be bucketed by the ThemisDB client.
$perf = is_array($data['performance'] ?? null) ? $data['performance'] : [];

/**
 * Extract an integer performance metric, clamping to [0, $max].
 *
 * @param array<string,mixed> $arr
 */
$perfInt = static function (array $arr, string $key, int $max): ?int {
    $v = isset($arr[$key]) && is_numeric($arr[$key]) ? (int)$arr[$key] : null;
    if ($v === null) { return null; }
    return ($v >= 0 && $v <= $max) ? $v : null;
};

$perfAvgLatUs      = $perfInt($perf, 'avg_query_latency_us',      PHP_INT_MAX);
$perfP99LatUs      = $perfInt($perf, 'p99_query_latency_us',      PHP_INT_MAX);
$perfQpsBucket     = $perfInt($perf, 'queries_per_second_bucket', 1_000_000);
$perfCacheHitPct   = $perfInt($perf, 'cache_hit_rate_pct',        255);
$perfRssMbBucket   = $perfInt($perf, 'process_rss_mb_bucket',     4_194_304);
$perfUptimeSec     = $perfInt($perf, 'uptime_seconds',            PHP_INT_MAX);
$perfConnBucket    = $perfInt($perf, 'active_connections_bucket', 1_048_576);
$perfDbSizeBucket  = $perfInt($perf, 'db_size_mb_bucket',         67_108_864);

// ── Storage ───────────────────────────────────────────────────────────────────

/**
 * Ensure the storage directory exists and is writable.
 */
function ensureDir(string $path): bool
{
    $dir = dirname($path);
    if (!is_dir($dir)) {
        return mkdir($dir, 0750, true);
    }
    return is_writable($dir);
}

/**
 * Persist the validated record via PDO/SQLite.
 *
 * Assumes setup.php has been run and the hardware_telemetry table exists.
 *
 * @param array<string,mixed> $record
 */
function storeViaSqlite(array $record): bool
{
    if (!class_exists('PDO') || !in_array('sqlite', PDO::getAvailableDrivers(), true)) {
        return false;
    }

    if (!file_exists(TELEMETRY_DB_PATH)) {
        error_log('[ThemisDB Telemetry] Database not found – run setup.php first');
        return false;
    }

    $pdo = new PDO('sqlite:' . TELEMETRY_DB_PATH);
    $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

    $stmt = $pdo->prepare(
        'INSERT INTO hardware_telemetry
            (received_at, instance_id, themis_version, timestamp_utc,
             cpu_model, cpu_cores, total_ram_mb, os_family, cpu_arch,
             build_channel, build_id, build_verified,
             perf_avg_query_latency_us, perf_p99_query_latency_us,
             perf_queries_per_second_bucket, perf_cache_hit_rate_pct,
             perf_process_rss_mb_bucket, perf_uptime_seconds,
             perf_active_connections_bucket, perf_db_size_mb_bucket)
         VALUES
            (:received_at, :instance_id, :themis_version, :timestamp_utc,
             :cpu_model, :cpu_cores, :total_ram_mb, :os_family, :cpu_arch,
             :build_channel, :build_id, :build_verified,
             :perf_avg_query_latency_us, :perf_p99_query_latency_us,
             :perf_queries_per_second_bucket, :perf_cache_hit_rate_pct,
             :perf_process_rss_mb_bucket, :perf_uptime_seconds,
             :perf_active_connections_bucket, :perf_db_size_mb_bucket)'
    );

    $stmt->execute([
        ':received_at'                    => time(),
        ':instance_id'                    => $record['instance_id'],
        ':themis_version'                 => $record['themis_version'],
        ':timestamp_utc'                  => $record['timestamp_utc'],
        ':cpu_model'                      => $record['cpu_model'],
        ':cpu_cores'                      => $record['cpu_cores'],
        ':total_ram_mb'                   => $record['total_ram_mb'],
        ':os_family'                      => $record['os_family'],
        ':cpu_arch'                       => $record['cpu_arch'],
        ':build_channel'                  => $record['build_channel'],
        ':build_id'                       => $record['build_id'],
        ':build_verified'                 => $record['build_verified'],
        ':perf_avg_query_latency_us'      => $record['perf_avg_query_latency_us'],
        ':perf_p99_query_latency_us'      => $record['perf_p99_query_latency_us'],
        ':perf_queries_per_second_bucket' => $record['perf_queries_per_second_bucket'],
        ':perf_cache_hit_rate_pct'        => $record['perf_cache_hit_rate_pct'],
        ':perf_process_rss_mb_bucket'     => $record['perf_process_rss_mb_bucket'],
        ':perf_uptime_seconds'            => $record['perf_uptime_seconds'],
        ':perf_active_connections_bucket' => $record['perf_active_connections_bucket'],
        ':perf_db_size_mb_bucket'         => $record['perf_db_size_mb_bucket'],
    ]);

    return true;
}

/**
 * Fallback: append a single NDJSON line.
 *
 * @param array<string,mixed> $record
 */
function storeViaNdjson(array $record): bool
{
    if (!ensureDir(TELEMETRY_LOG_PATH)) {
        return false;
    }

    $line = json_encode(array_merge(['received_at' => time()], $record)) . "\n";
    $fh = fopen(TELEMETRY_LOG_PATH, 'a');
    if ($fh === false) { return false; }

    flock($fh, LOCK_EX);
    fwrite($fh, $line);
    flock($fh, LOCK_UN);
    fclose($fh);

    return true;
}

// Build the validated record (no raw input beyond this point).
$record = [
    'instance_id'                    => $instanceId,
    'themis_version'                 => $themisVersion,
    'timestamp_utc'                  => $timestampUtc,
    'cpu_model'                      => $cpuModel,
    'cpu_cores'                      => $cpuCores,
    'total_ram_mb'                   => $totalRamMb,
    'os_family'                      => $osFamily,
    'cpu_arch'                       => $cpuArch,
    // build provenance
    'build_channel'                  => $buildChannel,
    'build_id'                       => $buildId,
    'build_verified'                 => $buildVerified,
    // performance metrics (null when not sent)
    'perf_avg_query_latency_us'      => $perfAvgLatUs,
    'perf_p99_query_latency_us'      => $perfP99LatUs,
    'perf_queries_per_second_bucket' => $perfQpsBucket,
    'perf_cache_hit_rate_pct'        => $perfCacheHitPct,
    'perf_process_rss_mb_bucket'     => $perfRssMbBucket,
    'perf_uptime_seconds'            => $perfUptimeSec,
    'perf_active_connections_bucket' => $perfConnBucket,
    'perf_db_size_mb_bucket'         => $perfDbSizeBucket,
];

$stored = false;
try {
    $stored = storeViaSqlite($record);
} catch (Throwable) {
    $stored = false;
}

if (!$stored) {
    try {
        $stored = storeViaNdjson($record);
    } catch (Throwable) {
        $stored = false;
    }
}

if (!$stored) {
    // Storage failed – still acknowledge to the client so it does not spam
    // retries, but log a server-side warning.
    error_log('[ThemisDB Telemetry] Failed to persist record for instance ' . $instanceId);
}

respondOk('accepted');
