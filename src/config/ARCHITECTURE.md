# Config Module — Architecture Guide

**Version:** 1.0  
**Last Updated:** 2026-02-24  
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
| `config_path_resolver.h` / `config_path_resolver.cpp` | Main path resolution logic (60+ mappings, LRU cache, metrics) |
| `lru_cache.h` | Generic LRU cache with per-entry TTL eviction |
| `path_mapping_metadata.h` | Deprecation date, removal date, migration guide URL per mapped path |
| `config_errors.h` | Typed exception hierarchy: `ConfigNotFoundException`, `MappingNotFoundException`, `InvalidPathException`, `ConfigPermissionException` |

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
│  4. does new path exist on filesystem? → return new path         │
│  5. does legacy path exist? → log deprecation warning → return   │
│  6. neither exists → throw ConfigNotFoundException               │
│                                                                  │
│  Metrics: total_resolves, cache_hits, legacy_fallbacks, errors   │
└──────────────────────────────────────────────────────────────────┘
```

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
| **Uses** | Filesystem (std::filesystem) | Path existence checks |
| **Uses** | spdlog | Deprecation warning logging |
| **Provides to** | Operators / tooling | `getMetadata()` for migration guides |

---

## 6. Threading & Concurrency Model

- `PATH_MAPPING` is a `const static` table initialized at compile time — zero-overhead reads.
- Metrics counters are `std::atomic<uint64_t>` — safe for concurrent increment without locks.
- `LRUCacheWithTTL` uses an internal `std::mutex` for thread safety.
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

| Constant | Value | Description |
|---|---|---|
| `LRU_CACHE_CAPACITY` | 1000 | Max cached path resolutions |
| `LRU_CACHE_TTL_MINUTES` | 5 | Cache entry TTL |
| `PATH_MAPPING` | 60+ entries | Static legacy→new mapping table |

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
