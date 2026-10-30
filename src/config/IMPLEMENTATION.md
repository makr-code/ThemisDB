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

## Invariants for changes

- Keep path normalization and validation ahead of file access. Path failures
  must not turn into a successful resolution. `resolve()` throws for absence;
  `tryResolve()` exposes an optional result.
- Environment overlays are limited to `dev` and `staging`; `prod` is the
  default. Environment changes clear the resolver cache.
- Keep cache state bounded and environment-sensitive. Defaults declared by the
  resolver are capacity 1,000 and TTL 300 seconds; parse or validation changes
  must continue to reject oversized and unsafe input per the contract.
- Schema file references remain document-local. Do not add network resolution
  for `$ref`; preserve collected diagnostics and distinguish parse errors from
  schema mismatches.
- Watch callbacks are notifications, not a reload transaction. Keep callbacks
  short, debounce bursts, and ensure stopping the watcher joins its worker.
- AES-GCM metadata must be validated before decryption. Rotation must not
  publish a partial re-encryption. Protect serialized snapshots because they
  include the key bytes.
- Audit and metrics must not include plaintext secrets. Audit entries currently
  describe requested/resolved paths, timestamp, fallback, and cache-hit state.

## Input bounds and failure behavior

The frozen contract declares limits for config file size, path length, schema
size, key/value lengths, top-level key count, nesting, watcher timing, and
collected validation errors. Consult the constants in
`config_contract.h` rather than duplicating numeric policy in implementation.
For every boundary, test just-below, at-limit, and over-limit inputs where
behavior is meaningful. Storage, permission, internal, and inconsistent-state
failures must remain diagnosable and must not silently permit access.

## Verification workflow

Use focused tests before broader checks:

1. Resolver or validation change: run matching `tests/config/test_config_*`
   targets, including the hardening suite when input handling changes.
2. Watcher or storage change: run `test_config_file_watcher`,
   `test_config_encrypted_store`, and the watcher/store hardening tests.
3. Metrics or audit change: run `test_config_metrics_scrape` and resolver/audit
   tests.
4. For performance-sensitive behavior, compare the release benchmark cases in
   [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md) against their
   tracked baseline.

See [`MODULE_GUIDE.md`](MODULE_GUIDE.md) for CMake/CTest entry points and
[`API.md`](API.md) for caller-visible semantics.
