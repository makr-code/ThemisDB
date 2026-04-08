# Config Module — Architecture Guide
<!-- status: current | validated: 2026-04-06 | source: src/config/ -->

**Version:** 1.0  
**Last Updated:** 2026-04-06  
**Module Path:** `src/config/`

---

## 1. Overview

The Config module provides backward-compatible configuration path resolution for ThemisDB.
As the project evolved from a flat configuration layout to a hierarchical directory structure,
this module bridges the two: it maps legacy config paths to their new locations, enabling a
migration window where both old and new paths are valid simultaneously.

Beyond path resolution, it provides an LRU cache for resolved results, structured deprecation
metadata, path-traversal prevention, and a typed exception hierarchy.

---

## 2. Design Principles

- **Backward Compatibility** – legacy config paths continue to work during the migration
  window; deprecation warnings guide operators toward new paths.
- **Security** – all paths are validated to prevent directory traversal (`..`) attacks.
- **Performance** – resolved paths are cached (LRU + TTL) to avoid repeated filesystem
  `exists()` calls in hot paths.
- **Observability** – all resolution attempts are tracked with lock-free atomic metrics
  (total resolves, cache hits, legacy fallbacks, errors).
- **Fail Fast** – invalid configurations detected at startup via typed exceptions.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `config_path_resolver.h` / `config_path_resolver.cpp` | Public header in `include/config/`; impl in `src/config/`. Main path resolution logic (60+ mappings, LRU cache, metrics, multi-env overlay) |
| `config_schema_validator.h` / `config_schema_validator.cpp` | Public header in `include/config/`; impl in `src/config/`. JSON Schema (Draft 7 subset) validation of YAML/JSON config files |
| `config_audit_log.h` / `config_audit_log.cpp` | Public header in `include/config/`; impl in `src/config/`. Bounded in-memory audit trail for config path accesses |
| `config_metrics_exporter.h` / `config_metrics_exporter.cpp` | Public header in `include/config/`; impl in `src/config/`. Prometheus text-format exporter; wired into `/metrics` endpoint |
| `config_encrypted_store.h` / `config_encrypted_store.cpp` | Public header in `include/config/`; impl in `src/config/`. AES-256-GCM encrypted key-value store with key rotation |
| `config_file_watcher.h` / `config_file_watcher.cpp` | Public header in `include/config/`; impl in `src/config/`. inotify/kqueue/ReadDirectoryChangesW filesystem watcher |
| `lru_cache.h` | Public header in `include/config/`. Generic LRU cache with per-entry TTL eviction |
| `path_mapping_metadata.h` | Public header in `include/config/`. Deprecation date, removal date, migration guide URL per mapped path |
| `config_errors.h` | Public header in `include/config/`. Typed exception hierarchy: `ConfigNotFoundException`, `MappingNotFoundException`, `InvalidPathException`, `ConfigPermissionException` |
| `config_migration_scanner_impl.h` | Public header in `include/config/`. Testable inline implementation for the `config_migration_scanner` CLI tool |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                      Caller (any module)                        │
│   auto path = ConfigPathResolver::resolve("ai/llm.yaml")        │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                  ConfigPathResolver (static)                     │
│                                                                  │
│  1. validate(path) → check for '..' traversal, null bytes        │
│  2. LRUCacheWithTTL<string,string>: cache hit? → return          │
│  3. lookup PATH_MAPPING table (60+ entries)                      │
│  4. does overlay path exist? (dev/staging only) → return         │
│  5. does new path exist on filesystem? → return new path         │
│  6. does legacy path exist? → log deprecation warning → return   │
│  7. neither exists → throw ConfigNotFoundException               │
│                                                                  │
│  Audit:   ConfigAuditLog (bounded ring-buffer, opt-in)           │
│  Metrics: total_resolves, cache_hits, legacy_fallbacks, errors   │
└──────────┬───────────────────────────────┬──────────────────────┘
           │ validate(config, schema)       │ metrics() / cacheStats() / deprecationReport()
