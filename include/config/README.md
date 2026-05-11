> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Config Module Headers
<!-- status: current | validated: 2026-05-11 | source: include/config/ -->

Public interfaces and declarations for the ThemisDB configuration module.

**Module Path:** `include/config/`
**Implementation:** `../../src/config/`

## Table of Contents

1. [Overview](#overview)
2. [Header Files](#header-files)
3. [Key Interfaces](#key-interfaces)
4. [Integration Guide](#integration-guide)
5. [Environment Variables](#environment-variables)
6. [Thread Safety](#thread-safety)
7. [Runtime Behavior, Errors, and Limits](#runtime-behavior-errors-and-limits)
8. [Troubleshooting](#troubleshooting)
9. [Further Reading](#further-reading)
10. [Installation](#installation)

## Overview

This directory contains the public API headers for ThemisDB's configuration subsystem.
These headers define the interfaces for legacy-to-new config path resolution, JSON/YAML
schema validation, encrypted config storage, audit logging, Prometheus metrics export,
filesystem hot-reload watching, and the migration scanner CLI tool.

### Key Components

| Header | Key Classes / Types | Purpose |
|--------|---------------------|---------|
| `config_path_resolver.h` | `ConfigPathResolver`, `CacheConfig`, `DeprecationEntry` | Legacy-to-new path mapping with LRU cache and multi-env overlay |
| `config_schema_validator.h` | `ConfigSchemaValidator`, `ValidationResult` | JSON Schema (Draft 7 subset) validation for YAML/JSON config files |
| `config_encrypted_store.h` | `ConfigEncryptedStore` | AES-256-GCM encrypted key-value store with key rotation |
| `config_audit_log.h` | `ConfigAuditLog`, `AuditEntry` | Bounded in-memory audit trail for config path accesses |
| `config_metrics_exporter.h` | `ConfigMetricsExporter` | Prometheus text-format metrics exporter for `/metrics` endpoint |
| `config_file_watcher.h` | `ConfigFileWatcher` | inotify/kqueue/ReadDirectoryChangesW filesystem change watcher |
| `config_errors.h` | `ConfigNotFoundException`, `InvalidPathException`, `MappingNotFoundException`, `ConfigPermissionException` | Typed exception hierarchy for config-related errors |
| `lru_cache.h` | `LRUCacheWithTTL<K,V>` | Generic LRU cache with per-entry TTL eviction |
| `path_mapping_metadata.h` | `PathMappingMetadata` | Deprecation date, removal date, and migration guide URL per mapped path |
| `config_migration_scanner_impl.h` | `ConfigMigrationScanner`, `ScanResult`, `ScanOptions` | Testable inline implementation for the `config_migration_scanner` CLI |

## Header Files

### `config_path_resolver.h`

**Purpose:** Legacy-to-new config path resolution with LRU caching, multi-environment overlay,
deprecation metadata, and Prometheus-compatible metrics.

**Key Types:**
- `ConfigEnvironment` — `DEV`, `STAGING`, `PROD` deployment environment enum
- `CacheConfig` — `capacity` and `ttl_seconds` read from env vars at process startup
- `DeprecationEntry` — `legacy_path`, `hit_count`, `removal_date`, `migration_guide_url`
- `ConfigPathResolver` — static utility; all public methods are thread-safe

**Public API summary:**

| Method | Returns | Description |
|--------|---------|-------------|
| `resolve(path)` | `std::string` | Resolve legacy path; throws `ConfigNotFoundException` on miss |
| `tryResolve(path)` | `std::optional<std::string>` | Non-throwing variant; returns `std::nullopt` on miss |
| `isLegacyPath(path)` | `bool` | True if `path` has an entry in the legacy mapping table |
| `getMetadata(path)` | `std::optional<PathMappingMetadata>` | Deprecation date, removal date, guide URL |
| `legacyPathMappings()` | `const std::map<…>&` | Read-only view of all legacy-to-new mappings |
| `metrics()` | `const Metrics&` | Atomic resolution counters (hits, misses, cache hits, fallbacks) |
| `deprecationReport()` | `std::vector<DeprecationEntry>` | Usage-sorted list of accessed legacy paths |
| `setEnvironment(env)` | `void` | Switch dev/staging/prod overlay (also clears cache) |
| `getEnvironment()` | `ConfigEnvironment` | Currently active deployment environment |
| `currentCacheConfig()` | `CacheConfig` | Active cache capacity and TTL |
| `setCachingEnabled(bool)` | `void` | Disable cache (useful in tests) |
| `resetMetrics()` | `void` | Zero all atomic counters |
| `setAuditLogEnabled(bool)` | `void` | Enable/disable in-memory audit trail |
| `auditLog()` | `std::vector<AuditEntry>` | Snapshot of all recorded audit entries |
| `clearAuditLog()` | `void` | Remove all audit entries |
| `setAuditLogMaxEntries(n)` | `void` | Cap audit log size (default 10,000) |
| `startHotReload(dir)` | `void` | Start inotify/kqueue watcher on `dir` |
| `stopHotReload()` | `void` | Stop background watcher thread |
| `registerSighupHandler()` | `void` | Install SIGHUP handler for cache invalidation |

---

### `config_schema_validator.h`

**Purpose:** Stateless static validator for YAML and JSON configuration files against
JSON Schema (Draft 7 subset). YAML is parsed via `yaml-cpp`; JSON via `nlohmann/json`.

**Key Types:**
- `ValidationResult` — `valid` (bool), `errors` (vector), `formatErrors()` helper
- `ConfigSchemaValidator` — static utility; no instance state

**Supported JSON Schema keywords:**
`type`, `properties`, `required`, `additionalProperties`, `minLength`, `maxLength`,
`pattern`, `format` (`date`, `date-time`, `email`, `uri`, `ipv4`, `ipv6`),
`minimum`, `maximum`, `exclusiveMinimum`, `exclusiveMaximum`, `minItems`, `maxItems`,
`items`, `uniqueItems`, `enum`, `const`, `allOf`, `anyOf`, `oneOf`, `not`,
`$ref` (document-internal JSON Pointer, RFC 6901), `$defs` / `definitions`

**Public API summary:**

| Method | Description |
|--------|-------------|
| `validate(path, schema)` | Validate a YAML/JSON file against an inline schema object |
| `validateWithSchemaFile(config, schema_path)` | Validate against a schema file on disk |
| `validateFromString(content, is_yaml, schema)` | Validate an in-memory YAML or JSON string |
| `loadAsJson(file_path)` | Parse a YAML/JSON file to `nlohmann::json` |
| `loadAsJson(content, is_yaml)` | Parse an in-memory YAML or JSON string |

---

### `config_encrypted_store.h`

**Purpose:** Thread-safe AES-256-GCM encrypted key-value store for sensitive configuration
values (passwords, API keys, connection strings). Each `set()` generates a fresh random
96-bit IV; authentication tags are verified on every `get()`.

**Key Type:** `ConfigEncryptedStore`

**Encryption properties:**

| Property | Value |
|----------|-------|
| Algorithm | AES-256-GCM (NIST SP 800-38D) |
| Key size | 256 bits (32 bytes), randomly generated |
| IV size | 96 bits (12 bytes), per-value random |
| Tag size | 128 bits (16 bytes), verified on every `get()` |

**Public API summary:**

| Method | Description |
|--------|-------------|
| `set(key, value)` | Encrypt and store `value` under `key` |
| `get(key)` | Decrypt and return stored value; throws if key absent or tag invalid |
| `tryGet(key)` | Non-throwing variant; returns `std::nullopt` if key absent |
| `contains(key)` | True if `key` exists in the store |
| `remove(key)` | Delete key and ciphertext |
| `clear()` | Remove all entries |
| `keys()` | Return list of all stored keys |
| `size()` | Number of stored entries |
| `rotateKey()` | Re-encrypt all entries under a new 256-bit key; returns new key version |
| `currentKeyVersion()` | Monotonically increasing rotation counter |
| `serialize()` | Export store to JSON (contains key in plaintext — protect before persisting) |
| `deserialize(json)` | Restore store from a previously serialised snapshot |

---

### `config_audit_log.h`

**Purpose:** Bounded, thread-safe in-memory ring-buffer recording every successful config
path resolution. Disabled by default; opt-in via `ConfigPathResolver::setAuditLogEnabled(true)`.

**Key Types:**
- `AuditEntry` — `requested_path`, `resolved_path`, `timestamp`, `is_legacy`, `is_cache_hit`
- `ConfigAuditLog` — bounded `std::deque<AuditEntry>` guarded by `std::mutex`

---

### `config_metrics_exporter.h`

**Purpose:** Static utility that serialises `ConfigPathResolver` metrics to
Prometheus text-exposition format for the `/metrics` scrape endpoint.

**Exported metrics:**

| Metric | Type | Description |
|--------|------|-------------|
| `themis_config_resolution_hits_total` | counter | Successful path resolutions |
| `themis_config_resolution_misses_total` | counter | Failed resolutions (path not found) |
| `themis_config_legacy_fallbacks_total` | counter | Times legacy path was used as fallback |
| `themis_config_new_path_hits_total` | counter | Times canonical new path was resolved |
| `themis_config_unmapped_requests_total` | counter | Requests for paths with no mapping |
| `themis_config_cache_hits_total` | counter | LRU cache hits |
| `themis_config_cache_misses_total` | counter | LRU cache misses |
| `themis_config_cache_hit_ratio` | gauge | Cache hit ratio 0.0–1.0 |
| `themis_config_cache_size` | gauge | Current entries in cache |
| `themis_config_cache_capacity` | gauge | Maximum cache capacity |
| `themis_config_cache_ttl_seconds` | gauge | Cache TTL in seconds |
| `themis_config_legacy_fallbacks_by_category_total{category}` | counter | Fallbacks per config category |

---

### `config_file_watcher.h`

**Purpose:** Cross-platform filesystem watcher for `.yaml`/`.json` config files.
Invokes a user-supplied callback after a 200 ms debounce window.

**Platform support:** Linux (`inotify`), macOS (`kqueue`), Windows (`ReadDirectoryChangesW`)

**Key Type:** `ConfigFileWatcher` — `start(dir, callback)`, `stop()`

Use `ConfigPathResolver::startHotReload("config")` to wire this into the resolver's
automatic cache flush on file changes.

---

### `config_errors.h`

**Purpose:** Typed exception hierarchy for config-related failures.

| Exception | Thrown When |
|-----------|-------------|
| `ConfigNotFoundException` | Neither new nor legacy path exists on disk |
| `MappingNotFoundException` | No mapping found for a legacy path |
| `InvalidPathException` | Path contains `..` (traversal attempt) or null bytes |
| `ConfigPermissionException` | Filesystem permission denied |
| `SchemaValidationException` | Config or schema file cannot be read/parsed |

All exceptions inherit from `std::runtime_error`.

---

### `lru_cache.h`

**Purpose:** Generic header-only LRU cache with per-entry TTL eviction.
Used internally by `ConfigPathResolver`; re-usable by other modules.

**Key Type:** `LRUCacheWithTTL<K, V>` — `get(key)`, `put(key, value)`, `invalidate()`,
`size()`, `capacity()`

---

### `path_mapping_metadata.h`

**Purpose:** Deprecation and removal-date metadata per mapped legacy path.

**Key Type:** `PathMappingMetadata` — `deprecated_date`, `removal_date`,
`migration_guide_url`, `category`, `isDeprecated()`, `getDeprecationMessage()`

---

### `config_migration_scanner_impl.h`

**Purpose:** Testable inline implementation of the `config_migration_scanner` CLI tool.
Scans a directory tree for files referencing legacy config paths and produces a report.

**Key Types:**
- `ScanOptions` — `root_path`, `output_format` (`text`/`json`/`csv`), `fix_in_place`, `dry_run`
- `ScanResult` — list of `{file_path, legacy_ref, new_ref, removal_date}` findings
- `ConfigMigrationScanner` — `scan(options)` → `ScanResult`

## Key Interfaces

### ConfigPathResolver

```cpp
#include "config/config_path_resolver.h"

// Resolve a legacy config path to its new location
std::string new_path = themis::config::ConfigPathResolver::resolve("ai/llm.yaml");

// Non-throwing variant
auto opt = themis::config::ConfigPathResolver::tryResolve("ai/llm.yaml");
if (opt) { /* use *opt */ }

// Get deprecation metadata for a mapped path
auto meta = themis::config::ConfigPathResolver::getMetadata("ai/llm.yaml");

// Set active deployment environment (dev / staging / prod)
themis::config::ConfigPathResolver::setEnvironment(
    themis::config::ConfigEnvironment::STAGING);

// Retrieve all resolution metrics
auto m = themis::config::ConfigPathResolver::metrics();

// Enable audit trail and retrieve entries
themis::config::ConfigPathResolver::setAuditLogEnabled(true);
auto entries = themis::config::ConfigPathResolver::auditLog();
```

### ConfigSchemaValidator

```cpp
#include "config/config_schema_validator.h"

nlohmann::json schema = R"({ "type": "object", "properties": { "port": { "type": "integer" } } })"_json;

// Validate a YAML/JSON file on disk
auto result = themis::config::ConfigSchemaValidator::validate("config/server.yaml", schema);
if (!result.valid) { /* result.errors */ }

// Validate an in-memory YAML or JSON string
auto r = themis::config::ConfigSchemaValidator::validateFromString(yaml_str, /*is_yaml=*/true, schema);
```

### ConfigEncryptedStore

```cpp
#include "config/config_encrypted_store.h"

themis::config::ConfigEncryptedStore store;
store.set("db.password", "s3cr3t");
std::string val = store.get("db.password");  // decrypts with AES-256-GCM
store.rotateKey();  // zero-downtime key rotation
```

### ConfigAuditLog

```cpp
#include "config/config_audit_log.h"

// Stand-alone usage (not through ConfigPathResolver)
themis::config::ConfigAuditLog log;
log.setEnabled(true);
log.record({"ai/llm.yaml", "config/ai/llm/main.yaml", now, false, true});
auto entries = log.snapshot();
```

### ConfigFileWatcher

```cpp
#include "config/config_file_watcher.h"

// Wire into ConfigPathResolver hot-reload
themis::config::ConfigPathResolver::startHotReload("config");
// …later…
themis::config::ConfigPathResolver::stopHotReload();
```

## Integration Guide

1. Include `${THEMIS_ROOT}/include` in your compiler include path.
2. Link against the config module sources (`src/config/config_path_resolver.cpp`, etc.).
3. Call `ConfigPathResolver::resolve(legacy_path)` wherever a config file path is needed.
4. Optionally call `ConfigPathResolver::registerSighupHandler()` at startup to enable SIGHUP-based cache invalidation.
5. Use `ConfigPathResolver::startHotReload("config")` to activate inotify/kqueue-based auto-reload.

## Environment Variables

| Variable | Default | Valid Range | Description |
|----------|---------|-------------|-------------|
| `THEMIS_CONFIG_CACHE_SIZE` | `1000` | `10`–`100000` | LRU cache capacity |
| `THEMIS_CONFIG_CACHE_TTL` | `300` | `1`–`86400` | Cache entry TTL (seconds) |
| `THEMIS_CONFIG_ENV` | `prod` | `dev`, `staging`, `prod` | Active deployment environment |

## Thread Safety

- `ConfigPathResolver` — static utility; all methods are thread-safe; uses `std::atomic` counters and an internal `std::mutex`-guarded `LRUCacheWithTTL`.
- `ConfigSchemaValidator` — stateless static methods; fully re-entrant.
- `ConfigEncryptedStore` — `std::shared_mutex` allowing concurrent readers and exclusive writers.
- `ConfigAuditLog` — `std::mutex`-guarded ring-buffer; `snapshot()` returns a copy.
- `ConfigFileWatcher` — dedicated background thread; callback is invoked on the watcher thread.
- `LRUCacheWithTTL<K,V>` — internal `std::mutex`; safe for concurrent use.

## Runtime Behavior, Errors, and Limits

### Resolution order (`ConfigPathResolver::resolve`)

1. Normalise path (strip `./`, convert backslashes)
2. Reject `..` components and absolute paths → `InvalidPathException`
3. Check LRU cache → return cached result (records audit entry if enabled)
4. Look up `PATH_MAPPING` table
5. Check new canonical path (`config/<category>/…`) → return if present
6. Check legacy path → return with `spdlog` deprecation warning if present
7. Neither path exists → throw `ConfigNotFoundException`

### Cache eviction

- Entries expire after `THEMIS_CONFIG_CACHE_TTL` seconds (default 300 s) measured from insertion.
- Oldest entry is evicted when the cache reaches `THEMIS_CONFIG_CACHE_SIZE` (default 1000).
- `setEnvironment()` clears the cache entirely to prevent cross-environment contamination.
- `ConfigPathResolver::setCachingEnabled(false)` bypasses the cache for every call (recommended in unit tests).

### Error cases

| Condition | Exception / Behaviour |
|-----------|----------------------|
| Path contains `..` | `InvalidPathException` thrown immediately |
| Path is absolute | `InvalidPathException` thrown immediately |
| Symlink resolves outside config root | `InvalidPathException` thrown |
| No mapping and no file on disk | `ConfigNotFoundException` |
| No mapping entry for legacy path | `MappingNotFoundException` |
| Filesystem permission error | `ConfigPermissionException` |
| `ConfigEncryptedStore::get()` with absent key | `std::out_of_range` |
| `ConfigEncryptedStore::get()` — tampered ciphertext | `std::runtime_error` (auth tag mismatch) |
| `ConfigSchemaValidator` — unreadable config file | `SchemaValidationException` |
| `ConfigSchemaValidator` — external `$ref` URI | validation error (SSRF guard) |
| `ConfigSchemaValidator` — cyclic `$ref` chain | validation error (cycle detected) |

### Limits

| Resource | Default | Configurable |
|----------|---------|-------------|
| LRU cache entries | 1,000 | `THEMIS_CONFIG_CACHE_SIZE` (10–100,000) |
| Cache entry TTL | 300 s | `THEMIS_CONFIG_CACHE_TTL` (1–86,400) |
| Audit log entries | 10,000 | `ConfigPathResolver::setAuditLogMaxEntries(n)` |
| Hot-reload debounce | 200 ms | not configurable |
| `config_migration_scanner` throughput | 10,000 files in < 5 s | — |

## Troubleshooting

### `ConfigNotFoundException` — path not found

1. Confirm the file exists: `ls config/<category>/<file>` and `ls config/<file>` (legacy).
2. Check `ConfigPathResolver::isLegacyPath("your/path")` to see whether a mapping exists.
3. If the file is in `config/dev/` or `config/staging/`, ensure `THEMIS_CONFIG_ENV` is set
   to the matching environment.
4. Run `config_migration_scanner --root .` to identify all legacy path references in source.

### High legacy-fallback rate in Prometheus metrics

- Inspect `themis_config_legacy_fallbacks_by_category_total` to identify the category.
- Retrieve `ConfigPathResolver::deprecationReport()` for a usage-sorted list.
- Run `config_migration_scanner --root /srv/themis --fix` to rewrite references in-place
  (creates `.bak` backups; use `--dry-run` to preview changes first).

### `ConfigEncryptedStore` serialise/deserialise mismatch

The serialised JSON snapshot contains the AES key in plaintext. Always wrap the output of
`serialize()` in a master-key envelope (e.g. age, sops, Vault transit) before writing to
disk or transmitting over a network. Failure to do so exposes the encryption key.

### `ConfigSchemaValidator` — YAML parse error treated as validation error

Invalid YAML (e.g. tab-indented blocks, unterminated strings) is reported as a
`ValidationResult` with `valid = false` and an appropriate error string — it is **not**
thrown. Inspect `result.errors` to see the `yaml-cpp` parse message.

### `ConfigFileWatcher` — no callbacks received on Linux

Verify that inotify is available: `cat /proc/sys/fs/inotify/max_user_watches`.
Increase the limit if it is exhausted: `echo 65536 | sudo tee /proc/sys/fs/inotify/max_user_watches`.

### Cache size / TTL changes have no effect

Environment variables (`THEMIS_CONFIG_CACHE_SIZE`, `THEMIS_CONFIG_CACHE_TTL`,
`THEMIS_CONFIG_ENV`) are read **once at process startup** during static initialisation.
A process restart is required for new values to take effect.

## Further Reading

| Document | Path | Description |
|----------|------|-------------|
| Module README | [`src/config/README.md`](../../src/config/README.md) | Full implementation overview — components, 60+ path mappings, usage examples |
| Architecture | [`src/config/ARCHITECTURE.md`](../../src/config/ARCHITECTURE.md) | Component diagrams, data-flow, threading model |
| Roadmap | [`src/config/ROADMAP.md`](../../src/config/ROADMAP.md) | Feature roadmap and production readiness checklist |
| Future Enhancements | [`src/config/FUTURE_ENHANCEMENTS.md`](../../src/config/FUTURE_ENHANCEMENTS.md) | Planned enhancements with design constraints and test strategies |
| Security | [`src/config/SECURITY.md`](../../src/config/SECURITY.md) | Threat model, security controls, and audit trail |
| Changelog | [`src/config/CHANGELOG.md`](../../src/config/CHANGELOG.md) | Version history and breaking changes |
| Module overview (DE) | [`docs/de/config/README.md`](../../docs/de/config/README.md) | German-language module overview |
| Module overview (EN) | [`docs/en/config/README.md`](../../docs/en/config/README.md) | English module overview |

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
