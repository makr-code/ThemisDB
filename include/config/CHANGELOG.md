<!-- Status: current | validated: 2026-04-08 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Config Module Public Headers

All notable changes to public headers in `include/config/`.  
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]
- Complete removal of all deprecated legacy path mappings (post-migration) — Issue #1665

## [2.0.0] — 2026-04-08
### Added
- `include/config/` public header directory created; all `.h` files moved from `src/config/` for
  consistency with other ThemisDB modules (auth, analytics, cache, etc.)
- `config_schema_validator.h`: `validateFromString(content, is_yaml, schema)` — in-memory validation
- `config_schema_validator.h`: `not` keyword support; `$ref`/`$defs` resolution; `format`; `uniqueItems`
- `config_encrypted_store.h`: `std::shared_mutex` for concurrent read path (lock upgrade)
- `config_file_watcher.h`: `ConfigFileWatcher` (inotify / kqueue / ReadDirectoryChangesW)

## [1.8.0] — 2026-03-22
### Added
- `config_schema_validator.h`: `not` keyword (JSON Schema Draft 7 §6.7)
- `config_encrypted_store.h`: read-path `std::shared_lock` allowing concurrent readers

## [1.7.0] — 2026-02-15
### Added
- `config_audit_log.h`: `ConfigAuditLog` bounded ring-buffer; `setAuditLogEnabled()` / `auditLog()` / `clearAuditLog()` API
- `config_encrypted_store.h`: AES-256-GCM encrypted key-value store with key rotation
- `config_path_resolver.h`: multi-environment overlay (`ConfigEnvironment`, `setEnvironment()`)
- `config_metrics_exporter.h`: Prometheus metrics export (`collect()`, `updateMetricsCollector()`, `registerWithRegistry()`)

## [1.6.0] — 2026-01-20
### Added
- `config_migration_scanner_impl.h`: `ConfigMigrationScanner` — dry-run / fix mode / JSON/CSV/text output
- `config_path_resolver.h`: `deprecationReport()` and `setAggregationEnabled(bool)`
- `config_path_resolver.h`: `startHotReload()` / `stopHotReload()` for SIGHUP + inotify hot-reload
- `config_path_resolver.h`: `currentCacheConfig()` — `CacheConfig{capacity, ttl_seconds}` observability

## [1.0.0] — 2024-01-01
### Added
- Initial public headers: `config_path_resolver.h`, `config_errors.h`, `lru_cache.h`, `path_mapping_metadata.h`
- 60+ legacy→new path mappings; LRU cache (capacity 1000, TTL 300 s)
- Path traversal prevention; symlink escape hardening
- `tryResolve()` optional API; typed exception hierarchy
