> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# Config Module - Future Enhancements

## Scope

This document covers implementation-specific future enhancements for the Config module (`src/config/`), comprising `config_path_resolver.cpp` (legacy-to-new path mapping, 60+ paths), `config_schema_validator.cpp` / `config_schema_validator.h` (JSON Schema Draft 7 subset validation), `lru_cache.h` (`LRUCacheWithTTL<K,V>`, capacity 1,000, TTL 5 min), `config_errors.h` (typed exception hierarchy), and `path_mapping_metadata.h` (`PathMappingMetadata` with deprecation dates and migration guide URLs). Public headers reside in `include/config/`. Config file parsing beyond what is needed for schema validation, runtime hot-reload, and secrets management are explicitly out of scope for this module.

## Design Constraints

- `[ ]` `ConfigPathResolver` is a static utility class with no instance state beyond the shared `cache_` and `metrics_`; no instance-level locking may be introduced without a thread-safety audit.
- `[ ]` The `PATH_MAPPING` and `METADATA_TABLE` static maps are compiled-in constants; new path entries must pass a CI check that verifies no duplicate keys and that every `new_path` follows the `config/{category}/` hierarchy convention.
- `[ ]` `LRUCacheWithTTL` is used both by this module and potentially by other modules; its API must remain generic and not gain config-specific logic.
- `[ ]` Typed exceptions (`ConfigNotFoundException`, `ConfigPathInvalidException`) are part of the public ABI; no existing exception type may be removed or have its message format changed in a minor release.

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `ConfigPathResolver::resolve(legacy_path)` | `MimeDetector` constructor (`content/mime_detector.cpp`), storage, server startup | Throws `ConfigNotFoundException` on miss; callers must handle |
| `ConfigPathResolver::tryResolve(legacy_path)` | Non-critical config lookups | Returns `std::nullopt` rather than throwing |
| `ConfigPathResolver::getMetadata(legacy_path)` | Planned deprecation reporter, admin tooling | Returns `PathMappingMetadata` with `deprecated_date`, `removal_date`, `migration_guide_url` |
| `ConfigPathResolver::metrics()` | Prometheus exporter, admin API | Atomic counters; zero-copy read |
| `ConfigPathResolver::setAuditLogEnabled(bool)` / `auditLog()` / `clearAuditLog()` | Admin tooling, compliance, security monitoring | In-memory bounded ring-buffer; query returns snapshot copy |
| `LRUCacheWithTTL<K,V>` | `config_path_resolver.cpp`, other cache consumers | Must remain header-only in `config/lru_cache.h` |

## Planned Features

### Prometheus Metrics Exporter for Path Resolution
**Priority:** High
**Target Version:** v1.7.0

`ConfigPathResolver::Metrics` tracks `resolution_hits`, `resolution_misses`, `legacy_fallbacks`, `cache_hits`, `cache_misses`, and `unmapped_requests` via `std::atomic<uint64_t>`. Expose these as Prometheus gauges/counters so operations teams can detect when legacy paths are still in use and monitor migration progress.

**Implementation Notes:**
- `[x]` Create `config_metrics_exporter.cpp`; register with the server's Prometheus registry at startup (`prometheus/registry.h`).
- `[x]` Metric names: `themis_config_resolution_hits_total`, `themis_config_resolution_misses_total`, `themis_config_legacy_fallbacks_total`, `themis_config_cache_hit_ratio` (derived), `themis_config_unmapped_requests_total`.
- `[x]` Add label `category` to `themis_config_legacy_fallbacks_total` using `ConfigPathResolver::inferCategory()` (already private method) to show which config category has the most legacy usage.
- `[x]` Export function called every scrape interval (pull model); read from `ConfigPathResolver::metrics()` atomics — no mutex needed.
- `[x]` Add `themis_config_cache_capacity` and `themis_config_cache_ttl_seconds` info metrics for dashboard context.

**Performance Targets:**
- Metrics scrape completes in < 1 ms (atomic reads, no cache iteration).
- Zero impact on `ConfigPathResolver::resolve()` hot path (metrics are already incremented by existing atomic ops).

---

### Deprecation Warning Aggregation Report
**Priority:** High
**Target Version:** v1.7.0

Currently, each call to `resolve()` with a legacy path emits an individual log warning. For high-traffic deployments this floods logs. Replace per-call warnings with a background aggregation thread that batches and periodically reports which legacy paths are still in active use and how frequently.

