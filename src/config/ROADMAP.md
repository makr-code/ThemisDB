# Config Module Roadmap
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->
<!-- validated: 2026-03-09 | status: current | source: src/config/ -->

## Current Status
Production-ready for legacy-to-new config path resolution with LRU caching, path validation, deprecation metadata, thread-safe metrics, deprecation warning aggregation, and Prometheus metrics export via the `/metrics` endpoint. Runtime hot-reload and YAML/JSON parsing are out of scope for this module.

## Completed ✅
- [x] Legacy-to-new config path mapping with 60+ path mappings
- [x] Filesystem fallback: tries new path first, then legacy path with deprecation warning
- [x] LRU cache (capacity and TTL configurable via `THEMIS_CONFIG_CACHE_SIZE` / `THEMIS_CONFIG_CACHE_TTL` env vars, defaults: 1000 / 300 s) for resolved paths
- [x] Path traversal prevention and normalization
- [x] Symlink escape detection: rejects symlinks resolving outside the config root
- [x] Deprecation and removal-date metadata per mapped path (all 60+ paths covered)
- [x] Migration guide links per deprecated path
- [x] Thread-safe metrics: hits, misses, cache hits, legacy fallbacks (std::atomic)
- [x] Optional API: `tryResolve()` returning `std::nullopt` on failure
- [x] Typed exception hierarchy for config-related errors
- [x] Coverage of AI/ML, security, compliance, performance, platform, networking, and monitoring categories
- [x] Deprecation warning aggregation report: `deprecationReport()` API, `setAggregationEnabled()`, background reporter thread (Issue: #1659)
- [x] Config audit trail: log which paths were accessed and when — `ConfigAuditLog` with bounded in-memory ring-buffer, `setAuditLogEnabled()` / `auditLog()` / `clearAuditLog()` API (Issue: #1668)
- [x] Automatic legacy path migration script with dry-run mode — `config_migration_scanner` CLI (`--root`, `--output text|json|csv`, `--dry-run`, `--fix`); unit-tested via `tests/test_config_migration_scanner.cpp` (Issue: #1661)
- [x] Configurable LRU cache size and TTL via environment variable (Issue: #1662)
- [x] Runtime hot-reload of resolved path cache on SIGHUP (Issue: #1667)
- [x] Multi-environment config overlay (dev/staging/prod path sets) (Issue: #1669)

## In Progress 🚧
- [I] Migration tooling to batch-rename legacy config files to new paths (Target: Q2 2026) (Issue: #1658)

## Planned Features 📋

### Short-term (Next 3-6 months)

### Long-term (6-12 months)
- [I] Complete removal of all deprecated legacy path mappings (post-migration) (Issue: #1665)
- [x] Integration with config validation (JSON Schema / YAML schema) via `ConfigSchemaValidator` — validates YAML/JSON config files against JSON Schema Draft 7 subset (Issue: #1666)
- [x] `$ref` and `$defs` resolution in `ConfigSchemaValidator` — document-internal `$ref` with JSON Pointer (RFC 6901) walk; supports `$defs` (Draft 2019-09) and `definitions` (Draft 4/6/7); cycle detection; SSRF guard rejects external URI refs (Issue: #3742)

## Implementation Phases

### Phase 1: Legacy Path Resolution and Caching (Status: Completed)
- [x] Built legacy-to-new path mapping table with 60+ entries across all config categories
- [x] Implemented filesystem fallback: tries new path first, emits deprecation warning on fallback
- [x] Implemented LRU cache with capacity 1000 and TTL 5 min for resolved paths
- [x] Added typed exception hierarchy (`ConfigNotFoundException`, `ConfigPathException`, etc.)
- [x] Implemented thread-safe metrics counters using `std::atomic` (hits, misses, legacy fallbacks)

### Phase 2: Security and API Hardening (Status: Completed)
- [x] Added path traversal prevention with `..` normalization and absolute-path rejection
- [x] Added deprecation and removal-date metadata per mapped path
- [x] Added migration guide URL per deprecated path
- [x] Implemented `tryResolve()` optional API returning `std::nullopt` on missing path

### Phase 3: Metadata Completion and Validation Hardening (Status: Completed)
- [x] Complete `METADATA_TABLE` entries for all 60+ mapped paths — every PATH_MAPPING key now has `deprecated_date`, `removal_date`, and `migration_guide_url` (Issue: #1676)
- [x] Harden absolute path validation to reject symlinks outside the config root (Issue: #1677)

### Phase 4: Tooling and Observability (Status: Completed)
- [x] Implement Prometheus metrics exporter for hit rate, miss rate, and legacy fallback rate (Issue: #1670)
- [x] Build deprecation report CLI to scan a deployment and list all legacy paths in use (Issue: #1671)
- [x] Make LRU cache size and TTL configurable via environment variables (`THEMIS_CONFIG_CACHE_SIZE`, `THEMIS_CONFIG_CACHE_TTL`) (Issue: #1672)
- [x] Add multi-environment config overlay support (dev/staging/prod path sets) (Issue: #1673)

## Production Readiness Checklist
- [x] Unit tests coverage > 80% — achieved via `tests/test_config_path_resolver.cpp` (1 339 lines), `tests/test_config_coverage.cpp` (777 lines), `tests/test_config_migration_scanner.cpp` (708 lines), `tests/test_config_schema_validator.cpp` (1 160 lines, including `$ref`/`$defs` tests), `tests/test_config_metrics_scrape.cpp` (metrics scrape latency < 1 ms gate) (Issue: #1674)
- [x] Integration tests (path resolution, LRU cache, fallback, metadata)
- [x] Performance benchmarks (cache hit rate, resolution latency) — `benchmarks/bench_config_path_resolver.cpp` (401 lines, commit 90c733a50) (Issue: #1675); migration scanner throughput (10K files < 5 s) — `benchmarks/bench_config_migration_scanner.cpp` (BM_ScanTree_10K)
- [x] Security audit (path traversal prevention, symlink escape hardening)
- [x] Documentation complete (`src/config/README.md`, `src/config/ARCHITECTURE.md`, `src/config/ROADMAP.md`, `src/config/FUTURE_ENHANCEMENTS.md`)
- [x] API stability guaranteed for ConfigPathResolver

## Known Issues & Limitations
- Does not parse or validate config file contents (YAML/JSON parsing is out of scope)
- Runtime hot-reload via SIGHUP is supported; calling `ConfigPathResolver::registerSighupHandler()` at startup installs the handler
- Secrets management and credential injection are explicitly out of scope
- Migration scanner available: `config_migration_scanner --fix` rewrites legacy path strings in config files in-place (creates `.bak` backups).

## Breaking Changes
- Removal of deprecated legacy path mappings is planned once migration tooling is released and a deprecation window expires
- Cache size and TTL are now configurable via `THEMIS_CONFIG_CACHE_SIZE` and `THEMIS_CONFIG_CACHE_TTL` environment variables (defaults: 1000 entries / 300 s)
