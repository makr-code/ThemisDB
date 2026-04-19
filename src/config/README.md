# Config Module
<!-- status: current | validated: 2026-04-06 | source: src/config/ -->

## Module Purpose

The Config module provides backward-compatible configuration path resolution and JSON/YAML schema validation for ThemisDB. It maps legacy flat-file config paths to their new hierarchical directory structure, enabling a seamless migration window where both old and new paths are supported simultaneously. It includes LRU caching for resolved paths, structured deprecation metadata, a typed exception hierarchy for config-related errors, and a `ConfigSchemaValidator` that validates YAML/JSON configuration files against JSON Schema (Draft 7 subset) definitions.

## Relevant Interfaces

| Interface / File | Role |
|-----------------|------|
| `config_path_resolver.h` / `config_path_resolver.cpp` | Legacy-to-new config path mapping with filesystem fallback |
| `config_schema_validator.h` / `config_schema_validator.cpp` | JSON Schema (Draft 7 subset) validation of YAML/JSON config files |
| `config_audit_log.h` / `config_audit_log.cpp` | Bounded in-memory audit trail for config path accesses |
| `config_metrics_exporter.h` / `config_metrics_exporter.cpp` | Prometheus text-format metrics exporter for the `/metrics` endpoint |
| `config_encrypted_store.h` / `config_encrypted_store.cpp` | AES-256-GCM encrypted key-value store for sensitive config values with key rotation |
| `lru_cache.h` | LRU cache with TTL for resolved path results |
| `path_mapping_metadata.h` | Deprecation and removal-date metadata per mapped path |
| `config_errors.h` | Typed exception hierarchy for config-related errors |
| `config_migration_scanner_impl.h` | Testable inline implementation for the `config_migration_scanner` CLI tool |

## Scope

**In Scope:**
- Legacy-to-new config path mapping with filesystem fallback
- LRU cache with TTL for resolved path results
- Path validation (path-traversal prevention, normalization)
- Deprecation/removal-date metadata per mapped path
- Thread-safe metrics tracking (hits, misses, cache hits, legacy fallbacks)
- Prometheus metrics export via `ConfigMetricsExporter::collect()` (served on `/metrics`)
- Typed exception hierarchy for config errors
- JSON Schema (Draft 7 subset) validation of YAML and JSON config files
- Config path access audit trail (bounded in-memory log with timestamps)
- **Encrypted config storage** (`ConfigEncryptedStore`): AES-256-GCM encryption, per-value random IV, authentication tag verification, zero-downtime key rotation