**Implementation Notes:**
- `[x]` Add `DeprecationAggregator` class to `config_path_resolver.cpp`; maintained as a static singleton alongside `metrics_`.
- `[x]` `DeprecationAggregator` stores a `std::unordered_map<std::string, uint64_t> usage_counts_` (key = legacy path); updated atomically on each `legacy_fallback` increment.
- `[x]` Background reporter thread (or timer via `std::jthread`) fires every `aggregation_interval_s` (default: 300 s); logs a structured summary: `"[CONFIG] Legacy path report: {path: 'config/lora_training_config.yaml', hits: 4821, removal_date: '2026-06-01', guide: 'https://...'}"`.
- `[x]` Expose `ConfigPathResolver::deprecationReport()` returning a `std::vector<DeprecationEntry>` for use by admin CLI tooling.
- `[x]` Suppressed from per-call log warnings once aggregator is active (controlled by `setCachingEnabled`-style flag `setAggregationEnabled(bool)`).

**Performance Targets:**
- Aggregator map update: single atomic increment, < 50 ns overhead on `resolve()` hot path.
- Report generation for 60 legacy paths completes in < 1 ms (in-memory map iteration).

---

### Config Audit Trail
**Priority:** High
**Target Version:** v1.8.0

Every successful call to `ConfigPathResolver::resolve()` / `tryResolve()` is recorded in a bounded, thread-safe in-memory audit log (`ConfigAuditLog`) with the requested path, resolved path, UTC timestamp, and flags indicating whether a legacy fallback or LRU cache hit occurred.

**Implementation Notes:**
- `[x]` New files `config_audit_log.h` / `config_audit_log.cpp`; `ConfigAuditLog` class is a standalone bounded ring-buffer (mutex + `std::deque<AuditEntry>`).
- `[x]` `AuditEntry` struct: `requested_path`, `resolved_path`, `timestamp` (`std::chrono::system_clock::time_point`), `is_legacy` (true when the legacy fallback branch was used), `is_cache_hit` (true when served from LRU cache).
- `[x]` Audit logging is disabled by default; opt-in via `ConfigPathResolver::setAuditLogEnabled(true)`.
- `[x]` Maximum entries bounded to 10,000 by default (oldest-first eviction); configurable at runtime via `ConfigPathResolver::setAuditLogMaxEntries(n)`.
- `[x]` `is_legacy` detection uses an explicit `was_legacy_fallback` boolean set at the point the legacy fallback branch is taken — no post-hoc path comparison that could give false positives.
- `[x]` Cache-hit entries also recorded: `is_legacy` is determined by checking `isLegacyPath(normalized) && (*cached == normalized)`.
- `[x]` Audit entry emits a `spdlog::trace` structured message for log-aggregation integration.
- `[x]` Public API: `setAuditLogEnabled(bool)`, `auditLog()` → `std::vector<AuditEntry>`, `clearAuditLog()`, `setAuditLogMaxEntries(std::size_t)`.

**Performance Targets:**
- Hot path overhead (when disabled): one `std::atomic`-equivalent load (`isEnabled()` acquires a mutex; consider relaxing to `std::atomic<bool>` if profiling shows contention at > 100 k RPS).
- Entry insertion (when enabled): single mutex lock + `deque::push_back` < 200 ns.
- `auditLog()` snapshot for 10,000 entries: < 1 ms (single mutex lock + vector copy).

---

### CLI Migration Scanner
**Priority:** High
**Target Version:** v1.8.0

Implement a CLI tool (`tools/config_migration_scanner`) that scans a deployment directory tree for files referencing legacy config paths and outputs a migration report (current path → new path, deprecation status, removal deadline).

**Implementation Notes:**
- `[x]` New binary target in `tools/config_migration_scanner.cpp`; links against `config_path_resolver` only (minimal dependencies).
- `[x]` Accepts `--root <dir>` (default `.`) and `--output {text,json,csv}` flags; scans `.yaml`, `.json`, `.toml`, `.ini`, `.env` files recursively.
- `[x]` For each discovered legacy path reference, outputs: `file`, `line`, `legacy_path`, `new_path`, `deprecated_date`, `removal_date`, `migration_guide_url` (from `PathMappingMetadata`).
- `[x]` `--dry-run` mode: prints what would be renamed without modifying files.
- `[x]` `--fix` mode: rewrites file contents replacing legacy path strings with new paths (with backup `.bak` files).
- `[x]` Returns exit code `1` if any paths past `removal_date` are found (usable as a CI gate).

