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
v1.x contract in [`../../include/config/config_contract.h`](../../include/config/config_contract.h)
and record intentional contract changes in module release documentation.

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
- `ConfigEncryptedStore::tryGet()` catches decryption and decoding exceptions
  and returns `std::nullopt`; preserve this caveat in callers and docs.

## Input bounds and failure behavior

`config_contract.h` declares limits for config file size, path length, schema
size, key/value lengths, top-level key count, nesting, watcher timing, and
collected validation errors. The contract is normative, but current
implementation enforcement is not verified for every declared limit.
**TODO:** compare each declared boundary with implementation and focused test
coverage before claiming that it is enforced. Add below/at/above boundary tests
for any confirmed gaps.

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