**Out of Scope:**
- Parsing or loading config file contents (YAML/JSON) beyond what is needed for schema validation
- Runtime configuration hot-reload
- Master-key envelope protection for serialised `ConfigEncryptedStore` snapshots (caller responsibility)

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
- **LRU Cache**: Resolved paths are cached to avoid repeated filesystem `exists()` calls. Capacity and TTL are configurable via environment variables (see [Environment Variables](#environment-variables) below).
- **Symlink Hardening**: `validatePath()` rejects symlinks that resolve outside the config root
- **Deprecation Aggregation**: `deprecationReport()` returns a usage-sorted snapshot of all legacy paths accessed since startup
- **Multi-Environment Overlay**: Dev/staging path sets allow environment-specific config overrides without touching production files (see [Multi-Environment Config Overlay](#multi-environment-config-overlay) below).

**Environment Variables:**

| Variable | Default | Valid Values / Range | Description |
|---|---|---|---|
| `THEMIS_CONFIG_CACHE_SIZE` | `1000` | `[10, 100000]` | Maximum number of entries in the path-resolution LRU cache |
| `THEMIS_CONFIG_CACHE_TTL` | `300` | `[1, 86400]` | Entry TTL in seconds; expired entries are evicted on next access |
| `THEMIS_CONFIG_ENV` | `prod` | `dev` \| `staging` \| `prod` (case-insensitive) | Active deployment environment for config overlay resolution |

Read the active runtime values via `ConfigPathResolver::currentCacheConfig()` and `ConfigPathResolver::getEnvironment()`.

When a variable is absent, empty, or invalid a warning is written to `stderr` and the default value is used.

**Thread Safety:**
- All public methods are safe for concurrent read access
- The `PATH_MAPPING` table is `const` and initialized at compile time
- Metrics use `std::atomic<uint64_t>`; no locks needed for reads
- `ConfigAuditLog` uses an internal `std::mutex`; audit recording is a separate lock acquisition from path resolution

### ConfigEncryptedStore
**Location:** `config_encrypted_store.h`, `config_encrypted_store.cpp`

Thread-safe, AES-256-GCM encrypted key-value store for sensitive configuration values (passwords, API tokens, connection strings). Each call to `set()` generates a fresh random 96-bit IV, ensuring that two encryptions of the same plaintext produce distinct ciphertexts. Authentication tags (128 bits) are verified on every `get()` call, so tampered data is detected before it is returned.

**Encryption scheme:**

| Property | Value |
|----------|-------|
| Algorithm | AES-256-GCM (NIST SP 800-38D) |
| Key size | 256 bits (32 bytes) |
| IV size | 96 bits (12 bytes), randomly generated per encryption |
| Tag size | 128 bits (16 bytes), verified on every decryption |
| IV source | `RAND_bytes` (OpenSSL CSPRNG) |

**Key rotation:**

`rotateKey()` atomically re-encrypts every stored value under a new randomly-generated 256-bit key. The operation is serialised under the internal mutex so no concurrent read can observe a partially-rotated store. The old key bytes are zero-filled before the `std::vector` holding them is dropped.

**Persistence:**

`serialize()` returns a JSON string containing the current key material and all encrypted blobs. `deserialize()` restores a store from such a snapshot. The serialised form contains the AES key in plaintext; callers must wrap the snapshot in a master-key envelope before writing it to persistent storage.

**Thread Safety:** All public methods acquire the internal `std::mutex`; safe for concurrent use from multiple threads.

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

### ConfigMetricsExporter
**Location:** `config_metrics_exporter.h`, `config_metrics_exporter.cpp`

Static utility that formats `ConfigPathResolver` metrics in Prometheus text-exposition format and exposes them on the server-wide `/metrics` scrape endpoint.

**Exported metrics:**

| Metric Name | Type | Description |
|---|---|---|
| `themis_config_resolution_hits_total` | counter | Successful path resolutions |
| `themis_config_resolution_misses_total` | counter | Failed resolutions (path not found) |
| `themis_config_legacy_fallbacks_total` | counter | Times legacy path was used as fallback |
| `themis_config_new_path_hits_total` | counter | Times new (canonical) path was resolved |
| `themis_config_unmapped_requests_total` | counter | Requests for paths with no mapping |
| `themis_config_cache_hits_total` | counter | LRU cache hits |
| `themis_config_cache_misses_total` | counter | LRU cache misses |
| `themis_config_cache_hit_ratio` | gauge | Cache hit / (hit + miss), 0.0–1.0 |
| `themis_config_cache_size` | gauge | Current number of entries in cache |
| `themis_config_cache_capacity` | gauge | Maximum cache capacity (info) |
| `themis_config_cache_ttl_seconds` | gauge | Cache entry TTL in seconds (info) |
| `themis_config_legacy_fallbacks_by_category_total{category}` | counter | Legacy fallbacks broken down by config category |
| `themis_config_legacy_fallbacks_all_total` | counter | Aggregate legacy fallbacks across all categories |

`collect()` performs a pure read unless a Prometheus registry is registered via `registerWithRegistry()`, in which case it also updates the registered counters using deltas before returning serialized text. `updateMetricsCollector()` pushes the same values into the central `MetricsCollector` singleton as `_current` gauges for Grafana dashboard integration.


### PathMappingMetadata
**Location:** `path_mapping_metadata.h`

Holds deprecation and removal timestamps, category, and a link to the migration guide for each mapped legacy path. Used to emit structured warnings when legacy paths are accessed.

### ConfigSchemaValidator
**Location:** `config_schema_validator.h`, `config_schema_validator.cpp`

Static utility that validates YAML and JSON config files — or in-memory strings — against JSON Schema (Draft 7 subset) definitions. YAML content is parsed via `yaml-cpp` and converted to an internal JSON representation before validation. JSON content is parsed directly with `nlohmann::json`. Schema file lookups use `ConfigPathResolver::tryResolve()` so that legacy-to-new path mapping applies automatically.

**Public API summary:**

| Method | Input | Description |
|--------|-------|-------------|
| `validate(path, schema)` | file path + schema object | Validate a YAML/JSON file against an inline schema |
| `validateWithSchemaFile(config_path, schema_path)` | two file paths | Validate a YAML/JSON file against a schema file |
| `validateFromString(content, is_yaml, schema)` | in-memory string + schema object | Validate a YAML or JSON string without touching the filesystem |
| `loadAsJson(file_path)` | file path | Parse a YAML/JSON file to `nlohmann::json` |
| `loadAsJson(content, is_yaml)` | in-memory string | Parse a YAML or JSON string to `nlohmann::json` |

**Supported JSON Schema keywords:**
- `type`, `properties`, `required`, `additionalProperties`
- `minLength`, `maxLength`, `pattern`, `format` (string; formats: `date`, `date-time`, `email`, `uri`, `ipv4`, `ipv6`)
- `minimum`, `maximum`, `exclusiveMinimum`, `exclusiveMaximum` (number/integer)
- `minItems`, `maxItems`, `items`, `uniqueItems` (array)
- `enum`, `const`
- `allOf`, `anyOf`, `oneOf` (schema composition)
- `$ref` with local `$defs` / `definitions` lookup (JSON Pointer RFC 6901 subset)

**`$ref` and `$defs` support:**

`ConfigSchemaValidator` resolves document-internal `$ref` values using a JSON Pointer walk (RFC 6901). Only refs beginning with `#` (document-local pointers) are supported. Both the Draft 2019-09 `$defs` keyword and the older Draft 4/6/7 `definitions` keyword are accepted as lookup targets. External URI references (e.g., `https://example.com/schema.json`) are rejected with a validation error to prevent SSRF.

- Nested references (a `$defs` entry that itself uses `$ref`) are fully resolved.
- Cyclic `$ref` chains are detected and reported as a validation error rather than causing infinite recursion.
- An unresolvable `$ref` is reported as an error in `ValidationResult`.
- `$ref` with local `$defs` / `definitions` lookup (JSON Pointer, RFC 6901 subset; external URI resolution is not supported)

**Schema Composition keywords:**

| Keyword | Semantics |
|---------|-----------|
| `allOf` | Value must satisfy **all** sub-schemas. Errors from every failing sub-schema are collected and reported. |
| `anyOf` | Value must satisfy **at least one** sub-schema. Passes silently on the first match. |
| `oneOf` | Value must satisfy **exactly one** sub-schema. Fails if zero or more than one sub-schemas match. |

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
- `yaml-cpp` — YAML file parsing used by `ConfigSchemaValidator`
- `nlohmann/json` — JSON file parsing and schema representation used by `ConfigSchemaValidator` and `ConfigEncryptedStore`
- `OpenSSL` (`libcrypto`) — AES-256-GCM encryption used by `ConfigEncryptedStore`

## Usage Examples

### Encrypted Config Storage

```cpp
#include "config/config_encrypted_store.h"

using namespace themis::config;

// --- Basic usage ---

ConfigEncryptedStore store;
store.set("db_password", "hunter2");
store.set("api_token",   "tok_abc123");

// Retrieve (decrypts and verifies authentication tag on every call)
std::string pw = store.get("db_password");  // "hunter2"

// Non-throwing variant (returns std::nullopt if key absent)
auto token = store.tryGet("api_token");
if (token) { /* use *token */ }

// --- Key rotation (zero-downtime, atomic) ---

uint32_t new_version = store.rotateKey();
// All stored values are now re-encrypted under a fresh AES-256 key.
// Values remain accessible without any change to callers.
assert(store.get("db_password") == "hunter2");
assert(store.currentKeyVersion() == new_version);

// --- Persistence ---

// Serialise to JSON (contains AES key in plaintext — protect before persisting)
std::string snapshot = store.serialize();

// Restore from snapshot
ConfigEncryptedStore restored;
restored.deserialize(snapshot);
assert(restored.get("db_password") == "hunter2");
assert(restored.currentKeyVersion() == store.currentKeyVersion());
```

### Config Path Resolution

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

// Prometheus metrics export (used by MonitoringApiHandler at /metrics scrape)
#include "config/config_metrics_exporter.h"
std::string prom_text = ConfigMetricsExporter::collect();
// Returns Prometheus text-exposition format string with HELP/TYPE annotations.

// Sync into MetricsCollector for Grafana dashboard gauges
ConfigMetricsExporter::updateMetricsCollector();
// Query the active cache configuration (may differ from defaults if env vars are set)
auto cfg = ConfigPathResolver::currentCacheConfig();
// cfg.capacity, cfg.ttl_seconds

// Enumerate all known legacy paths (e.g. for tooling)
for (const auto& [legacy, new_path] : ConfigPathResolver::legacyPathMappings()) {
    // ...
}

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

## Multi-Environment Config Overlay

`ConfigPathResolver` supports dev, staging, and prod path sets via an overlay
directory mechanism.  When the active environment is `DEV` or `STAGING`, the
resolver probes an environment-specific overlay directory **before** the
standard config root:

| Environment | Overlay root | Activated by |
|---|---|---|
| `DEV` | `config/dev/` | `THEMIS_CONFIG_ENV=dev` or `setEnvironment(ConfigEnvironment::DEV)` |
| `STAGING` | `config/staging/` | `THEMIS_CONFIG_ENV=staging` or `setEnvironment(ConfigEnvironment::STAGING)` |
| `PROD` | *(no overlay)* | default; `THEMIS_CONFIG_ENV=prod` or `setEnvironment(ConfigEnvironment::PROD)` |

**Resolution order** (example: `config/lora_training_config.yaml` in DEV):

1. `config/dev/ai_ml/lora_training_config.yaml` ← overlay (checked first)
2. `config/ai_ml/lora_training_config.yaml`     ← canonical new path
3. `config/lora_training_config.yaml`            ← legacy fallback (with deprecation warning)

If the overlay file is absent the resolver falls through to the next path
without error.  Overlay directories are located under the repository root at
`config/dev/` and `config/staging/`; each contains a `README.md` with usage
guidelines.

**Programmatic API:**

```cpp
#include "config/config_path_resolver.h"
using namespace themis::config;

// Set active environment (also clears the LRU cache)
ConfigPathResolver::setEnvironment(ConfigEnvironment::DEV);

// Query active environment
ConfigEnvironment env = ConfigPathResolver::getEnvironment();
// env == ConfigEnvironment::DEV
```

**Cache isolation:** Cache keys include the active environment name
(`"dev:config/lora_training_config.yaml"`) to prevent cross-environment cache
poisoning.  `setEnvironment()` clears the cache atomically.

## Environment Variables

The following environment variables are read **once at process startup** (during static initialization) and cannot be changed at runtime.

| Variable | Default | Valid Values / Range | Description |
|---|---|---|---|
| `THEMIS_CONFIG_CACHE_SIZE` | `1000` | `[10, 100000]` | LRU cache capacity (max number of cached path resolutions) |
| `THEMIS_CONFIG_CACHE_TTL` | `300` | `[1, 86400]` | LRU cache TTL in seconds (300 = 5 minutes) |
| `THEMIS_CONFIG_ENV` | `prod` | `dev` \| `staging` \| `prod` (case-insensitive) | Active deployment environment for config overlay resolution |

When a variable is absent, empty, not a valid integer, or outside its valid range, a warning is written to `stderr` and the default value is used. Values outside the valid range are rejected to prevent pathological configurations (e.g., a zero-capacity cache or a TTL longer than one day).

**Example:**

```bash
# Large deployment with many config paths, running in dev overlay mode
THEMIS_CONFIG_CACHE_SIZE=5000 THEMIS_CONFIG_CACHE_TTL=60 THEMIS_CONFIG_ENV=dev ./themisdb
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

// --- In-memory YAML/JSON validation (no file required) ---
// Parse and validate a YAML or JSON string without writing it to disk.
// Useful for dynamic config editing, unit tests, and server-side hot-checks.

// Parse an in-memory YAML string to nlohmann::json
const std::string yaml_content = "port: 8080\nhost: localhost\n";
nlohmann::json parsed = ConfigSchemaValidator::loadAsJson(yaml_content, true /*is_yaml*/);

// Parse an in-memory JSON string to nlohmann::json
const std::string json_content = R"({"port": 8080, "host": "localhost"})";
nlohmann::json parsed_json = ConfigSchemaValidator::loadAsJson(json_content, false /*is_yaml*/);

// Validate an in-memory YAML string directly against a schema
nlohmann::json inmem_schema = R"({
    "type": "object",
    "required": ["host", "port"],
    "properties": {
        "host": { "type": "string" },
        "port": { "type": "integer", "minimum": 1, "maximum": 65535 }
    }
})"_json;

auto inmem_result = ConfigSchemaValidator::validateFromString(yaml_content, true, inmem_schema);
if (!inmem_result.valid) {
    spdlog::error("In-memory config validation failed:\n{}", inmem_result.formatErrors());
}

// Same API works for JSON strings — set is_yaml=false
auto inmem_json_result = ConfigSchemaValidator::validateFromString(json_content, false, inmem_schema);

// Edge cases for in-memory validation:
//  - Invalid YAML (e.g. tab-indented block) is reported as a validation error,
//    not thrown, so callers can inspect result.errors without try/catch.
//  - Invalid JSON string is similarly reported as a validation error.
//  - result.config_path is set to "<string>" for in-memory calls (no file path).
//  - $ref / $defs, allOf / anyOf / oneOf, format and all other schema keywords
//    work identically in validateFromString and validate.

// --- $ref and $defs: reusable schema fragments ---
// Define shared type definitions in "$defs" and reference them via "$ref".
// Both "$defs" (Draft 2019-09) and "definitions" (Draft 4/6/7) are supported.
nlohmann::json schema_with_defs = R"({
    "$defs": {
        "Port": { "type": "integer", "minimum": 1, "maximum": 65535 },
        "NonEmptyString": { "type": "string", "minLength": 1 },
        "ServerConfig": {
            "type": "object",
            "required": ["host", "port"],
            "properties": {
                "host": { "$ref": "#/$defs/NonEmptyString" },
                "port": { "$ref": "#/$defs/Port" }
            }
        }
    },
    "$ref": "#/$defs/ServerConfig"
})"_json;

auto result3 = ConfigSchemaValidator::validate("config/server.yaml", schema_with_defs);
if (!result3.valid) {
    spdlog::error("Config validation failed:\n{}", result3.formatErrors());
}
// Notes:
//  - External URI refs (e.g. "https://...") are rejected to prevent SSRF.
//  - Cyclic $ref chains are detected and reported as a validation error.
//  - Nested $ref resolution (a $defs entry referencing another $defs entry) is supported.
// Schema composition: allOf, anyOf, oneOf
// allOf — value must satisfy ALL sub-schemas (errors from all failing branches are reported)
nlohmann::json allof_schema = R"({
    "allOf": [
        { "type": "object" },
        { "required": ["host", "port"] },
        { "properties": { "port": { "minimum": 1, "maximum": 65535 } } }
    ]
})"_json;

// anyOf — value must satisfy AT LEAST ONE sub-schema
nlohmann::json anyof_schema = R"({
    "properties": {
        "log_level": {
            "anyOf": [
                { "type": "string", "enum": ["debug", "info", "warn", "error"] },
                { "type": "integer", "minimum": 0, "maximum": 5 }
            ]
        }
    }
})"_json;

// oneOf — value must satisfy EXACTLY ONE sub-schema (discriminated union)
nlohmann::json oneof_schema = R"({
    "oneOf": [
        {
            "type": "object",
            "required": ["type", "port"],
            "properties": {
                "type": { "const": "tcp" },
                "port": { "type": "integer" }
            },
            "additionalProperties": false
        },
        {
            "type": "object",
            "required": ["type", "path"],
            "properties": {
                "type": { "const": "unix" },
                "path": { "type": "string" }
            },
            "additionalProperties": false
        }
    ]
})"_json;

// $ref with $defs — reusable schema fragments (local references only)
nlohmann::json ref_schema = R"({
    "$defs": {
        "Port": { "type": "integer", "minimum": 1, "maximum": 65535 }
    },
    "type": "object",
    "properties": {
        "port":       { "$ref": "#/$defs/Port" },
        "admin_port": { "$ref": "#/$defs/Port" }
    }
})"_json;

// format — enforce well-known string formats (date, date-time, email, uri, ipv4, ipv6)
// Unknown format identifiers are silently accepted (informational keyword per JSON Schema spec).
nlohmann::json format_schema = R"({
    "type": "object",
    "properties": {
        "created_at":  { "type": "string", "format": "date" },
        "updated_at":  { "type": "string", "format": "date-time" },
        "contact":     { "type": "string", "format": "email" },
        "endpoint":    { "type": "string", "format": "uri" },
        "server_ip":   { "type": "string", "format": "ipv4" },
        "ipv6_addr":   { "type": "string", "format": "ipv6" }
    }
})"_json;
// Valid values: "2026-03-11", "2026-03-11T09:30:00Z", "user@example.com",
//               "https://api.example.com/v1", "192.168.1.1", "2001:db8::1"

// uniqueItems — require all array elements to be distinct
nlohmann::json unique_schema = R"({
    "type": "object",
    "properties": {
        "tags":    { "type": "array", "uniqueItems": true },
        "modules": { "type": "array", "items": { "type": "string" }, "uniqueItems": true }
    }
})"_json;
// A config file containing {"tags": ["a", "b", "a"]} will fail validation
// because index 0 and index 2 are equal.  {"tags": ["a", "b", "c"]} passes.
```

## Migration Scanner Tool

**Location:** `tools/config_migration_scanner.cpp`

A standalone CLI tool that scans a deployment directory tree for files referencing legacy config paths and outputs a migration report.

```bash
# Text report (default)
config_migration_scanner --root /srv/themis

# JSON report
config_migration_scanner --root /srv/themis --output json

# CSV report
config_migration_scanner --root /srv/themis --output csv

# Dry-run: show what --fix would change
config_migration_scanner --root /srv/themis --dry-run --fix

# Rewrite files in-place (creates .bak backups)
config_migration_scanner --root /srv/themis --fix
```

**Exit codes:**
- `0` – No overdue (past removal_date) legacy paths found
- `1` – At least one path past its `removal_date` was found (usable as a CI gate)
- `2` – Argument / usage error

## Production Readiness

**Current Status: Production Ready**

- All public methods are thread-safe for concurrent read access
- Path-traversal prevention and symlink escape hardening are enforced via `validatePath()`
- LRU cache avoids repeated filesystem calls under load; capacity and TTL are configurable at runtime via env vars
- Complete deprecation metadata for all 60+ mapped paths in `METADATA_TABLE`
- Known limitations:
  - HTTP/network config paths are not validated for reachability; only filesystem presence is checked
  - Migration tooling (`config_migration_scanner`) scans for path references but does not handle binary files

## Scientific References

1. Saltzer, J. H., & Schroeder, M. D. (1975). **The Protection of Information in Computer Systems**. *Proceedings of the IEEE*, 63(9), 1278–1308. https://doi.org/10.1109/PROC.1975.9939

2. Nygard, M. T. (2018). **Release It!: Design and Deploy Production-Ready Software (2nd ed.)**. Pragmatic Bookshelf. ISBN: 978-1-680-50239-8

3. Krioukov, A., Baig, L., Treuhaft, S., Ungureanu, C., Bhatia, K., Rolia, J., & Talwar, V. (2011). **Napsack: Solving Conflicts Among Distributed Configuration Requirements**. *Proceedings of the 6th ACM European Conference on Computer Systems (EuroSys)*, 331–344. https://doi.org/10.1145/1966445.1966475

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.
