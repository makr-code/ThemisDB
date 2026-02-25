# Config Module

## Module Purpose

The Config module provides backward-compatible configuration path resolution for ThemisDB. It maps legacy flat-file config paths to their new hierarchical directory structure, enabling a seamless migration window where both old and new paths are supported simultaneously. It includes LRU caching for resolved paths, structured deprecation metadata, and a typed exception hierarchy for config-related errors.

## Relevant Interfaces

| Interface / File | Role |
|-----------------|------|
| `config_path_resolver.h` / `config_path_resolver.cpp` | Legacy-to-new config path mapping with filesystem fallback |
| `config_audit_log.h` / `config_audit_log.cpp` | Bounded in-memory audit trail for config path accesses |
| `lru_cache.h` | LRU cache with TTL for resolved path results |
| `path_mapping_metadata.h` | Deprecation and removal-date metadata per mapped path |
| `config_errors.h` | Typed exception hierarchy for config-related errors |

## Scope

**In Scope:**
- Legacy-to-new config path mapping with filesystem fallback
- LRU cache with TTL for resolved path results
- Path validation (path-traversal prevention, normalization)
- Deprecation/removal-date metadata per mapped path
- Thread-safe metrics tracking (hits, misses, cache hits, legacy fallbacks)
- Typed exception hierarchy for config errors
- Config path access audit trail (bounded in-memory log with timestamps)

**Out of Scope:**
- Parsing or loading config file contents (YAML/JSON)
- Runtime configuration hot-reload
- Configuration schema validation
- Secrets management or credential injection

## Key Components

### ConfigPathResolver
**Location:** `config_path_resolver.h`, `config_path_resolver.cpp`

Static utility that resolves legacy config paths to their new hierarchical locations. Checks the new path first, then falls back to the legacy path with a deprecation warning.

**Features:**
- **Path Mapping Table**: 60+ mappings covering AI/ML, security, compliance, performance, platform, networking, and monitoring categories
- **Filesystem Fallback**: Tries the new path first; if absent, uses the legacy path and emits a `spdlog` warning
- **Optional API**: `tryResolve()` returns `std::nullopt` instead of throwing on failure
- **Metadata Lookup**: `getMetadata()` returns deprecation date, removal date, and migration guide link per path
- **Thread-Safe Metrics**: All counters use `std::atomic` — safe for concurrent reads with no locking
- **LRU Cache**: Resolved paths are cached (capacity 1000, TTL 5 min) to avoid repeated filesystem `exists()` calls
- **Audit Trail**: Optional bounded in-memory audit log records every successful resolution with timestamp and legacy/cache-hit flags

**Thread Safety:**
- All public methods are safe for concurrent read access
- The `PATH_MAPPING` table is `const` and initialized at compile time
- Metrics use `std::atomic<uint64_t>`; no locks needed for reads
- `ConfigAuditLog` uses an internal `std::mutex`; audit recording is a separate lock acquisition from path resolution

### ConfigAuditLog
**Location:** `config_audit_log.h`, `config_audit_log.cpp`

Bounded, thread-safe in-memory audit trail for config path accesses. Disabled by default; enabled via `ConfigPathResolver::setAuditLogEnabled(true)`. Each successful resolution appends an `AuditEntry` containing:

| Field | Type | Description |
|-------|------|-------------|
| `requested_path` | `std::string` | The path as originally passed by the caller |
| `resolved_path` | `std::string` | The final filesystem path returned |
| `timestamp` | `std::chrono::system_clock::time_point` | UTC time of the access |
| `is_legacy` | `bool` | `true` if the legacy fallback path was used |
| `is_cache_hit` | `bool` | `true` if the result was served from the LRU cache |

The log is bounded (default 10,000 entries); oldest entries are evicted when the limit is reached. Failed resolutions are never recorded.

### LRUCacheWithTTL
**Location:** `lru_cache.h`

Generic LRU cache with per-entry TTL eviction. Used internally by `ConfigPathResolver` to cache resolved paths.

### PathMappingMetadata
**Location:** `path_mapping_metadata.h`

Holds deprecation and removal timestamps, category, and a link to the migration guide for each mapped legacy path. Used to emit structured warnings when legacy paths are accessed.

### Config Exception Hierarchy
**Location:** `config_errors.h`

Typed exceptions for config-related failures:

| Exception | Thrown When |
|---|---|
| `ConfigNotFoundException` | Neither new nor legacy path exists on disk |
| `MappingNotFoundException` | No mapping found for a legacy path |
| `InvalidPathException` | Path contains `..` (traversal attempt) or is otherwise invalid |
| `ConfigPermissionException` | Filesystem permission denied |

