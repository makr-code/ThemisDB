> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# Config Module Roadmap
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->
<!-- validated: 2026-04-06 | status: current | source: src/config/ -->

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
- [x] Encrypted config storage with AES-256-GCM and key rotation (Issue: resolved via PR)

### Long-term (6-12 months)
- [I] Complete removal of all deprecated legacy path mappings (post-migration) (Issue: #1665)
- [x] Integration with config validation (JSON Schema / YAML schema) via `ConfigSchemaValidator` — validates YAML/JSON config files against JSON Schema Draft 7 subset (Issue: #1666)
- [x] `$ref` and `$defs` resolution in `ConfigSchemaValidator` — document-internal `$ref` with JSON Pointer (RFC 6901) walk; supports `$defs` (Draft 2019-09) and `definitions` (Draft 4/6/7); cycle detection; SSRF guard rejects external URI refs (Issue: #3742)
- [x] `format` and `uniqueItems` validators in `ConfigSchemaValidator` — `format` enforces `date`, `date-time`, `email`, `uri`, `ipv4`, `ipv6` patterns; `uniqueItems` rejects arrays with duplicate elements; 20 dedicated tests; usage examples in `src/config/README.md` (Milestone: v2.0.0)
- [x] In-memory YAML/JSON validation via `ConfigSchemaValidator::validateFromString(content, is_yaml, schema)` — validates an in-memory YAML or JSON string against a JSON Schema without requiring a file; `loadAsJson(content, is_yaml)` string overload for inline parsing; parse errors reported as `ValidationResult` errors; 10 dedicated tests (Milestone: v2.0.0)
- [x] `not` keyword support in `ConfigSchemaValidator` — asserts that a value does NOT validate against the specified sub-schema; 8 dedicated tests covering type, enum, complex, and top-level `not` schemas (Milestone: v2.0.0)
- [x] `ConfigEncryptedStore` read-path lock upgrade — replaced `std::mutex` (exclusive) with `std::shared_mutex`; `get`, `tryGet`, `contains`, `keys`, `size`, `currentKeyVersion`, `serialize` now use `std::shared_lock` allowing concurrent reads; write operations (`set`, `remove`, `clear`, `rotateKey`, `deserialize`) retain `std::unique_lock`; re-encryption under `rotateKey` holds `unique_lock` for full duration to maintain atomicity (Issue: resolved in v1.8.0)

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

### Phase 5: inotify/kqueue Hot-Reload (Status: Completed – v1.8.0)
- [x] `ConfigFileWatcher`: Linux inotify, macOS kqueue, Windows ReadDirectoryChangesW – watches config directory tree for `.yaml`/`.json` changes
- [x] Wired into `ConfigPathResolver::startHotReload()` / `stopHotReload()` alongside existing SIGHUP path
- [x] 200 ms debounce settling window prevents spurious flushes during editor save-then-rename sequences
- [x] Focused tests in `tests/test_config_file_watcher.cpp`; CI in `.github/workflows/config-file-watcher-ci.yml`

### Phase 6: Schema Composition Completeness and Encrypted Store Concurrency (Status: Completed – v1.8.0)
- [x] `ConfigSchemaValidator` `not` keyword: validates that a value does NOT match the provided sub-schema; 8 dedicated tests in `tests/test_config_schema_validator.cpp`
- [x] `ConfigEncryptedStore` read-path lock upgrade: `std::mutex` replaced with `std::shared_mutex`; concurrent readers via `std::shared_lock`; writes retain `std::unique_lock`; CI in `.github/workflows/config-encrypted-store-lock-upgrade-ci.yml`

## Production Readiness Checklist
- [x] Unit tests coverage > 80% — achieved via `tests/test_config_path_resolver.cpp` (1 339 lines), `tests/test_config_coverage.cpp` (777 lines), `tests/test_config_migration_scanner.cpp` (708 lines), `tests/test_config_schema_validator.cpp` (1 300+ lines, including `$ref`/`$defs` tests, `loadAsJson` string-overload tests, and `not` keyword tests), `tests/test_config_metrics_scrape.cpp` (metrics scrape latency < 1 ms gate), `tests/test_config_encrypted_store.cpp` (encryption, key rotation, serialisation, thread safety, concurrent readers) (Issue: #1674)
- [x] Integration tests (path resolution, LRU cache, fallback, metadata)
- [x] Performance benchmarks (cache hit rate, resolution latency) — `benchmarks/bench_config_path_resolver.cpp` (401 lines, commit 90c733a50) (Issue: #1675); migration scanner throughput (10K files < 5 s) — `benchmarks/bench_config_migration_scanner.cpp` (BM_ScanTree_10K)
- [x] Security audit (path traversal prevention, symlink escape hardening)
- [x] Documentation complete (`src/config/README.md`, `src/config/ARCHITECTURE.md`, `src/config/ROADMAP.md`, `src/config/FUTURE_ENHANCEMENTS.md`)
- [x] API stability guaranteed for ConfigPathResolver

## Known Issues & Limitations
- Does not parse or validate config file contents (YAML/JSON parsing is out of scope)
- Runtime hot-reload via SIGHUP is supported; calling `ConfigPathResolver::registerSighupHandler()` at startup installs the handler
- inotify/kqueue/ReadDirectoryChangesW file-system hot-reload is available; call `ConfigPathResolver::startHotReload("config")` to activate automatic cache flushes on config file changes
- `ConfigEncryptedStore` serialises the AES-256 key in plaintext inside the JSON snapshot — callers must wrap the snapshot in a master-key envelope before writing to disk
- Migration scanner available: `config_migration_scanner --fix` rewrites legacy path strings in config files in-place (creates `.bak` backups).

## Breaking Changes
- Removal of deprecated legacy path mappings is planned once migration tooling is released and a deprecation window expires
- Cache size and TTL are now configurable via `THEMIS_CONFIG_CACHE_SIZE` and `THEMIS_CONFIG_CACHE_TTL` environment variables (defaults: 1000 entries / 300 s)