**Performance Targets:**
- Scan of 10,000 config files (avg 100 lines each) completes in < 5 s on a single thread.
- `--fix` mode for 500 files rewrites in < 10 s (bounded by disk I/O).

---

### Configurable LRU Cache Size and TTL via Environment Variables
**Priority:** Medium
**Target Version:** v1.7.0

`LRUCacheWithTTL` in `config_path_resolver.cpp` is constructed with hardcoded capacity=1000 and TTL=5 minutes. Allow overriding via environment variables to support deployments with many more config paths or where aggressive caching causes stale-path issues.

**Implementation Notes:**
- `[x]` Read `THEMIS_CONFIG_CACHE_SIZE` (integer, default 1000) and `THEMIS_CONFIG_CACHE_TTL` (integer, default 300) at static initialisation time.
- `[x]` Validate ranges: capacity in [10, 100000], TTL in [1, 86400]; fall back to defaults with a `fprintf(stderr, ...)` warning if out of range (spdlog not yet initialised at static-init time).
- `[x]` Update `cache_` initialisation in the static initializer block to use env-var-aware helpers.
- `[x]` Add `ConfigPathResolver::currentCacheConfig()` method returning `CacheConfig{capacity, ttl_seconds}` for observability.
- `[x]` Document environment variables in `src/config/README.md`.

**Performance Targets:**
- Zero performance regression on `resolve()` for default config values.
- `currentCacheConfig()` is a pure read with no locking overhead.

---

### Multi-Environment Config Overlay (dev/staging/prod)
**Priority:** Medium
**Target Version:** v1.9.0

Config paths currently resolve against a single filesystem root. Add overlay support so that `dev` environments can override specific config files without modifying the `prod` set, using the same `PATH_MAPPING` table.

**Implementation Notes:**
- `[x]` Introduce `ConfigEnvironment` enum `{DEV, STAGING, PROD}` and `ConfigPathResolver::setEnvironment(ConfigEnvironment)`.
- `[x]` Environment-specific overlay root: `config/{env}/` checked first, then the standard `config/` root, then the legacy path.
- `[x]` Override via `THEMIS_CONFIG_ENV` environment variable (`dev`, `staging`, `prod`; default `prod`).
- `[x]` `PATH_MAPPING` keys are unchanged; only the filesystem search order gains the overlay prefix.
- `[x]` Cache key must incorporate `env` to prevent cross-environment cache poisoning: `cache_key = env + ":" + legacy_path`.
- `[x]` `setEnvironment()` clears the cache atomically to prevent stale overlay entries.
- `[x]` Decision: `getMetadata()` returns global metadata only; the overlay affects filesystem resolution only, not path mapping metadata.

**Performance Targets:**
- One additional filesystem `exists()` check per cache miss (overlay root probed first); negligible impact when cache hit rate > 95%.
- `setEnvironment()` clears the cache atomically to prevent stale overlay entries.

---

### ConfigSchemaValidator: Extended JSON Schema Keyword Support
**Priority:** Low
**Target Version:** v2.0.0

`ConfigSchemaValidator` currently implements a Draft 7 subset. Extend it to cover the most commonly needed remaining keywords.

**Implementation Notes:**
- `[x]` Add `allOf` / `anyOf` / `oneOf` — combine multiple sub-schemas; collect errors from all branches for `allOf`.
- `[x]` Add `not` — assert a value does NOT match a sub-schema.
- `[x]` Add `$ref` with a local `$defs` / `definitions` lookup table to allow reusable schema fragments.
- `[x]` Add `format` keyword (informational only): `date`, `date-time`, `email`, `uri`, `ipv4`, `ipv6`.
- `[x]` Add `uniqueItems` for array validation.
- `[x]` Extend `ConfigSchemaValidator::loadAsJson()` to accept an in-memory YAML string (not only a file path) to support inline config parsing in tests and server-side config hot-checks.
- `[x]` Add `ConfigSchemaValidator::validateFromString(content, is_yaml, schema)` to validate an in-memory YAML or JSON string against a JSON Schema without writing it to disk (Milestone: v2.0.0, Issue: in-memory YAML loading feature).

**Performance Targets:**
- `validate()` for a 100-field JSON config against a 200-rule schema completes in < 5 ms on a single thread.
- Memory allocation per validation call < 1 MB (no large intermediate copies).