┌──────────▼──────────────────────┐  ┌─────▼──────────────────────────────────────────────┐
│   ConfigSchemaValidator (static) │  │              ConfigMetricsExporter (static)         │
│                                  │  │                                                      │
│  validate(config_path, schema)   │  │  collect()             → Prometheus text format      │
│  validateWithSchemaFile(...)     │  │  updateMetricsCollector() → push to MetricsCollector │
│  validateFromString(str,yaml,…)  │  └──────────────────────────────────────────────────────┘
│  loadAsJson(file_path)           │
│  loadAsJson(content, is_yaml)    │
│  (YAML/JSON, Draft-7 subset)     │
└──────────────────────────────────┘
```

---

## 3.3 ConfigSchemaValidator

`ConfigSchemaValidator` (in `config_schema_validator.h` / `config_schema_validator.cpp`) is a
standalone static utility for validating YAML and JSON config files — or in-memory strings —
against a JSON Schema (Draft 7 subset). It integrates with `ConfigPathResolver` for schema
file lookups so that legacy-to-new path mapping is applied automatically when loading schema
files.

**Public API:**

| Method | Input | Description |
|--------|-------|-------------|
| `validate(config_path, schema)` | file path + schema object | Validate a YAML/JSON file against an inline schema |
| `validateWithSchemaFile(cfg, schema_path)` | two file paths | Validate a YAML/JSON file against a schema file |
| `validateFromString(content, is_yaml, schema)` | in-memory string + schema | Validate a YAML or JSON string without touching the filesystem |
| `loadAsJson(file_path)` | file path | Parse a YAML/JSON file to `nlohmann::json` |
| `loadAsJson(content, is_yaml)` | in-memory string | Parse a YAML or JSON string to `nlohmann::json` |

**Supported keywords:** `type`, `properties`, `required`, `additionalProperties`,
`minLength`, `maxLength`, `pattern`, `minimum`, `maximum`, `exclusiveMinimum`,
`exclusiveMaximum`, `minItems`, `maxItems`, `items`, `enum`, `const`,
`allOf`, `anyOf`, `oneOf`, `$ref` with `$defs`/`definitions`, `format`, `uniqueItems`.

**YAML → JSON conversion:** yaml-cpp `Node` → `nlohmann::json` (type inference: null, bool,
int, float, string).

**In-memory validation:** `validateFromString` and `loadAsJson(content, is_yaml)` allow
callers to parse and validate YAML or JSON content held in a string without writing it to
disk first.  Parse errors are returned inside `ValidationResult` rather than thrown.
`result.config_path` is set to `"<string>"` for in-memory calls.

**Error reporting:** `ValidationResult` collects all validation errors and warnings before
returning, enabling callers to receive the full list of schema violations in one pass.

---

## 3.4 ConfigAuditLog

`ConfigAuditLog` (in `config_audit_log.h` / `config_audit_log.cpp`) is an opt-in, bounded
ring-buffer that records every successful `resolve()` / `tryResolve()` call.  It is disabled
by default and carries no overhead when disabled.

Each `AuditEntry` captures: `requested_path`, `resolved_path`, `timestamp`,
`is_legacy` (true when the legacy fallback branch was used), and `is_cache_hit`.

---

## 4. Data Flow

### 4.1 Successful Resolution (new path)

```
resolve("ai/llm.yaml")
    │
    ├─ validate → OK
    ├─ cache hit? → no (first call)
    ├─ PATH_MAPPING["ai/llm.yaml"] → "config/ai/llm/main.yaml"
    ├─ filesystem::exists("config/ai/llm/main.yaml") → true
    ├─ cache.put("ai/llm.yaml", "config/ai/llm/main.yaml")
    └─ return "config/ai/llm/main.yaml"
