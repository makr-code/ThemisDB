/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            setup.php                                          ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-04-15 18:00:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     269                                            ║
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
 * ThemisDB – Telemetry Database Setup
 *
 * File:    setup.php
 * Host:    https://api.themisdb.org/setup.php
 * Version: 1.0.0
 *
 * Run this script ONCE after deployment to initialise the SQLite database and
 * create all required tables and indexes.  Subsequent runs are idempotent –
 * existing data is never modified.
 *
 * Usage (CLI):
 *   php api/setup.php
 *
 * Usage (browser – protect with HTTP Basic Auth or IP allowlist in production):
 *   https://api.themisdb.org/setup.php?token=<SETUP_TOKEN>
 *
 * Environment variable / define:
 *   THEMIS_SETUP_TOKEN  – secret token required when called via HTTP.
 *                         Set it in the web-server environment or in a
 *                         .env file loaded before this script.
 *                         CLI invocation skips the token check.
 */

declare(strict_types=1);

// ── Configuration ─────────────────────────────────────────────────────────────

/** Path to the SQLite database file (same value as in telemetry.php). */
define('TELEMETRY_DB_PATH', __DIR__ . '/../data/telemetry/telemetry.sqlite3');

/** Secret token required for browser-based setup (HTTP only). */
define('SETUP_TOKEN', getenv('THEMIS_SETUP_TOKEN') ?: 'change-me-before-deploying');

// ── Access control ────────────────────────────────────────────────────────────

$isCli = (PHP_SAPI === 'cli');

if (!$isCli) {
    $provided = $_GET['token'] ?? '';
    if (!hash_equals(SETUP_TOKEN, $provided)) {
        http_response_code(403);
        header('Content-Type: application/json; charset=utf-8');
        echo json_encode(['ok' => false, 'error' => 'Forbidden – invalid or missing token']);
        exit;
    }
}

// ── Helpers ───────────────────────────────────────────────────────────────────