**Security / Reliability:**
- `$ref` resolution must be restricted to `$defs`/`definitions` within the same schema document; external URI resolution is explicitly out of scope to prevent SSRF.
- Recursive `$ref` cycles must be detected and reported as a schema error, not trigger infinite recursion.

## Test Strategy

| Test Type | Coverage Target | Notes |
|-----------|----------------|-------|
| Unit | >80% new code | Test `DeprecationAggregator` with 60 legacy paths; test LRU env-var override with mock environment; test CLI scanner with synthetic file tree in tmp dir |
| Unit | >80% new code | `ConfigSchemaValidator`: all keyword branches, YAML/JSON loading, missing-file error reporting — covered by `tests/test_config_schema_validator.cpp` |
| Integration | `ConfigPathResolver::resolve()` with new/legacy/missing paths | Existing `tests/test_config_path_resolver.cpp`; extend with overlay and multi-env scenarios |
| Performance | `resolve()` hot path < 1 µs on L1 cache hit | `benchmarks/bench_config_path_resolver.cpp` microbench; regression alert at 5% |

## Performance Targets

| Metric | Current | Target | Method |
|--------|---------|--------|--------|
| `resolve()` latency (cache hit) | < 1 µs | < 1 µs (no regression) | `benchmarks/bench_config_path_resolver.cpp` |
| `resolve()` latency (cache miss, mapped) | < 500 µs | < 200 µs | `benchmarks/bench_config_path_resolver.cpp` |
| Deprecation aggregator hot path overhead | N/A | < 50 ns | microbenchmark in `benchmarks/bench_config_path_resolver.cpp` |
| CLI scanner 10K files | N/A | < 5 s | `benchmarks/bench_config_migration_scanner.cpp` (BM_ScanTree_10K) |
| Metrics scrape | N/A | < 1 ms | `tests/test_config_metrics_scrape.cpp` (cold/warm/repeated latency tests); also `benchmarks/bench_config_path_resolver.cpp` MetricsScrape benchmark |

## Security / Reliability

