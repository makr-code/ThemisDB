---
Author: ThemisDB Contributors
Created: 2026-10-21
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
---

# Implementation Notes for user_storage_encrypted

Exact model backend not verifiable from final document text alone.

## Backend: `gocryptfs_backend.cpp`

`GocryptfsBackend` validates encrypted-directory and mount-point paths in
`createContainer()` and `mountContainer()`, checks mount state, resolves key
material, and invokes gocryptfs via argument vectors rather than a shell.
Key bytes are supplied through a stdin pipe on supported POSIX builds.
`checkAvailability()` probes `gocryptfs` and, on Linux, `/dev/fuse`.
The backend's public constructor accepts an optional non-owning
`KeyDerivationService*`; with no service it uses the supplied key material
directly. The multi-level plugin currently constructs it without a KDF.

Do not generalize this to “all external commands validate all arguments”:
availability probes use constant command arguments, and stale-mount cleanup
uses its own process-execution path. Some backend command paths use timed
pipe I/O, but the source does not establish one end-to-end process timeout
guarantee for every invocation.

## Key derivation: `key_derivation_service.cpp`

`Argon2idKeyDerivationService` implements both
`IKeyDerivationService` (Result-based methods) and
`KeyDerivationService` (domain-separated vector methods). CMake enables
Argon2id when libargon2 is found; otherwise code uses a PBKDF2-HMAC-SHA256
fallback. `setDeriveKeyFn()` is a test/custom bridge; if the injected callback
throws, `deriveKey()` falls through to the built-in implementation.

The constructor's `Argon2idParams` are not shared by both APIs: the built-in
Argon2id path in `deriveKey()` uses fixed `kTimeCost`, `kMemoryCost`,
`kParallelism`, and `kKeyLength`, ignoring `params_`. The domain-separated
`derive()` path uses `params_` for Argon2id. Its PBKDF2 fallback uses the
configured output length but a fixed 310,000 rounds, not `params_.iterations`.

`GocryptfsBackend::resolveKey()` stores a raw salt file at
`<encrypted_dir>/.themis_kdf_salt` when a KDF service is configured. The
multi-level orchestrator does not configure this service. Salt persistence
does not itself establish permissions, backup, or key-version policy.

`Argon2idKeyDerivationService::derive()` clears its temporary concatenated
password buffer after the KDF call, but does not wipe the caller-owned
master-key or salt inputs. Other temporary key buffers have only path-specific
cleanup; this is not the frozen contract's general passphrase-and-salt zeroing
guarantee. See [API contract comparison](API.md).

## Rotation: `key_rotation_scheduler.cpp`

The scheduler has one dedicated polling thread for eligible automatic
schedules, per-security-level state, and optional `IRotationStore` persistence.
Scheduled callbacks are called with `(level, true, "")`; thrown callback
exceptions are swallowed. The scheduler then advances and persists
`last_check_ms`, including after a thrown callback. `isRotationDue()` and
`getNextRotationTime()` only check/report schedule timestamps.
`triggerRotation()` manually invokes the callback on the caller's thread,
without checking due time or `auto_rotate`; it is not a storage rotation. The
scheduler mutex is held during callbacks, so re-entry into scheduler methods
can deadlock. `shutdown()` wakes the condition variable and joins the polling
thread.

The separate `MultiLevelEncryptedStorage::rotateKey()` calls the configured
key provider, creates and mounts a new container, copies data, attempts
to unmount both containers, renames directories, and remounts. It is not called by
`triggerRotation()` or wired to parsed scheduler configuration. Some
intermediate unmount and rollback results are ignored; remount failure can
return after the new directory is installed, with the old directory left as a
backup and no code rollback. Successful remount is followed by best-effort
backup removal; failure only logs a warning. The unmount/rename/remount
sequence provides no zero-downtime guarantee. The source does not
establish a transactional rotation or guarantee that the old key remains
active on every failure path.

## Orchestration: `multi_level_storage.cpp`

Configuration is read from `multi_level_storage.levels`. Plain entries use
`path`; encrypted entries use `encrypted_dir`, `mount_point`, and nested
`encryption.key_id` / provider fields. Validation checks required strings but
does not implement every security or deployment policy.

Initialization reconciles mount state, initializes each level, probes
gocryptfs for encrypted entries, resolves the provider, creates a container
when needed, and mounts it. User/group records are JSON files in per-level
`users/` and `groups/` paths. `checkLevelHealth()` checks backend presence
and mount state, not provider or scheduler health.

Metrics are emitted as Prometheus text. Mount and rotation counters are
updated by their respective operations, but the current source does not
update `container_size_bytes`. `unmountAll()` currently discards individual
unmount errors and returns success; callers should not infer successful
unmount from its return value alone.

## Contract and source references

The header `user_storage_encrypted_api_contract.h` describes a separate
error-value contract and includes guarantees not all demonstrated by the
operational source. See [API.md](API.md) for the discrepancy and follow-up
needed. Canonical implementation references:

- `include/user_storage_encrypted/gocryptfs_backend.hpp`
- `include/user_storage_encrypted/key_derivation_service.hpp`
- `include/user_storage_encrypted/key_rotation_scheduler.hpp`
- `include/user_storage_encrypted/multi_level_storage.hpp`
- `src/user_storage_encrypted/CMakeLists.txt`
- `src/user_storage_encrypted/gocryptfs_backend.cpp`
- `src/user_storage_encrypted/key_derivation_service.cpp`
- `src/user_storage_encrypted/key_rotation_scheduler.cpp`
- `src/user_storage_encrypted/multi_level_storage.cpp`
