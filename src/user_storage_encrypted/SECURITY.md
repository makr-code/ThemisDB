---
Author: ThemisDB Contributors
Created: 2026-10-30
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
---

# Security - User Storage Encrypted Module

Exact model backend not verifiable from final document text alone.

Report vulnerabilities privately through [project SECURITY.md](../../SECURITY.md).
This page describes observed implementation controls and limitations; it is
not a security certification or guarantee of host/key-provider security.

## Trust boundaries

- Host OS, gocryptfs executable, FUSE, and mount permissions.
- Key-provider access and returned key material.
- KDF build selection and salt-file persistence, when using a KDF-enabled
  backend directly.
- Multi-level JSON configuration and filesystem paths.
- Rotation callbacks, persisted state, and container replacement.

## Controls visible in source

- `createContainer()` and `mountContainer()` validate the encrypted-directory
  and mount-point paths before their gocryptfs lifecycle command.
- Backend commands are executed through argument vectors rather than a shell.
- The backend supplies key material through stdin on supported POSIX builds.
- Empty master keys and empty salts are rejected by `deriveKey()`; the
  domain-separated `derive()` path requires at least an 8-byte salt.
- The KDF build uses Argon2id when libargon2 is found and a PBKDF2-HMAC-SHA256
  fallback otherwise.
- Linux backend availability checks probe `gocryptfs` and `/dev/fuse`.

These controls do not establish that all process paths have one bounded
timeout, that key material is never present in memory, or that every cleanup
failure is surfaced.

The frozen contract's key-zeroing statement is not a general runtime guarantee.
The KDF clears its temporary concatenated password in `derive()`, but not the
caller-owned master-key or salt inputs. The backend clears a temporary hex
string in one byte-vector stdin path; other copies, including the string-based
stdin input, are not wiped by that code. See the [contract comparison](API.md).

## Important limitations

- The multi-level plugin constructs `GocryptfsBackend` without a KDF service;
  its parsed configuration does not enable per-container derivation.
- Rotation settings are parsed but are not connected to
  `KeyRotationScheduler`. The scheduler is a separate API.
- Scheduler callback exceptions are swallowed; the check timestamp is still
  advanced/persisted.
- The orchestrator's `unmountAll()` ignores individual unmount errors.
- Some intermediate unmount errors in `rotateKey()` are ignored. Do not
  assume an atomic or zero-downtime rotation guarantee.
- `KeyRotationScheduler::triggerRotation()` invokes only its callback; it does
  not run `MultiLevelEncryptedStorage::rotateKey()`. Automatic callbacks and
  manual callback invocations have different thread contexts. The callback
  non-reentry rule is a contract restriction, not a runtime safety mechanism.
- `OFFEN` is an explicitly supported unencrypted level. It is not equivalent
  in confidentiality to the encrypted levels and should not be used as an
  implicit recovery fallback.
- The frozen contract header describes stronger guarantees than some current
  operational paths demonstrate. See [API.md](API.md).

## Operational recommendations

Restrict the service account, mount points, encrypted directories, and salt
files according to deployment policy. Keep the matching salt and approved
provider key versions available for recovery. Independently validate host
authorization, audit logging, backup/recovery, and key-rotation procedures;
the module source does not establish those deployment controls.

On suspected key exposure or mount compromise, stop access to the affected
level if safe, preserve host and provider evidence, and follow the project's
incident process in [SOP.md](../../SOP.md). Verify recovery with the retained
salt and approved key version before reopening access.

## Source references

- `include/user_storage_encrypted/gocryptfs_backend.hpp`
- `include/user_storage_encrypted/key_derivation_service.hpp`
- `include/user_storage_encrypted/key_rotation_scheduler.hpp`
- `include/user_storage_encrypted/multi_level_storage.hpp`
- `src/user_storage_encrypted/gocryptfs_backend.cpp`
- `src/user_storage_encrypted/key_derivation_service.cpp`
- `src/user_storage_encrypted/key_rotation_scheduler.cpp`
- `src/user_storage_encrypted/multi_level_storage.cpp`
