# Config Module
<!-- status: current | validated: 2026-05-11 | primary: src/config/ -->

**Module:** `src/config/`
**Date:** 11 May 2026
**Version:** 0.0.34
**Status:** ✅ Production-Ready

---

## Overview

The Config module provides backward-compatible configuration path resolution for ThemisDB.
As the project evolved from a flat configuration layout to a hierarchical directory structure,
this module bridges the two: it maps legacy config paths to their new locations, enabling a
migration window where both old and new paths are valid simultaneously.

**Key capabilities:**
- **Path resolution** — 60+ legacy-to-new path mappings with filesystem fallback
- **LRU cache** — resolved paths are cached; capacity and TTL configurable via environment variables
- **Schema validation** — YAML/JSON config files validated against JSON Schema (Draft 7 subset)
- **Encrypted config storage** — AES-256-GCM key-value store with zero-downtime key rotation
- **Audit trail** — opt-in bounded ring-buffer recording all path accesses
- **Prometheus metrics** — resolution rate, cache hit ratio, legacy fallbacks per category
- **Hot reload** — inotify/kqueue/ReadDirectoryChangesW filesystem watcher with 200 ms debounce
- **Migration scanner** — CLI tool to find and replace legacy path references in a deployment

---

## Components

| File | Role |
|------|------|
| `config_path_resolver.h` / `.cpp` | Core path resolution logic, LRU cache, metrics, multi-env overlay |
| `config_schema_validator.h` / `.cpp` | JSON Schema (Draft 7 subset) validation of YAML/JSON config files |
| `config_encrypted_store.h` / `.cpp` | AES-256-GCM encrypted key-value store with key rotation |
| `config_audit_log.h` / `.cpp` | Bounded in-memory audit trail for config path accesses |
| `config_metrics_exporter.h` / `.cpp` | Prometheus text-format exporter for `/metrics` endpoint |
| `config_file_watcher.h` / `.cpp` | Cross-platform filesystem watcher (inotify / kqueue / ReadDirectoryChangesW) |
| `lru_cache.h` | Generic LRU cache with per-entry TTL eviction (header-only) |
| `path_mapping_metadata.h` | Deprecation date, removal date, migration guide URL per mapped path |
| `config_errors.h` | Typed exception hierarchy for config-related errors |
| `config_migration_scanner_impl.h` | Testable inline implementation of the `config_migration_scanner` CLI |

---

## Quick Start

### Resolve a config path

```cpp
#include "config/config_path_resolver.h"
using namespace themis::config;

// Throws ConfigNotFoundException if neither new nor legacy path exists
std::string path = ConfigPathResolver::resolve("config/lora_training_config.yaml");
// → "config/ai_ml/lora_training_config.yaml" (canonical new path)
// → "config/lora_training_config.yaml" + deprecation warning (legacy fallback)

// Non-throwing variant
auto opt = ConfigPathResolver::tryResolve("config/pii_patterns.yaml");
if (opt) {
    // *opt contains the resolved path
}
```

### Validate a YAML/JSON config file

```cpp
#include "config/config_schema_validator.h"
using namespace themis::config;

nlohmann::json schema = R"({
    "type": "object",
    "required": ["host", "port"],
    "properties": {
        "host": { "type": "string" },
        "port": { "type": "integer", "minimum": 1, "maximum": 65535 }
    }
})"_json;

auto result = ConfigSchemaValidator::validate("config/server.yaml", schema);
if (!result.valid) {
    spdlog::error("Validation failed:\n{}", result.formatErrors());
}
```

### Store and retrieve an encrypted config value

```cpp
#include "config/config_encrypted_store.h"
using namespace themis::config;

ConfigEncryptedStore store;
store.set("db_password", "hunter2");
std::string pw = store.get("db_password");  // decrypts with AES-256-GCM

// Zero-downtime key rotation
uint32_t version = store.rotateKey();
assert(store.get("db_password") == "hunter2");

// Persist (wrap in master-key envelope before writing to disk)
std::string snapshot = store.serialize();
```

### Enable the audit trail

```cpp
ConfigPathResolver::setAuditLogEnabled(true);
std::string path = ConfigPathResolver::resolve("config/pii_patterns.yaml");

for (const auto& entry : ConfigPathResolver::auditLog()) {
    // entry.requested_path, entry.resolved_path, entry.timestamp,
    // entry.is_legacy, entry.is_cache_hit
}
```

### Hot-reload on config file changes

```cpp
// Start inotify/kqueue watcher — flushes the LRU cache on .yaml/.json changes
ConfigPathResolver::startHotReload("config");

// …later, at shutdown…
ConfigPathResolver::stopHotReload();
```

---

## Environment Variables

Read **once at process startup**; a process restart is required to apply changes.

| Variable | Default | Valid Range | Description |
|----------|---------|-------------|-------------|
| `THEMIS_CONFIG_CACHE_SIZE` | `1000` | `10`–`100000` | LRU cache capacity (max cached resolutions) |
| `THEMIS_CONFIG_CACHE_TTL` | `300` | `1`–`86400` | Cache entry TTL in seconds |
| `THEMIS_CONFIG_ENV` | `prod` | `dev`, `staging`, `prod` | Active deployment environment for overlay resolution |

Out-of-range or non-integer values are rejected; a warning is written to `stderr` and the
default is used.

**Example:**

```bash
THEMIS_CONFIG_CACHE_SIZE=5000 THEMIS_CONFIG_CACHE_TTL=60 THEMIS_CONFIG_ENV=dev ./themisdb
```

