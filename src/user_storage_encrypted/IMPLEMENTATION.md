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

# Implementation Notes for user_storage_encrypted

Exact model backend not verifiable from final document text alone.

This document explains how the module's implementation maps to the contract in `include/user_storage_encrypted/user_storage_encrypted_api_contract.h` and the concrete source under `src/user_storage_encrypted`.

## Primary implementation files

### `gocryptfs_backend.cpp`

This file implements the backend lifecycle that creates and manages gocryptfs-backed encrypted directories.

The implementation includes:

- path validation (`validatePath` and `validateHexKey`)
- rejection of unsafe gocryptfs flags
- secure zeroization helper for transient key buffers
- `checkAvailability()` for binary and FUSE validation
- mount and unmount behavior for encrypted containers
- stale mount cleanup and command execution wrappers

The substantive invariant is that the module validates all arguments before it invokes external commands; command injection prevention is a deliberate implementation concern, not a secondary cleanup step.

### `key_derivation_service.cpp`

This file builds the KDF layer. The module uses Argon2id when the environment supports it and PBKDF2-HMAC-SHA256 as a fallback. The underlying code validates empty master keys and salts, generates random salts, and serializes salt persistence when needed.

Important implementation semantics:

- empty master_key or salt is rejected
- the injected derive-key callback is treated as optional and non-authoritative
- build-time `THEMIS_HAS_ARGON2` changes the real implementation path
- the fallback path is explicitly documented as not compatible with legacy key derivation results

### `key_rotation_scheduler.cpp`

This file implements rotation scheduling and persistence. It stores per-level state keyed by security level, persists JSON metadata such as `last_check_ms` and `interval_days`, and uses a dedicated thread to evaluate schedule health.

The scheduler intentionally ensures that shutdown wakes the thread promptly and that failed callback attempts are swallowed without corrupting schedule state. That is a design trade-off: it prioritizes scheduler liveness and resilience while still requiring operator observability outside the scheduler itself.

### `multi_level_storage.cpp`

This file is the orchestration layer. It:

- loads configuration
- validates level structure
- reconciles stale mounts left by earlier crashes
- initializes each configured level
- exposes mount/unmount/rotation behavior
- provides health and metrics output

The class also includes explicit log and error handling for stale mount cleanup, which is a direct operational measure to reduce divergence across restarts.

## Invariants and failure handling

The source consistently enforces the contract's fail-closed posture:

- invalid configuration does not continue silently
- unavailability of gocryptfs or FUSE is treated as an explicit error
- rotation callbacks are invoked with explicit state
- stale mounts are cleaned up but not treated as the caller's failure to recover

The module does not hide backend failures behind a generic success path. Instead, it surfaces them as errors or logs and keeps them visible for operator diagnosis.

## Observability and metrics

`MultiLevelEncryptedStorage::getMetricsText()` emits Prometheus-style metrics. The public API comments in `include/user_storage_encrypted/multi_level_storage.hpp` list the relevant families:

- `user_storage_mounts_active`
- `user_storage_mount_operations_total`
- `user_storage_key_rotations_total`
- `user_storage_container_size_bytes`

The implementation is designed to be readable by operators and integration tests without requiring deep internal knowledge of the low-level backend.

## Known implementation gaps

The source and roadmap together make the remaining work clear:

- introduce broader invalid-path and host-environment validation coverage
- harden scheduler failure handling across callback and persistence edge cases
- increase end-to-end encrypted lifecycle test coverage
- keep benchmark baselines aligned with production-like mount patterns

These gaps are part of the module's active roadmap and should be treated as expected follow-up work rather than as undocumented defects.
