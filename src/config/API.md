---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-31"
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

This page is a developer index for the caller-facing API declared in
`include/config/`. Declarations and implementation behavior are authoritative;
the contract header describes intended v1.x invariants and must not be read as
proof that every invariant is currently enforced. See
[`IMPLEMENTATION.md`](IMPLEMENTATION.md) for known code/contract gaps.

## Resolution — `ConfigPathResolver`

- `resolve(path)` delegates to `tryResolve(path)` and throws
  `ConfigNotFoundException` when it returns no path. The exception exposes the
  requested and attempted paths.
- `tryResolve(path)` returns `std::optional<std::string>`; invalid or missing
  paths can return `std::nullopt`.
- `mapLegacyToNew(path)`, `isLegacyPath(path)`, `legacyPathMappings()`, and
  `getMetadata(path)` expose the in-process legacy mapping table and metadata.
- `setEnvironment()` / `getEnvironment()` select `DEV`, `STAGING`, or `PROD`.
  `THEMIS_CONFIG_ENV` initializes the process environment; absent or
  unrecognized values select `PROD`. A change clears the shared resolver cache.
- `startHotReload(dir, debounce)` returns a startup result. On a delivered
  change, the resolver callback clears its path cache; it does not parse or
  apply application settings. `stopHotReload()` stops the watcher.
- Metrics, fallback aggregation, audit, cache and environment controls are
  static process-wide state. Resets and changes therefore affect all callers.

## Validation — `ConfigSchemaValidator`

- `validate(config_path, schema)` loads a JSON/YAML file and returns a
  `ValidationResult`; load/parse errors are added to `errors`.
- `validateWithSchemaFile(config_path, schema_path)` resolves the schema path
  with `ConfigPathResolver::tryResolve()` when possible, then loads schema and
  config files. Its result records the resolved schema path.
- `loadAsJson(file_path)` selects YAML by `.yaml`/`.yml` suffix and otherwise
  uses the JSON parser. `loadAsJson(content, is_yaml)` uses the explicit
  format flag. Parse/load failures throw `SchemaValidationException`.
- `validateFromString(content, is_yaml, schema)` returns diagnostics with
  `config_path` set to `<string>`.
- `ValidationResult` contains `valid`, `errors`, `warnings`, `config_path`, and
  `schema_path`. Check `valid` and preserve both errors and warnings; a
  successful parse alone does not mean the schema matched.

## Watching and observability

- `ConfigFileWatcher::start()` returns false if the watch path is missing or
  platform setup fails. `stop()` is idempotent; the destructor stops the
  worker. The callback is debounced and is not a transactional config reload.
  The implementation watches `.yaml`, `.yml`, and `.json`; platform backends
  are Linux inotify, Apple kqueue and Windows directory notifications.
- `ConfigMetricsExporter::collect()` produces Prometheus text exposition;
  `registerWithRegistry()` integrates with a Prometheus registry.
- `ConfigAuditLog` is disabled by default, holds up to 10,000 entries by
  default, and records entries only when enabled. A maximum below one clamps
  to one. The in-memory log is not durable audit storage.

## Encrypted store — `ConfigEncryptedStore`

- `set(key, plaintext)` rejects an empty key; `get()` throws
  `ConfigKeyNotFoundException` for absence. `tryGet()` returns `std::nullopt`
  only for an absent key and propagates decryption failures.
- `remove()` reports whether an entry existed; `contains()`, `keys()`, `size()`,
  and `clear()` inspect or mutate the in-memory store.
- `rotateKey()` returns the new key version and atomically re-encrypts the store.
  It holds an exclusive lock throughout, so concurrent reads and writes wait
  until rotation completes.
- `serialize()` / `deserialize()` persist a JSON snapshot representation. The
  snapshot includes key bytes and must be protected as secret material.
- The object is non-copyable and non-movable. `get()` and `tryGet()` report
  crypto failures explicitly. Malformed snapshot input to `deserialize()` is
  reported as an error.

Detailed contracts: [`config_path_resolver.h`](../../include/config/config_path_resolver.h),
[`config_schema_validator.h`](../../include/config/config_schema_validator.h),
[`config_file_watcher.h`](../../include/config/config_file_watcher.h),
[`config_encrypted_store.h`](../../include/config/config_encrypted_store.h),
[`config_errors.h`](../../include/config/config_errors.h).

**Important error distinction:** `ConfigEncryptedStore::get()` throws for a
missing key. `tryGet()` represents absence with `std::nullopt`; decryption
failures propagate as `ConfigEncryptionException`.
`serialize()` includes the current key bytes (base64-encoded), so its output is
secret material, not encrypted backup.
