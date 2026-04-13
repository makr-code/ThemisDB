<!-- Status: current | validated: 2026-04-08 -->
<!-- Links: README.md · ROADMAP.md · AUDIT.md · SECURITY.md -->

# Config Module — Public Header Architecture

**Version:** 2.0.0
**Last Updated:** 2026-04-08
**Module Path:** `include/config/`

---

## 1. Overview

The `include/config/` directory contains the public C++ headers for the ThemisDB Config module.
All consumer code (server, test, plugin, CLI tool) should include these headers via the
`config/<header>.h` include path.  Implementation (`.cpp`) files are located in `src/config/`.

---

## 2. Public Header Inventory

| Header | Namespace | Key Types | Role |
|--------|-----------|-----------|------|
| `config_path_resolver.h` | `themis::config` | `ConfigPathResolver`, `ConfigEnvironment`, `CacheConfig`, `DeprecationEntry` | Legacy→new path mapping, LRU cache, multi-env overlay |
| `config_schema_validator.h` | `themis::config` | `ConfigSchemaValidator`, `ValidationResult` | JSON Schema Draft 7 subset validation for YAML/JSON |
| `config_encrypted_store.h` | `themis::config` | `ConfigEncryptedStore` | AES-256-GCM encrypted config key-value store |
| `config_audit_log.h` | `themis::config` | `ConfigAuditLog`, `AuditEntry` | Bounded ring-buffer audit trail |
| `config_metrics_exporter.h` | `themis::config` | `ConfigMetricsExporter` | Prometheus text-format exporter |
| `config_file_watcher.h` | `themis::config` | `ConfigFileWatcher` | Filesystem hot-reload watcher |
| `config_errors.h` | `themis::config` | `ConfigException`, `ConfigNotFoundException`, `MappingNotFoundException`, `InvalidPathException`, `ConfigPermissionException` | Typed exception hierarchy |
| `lru_cache.h` | `themis::config` | `LRUCacheWithTTL<K,V>` | Generic LRU cache with per-entry TTL |
| `path_mapping_metadata.h` | `themis::config` | `PathMappingMetadata` | Per-path deprecation/removal metadata |
| `config_migration_scanner_impl.h` | `themis::config` | `ConfigMigrationScanner`, `ScanResult`, `ScanOptions` | Migration scanner inline implementation |

---

## 3. Include Path Convention

All public headers must be included as:

```cpp
#include "config/<header>.h"
```

The `include/` root must be in the compiler's include search path
(added via `include_directories(${CMAKE_SOURCE_DIR}/include)` in CMake).

---

## 4. Dependency Graph

```
config_path_resolver.h
    ├── config_errors.h
    ├── config_audit_log.h
    ├── lru_cache.h
    └── path_mapping_metadata.h

config_schema_validator.h
    └── config_errors.h

config_encrypted_store.h
    └── config_errors.h

config_metrics_exporter.h
    └── (prometheus/registry.h — optional, guarded by THEMIS_HAS_PROMETHEUS)

config_file_watcher.h
    └── (no public dependencies)

config_migration_scanner_impl.h
    ├── config_path_resolver.h
    └── path_mapping_metadata.h
```

---

## 5. ABI Stability

Public types in this directory are part of the ThemisDB stable ABI:

- `ConfigPathResolver` method signatures — stable since v1.0.0
- Exception types in `config_errors.h` — no message-format changes in minor releases
- `ValidationResult` fields — additive-only changes allowed

---

## 6. References

- `src/config/ARCHITECTURE.md` — full component architecture with data-flow diagrams
- `src/config/README.md` — implementation guide (`.cpp` files, 60+ path mappings, usage examples)
- `src/config/ROADMAP.md` — feature roadmap and production readiness checklist