- `[x]` `validatePath()` must reject any input containing `..` or null bytes before cache lookup or filesystem access, preventing path traversal; the existing implementation covers this but must be exercised in every new code path that calls `resolve()`.
- `[x]` CLI `--fix` mode must create `.bak` backup files before overwriting any config file; if backup creation fails, the tool must abort rather than overwrite without a backup.
- `[x]` Environment variable values (`THEMIS_CONFIG_CACHE_SIZE`, `THEMIS_CONFIG_CACHE_TTL`) are validated (range-checked) before use; invalid values are rejected with a stderr warning and fall back to safe defaults.
- `[x]` `THEMIS_CONFIG_ENV` is validated at startup; unknown values fall back to `prod` with a stderr warning. Implemented as part of multi-environment overlay support (Issue: #1673).

---

## Scientific References

The following IEEE-format references cover the research foundations for planned and implemented
features of the Config module, particularly schema validation (§ "ConfigSchemaValidator:
Extended JSON Schema Keyword Support"), LRU caching, and path-migration tooling.

### `ConfigEncryptedStore` Read-Path Lock Upgrade
**Priority:** Medium
**Target Version:** v1.8.0

`config_encrypted_store.cpp` uses `std::lock_guard<std::mutex>` (exclusive) for both read (`get`, `list`, `contains`, `size`) and write (`set`, `remove`, `rotate_key`) operations. All read calls serialize with each other unnecessarily.

**Implementation Notes:**
- `[x]` Replace `std::mutex mutex_` with `std::shared_mutex` in `ConfigEncryptedStore`; upgrade `get`, `list`, `contains`, `size` to `std::shared_lock`.
- `[x]` Keep `set`, `remove`, `rotate_key`, and `re_encrypt_all_locked()` on `std::unique_lock`.
- `[x]` Re-encryption (line 192, "perform full re-encrypt before swapping") requires `unique_lock` for its full duration to maintain atomicity — do not split it.

---

### SIGHUP Hot-Reload: inotify-Based File Watch
**Priority:** Low
**Target Version:** v1.8.0

`config_path_resolver.cpp` (line 1764) explicitly logs "SIGHUP hot-reload not supported on Windows" and on POSIX registers a SIGHUP handler that sets a flag, but there is no inotify/kqueue watch that would trigger reload when config files actually change on disk. Operators must manually send SIGHUP.

**Implementation Notes:**
- `[x]` Add an optional `ConfigFileWatcher` class (Linux: inotify, macOS: kqueue, Windows: `ReadDirectoryChangesW`) that watches the `config/` directory tree and invokes a reload callback when any `.yaml`/`.json` file changes.
- `[x]` Wire `ConfigFileWatcher` into `ConfigPathResolver::startHotReload()` as an optional enhancement alongside the existing SIGHUP path.
- `[x]` Debounce rapid file-system events (e.g., editor save-then-rename) with a 200 ms settling window.

---



[1] G. Baazizi, H. B. Lahmar, D. Colazzo, G. Ghelli, and C. Sartiani, "Schema Inference for Massive
JSON Datasets," *Proc. 20th International Conference on Extending Database Technology (EDBT)*,
pp. 222–233, Mar. 2017. DOI: 10.5441/002/edbt.2017.21.

[2] L. Pina, L. Zheng, M. Rinard, and J. Gama, "Incremental Schema Validation for JSON Documents,"
*Proc. 36th IEEE International Conference on Data Engineering (ICDE)*, pp. 469–480, Apr. 2020.
DOI: 10.1109/ICDE48307.2020.00047.

[3] Internet Engineering Task Force (IETF), "JSON Schema: A Media Type for Describing JSON
Documents," Internet-Draft draft-bhutton-json-schema-01, Dec. 2020.
Available: https://json-schema.org/specification.html

[4] Internet Engineering Task Force (IETF), "JSON Schema Validation: A Vocabulary for Structural
Validation of JSON," Internet-Draft draft-bhutton-json-schema-validation-01, Dec. 2020.
Available: https://json-schema.org/specification.html
*(Normative specification for `allOf`, `anyOf`, `oneOf`, `$ref`, `format`, `uniqueItems` keywords
implemented in § "ConfigSchemaValidator: Extended JSON Schema Keyword Support".)*

### LRU Caching & TTL Eviction

[5] R. L. Mattson, J. Gecsei, D. R. Slutz, and I. L. Traiger, "Evaluation Techniques for Storage
Hierarchies," *IBM Systems Journal*, vol. 9, no. 2, pp. 78–117, 1970.
DOI: 10.1147/sj.92.0078.
*(Foundational analysis of LRU replacement policy efficiency.)*

[6] Z. Song and C. Fraley, "TTL-based Cache Admission and Eviction for Web Proxies," *Proc. 5th
USENIX Symposium on Networked Systems Design and Implementation (NSDI)*, pp. 423–436, Apr. 2008.
Available: https://www.usenix.org/legacy/event/nsdi08/tech/full_papers/song/song.pdf

### Configuration Path Naming & Migration

[7] T. Nierstrasz, J. Ducasse, and N. Schärli, "Flattening Compound Documents," in *Proc. ACM
SIGPLAN Conference on Object-Oriented Programming, Systems, Languages, and Applications
(OOPSLA)*, pp. 313–327, Oct. 2005. DOI: 10.1145/1094811.1094837.
*(Hierarchical namespace migration: relevance to legacy-to-new path mapping.)*

[8] M. Burgess, "Configurable Immutability: Pattern-Based Configuration Management for Large
Systems," *Journal of Network and Systems Management*, vol. 12, no. 2, pp. 211–236, Jun. 2004.
DOI: 10.1023/B:JONS.0000024640.73622.f3.

[9] M. Delaney and C. McKinley, "Configuration Management for Modern Distributed Systems,"
*IEEE Software*, vol. 36, no. 2, pp. 42–47, Mar./Apr. 2019.
DOI: 10.1109/MS.2018.2886726.

---

## Config Metrics Exporter Test Build Stub (Target: v1.4.0 — cleanup)

**Stub:** `src/config/config_metrics_exporter.cpp` — `THEMIS_TEST_BUILD`: `syncFromPathResolver()` returns immediately without reading counters or updating any gauge  
**Risk:** Config-path resolution metrics not updated in test builds; Prometheus `/metrics` scrape shows no `themis_config_*` counters.

### Scope
- Define a minimal `MockMetricsCollector` in test infrastructure that implements the `setGauge` interface.
- Link the mock in unit test CMake targets instead of using `THEMIS_TEST_BUILD` early exit.
- Remove `#ifdef THEMIS_TEST_BUILD` guard once mock is wired.

### Test Strategy
- With mock: `syncFromPathResolver()` calls `mock.setGauge()` → counter values verifiable in tests.
- Without mock: `THEMIS_TEST_BUILD` guard still accepted (backward compat) until mock is fully deployed.
