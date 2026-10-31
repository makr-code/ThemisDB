---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Config Public Headers

## Scope

This index describes declarations under `include/config/`. The public headers
are the declaration source for resolver, validator, watcher, error, audit,
metrics and encrypted-store interfaces. Runtime behavior is implemented under
`src/config/`; see [`../../src/config/README.md`](../../src/config/README.md)
and [`../../src/config/API.md`](../../src/config/API.md).

## Header inventory

| Header | Main declarations | Role |
|---|---|---|
| `config_path_resolver.h` | `ConfigPathResolver`, `ConfigEnvironment` | Static path lookup, mapping, cache, environment, watcher and audit controls |
| `config_schema_validator.h` | `ConfigSchemaValidator`, `ValidationResult` | JSON/YAML loading and schema diagnostics |
| `config_file_watcher.h` | `ConfigFileWatcher` | Debounced filesystem-change notification |
| `config_encrypted_store.h` | `ConfigEncryptedStore`, `ConfigEncryptedBlob`, store exceptions | In-memory encrypted values, rotation and snapshot serialization |
| `config_errors.h` | `ConfigException` and derived error types | Resolver and schema diagnostic exceptions |
| `config_audit_log.h` | `AuditEntry`, `ConfigAuditLog` | Bounded in-memory path-resolution records |
| `config_metrics_exporter.h` | `ConfigMetricsExporter` | Resolver/cache metrics collection and export |
| `config_contract.h` | `ConfigFailureClass`, contract constants and `isFailClosedClass()` | Intended v1.x cross-component contract; implementation coverage is tracked in `src/config/MODULE_GAPS.md` |
| `lru_cache.h` | `LRUCacheWithTTL<Key, Value>` | Generic synchronized LRU cache with TTL |
| `path_mapping_metadata.h` | `PathMappingMetadata` | Legacy-path deprecation metadata |
| `config_migration_scanner_impl.h` | `cms::ScanMatch` and inline scan/fix helpers | Testable helper used by the migration-scanner tool; not a runtime configuration API |

## Integration notes

- Types are declared in `themis::config`, except the migration-scanner helper,
  which uses namespace `cms`.
- `ConfigPathResolver` state and controls are static and process-wide.
- `ConfigFileWatcher` callback signals changes; it does not reload consumer
  settings.
- `ConfigEncryptedStore::serialize()` includes the encryption key bytes; protect
  its output as secret material.
- A declared contract constant is not by itself evidence that a code path
  enforces that limit. See the implementation gap list.

## Related documentation

- [`../../src/config/API.md`](../../src/config/API.md)
- [`../../src/config/ARCHITECTURE.md`](../../src/config/ARCHITECTURE.md)
- [`../../tests/config/README.md`](../../tests/config/README.md)
- `include/config/*.h`