## Architecture

```
Caller
  │
  └─► ConfigPathResolver::resolve(legacy_path)
            │
            ├─ normalizePath()         ← strip "./" and backslashes
            ├─ validatePath()          ← reject ".." traversal
            ├─ LRUCacheWithTTL::get()  ← return if cached (+ audit entry if enabled)
            │
            ├─ mapLegacyToNew()        ← look up PATH_MAPPING table
            │
            ├─ filesystem::exists(new_path)?  → return new path (+ audit entry if enabled)
            │
            └─ filesystem::exists(legacy_path)?
                  ├─ yes → log deprecation warning, return legacy path (+ audit entry if enabled)
                  └─ no  → throw ConfigNotFoundException (no audit entry)
```

## Dependencies

### Internal Dependencies
- `config/lru_cache.h` — LRU cache with TTL
- `config/path_mapping_metadata.h` — deprecation metadata struct
- `config/config_errors.h` — typed exception hierarchy
- `config/config_audit_log.h` — bounded in-memory audit trail

### External Dependencies
- `spdlog` — structured logging for deprecation warnings and debug traces
- `<filesystem>` (C++17) — file existence checks and path manipulation

## Usage Examples

```cpp
#include "config/config_path_resolver.h"

using namespace themis::config;

// Resolve a legacy path (throws ConfigNotFoundException if not found)
std::string path = ConfigPathResolver::resolve("config/lora_training_config.yaml");
// Returns "config/ai_ml/lora_training_config.yaml" if new path exists,
// or "config/lora_training_config.yaml" with a deprecation warning if only legacy exists.

// Non-throwing variant
auto opt = ConfigPathResolver::tryResolve("config/pii_patterns.yaml");
if (opt) {
    // use *opt
}

// Check if a path is a known legacy path
if (ConfigPathResolver::isLegacyPath("config/rbac_roles.json")) {
    // suggest migration
}

// Retrieve deprecation metadata
auto meta = ConfigPathResolver::getMetadata("config/lora_training_config.yaml");
if (meta && meta->isDeprecated()) {
    // meta->getDeprecationMessage() returns a human-readable message
}

// Inspect resolution metrics
const auto& m = ConfigPathResolver::metrics();
// m.new_path_hits, m.legacy_fallbacks, m.cache_hits, etc.

// Disable caching (e.g., in tests)
ConfigPathResolver::setCachingEnabled(false);
ConfigPathResolver::resetMetrics();

// Enable config path audit trail
ConfigPathResolver::setAuditLogEnabled(true);

std::string path2 = ConfigPathResolver::resolve("config/pii_patterns.yaml");

// Query all recorded audit entries (oldest first)
for (const auto& entry : ConfigPathResolver::auditLog()) {
    // entry.requested_path  — original caller path
    // entry.resolved_path   — path that was returned
    // entry.timestamp       — std::chrono::system_clock::time_point
    // entry.is_legacy       — true if legacy fallback was used
    // entry.is_cache_hit    — true if served from LRU cache
}

// Clear audit entries and disable logging
ConfigPathResolver::clearAuditLog();
ConfigPathResolver::setAuditLogEnabled(false);

// Limit audit log to 500 entries (oldest are evicted when limit is reached)
ConfigPathResolver::setAuditLogMaxEntries(500);
```

## Production Readiness

**Current Status: Beta**

- All public methods are thread-safe for concurrent read access
- Path-traversal prevention is enforced via `validatePath()`
- LRU cache avoids repeated filesystem calls under load
- Known limitations:
  - The metadata table (`METADATA_TABLE`) only contains entries for a small subset of mapped paths; remaining paths have auto-generated metadata with no deprecation dates
  - Absolute path validation in `validatePath()` is basic; production deployments should harden this check for their filesystem layout
  - HTTP/network config paths are not validated for reachability; only filesystem presence is checked

## Scientific References

1. Saltzer, J. H., & Schroeder, M. D. (1975). **The Protection of Information in Computer Systems**. *Proceedings of the IEEE*, 63(9), 1278–1308. https://doi.org/10.1109/PROC.1975.9939

2. Nygard, M. T. (2018). **Release It!: Design and Deploy Production-Ready Software (2nd ed.)**. Pragmatic Bookshelf. ISBN: 978-1-680-50239-8

3. Krioukov, A., Baig, L., Treuhaft, S., Ungureanu, C., Bhatia, K., Rolia, J., & Talwar, V. (2011). **Napsack: Solving Conflicts Among Distributed Configuration Requirements**. *Proceedings of the 6th ACM European Conference on Computer Systems (EuroSys)*, 331–344. https://doi.org/10.1145/1966445.1966475
