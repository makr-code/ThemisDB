# Config Module

## Module Purpose

The Config module provides backward-compatible configuration path resolution and JSON/YAML schema validation for ThemisDB. It maps legacy flat-file config paths to their new hierarchical directory structure, enabling a seamless migration window where both old and new paths are supported simultaneously. It includes LRU caching for resolved paths, structured deprecation metadata, a typed exception hierarchy for config-related errors, and a `ConfigSchemaValidator` that validates YAML/JSON configuration files against JSON Schema (Draft 7 subset) definitions.

## Relevant Interfaces

| Interface / File | Role |
|-----------------|------|
| `config_path_resolver.h` / `config_path_resolver.cpp` | Legacy-to-new config path mapping with filesystem fallback |
| `config_schema_validator.h` / `config_schema_validator.cpp` | JSON Schema (Draft 7 subset) validation of YAML/JSON config files |
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
- JSON Schema (Draft 7 subset) validation of YAML and JSON config files

**Out of Scope:**
- Parsing or loading config file contents (YAML/JSON) beyond what is needed for schema validation
- Runtime configuration hot-reload
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

**Thread Safety:**
- All public methods are safe for concurrent read access
- The `PATH_MAPPING` table is `const` and initialized at compile time
- Metrics use `std::atomic<uint64_t>`; no locks needed for reads

### LRUCacheWithTTL
**Location:** `lru_cache.h`

Generic LRU cache with per-entry TTL eviction. Used internally by `ConfigPathResolver` to cache resolved paths.

### PathMappingMetadata
**Location:** `path_mapping_metadata.h`

Holds deprecation and removal timestamps, category, and a link to the migration guide for each mapped legacy path. Used to emit structured warnings when legacy paths are accessed.

### ConfigSchemaValidator
**Location:** `config_schema_validator.h`, `config_schema_validator.cpp`

Static utility that validates YAML and JSON config files against JSON Schema (Draft 7 subset) definitions. YAML files are loaded via `yaml-cpp` and converted to an internal JSON representation before validation. JSON files are parsed directly with `nlohmann::json`. Schema file lookups use `ConfigPathResolver::tryResolve()` so that legacy-to-new path mapping applies automatically.

**Supported JSON Schema keywords:**
- `type`, `properties`, `required`, `additionalProperties`
- `minLength`, `maxLength`, `pattern` (string)
- `minimum`, `maximum`, `exclusiveMinimum`, `exclusiveMaximum` (number/integer)
- `minItems`, `maxItems`, `items` (array)
- `enum`, `const`

**Thread Safety:** All public methods are stateless static functions; safe for concurrent use.

### Config Exception Hierarchy
**Location:** `config_errors.h`

Typed exceptions for config-related failures:

| Exception | Thrown When |
|---|---|
| `ConfigNotFoundException` | Neither new nor legacy path exists on disk |
| `MappingNotFoundException` | No mapping found for a legacy path |
| `InvalidPathException` | Path contains `..` (traversal attempt) or is otherwise invalid |
| `ConfigPermissionException` | Filesystem permission denied |
| `SchemaValidationException` | A config or schema file cannot be read or parsed by `ConfigSchemaValidator` |

## Architecture

```
Caller
  │
  └─► ConfigPathResolver::resolve(legacy_path)
            │
            ├─ normalizePath()         ← strip "./" and backslashes
            ├─ validatePath()          ← reject ".." traversal
            ├─ LRUCacheWithTTL::get()  ← return if cached
            │
            ├─ mapLegacyToNew()        ← look up PATH_MAPPING table
            │
            ├─ filesystem::exists(new_path)?  → return new path
            │
            └─ filesystem::exists(legacy_path)?
                  ├─ yes → log deprecation warning, return legacy path
                  └─ no  → throw ConfigNotFoundException
```

## Dependencies

### Internal Dependencies
- `config/lru_cache.h` — LRU cache with TTL
- `config/path_mapping_metadata.h` — deprecation metadata struct
- `config/config_errors.h` — typed exception hierarchy

### External Dependencies
- `spdlog` — structured logging for deprecation warnings and debug traces
- `<filesystem>` (C++17) — file existence checks and path manipulation
- `yaml-cpp` — YAML file parsing used by `ConfigSchemaValidator`
- `nlohmann/json` — JSON file parsing and schema representation used by `ConfigSchemaValidator`

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
```

```cpp
#include "config/config_schema_validator.h"

using namespace themis::config;

// Validate a YAML config file against an inline JSON Schema
nlohmann::json schema = R"({
    "type": "object",
    "required": ["host", "port"],
    "properties": {
        "host": { "type": "string" },
        "port": { "type": "integer", "minimum": 1, "maximum": 65535 },
        "worker_threads": { "type": "integer", "minimum": 1 }
    }
})"_json;

auto result = ConfigSchemaValidator::validate("config/server.yaml", schema);
if (!result.valid) {
    spdlog::error("Config validation failed:\n{}", result.formatErrors());
}

// Validate against a JSON Schema file on disk
// (schema_path is resolved via ConfigPathResolver for legacy/new path mapping)
auto result2 = ConfigSchemaValidator::validateWithSchemaFile(
    "config/server.yaml",
    "config/schema/server.schema.json");

// Load any YAML or JSON file as nlohmann::json (e.g., for custom processing)
nlohmann::json data = ConfigSchemaValidator::loadAsJson("config/server.yaml");
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
