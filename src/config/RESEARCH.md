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

# Config Module Research

## Purpose and scope

This note records the engineering questions and evidence for config resolution,
schema validation, change notification, and encrypted value storage. It is for
maintainers choosing or evaluating changes to `src/config`; it is not a claim
that the module uses machine-learning models. The normative v1.x constraints are
in [`../../include/config/config_contract.h`](../../include/config/config_contract.h);
implementation and test evidence remains authoritative for delivered behavior.

## Questions and assumptions

- **Resolution:** can legacy-to-hierarchical mapping, environment overlays, and
  optional caching preserve deterministic path selection? `ConfigPathResolver`
  keys cached results by environment and normalized path. `THEMIS_CONFIG_ENV`
  accepts `dev`, `staging`, or `prod`; missing or unrecognized values select
  `prod`.
- **Validation:** can JSON and YAML configurations be checked without fetching
  remote schema references? `ConfigSchemaValidator` resolves document-local
  `$ref` values only; `validateWithSchemaFile` resolves the schema path through
  `ConfigPathResolver`.
- **Change detection:** can a config change invalidate path-resolution state
  without doing callback work on the application thread? `ConfigFileWatcher`
  has platform-specific backends and debounces callbacks. The resolver callback
  clears its cache; callers remain responsible for reloading and validating
  configuration.
- **Secret storage:** can the store detect ciphertext tampering and re-encrypt
  values on rotation? Its implementation uses AES-256-GCM and re-encrypts the
  complete in-memory store before replacing the current key.

These are subsystem design assumptions, not independent evidence of production
availability or a substitute for deployment-level tests.

## Evaluation and reproducibility

Use the focused suites under `tests/config/` to reproduce behavior:

- `test_config_path_resolver.cpp` and
  `test_config_hardening_resolver_validator.cpp` cover mapping, resolution,
  unsafe paths, bounds, local-reference restrictions, and validation edges.
- `test_config_schema_validator.cpp` covers JSON/YAML parsing and supported
  schema keywords.
- `test_config_file_watcher.cpp` and
  `test_config_hardening_watcher_store.cpp` cover notifications, debounce,
  deletion, and watcher bounds.
- `test_config_encrypted_store.cpp` and
  `test_config_hardening_watcher_store.cpp` cover authenticated encryption,
  rotation, serialization, and malformed metadata.

For performance, use the benchmark cases and thresholds referenced by
[`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md) and
[`ROADMAP.md`](ROADMAP.md). Those files distinguish mapped release gates from
broader paths that still need benchmark coverage; do not infer a measured result
from a threshold alone.

## Trade-offs and limits

- File watchers report changes; they do not prove a reload succeeded or that a
  consumer adopted the new settings.
- The validator implements a JSON Schema subset. Unsupported keywords and
  unknown string formats must not be assumed to provide enforcement.
- `ConfigEncryptedStore::serialize()` includes the encryption key bytes, encoded
  as base64. A serialized snapshot therefore requires secret-grade protection;
  base64 is not encryption or external key management.
- Benchmark results depend on the release profile, hardware, and workload.
  Keep the benchmark manifest and baseline with any performance conclusion.

## Evidence references

- [`../config/config_path_resolver.cpp`](config_path_resolver.cpp)
- [`../config/config_schema_validator.cpp`](config_schema_validator.cpp)
- [`../config/config_file_watcher.cpp`](config_file_watcher.cpp)
- [`../config/config_encrypted_store.cpp`](config_encrypted_store.cpp)
- [`../../tests/config/`](../../tests/config/README.md)
- [`ARCHITECTURE.md`](ARCHITECTURE.md) · [`API.md`](API.md)
