---
Author: "ThemisDB Contributors"
Created: "2026-11-01"
Last Updated: "2026-11-01"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level2"
SOT_Domain: "module-behavior"
Canonical_References: "src/user_storage_encrypted/README.md; src/user_storage_encrypted/ARCHITECTURE.md; src/user_storage_encrypted/API.md; src/user_storage_encrypted/AUDIT.md; include/user_storage_encrypted/ARCHITECTURE.md; include/user_storage_encrypted/multi_level_storage.hpp; src/user_storage_encrypted/multi_level_storage.cpp; src/user_storage_encrypted/CMakeLists.txt; tests/user_storage_encrypted/README.md; tests/user_storage_encrypted/CMakeLists.txt; benchmarks/user_storage_encrypted/README.md; benchmarks/user_storage_encrypted/CMakeLists.txt"
source: "src/user_storage_encrypted/README.md; src/user_storage_encrypted/ARCHITECTURE.md; src/user_storage_encrypted/API.md; include/user_storage_encrypted/multi_level_storage.hpp; src/user_storage_encrypted/multi_level_storage.cpp; tests/user_storage_encrypted/CMakeLists.txt; benchmarks/user_storage_encrypted/CMakeLists.txt"
source_class: "secondary"
source_weight: 60
Related: "#7392; DOC-WEEKLY-2026-44"
---

> Exact model backend not verifiable from final document text alone.

# User Storage Encrypted — Developer LLM Wiki

This page is secondary implementation context, not an API contract, security
assessment, or production-readiness declaration. Follow the current source,
public headers, focused tests, and module documentation for the exact path being
changed.

## Module boundary and dataflow

The in-tree plugin is implemented under
[`src/user_storage_encrypted/`](../../src/user_storage_encrypted/) and exposes
operational types under `themis::plugins::user_storage` in
[`include/user_storage_encrypted/`](../../include/user_storage_encrypted/).
The separate `user_storage_encrypted_api_contract.h` uses
`themis::user_storage_encrypted`; its documented guarantees do not replace the
operational API behavior. The guarantee-by-guarantee comparison is in the
module's [`API.md`](../../src/user_storage_encrypted/API.md).

`MultiLevelEncryptedStorage` validates level configuration, chooses a key
provider, initializes each level, and uses `GocryptfsBackend` for encrypted
container lifecycle. Plain levels use their configured filesystem path. User
and group records are JSON files beneath the selected level's `users/` and
`groups/` directories.

## Configuration and independent APIs

The implemented configuration root is `multi_level_storage.levels`. Plain
levels require `name`, `encrypted: false`, and `path`; encrypted levels require
`name`, `encrypted: true`, `encrypted_dir`, `mount_point`, and
`encryption.key_id`. Provider settings are read from the nested `encryption`
object. The shipped
[`storage_config.yaml.example`](../../plugins/user_storage_encrypted/config/storage_config.yaml.example)
is YAML; convert it to the JSON shape expected by
`MultiLevelEncryptedStorage::initialize()` before passing it to the runtime.

The plugin constructs its gocryptfs backend without a KDF service. The
standalone backend can receive one, but plugin configuration does not enable
that integration. `KeyRotationScheduler` is also a separate API: parsing
rotation configuration does not start it or connect its callback to
`MultiLevelEncryptedStorage::rotateKey()`. The scheduler callback and explicit
storage rotation have different behavior; neither the schedule check nor manual
`triggerRotation()` performs storage rotation.

## Runtime limitations to preserve

- Encrypted lifecycle depends on gocryptfs and host FUSE support. Platform
  support and unsupported paths are detailed in the module source and `README`.
- Stale-mount reconciliation is best-effort; failed cleanup is non-fatal and
  does not establish a healthy mount.
- Health checks inspect backend presence and mounted state, not provider,
  key-retrieval, or scheduler health.
- `unmountAll()` ignores per-level unmount results; independently verify mount
  state where cleanup matters.
- `container_size_bytes` is emitted but is not updated by current source.
- Key cleanup covers selected temporary buffers, not caller-owned inputs
  generally. Do not infer the frozen API contract's broader zeroization claims.
- The operational `Result<T>` surface and the separate API-contract error enum
  are distinct interfaces.

## Build, tests, and benchmarks

`THEMIS_PLUGIN_USER_STORAGE_ENCRYPTED` controls the shared-library target
`themis_user_storage_encrypted`; building it does not by itself prove the host
loads the plugin. Optional HTTP handler compilation is feature-gated and does
not prove HTTP route registration.

Focused test registration is in
[`tests/user_storage_encrypted/CMakeLists.txt`](../../tests/user_storage_encrypted/CMakeLists.txt).
Registration depends on the configured test build. Current E2E tests skip
unconditionally, and an unregistered test source is not execution evidence.
Benchmark registration is in
[`benchmarks/user_storage_encrypted/CMakeLists.txt`](../../benchmarks/user_storage_encrypted/CMakeLists.txt);
some cases use stubs or sleeps, so source labels and thresholds are not
measurements or performance gates.

## Generated evidence and source priority

[`DOXYGEN.md`](../../src/user_storage_encrypted/DOXYGEN.md) is a derived
structure/function snapshot, not an API authority. The current
[`MODULE_GAPS.md`](../../src/user_storage_encrypted/MODULE_GAPS.md) is an
automated scanner report. Treat findings as candidates for code-level
verification, not confirmed defects. Check current implementation and tests
before relying on a finding.

For code changes, consult module
[`README.md`](../../src/user_storage_encrypted/README.md),
[`ARCHITECTURE.md`](../../src/user_storage_encrypted/ARCHITECTURE.md), and
[`API.md`](../../src/user_storage_encrypted/API.md), then verify the exact
behavior in public headers, implementation, and test/benchmark CMake
registration. Root governance takes precedence over this wiki summary.
