> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Config Module

All notable changes to the Config module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]
- Complete removal of all deprecated legacy path mappings (post-migration) — Issue #1665
- Migration tooling to batch-rename legacy config files to new paths (in progress) — Issue #1658

## [1.8.0] — 2026-03-22
### Added
- `ConfigSchemaValidator` `not` keyword: validates that a value does NOT match the provided sub-schema (JSON Schema Draft 7 §6.7); 8 dedicated tests covering type, enum, complex sub-schemas, and top-level `not`
- `ConfigEncryptedStore` read-path concurrency upgrade: `std::mutex` replaced with `std::shared_mutex`; read operations (`get`, `tryGet`, `contains`, `keys`, `size`, `currentKeyVersion`, `serialize`) now use `std::shared_lock`, allowing concurrent reads without serialisation; write operations (`set`, `remove`, `clear`, `rotateKey`, `deserialize`) retain `std::unique_lock`; `rotateKey` re-encryption holds `unique_lock` for full duration to guarantee atomicity; `ConcurrentReadersDoNotBlockEachOther` test (16 concurrent readers) added to `tests/test_config_encrypted_store.cpp`

## [2.0.0] — 2026-03-09
### Added
- `ConfigSchemaValidator`: JSON Schema Draft 7 subset validation for YAML/JSON config files
- `$ref` and `$defs` resolution in `ConfigSchemaValidator`: document-internal `$ref` with JSON Pointer (RFC 6901); supports `$defs` (Draft 2019-09) and `definitions` (Draft 4/6/7); cycle detection; SSRF guard rejects external URI refs (Issue #3742)
- `format` keyword support: enforces `date`, `date-time`, `email`, `uri`, `ipv4`, `ipv6` patterns (20 dedicated tests)
- `uniqueItems` keyword: rejects arrays with duplicate elements
- `validateFromString(content, is_yaml, schema)`: in-memory YAML/JSON validation without requiring a file; 10 dedicated tests

### Changed
- `ConfigSchemaValidator` now performs full Draft 7 subset validation including `$ref` resolution

## [1.7.0] — 2026-02-15
### Added
- `ConfigAuditLog`: bounded in-memory ring-buffer audit trail logging which config paths were accessed and when; `setAuditLogEnabled()`/`auditLog()`/`clearAuditLog()` API (Issue #1668)
- Encrypted config storage: `ConfigEncryptedStore` with AES-256-GCM and key rotation (`config_encrypted_store.cpp`)
- Multi-environment config overlay: dev/staging/prod path sets (Issue #1669)
- Prometheus metrics exporter for hit rate, miss rate, and legacy fallback rate (`config_metrics_exporter.cpp`) (Issue #1670)

## [1.6.0] — 2026-01-20
### Added
- Automatic legacy path migration script with dry-run mode: `config_migration_scanner` CLI (`--root`, `--output text|json|csv`, `--dry-run`, `--fix`) (Issue #1661)
- Deprecation warning aggregation report: `deprecationReport()` API, `setAggregationEnabled()`, background reporter thread (Issue #1659)
- Runtime hot-reload of resolved path cache on SIGHUP (Issue #1667)
- LRU cache size and TTL configurable via `THEMIS_CONFIG_CACHE_SIZE` and `THEMIS_CONFIG_CACHE_TTL` environment variables (Issue #1662)

## [1.0.0] — 2024-01-01
### Added
- Legacy-to-new config path mapping with 60+ path mappings across AI/ML, security, compliance, performance, platform, networking, and monitoring categories
- Filesystem fallback: tries new path first, then legacy path with deprecation warning
- LRU cache (capacity 1000, TTL 300s) for resolved paths
- Path traversal prevention: `..` normalization and absolute-path rejection
- Symlink escape detection: rejects symlinks resolving outside the config root
- Deprecation and removal-date metadata per mapped path with migration guide links
- Thread-safe metrics (hits, misses, cache hits, legacy fallbacks) via `std::atomic`
- `tryResolve()` returning `std::nullopt` on failure
- Typed exception hierarchy: `ConfigNotFoundException`, `ConfigPathException`, etc.