function out(string $msg): void
{
    if (PHP_SAPI === 'cli') {
        echo $msg . "\n";
    } else {
        echo htmlspecialchars($msg, ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8') . "<br>\n";
    }
}

function fail(string $msg): never
{
    out('ERROR: ' . $msg);
    if (PHP_SAPI !== 'cli') {
        http_response_code(500);
    }
    exit(1);
}

// ── Pre-flight checks ─────────────────────────────────────────────────────────

if (!class_exists('PDO')) {
    fail('PHP PDO extension is not available.');
}
if (!in_array('sqlite', PDO::getAvailableDrivers(), true)) {
    fail('PDO SQLite driver (pdo_sqlite) is not available.');
}

$dbDir = dirname(TELEMETRY_DB_PATH);
if (!is_dir($dbDir)) {
    if (!mkdir($dbDir, 0750, true)) {
        fail('Cannot create database directory: ' . $dbDir);
    }
    out('Created directory: ' . $dbDir);
}
if (!is_writable($dbDir)) {
    fail('Database directory is not writable: ' . $dbDir);
}

// ── Connect ───────────────────────────────────────────────────────────────────

out('Connecting to SQLite database: ' . TELEMETRY_DB_PATH);

try {
    $pdo = new PDO('sqlite:' . TELEMETRY_DB_PATH);
    $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
} catch (PDOException $e) {
    fail('Cannot open database: ' . $e->getMessage());
}

// ── Schema ────────────────────────────────────────────────────────────────────

out('Applying schema …');

$pdo->exec('PRAGMA journal_mode = WAL');
$pdo->exec('PRAGMA foreign_keys = ON');

/**
 * Main telemetry table.
 *
 * Columns:
 *   id             – auto-increment primary key
 *   received_at    – Unix timestamp when this server received the payload
 *   instance_id    – random UUID v4, ephemeral per ThemisDB process lifetime
 *   themis_version – semver string of the sending ThemisDB instance
 *   timestamp_utc  – Unix timestamp reported by the ThemisDB instance itself
 *   cpu_model      – CPU brand string (nullable)
 *   cpu_cores      – logical CPU count (nullable)
 *   total_ram_mb   – total RAM in MiB, bucketed to 1 024 MiB (nullable)
 *   os_family      – "Linux" | "Windows" | "macOS" | "BSD" (nullable)
 *   cpu_arch       – "x86_64" | "aarch64" | … (nullable)
 *
 * Build provenance columns:
 *   build_channel  – "official" | "community"
 *   build_id       – short Git SHA (7 hex chars), e.g. "a1b2c3d"
 *   build_verified – 1 when the Ed25519 signature was valid, 0 otherwise
 *
 * Performance columns (all nullable, present only when include_performance=true):
 *   perf_avg_query_latency_us      – average query latency (µs)
 *   perf_p99_query_latency_us      – P99 query latency (µs)
 *   perf_queries_per_second_bucket – QPS bucketed to nearest power of 2
 *   perf_cache_hit_rate_pct        – cache hit rate 0–100; 255 = unavailable
 *   perf_process_rss_mb_bucket     – process RSS in MiB, 64 MiB buckets
 *   perf_uptime_seconds            – process uptime in seconds
 *   perf_active_connections_bucket – active connections, power-of-2 bucket
 *   perf_db_size_mb_bucket         – on-disk DB size in MiB, 512 MiB buckets
 */
$pdo->exec(
    <<<SQL
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
        -- build provenance
        build_channel  TEXT    NOT NULL DEFAULT 'community',
        build_id       TEXT    NOT NULL DEFAULT 'unknown',
        build_verified INTEGER NOT NULL DEFAULT 0,
        -- performance metrics (bucketed, anonymous)
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
out('  ✓  Table hardware_telemetry (hardware + build provenance + performance columns)');

// ── Migration: add build provenance + performance columns to existing deployments
$perfColumns = [
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

// Retrieve existing column names.
$existingColumns = [];
foreach ($pdo->query('PRAGMA table_info(hardware_telemetry)') as $col) {
    $existingColumns[] = $col['name'];
}

foreach ($perfColumns as $colName => $colType) {
    if (!in_array($colName, $existingColumns, true)) {
        $pdo->exec("ALTER TABLE hardware_telemetry ADD COLUMN {$colName} {$colType}");
        out("  ✓  Migrated: added column {$colName}");
    }
}

// Indexes for common analytical queries.
$pdo->exec(
    <<<SQL
    CREATE INDEX IF NOT EXISTS idx_ht_received_at
        ON hardware_telemetry (received_at)
    SQL
);
out('  ✓  Index idx_ht_received_at');

$pdo->exec(
    <<<SQL
    CREATE INDEX IF NOT EXISTS idx_ht_themis_version
        ON hardware_telemetry (themis_version)
    SQL
);
out('  ✓  Index idx_ht_themis_version');

$pdo->exec(
    <<<SQL
    CREATE INDEX IF NOT EXISTS idx_ht_os_arch
        ON hardware_telemetry (os_family, cpu_arch)
    SQL
);
out('  ✓  Index idx_ht_os_arch');

$pdo->exec(
    <<<SQL
    CREATE INDEX IF NOT EXISTS idx_ht_build_channel
        ON hardware_telemetry (build_channel, build_verified)
    SQL
);
out('  ✓  Index idx_ht_build_channel');

// ── Verify ────────────────────────────────────────────────────────────────────

$row = $pdo->query(
    "SELECT COUNT(*) AS cnt FROM sqlite_master
      WHERE type = 'table' AND name = 'hardware_telemetry'"
)->fetch(PDO::FETCH_ASSOC);

if ((int)($row['cnt'] ?? 0) !== 1) {
    fail('Table hardware_telemetry was not created successfully.');
}

out('');
out('Setup complete.  Database is ready at: ' . TELEMETRY_DB_PATH);

if (!$isCli) {
    http_response_code(200);
    header('Content-Type: text/plain; charset=utf-8');
}