---

## Multi-Environment Overlay

When the active environment is `DEV` or `STAGING`, the resolver probes an overlay directory
before the standard config root:

| Environment | Overlay root | Activated by |
|-------------|-------------|--------------|
| `DEV` | `config/dev/` | `THEMIS_CONFIG_ENV=dev` |
| `STAGING` | `config/staging/` | `THEMIS_CONFIG_ENV=staging` |
| `PROD` | *(no overlay)* | default |

**Resolution order (example: DEV, path `config/lora_training_config.yaml`):**

1. `config/dev/ai_ml/lora_training_config.yaml` — overlay (checked first)
2. `config/ai_ml/lora_training_config.yaml` — canonical new path
3. `config/lora_training_config.yaml` — legacy fallback (with deprecation warning)

Cache keys include the active environment name to prevent cross-environment cache poisoning.
`setEnvironment()` clears the cache atomically.

---

## Prometheus Metrics

| Metric | Type | Description |
|--------|------|-------------|
| `themis_config_resolution_hits_total` | counter | Successful path resolutions |
| `themis_config_resolution_misses_total` | counter | Failed resolutions (path not found) |
| `themis_config_legacy_fallbacks_total` | counter | Times the legacy path was used as fallback |
| `themis_config_new_path_hits_total` | counter | Times the canonical new path was resolved directly |
| `themis_config_unmapped_requests_total` | counter | Requests for paths with no mapping |
| `themis_config_cache_hits_total` | counter | LRU cache hits |
| `themis_config_cache_misses_total` | counter | LRU cache misses |
| `themis_config_cache_hit_ratio` | gauge | Cache hit ratio, 0.0–1.0 |
| `themis_config_cache_size` | gauge | Current number of entries in cache |
| `themis_config_cache_capacity` | gauge | Maximum cache capacity |
| `themis_config_cache_ttl_seconds` | gauge | Cache entry TTL in seconds |
| `themis_config_legacy_fallbacks_by_category_total{category}` | counter | Legacy fallbacks per config category |

---

## Migration Scanner

```bash
# Text report (default)
config_migration_scanner --root /srv/themis

# JSON report
config_migration_scanner --root /srv/themis --output json

# CSV report
config_migration_scanner --root /srv/themis --output csv

# Dry-run: show what --fix would change
config_migration_scanner --root /srv/themis --dry-run --fix

# Rewrite files in-place (creates .bak backups)
config_migration_scanner --root /srv/themis --fix
```

**Exit codes:** `0` = no overdue paths · `1` = at least one path past its `removal_date` · `2` = argument error

---

## Exception Hierarchy

All exceptions inherit from `std::runtime_error`.

| Exception | Thrown When |
|-----------|-------------|
| `ConfigNotFoundException` | Neither new nor legacy path exists on disk |
| `MappingNotFoundException` | No mapping found for the requested legacy path |
| `InvalidPathException` | Path contains `..` (traversal attempt), null bytes, or is absolute |
| `ConfigPermissionException` | Filesystem permission denied |
| `SchemaValidationException` | Config or schema file cannot be read or parsed |

---

## Thread Safety

| Component | Guarantee |
|-----------|-----------|
| `ConfigPathResolver` | All public methods thread-safe; `std::atomic` metrics, `std::mutex`-guarded LRU cache |
| `ConfigSchemaValidator` | Stateless static methods; fully re-entrant |
| `ConfigEncryptedStore` | `std::shared_mutex`: concurrent readers, exclusive writers |
| `ConfigAuditLog` | `std::mutex`-guarded ring-buffer; `snapshot()` returns a copy |
| `ConfigFileWatcher` | Dedicated background thread; callback invoked on watcher thread |
| `LRUCacheWithTTL<K,V>` | Internal `std::mutex`; safe for concurrent use |

---

## Documentation in this Folder

| File | Description |
|------|-------------|
| [README.md](README.md) | This page — module overview and quick-start |
| [PRIMARY_SOURCES.md](PRIMARY_SOURCES.md) | Auto-generated index of all primary Markdown files |

---

## Further Reading (Primary Sources)

| Document | Path | Description |
|----------|------|-------------|
| Module README | [`src/config/README.md`](../../../src/config/README.md) | Full component description, interfaces, configuration, usage examples |
| Architecture | [`src/config/ARCHITECTURE.md`](../../../src/config/ARCHITECTURE.md) | Component diagrams, data flows, threading model |
| Roadmap | [`src/config/ROADMAP.md`](../../../src/config/ROADMAP.md) | Implementation status and planned features |
| Future Enhancements | [`src/config/FUTURE_ENHANCEMENTS.md`](../../../src/config/FUTURE_ENHANCEMENTS.md) | Detailed planning for future features with design constraints and test strategies |
| Security | [`src/config/SECURITY.md`](../../../src/config/SECURITY.md) | Threat model, security controls, and audit findings |
| Changelog | [`src/config/CHANGELOG.md`](../../../src/config/CHANGELOG.md) | Version history and breaking changes |
| Public Headers | [`include/config/README.md`](../../../include/config/README.md) | Public API headers with per-header documentation |

---

## Related Modules

- Security module — `src/security/` — encryption, TLS, key management
- Observability module — `src/observability/` — Prometheus, Grafana, tracing

## Installation

The config module is included as part of ThemisDB. Add the public headers to your CMake target:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

Build ThemisDB first:

```bash
cmake --preset linux-ninja-release
cmake --build --preset linux-ninja-release
```
