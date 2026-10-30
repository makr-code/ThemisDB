---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Config Module API

## Purpose

This page summarizes the caller-facing interfaces under `include/config/`.
Header declarations and their Doxygen comments are the API source of truth;
the v1.x cross-component invariants are in
[`../../include/config/config_contract.h`](../../include/config/config_contract.h).

## Resolution — `ConfigPathResolver`

- `resolve(path)` returns the resolved filesystem path or throws
  `ConfigNotFoundException`; the exception exposes the requested path and the
  paths attempted.
- `tryResolve(path)` returns an optional path. Invalid or unavailable paths do
  not produce a usable result.
- `mapLegacyToNew(path)`, `isLegacyPath(path)`, and `getMetadata(path)` expose
  mapping and deprecation information.
- `setEnvironment()` / `getEnvironment()` select `DEV`, `STAGING`, or `PROD`.
  Changing the environment clears the resolution cache. The environment
  variable is `THEMIS_CONFIG_ENV`; absent, unknown, or rejected values default
  to production.
- `startHotReload(dir, debounce)` starts a watcher and returns whether startup
  succeeded. It clears the path cache after watched config changes; it does not
  reload application settings. `stopHotReload()` stops and releases it.
- Metrics, fallback aggregation, audit-log controls, and cache controls are
  static process-level interfaces. Resetting metrics/cache affects shared
  resolver state, not only the caller.

## Validation — `ConfigSchemaValidator`

- `validate(config_path, schema)` parses a file and returns `ValidationResult`.
- `validateWithSchemaFile(config_path, schema_path)` loads both files and
  resolves the schema path through the resolver.
- `loadAsJson(file_path)` and `loadAsJson(content, is_yaml)` parse to
  `nlohmann::json`; parse/load failures can throw.
- `validateFromString(content, is_yaml, schema)` returns validation diagnostics
  with `config_path` set to `<string>`.
- `ValidationResult` contains `valid`, `errors`, `warnings`, `config_path`, and
  `schema_path`. Check `valid` and preserve both errors and warnings; a
  successful parse alone does not mean the schema matched.

## Watching and observability

- `ConfigFileWatcher::start()` returns false if it cannot start (including a
  missing watch path); `stop()` is idempotent and the destructor stops the
  worker. The callback is debounced and runs on the watcher path, not as a
  transactional config reload.
- `ConfigMetricsExporter::collect()` produces Prometheus text exposition;
  `registerWithRegistry()` integrates with a Prometheus registry.
- `ConfigAuditLog` is disabled by default, bounded to 10,000 entries by default,
  and records resolution events only when enabled. Setting a maximum below one
  clamps it to one.

## Encrypted store — `ConfigEncryptedStore`

- `set(key, plaintext)` rejects an empty key; `get()` throws
  `ConfigKeyNotFoundException` for absence, while `tryGet()` returns
  `std::nullopt`.
- `remove()` reports whether an entry existed; `contains()`, `keys()`, `size()`,
  and `clear()` inspect or mutate the in-memory store.
- `rotateKey()` returns the new key version and re-encrypts the store.
- `serialize()` / `deserialize()` persist a JSON snapshot representation. The
  snapshot includes key bytes and must be protected as secret material.
- The object is non-copyable and non-movable. Encryption, malformed snapshot,
  and authentication failures are explicit errors, not an empty-value result.

Detailed contracts: [`config_path_resolver.h`](../../include/config/config_path_resolver.h),
[`config_schema_validator.h`](../../include/config/config_schema_validator.h),
[`config_file_watcher.h`](../../include/config/config_file_watcher.h),
[`config_encrypted_store.h`](../../include/config/config_encrypted_store.h),
[`config_errors.h`](../../include/config/config_errors.h).
