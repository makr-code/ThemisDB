---
Author: ThemisDB Contributors
Created: 2026-10-21
Last Updated: 2026-10-21
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: unknown
Human_Review: true
Human_Author: ThemisDB Contributors
---

# Research Notes for user_storage_encrypted

Exact model backend not verifiable from final document text alone.

This module exists to make encrypted user storage a bounded, observable subsystem instead of a loose collection of shell invocations and ad hoc key handling. The source under `src/user_storage_encrypted` shows three design anchors: a `gocryptfs` backend boundary, an Argon2id/PBKDF2 key derivation service, and a scheduler-driven multi-level storage orchestrator.

## Problem framing

The module's public contract in `include/user_storage_encrypted/user_storage_encrypted_api_contract.h` defines fail-closed semantics for backend, key, and rotation failures. In practice, the implementation enforces this through:

- backend validation before mount operations (`gocryptfs_backend.cpp`)
- strict key derivation validation in `key_derivation_service.cpp`
- rotation state persistence and scheduled callbacks in `key_rotation_scheduler.cpp`
- configuration validation and stale-mount reconciliation in `multi_level_storage.cpp`

The design assumes host availability is not guaranteed. `checkAvailability()` validates the `gocryptfs` binary and host/FUSE support while `reconcileStaleMounts()` cleans up stale mount state left behind by prior crashes. The module therefore treats the OS environment as a first-class failure domain rather than an implementation detail.

## Key design decisions

### 1. Backend isolation

`GocryptfsBackend` is intentionally a narrow integration layer around the host command line. Its implementation validates paths, rejects unsafe shell metacharacters, and uses a controlled pipe-based stdin delivery path for key material. This reduces command injection risk and keeps encrypted mount behavior inside a single backend contract.

The `CommandArgumentValidator` helpers in `gocryptfs_backend.cpp` explicitly reject traversal attempts (`..`) and unknown gocryptfs flags. This is a source-grounded design choice: the module assumes untrusted paths and mount configuration should be rejected before a subprocess is launched.

### 2. KDF reasoning

`Argon2idKeyDerivationService` is the module's primary production KDF path when `libargon2` is available. The implementation uses `argon2id_hash_raw()` with fixed time and memory parameters. The CMake logic in `src/user_storage_encrypted/CMakeLists.txt` sets `THEMIS_HAS_ARGON2` when `libargon2` is found; otherwise the module falls back to PBKDF2-HMAC-SHA256 and notes the compatibility split in the source comments.

This means:

- Argon2id is treated as the default strong path.
- The runtime injection bridge in `setDeriveKeyFn()` is intentionally supplemental for testing or custom providers.
- The module and roadmap explicitly warn that older iterative SHA-256-derived keys are not compatible with the current fallback path.

### 3. Rotation and orchestration

`KeyRotationScheduler` persists rotation state in JSON via `IRotationStore` implementations and keeps a dedicated thread to poll configured levels. `MultiLevelEncryptedStorage` then orchestrates mount lifecycle, key provider selection, and failure-bounded level configuration.

The module does not assume all security levels are equally sensitive. `SecurityLevel` values (`OFFEN`, `VS_NFD`, `GEHEIM`, `STRENG_GEHEIM`) map to different storage policies, with stronger levels using stricter provider assumptions and shorter rotation intervals as defined in `include/user_storage_encrypted/user_models.hpp` and `multi_level_storage.hpp`.

## Trade-offs and boundaries

### Security trade-offs

- The backend depends on gocryptfs and FUSE support, so availability is a hard operational dependency.
- Key material is handled in-memory and then zeroed as part of a best-effort cleanup path; the code does not pretend that memory clearing is a substitute for strong system security.
- Rotation relies on explicit callbacks and persisted state, which makes operation observable but also requires disciplined scheduler and callback design.

### Operational trade-offs

- `reconcileStaleMounts()` chooses to continue startup after a failed unmount rather than hard-failing, so the module remains resilient to previous crashes but may leave a degraded state visible to operators.
- Storage metrics are intentionally simple and Prometheus-ready; they prioritize explicit operational visibility over rich analytics.

## Validation approach

The implementation already indicates the intended validation path:

- backend availability and mount validation under `gocryptfs_backend.cpp`
- KDF derivation and salt persistence under `key_derivation_service.cpp`
- rotation scheduling and persistence under `key_rotation_scheduler.cpp`
- level validation, stale mount cleanup, and orchestration under `multi_level_storage.cpp`

This module is best validated through integration tests that exercise the real lifecycle boundary (create mount, rotate, unmount, remount) rather than only unit-level key derivation checks.

## Current status

The source shows the module is production-oriented but still operating under residual hardening work. The roadmap in `src/user_storage_encrypted/ROADMAP.md` explicitly tracks mount and scheduler hardening, benchmark re-baselining, and wider integration coverage. That means the research position is not "complete and closed," but "functionally production-ready with continuing hardening and operator evidence work."
