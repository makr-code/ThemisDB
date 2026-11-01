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

# Modeling Notes for user_storage_encrypted

Exact model backend not verifiable from final document text alone.

The implementation models four `SecurityLevel` enum values and a
`LevelConfig` map. The enum names and string conversion are defined in
`include/user_storage_encrypted/security_level.hpp`; actual paths, provider
selection, and rotation fields are configured per level in
`multi_level_storage.hpp` and `multi_level_storage.cpp`.

## Configuration model

`MultiLevelEncryptedStorage::initialize()` expects
`multi_level_storage.levels`. Plain entries require `name`, `encrypted:
false`, and `path`. Encrypted entries require `name`, `encrypted: true`,
`encrypted_dir`, `mount_point`, and `encryption.key_id`. Provider selection is
read from the nested `encryption` object; the source handles `vault`, `hsm`,
and `mock`.

Descriptions that map `VS_NFD`/`GEHEIM` to Vault, `STRENG_GEHEIM` to HSM, or
set specific rotation periods describe the shipped example configuration,
not policy defaults enforced by the enum. The example's settings are in
`plugins/user_storage_encrypted/config/storage_config.yaml.example`.

## Runtime flow and boundaries

1. The plugin parses and validates level configuration.
2. Initialization attempts stale-mount reconciliation.
3. Each configured encrypted level constructs `GocryptfsBackend`, checks
   availability, resolves its provider, creates a container when needed, and
   mounts it. Plain levels create their configured directory.
4. User and group records are JSON files beneath each level's mounted/base
   directory.
5. Health and metrics are exposed by `MultiLevelEncryptedStorage`.

The current multi-level plugin constructs `GocryptfsBackend` without a KDF
service. The backend can be constructed directly with a
`KeyDerivationService*`, but this is not configured by `MultiLevelEncryptedStorage`.

`rotation` fields are parsed but not used to configure a scheduler.
`KeyRotationScheduler` is a separate API, and automatic rotation requires
caller wiring. `rotateKey()` is a distinct orchestrator operation.

## Failure and observability boundaries

Operational methods generally return `Result<T>` error strings; they do not
uniformly produce the frozen contract enum. Some failure paths are not
propagated: `unmountAll()` ignores per-level unmount results; scheduler
callback exceptions are swallowed and the schedule timestamp is advanced;
the container-size metric is declared/emitted but not updated by current
source. `checkLevelHealth()` checks backend presence and mount state only.

`user_storage_encrypted_api_contract.h` is a separate contract namespace with
stronger guarantees in comments than all current implementation paths
demonstrate. Treat the discrepancy as unresolved rather than assuming
atomicity or fail-closed behavior beyond the implemented path.

## Source references

- `include/user_storage_encrypted/security_level.hpp`
- `include/user_storage_encrypted/multi_level_storage.hpp`
- `include/user_storage_encrypted/user_models.hpp`
- `include/user_storage_encrypted/key_rotation_scheduler.hpp`
- `src/user_storage_encrypted/multi_level_storage.cpp`
- `src/user_storage_encrypted/key_rotation_scheduler.cpp`
- `plugins/user_storage_encrypted/config/storage_config.yaml.example`
