# Config Module - Future Enhancements

## Scope

This document covers implementation-specific future enhancements for the Config module (`src/config/`), comprising `config_path_resolver.cpp` (legacy-to-new path mapping, 60+ paths), `config_path_resolver.h`, `lru_cache.h` (`LRUCacheWithTTL<K,V>`, capacity 1,000, TTL 5 min), `config_errors.h` (typed exception hierarchy), and `path_mapping_metadata.h` (`PathMappingMetadata` with deprecation dates and migration guide URLs). Config file parsing, YAML/JSON schema validation, secrets management, and runtime hot-reload are explicitly out of scope for this module.

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
| `LRUCacheWithTTL<K,V>` | `config_path_resolver.cpp`, other cache consumers | Must remain header-only in `config/lru_cache.h` |

## Planned Features

### Prometheus Metrics Exporter for Path Resolution
**Priority:** High
**Target Version:** v1.7.0

`ConfigPathResolver::Metrics` tracks `resolution_hits`, `resolution_misses`, `legacy_fallbacks`, `cache_hits`, `cache_misses`, and `unmapped_requests` via `std::atomic<uint64_t>`. Expose these as Prometheus gauges/counters so operations teams can detect when legacy paths are still in use and monitor migration progress.

**Implementation Notes:**
- `[ ]` Create `config_metrics_exporter.cpp`; register with the server's Prometheus registry at startup (`prometheus/registry.h`).
- `[ ]` Metric names: `themis_config_resolution_hits_total`, `themis_config_resolution_misses_total`, `themis_config_legacy_fallbacks_total`, `themis_config_cache_hit_ratio` (derived), `themis_config_unmapped_requests_total`.
- `[ ]` Add label `category` to `themis_config_legacy_fallbacks_total` using `ConfigPathResolver::inferCategory()` (already private method) to show which config category has the most legacy usage.
- `[ ]` Export function called every scrape interval (pull model); read from `ConfigPathResolver::metrics()` atomics — no mutex needed.
- `[ ]` Add `themis_config_cache_capacity` and `themis_config_cache_ttl_seconds` info metrics for dashboard context.

**Performance Targets:**
- Metrics scrape completes in < 1 ms (atomic reads, no cache iteration).
- Zero impact on `ConfigPathResolver::resolve()` hot path (metrics are already incremented by existing atomic ops).

---

### Deprecation Warning Aggregation Report
**Priority:** High
**Target Version:** v1.7.0

Currently, each call to `resolve()` with a legacy path emits an individual log warning. For high-traffic deployments this floods logs. Replace per-call warnings with a background aggregation thread that batches and periodically reports which legacy paths are still in active use and how frequently.

**Implementation Notes:**
- `[ ]` Add `DeprecationAggregator` class to `config_path_resolver.cpp`; maintained as a static singleton alongside `metrics_`.
- `[ ]` `DeprecationAggregator` stores a `std::unordered_map<std::string, uint64_t> usage_counts_` (key = legacy path); updated atomically on each `legacy_fallback` increment.
- `[ ]` Background reporter thread (or timer via `std::jthread`) fires every `aggregation_interval_s` (default: 300 s); logs a structured summary: `"[CONFIG] Legacy path report: {path: 'config/lora_training_config.yaml', hits: 4821, removal_date: '2026-06-01', guide: 'https://...'}"`.
- `[ ]` Expose `ConfigPathResolver::deprecationReport()` returning a `std::vector<DeprecationEntry>` for use by admin CLI tooling.
- `[ ]` Suppressed from per-call log warnings once aggregator is active (controlled by `setCachingEnabled`-style flag `setAggregationEnabled(bool)`).

**Performance Targets:**
- Aggregator map update: single atomic increment, < 50 ns overhead on `resolve()` hot path.
- Report generation for 60 legacy paths completes in < 1 ms (in-memory map iteration).

---

### CLI Migration Scanner
**Priority:** High
**Target Version:** v1.8.0

Implement a CLI tool (`tools/config_migration_scanner`) that scans a deployment directory tree for files referencing legacy config paths and outputs a migration report (current path → new path, deprecation status, removal deadline).

**Implementation Notes:**
- `[ ]` New binary target in `tools/config_migration_scanner.cpp`; links against `config_path_resolver` only (minimal dependencies).
- `[ ]` Accepts `--root <dir>` (default `.`) and `--output {text,json,csv}` flags; scans `.yaml`, `.json`, `.toml`, `.ini`, `.env` files recursively.
- `[ ]` For each discovered legacy path reference, outputs: `file`, `line`, `legacy_path`, `new_path`, `deprecated_date`, `removal_date`, `migration_guide_url` (from `PathMappingMetadata`).
- `[ ]` `--dry-run` mode: prints what would be renamed without modifying files.
- `[ ]` `--fix` mode: rewrites file contents replacing legacy path strings with new paths (with backup `.bak` files).
- `[ ]` Returns exit code `1` if any paths past `removal_date` are found (usable as a CI gate).

