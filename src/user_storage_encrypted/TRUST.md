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

# Trust and Risk Model for user_storage_encrypted

Exact model backend not verifiable from final document text alone.

This module's trust model is straightforward: the system is trusted only when the encrypted backend, the key derivation path, and the rotation scheduler all remain valid. The public contract in `include/user_storage_encrypted/user_storage_encrypted_api_contract.h` is explicit about fail-closed handling: backend unavailability, KDF failure, and internal errors are treated as denial conditions.

## Trust boundaries

The module spans multiple trust boundaries:

- host OS / gocryptfs boundary
- key material and salt boundary
- configuration validation boundary
- mount lifecycle boundary
- scheduler and persistence boundary

These boundaries matter because the module does not assume that any one layer is inherently safe. Valid configuration and successful backend validation are required before an encrypted storage layer is treated as operational.

## Risk model

The primary risks are:

1. invalid or hostile paths reaching subprocess execution
2. absent or unsupported backend dependencies
3. weak or incompatible key derivation assumptions
4. stale mount residue from prior failures
5. rotation state corruption or callback misfire

The source addresses these risks directly through validation, fail-closed logic, explicit error states, and stale mount cleanup. In other words, the module is designed to fail loudly when trust cannot be established rather than silently degrade into a lower-security mode.

## Security controls implemented in source

The implementation demonstrates several controls:

- path sanitization and flag validation in `gocryptfs_backend.cpp`
- Argon2id and PBKDF2 KDF decisions in `key_derivation_service.cpp`
- strict rotation state persistence and interval checks in `key_rotation_scheduler.cpp`
- configuration validation and stale mount reconciliation in `multi_level_storage.cpp`

These controls are not mere comments; they are actual runtime checks embedded in the logic path.

## Trust assumptions that are not safe

The following assumptions should not be treated as valid in production:

- a missing gocryptfs binary is a harmless nuisance
- a key derivation fallback can silently replace a stronger production path
- rotation can continue without persistent state validation
- stale mount state is always harmless and can be ignored
- a partially successful mount should be considered a healthy storage outcome

The code contradicts each of these assumptions by using explicit error propagation and fail-closed behavior.

## Incident handling

When trust breaks, the module is designed to preserve clear evidence and stop the unsafe flow.

Recommended handling pattern:

1. isolate the failing level or backend path
2. preserve logs, metrics, and scheduler state
3. determine whether the issue is in host support, KDF path, or rotation logic
4. restore a known-good configuration and re-check health before re-enabling storage operations

### Incident priority

High-priority incidents include:

- repeated backend unavailable errors
- key derivation failure across multiple containers
- unmount or stale-mount loops at startup
- rotation state corruption after a restart

These are all explicitly visible in source and should not be treated as routine operational noise.

## Current trust posture

The module is source-grounded and production-oriented, but it is not presented as a purely immutable trust boundary. The roadmap and the implementation comments explicitly note continuing hardening work. The correct posture is: strong runtime controls with remaining operational hardening, not blanket trust that all future operating conditions are closed.
