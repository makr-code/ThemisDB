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

# Config Module Implementation

## Purpose and ownership

This guide is for developers implementing changes in `src/config`. Public
interfaces are in [`../../include/config/`](../../include/config/README.md);
implementations are in this directory and focused tests are in
[`../../tests/config/`](../../tests/config/README.md). Preserve the frozen
v1.x intended policy in
[`../../include/config/config_contract.h`](../../include/config/config_contract.h),
but distinguish declared values from runtime-enforced guarantees as recorded in
[`MODULE_GAPS.md`](MODULE_GAPS.md).

## Implementation surfaces

| Concern | Implementation | Contract and checks |
|---|---|---|
| Path mapping, environment overlays, cache, fallback metrics, audit | `config_path_resolver.cpp` | `config_path_resolver.h`; `test_config_path_resolver.cpp`; resolver hardening tests |
| JSON/YAML parsing and schema checks | `config_schema_validator.cpp` | `config_schema_validator.h`; `test_config_schema_validator.cpp`; validator hardening tests |
| Platform file-change notification | `config_file_watcher.cpp` | `config_file_watcher.h`; watcher and hardening tests |
| Authenticated encrypted key/value storage | `config_encrypted_store.cpp` | `config_encrypted_store.h`; encrypted-store and hardening tests |
| Metrics and bounded access audit | `config_metrics_exporter.cpp`, `config_audit_log.cpp` | corresponding headers; metrics and resolver tests |

## Verified behavior to preserve

- Keep path normalization and validation ahead of file access. Path failures
  must not turn into a successful resolution. `resolve()` throws for absence;
  `tryResolve()` exposes an optional result.
- Environment overlays are limited to `dev` and `staging`; `prod` is the
  default. Environment changes clear the resolver cache.
- Cache capacity and TTL default to 1,000 and 300 seconds. The resolver reads
  `THEMIS_CONFIG_CACHE_SIZE` (10–100,000) and `THEMIS_CONFIG_CACHE_TTL`
  (1–86,400) during static initialization; invalid values fall back to defaults.
- Schema `$ref` handling is document-local; external references are reported as
  errors. Implemented keyword support is a subset and is not equivalent to full
  Draft 7 conformance.
- Watch callbacks are notifications, not a reload transaction. Keep callbacks
  short, debounce bursts, and ensure stopping the watcher joins its worker.
- AES-GCM metadata must be validated before decryption. Rotation must not
  publish a partial re-encryption. Protect serialized snapshots because they
  include the key bytes.
- The audit entry shape contains requested/resolved paths, timestamp,
  legacy-fallback and cache-hit flags. Do not treat this in-memory path log as a
  durable or operator-attributed change audit.
- `ConfigEncryptedStore::tryGet()` returns `std::nullopt` only for an absent
  entry. Decryption failures propagate as `ConfigEncryptionException`.
- `ConfigEncryptedStore::rotateKey()` uses the exclusive store lock across
  re-encryption and commit. Concurrent reads and writes block until it finishes;
  failed preparation leaves the previous key and entries in place.

## Input bounds and failure behavior

`config_contract.h` contains policy constants that are not all enforced. The
current code does not enforce its declared file/schema/path/key/value size,
top-level-key, nesting, or collected-error-count caps. Watcher interval and
operation-timeout constants do not control the event-driven platform backends.
The resolver does reject traversal and embedded NULs; schema checks can impose
per-schema string constraints such as `maxLength`. The detailed audit is in
[`MODULE_GAPS.md`](MODULE_GAPS.md); do not rely on an unenforced policy constant
as an input safety boundary.

## Verification workflow

Use focused tests before broader checks:

1. Resolver or validation change: select the corresponding root CMake focused
   target and CTest name documented in [`MODULE_GUIDE.md`](MODULE_GUIDE.md),
   including a hardening target when input handling changes.
2. Watcher or storage change: run `test_config_file_watcher_focused`,
   `test_config_encrypted_store_focused`, and
   `test_config_hardening_watcher_store_focused`.
3. Metrics or audit change: run `test_config_metrics_scrape_focused` and
   `test_config_path_resolver_focused`.
4. For performance-sensitive behavior, use the benchmark inventory in
   [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md) against their
   tracked baseline. Benchmark labels are targets, not evidence that a baseline
   passed.

See [`MODULE_GUIDE.md`](MODULE_GUIDE.md) for CMake/CTest entry points and
[`API.md`](API.md) for caller-visible semantics.