**Performance Targets:**
- Scan of 10,000 config files (avg 100 lines each) completes in < 5 s on a single thread.
- `--fix` mode for 500 files rewrites in < 10 s (bounded by disk I/O).

---

### Configurable LRU Cache Size and TTL via Environment Variables
**Priority:** Medium
**Target Version:** v1.7.0

`LRUCacheWithTTL` in `config_path_resolver.cpp` is constructed with hardcoded capacity=1000 and TTL=5 minutes. Allow overriding via environment variables to support deployments with many more config paths or where aggressive caching causes stale-path issues.

**Implementation Notes:**
- `[ ]` Read `THEMIS_CONFIG_CACHE_CAPACITY` (integer, default 1000) and `THEMIS_CONFIG_CACHE_TTL_SECONDS` (integer, default 300) at static initialisation time.
- `[ ]` Validate ranges: capacity in [10, 100000], TTL in [1, 86400]; fall back to defaults with a warning log if out of range.
- `[ ]` Update `cache_` initialisation in the static initializer block (currently `LRUCacheWithTTL<...> cache_(1000, 300s)`).
- `[ ]` Add `ConfigPathResolver::currentCacheConfig()` method returning `{capacity, ttl_seconds}` for observability.
- `[ ]` Document environment variables in `src/config/README.md` and `SETUP.md`.

**Performance Targets:**
- Zero performance regression on `resolve()` for default config values.
- `currentCacheConfig()` is a pure read with no locking overhead.

---

### Multi-Environment Config Overlay (dev/staging/prod)
**Priority:** Medium
**Target Version:** v1.9.0

Config paths currently resolve against a single filesystem root. Add overlay support so that `dev` environments can override specific config files without modifying the `prod` set, using the same `PATH_MAPPING` table.

**Implementation Notes:**
- `[ ]` Introduce `ConfigEnvironment` enum `{DEV, STAGING, PROD}` and `ConfigPathResolver::setEnvironment(ConfigEnvironment)`.
- `[ ]` Environment-specific overlay root: `config/{env}/` checked first, then the standard `config/` root, then the legacy path.
- `[ ]` Override via `THEMIS_CONFIG_ENV` environment variable (`dev`, `staging`, `prod`; default `prod`).
- `[ ]` `PATH_MAPPING` keys are unchanged; only the filesystem search order gains the overlay prefix.
- `[ ]` Cache key must incorporate `env` to prevent cross-environment cache poisoning: `cache_key = env + ":" + legacy_path`.
- `[?]` Decision needed: should `getMetadata()` return environment-specific `PathMappingMetadata` or only the global metadata?

**Performance Targets:**
- One additional filesystem `exists()` check per cache miss (overlay root probed first); negligible impact when cache hit rate > 95%.
- `setEnvironment()` clears the cache atomically to prevent stale overlay entries.

## Test Strategy

| Test Type | Coverage Target | Notes |
|-----------|----------------|-------|
| Unit | >80% new code | Test `DeprecationAggregator` with 60 legacy paths; test LRU env-var override with mock environment; test CLI scanner with synthetic file tree in tmp dir |
| Integration | `ConfigPathResolver::resolve()` with new/legacy/missing paths | Existing `tests/config/config_path_resolver_test.cpp`; extend with overlay and multi-env scenarios |
| Performance | `resolve()` hot path < 1 µs on L1 cache hit | `benchmarks/config_bench.cpp` microbench; regression alert at 5% |

## Performance Targets

| Metric | Current | Target | Method |
|--------|---------|--------|--------|
| `resolve()` latency (cache hit) | < 1 µs | < 1 µs (no regression) | `benchmarks/config_bench.cpp` |
| `resolve()` latency (cache miss, mapped) | < 500 µs | < 200 µs | `benchmarks/config_bench.cpp` |
| Deprecation aggregator hot path overhead | N/A | < 50 ns | microbenchmark in `benchmarks/config_bench.cpp` |
| CLI scanner 10K files | N/A | < 5 s | `tests/config/scanner_bench.cpp` |
| Metrics scrape | N/A | < 1 ms | `tests/config/metrics_scrape_test.cpp` |

## Security / Reliability

- `[ ]` `validatePath()` must reject any input containing `..` or null bytes before cache lookup or filesystem access, preventing path traversal; the existing implementation covers this but must be exercised in every new code path that calls `resolve()`.
- `[ ]` CLI `--fix` mode must create `.bak` backup files before overwriting any config file; if backup creation fails, the tool must abort rather than overwrite without a backup.
- `[ ]` Environment variable values (`THEMIS_CONFIG_CACHE_CAPACITY`, `THEMIS_CONFIG_ENV`) must be sanitised before use; reject values containing path separators or shell metacharacters.
