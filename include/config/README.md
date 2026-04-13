# Config Module Headers
<!-- status: current | validated: 2026-04-08 | source: include/config/ -->

Public interfaces and declarations for the ThemisDB configuration module.

## Table of Contents

1. [Overview](#overview)
2. [Header Files](#header-files)
3. [Key Interfaces](#key-interfaces)
4. [Integration Guide](#integration-guide)
5. [Environment Variables](#environment-variables)
6. [Thread Safety](#thread-safety)

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

## Further Reading

- `src/config/README.md` — implementation overview (`.cpp` files, 60+ path mappings)
- `src/config/ARCHITECTURE.md` — component architecture and data-flow diagrams
- `src/config/ROADMAP.md` — feature roadmap and production readiness checklist
- `src/config/SECURITY.md` — security considerations and audit findings
