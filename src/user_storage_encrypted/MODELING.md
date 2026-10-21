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

# Modeling Notes for user_storage_encrypted

Exact model backend not verifiable from final document text alone.

This module models encrypted user storage as a security-tiered container system. The canonical contract is spread across `include/user_storage_encrypted/security_level.hpp`, `user_models.hpp`, `multi_level_storage.hpp`, and `user_storage_encrypted_api_contract.h`.

## Security-level model

The module defines four levels:

| Level | Name | Intended behavior |
|---|---|---|
| `OFFEN` | `offen` | unencrypted baseline storage path used only where the system explicitly chooses a plain layer |
| `VS_NFD` | `vs-nfd` | encrypted storage with Vault-based key provider |
| `GEHEIM` | `geheim` | encrypted storage with stricter vault behavior |
| `STRENG_GEHEIM` | `streng-geheim` | encrypted storage with HSM-backed key provider |

The naming and behavior are implemented in `security_level.hpp`, and the `LevelConfig` structure in `multi_level_storage.hpp` carries the actual runtime configuration for each level. The module uses a map of `SecurityLevel` to `LevelConfig`, with per-level fields for mount path, backend, key provider, rotation interval, and notification policies.

`OFFEN` is a policy-level baseline rather than a privileged encrypted-storage tier. Its plain storage path is intentionally different from the fail-closed semantics used for encrypted levels. The fail-closed contract in `user_storage_encrypted_api_contract.h` remains relevant for any level that depends on backend or KDF availability, but operators should not treat the `OFFEN` path as a security-equivalent replacement for encrypted storage.

## Runtime model

The runtime model is intentionally simple and bounded:

1. A security level resolves to a `LevelConfig`.
2. That config identifies whether the level is encrypted.
3. The module initializes the level-specific backend and key provider.
4. It mounts or reuses a container at the configured path.
5. User and group records are stored in a level-specific file layout.
6. Rotation and failure-handling logic are evaluated at the level boundary.

This is more than a storage abstraction; it is a policy model. The operational guarantee is not "everything is encrypted" but "each security tier chooses a configured backend, key provider, and rotation schedule while failing closed on invalid conditions."

## Key-provider model

`MultiLevelEncryptedStorage` stores provider metadata per level in `LevelConfig` and later resolves the concrete provider through `getKeyProvider()`. The header imports security dependencies such as `vault_key_provider.h` and `hsm_key_provider_adapter.h`; a `mock_key_provider.h` reference is present in the source tree for test and validation use, but it is not a production key-provider strategy and should not be treated as a deployable runtime backend.

Important invariants:

- a missing or invalid provider is not silently accepted
- rotation state is tracked per security level
- stronger levels must be able to reject or fail closed when provider assumptions break

## Rotation model

The rotation model is scheduler-driven. `KeyRotationScheduler` uses a `RotationSchedule` map keyed by `SecurityLevel`; callbacks can trigger rotation when the interval is exceeded. The schedule also persists `last_check_ms` and `interval_days` into a backing store using JSON and a key naming convention such as `user_storage:rotation_state:{level}`.

This creates a durable model where the scheduler can recover from restarts without losing the notion of when a rotation was last checked.

## Failure domains

The source identifies these main failure domains:

- backend availability (`gocryptfs` not installed or unsupported FUSE)
- path validation failures (`..`, invalid chars, bad flags)
- key derivation failures (empty keys, invalid salts, Argon2id/PBKDF2 failure)
- stale mount residue after crash or process restart
- rotation callback or persistence failures

These failures are treated as explicit runtime conditions, not as hidden fallbacks. The module is designed to keep a failure isolated to the affected security level and to expose the condition to logs and diagnostics.

## Data ownership and lifecycle model

The implementation keeps user and group data in level-scoped file paths and relies on container-level metadata to determine mount state. `MultiLevelEncryptedStorage` is therefore a container orchestrator, not a generic data store abstraction.

The intended lifecycle is:

- configure security levels
- validate and reconcile state
- initialize each level
- mount all configured containers
- rotate keys on schedule
- fail closed under backend or KDF errors
- unmount and clean up when shutting down

## Limitations of the current model

The model is deliberately bounded to host-environment control, not general-purpose encrypted object storage. It does not model distributed consensus, cross-node replication, or complex key escrow flows beyond the provider and scheduler abstractions already visible in the code. The module's roadmap and design comments acknowledge that stronger operational hardening and wider integration coverage remain outstanding work.

## Source-grounded summary

The code makes the model explicit:

- `multi_level_storage.cpp` configures and validates level layouts
- `gocryptfs_backend.cpp` defines backend validation and mount failure boundaries
- `key_derivation_service.cpp` defines KDF behavior and compatibility constraints
- `key_rotation_scheduler.cpp` defines scheduling and persistence

The modeling is therefore not speculative: it is the direct runtime structure encoded in the current module implementation.