```

### 4.2 Legacy Fallback with Deprecation Warning

```
resolve("llm_config.yaml")
    │
    ├─ validate → OK
    ├─ cache hit? → no
    ├─ PATH_MAPPING["llm_config.yaml"] → "config/ai/llm/main.yaml"
    ├─ filesystem::exists("config/ai/llm/main.yaml") → false
    ├─ filesystem::exists("llm_config.yaml") → true
    ├─ spdlog::warn("Deprecated config path: llm_config.yaml. Use: config/ai/llm/main.yaml. Removal: 2026-06-01")
    ├─ metrics.legacy_fallbacks++
    ├─ cache.put("llm_config.yaml", "llm_config.yaml")
    └─ return "llm_config.yaml"
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Used by** | All modules that load config files | `ConfigPathResolver::resolve()` |
| **Used by** | `MonitoringApiHandler` (`/metrics` endpoint) | `ConfigMetricsExporter::collect()`, `ConfigMetricsExporter::updateMetricsCollector()` |
| **Uses** | Filesystem (std::filesystem) | Path existence checks |
| **Uses** | spdlog | Deprecation warning logging |
| **Uses** | `observability::MetricsCollector` | `updateMetricsCollector()` pushes gauges for Grafana integration |
| **Provides to** | Operators / tooling | `getMetadata()` for migration guides |

---

## 6. Threading & Concurrency Model

- `PATH_MAPPING` is a `const static` table initialized at compile time — zero-overhead reads.
- Metrics counters are `std::atomic<uint64_t>` — safe for concurrent increment without locks.
- `LRUCacheWithTTL` uses an internal `std::mutex` for thread safety.
- `ConfigAuditLog` uses an internal `std::mutex`; audit recording is a separate lock acquisition from path resolution.
- `ConfigSchemaValidator` exposes only stateless static methods — safe for concurrent use with no shared state.
- `ConfigPathResolver` is stateless (static methods only); multiple threads may call it
  simultaneously without coordination.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| LRU cache | Capacity 1000, TTL 5 min; avoids repeated filesystem `exists()` calls |
| Compile-time mapping table | Constant `std::unordered_map`; O(1) average lookup |
| Lock-free metrics | Atomic counters; no contention on read paths |

---

## 8. Security Considerations

- **Path traversal prevention**: any path containing `..` throws `InvalidPathException`.
- **Null byte prevention**: paths with embedded null bytes are rejected.
- **No credential resolution**: this module resolves file paths only; it does not read or
  parse config file contents — secrets management is handled elsewhere.
- **Permission errors**: `ConfigPermissionException` is thrown (not silently logged) so
  callers cannot proceed with inaccessible config.

---

## 9. Configuration

The Config module itself has no runtime configuration file. Its behavior is controlled by:

| Parameter | Default | Env Variable | Description |
|---|---|---|---|
| `LRU_CACHE_CAPACITY` | 1000 | `THEMIS_CONFIG_CACHE_SIZE` | Max cached path resolutions (valid range: 10–100000) |
| `LRU_CACHE_TTL_SECONDS` | 300 | `THEMIS_CONFIG_CACHE_TTL` | Cache entry TTL in seconds (valid range: 1–86400) |
| `CONFIG_ENVIRONMENT` | `prod` | `THEMIS_CONFIG_ENV` | Active deployment environment: `dev`, `staging`, or `prod` |
| `PATH_MAPPING` | 60+ entries | — | Static legacy→new mapping table (compile-time constant) |

Both `THEMIS_CONFIG_CACHE_SIZE` and `THEMIS_CONFIG_CACHE_TTL` are read once at static initialization. Values outside the valid range cause a `stderr` warning and fall back to the defaults.

---

## 10. Error Handling

| Exception | Thrown When |
|---|---|
| `ConfigNotFoundException` | Neither new nor legacy path exists on disk |
| `MappingNotFoundException` | No mapping found for the given legacy path |
| `InvalidPathException` | Path contains `..` or null bytes |
| `ConfigPermissionException` | Filesystem returns permission denied |

The non-throwing variant `tryResolve()` returns `std::nullopt` instead of throwing, for
callers that prefer to check for presence without exception handling.

---

## 11. Known Limitations & Future Work

- Runtime hot-reload of config files is not provided by this module.
- YAML/JSON parsing is not in scope; consumers parse the resolved path themselves.
- The path mapping table is hard-coded; dynamic mapping registration is not supported.
- Removal of deprecated paths (per metadata removal dates) requires manual table cleanup.

---

## 12. References

- `src/config/README.md` — module overview
- `src/config/FUTURE_ENHANCEMENTS.md` — roadmap
- `docs/config_migration_guide.md` — migration guide for operators
- `ARCHITECTURE.md` (root) — full system architecture
