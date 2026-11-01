---
Author: ThemisDB Contributors
Created: 2026-10-21
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
---

# Trust and Risk Model for user_storage_encrypted

Exact model backend not verifiable from final document text alone.

The module relies on trusted host filesystem/mount behavior, gocryptfs and
FUSE, provider/key state, configuration paths, and (where used) KDF salt and
rotation-state persistence. This page is a source-based risk summary, not a
production-readiness certification.

## Risks and implementation boundaries

| Risk | Source-visible behavior / limit |
|---|---|
| Unsafe path reaching gocryptfs lifecycle call | `createContainer()` and `mountContainer()` validate paths and use argument vectors; stale-mount cleanup has a separate process path. |
| Missing host backend | `checkAvailability()` probes gocryptfs and Linux `/dev/fuse`; host permissions still need deployment validation. |
| KDF mode or key mismatch | CMake selects Argon2id when libargon2 exists, otherwise PBKDF2-HMAC-SHA256. Compatibility with earlier data must be confirmed by its owner. |
| Salt loss | A KDF-enabled direct backend stores a raw salt file under the encrypted directory; preserve it with the corresponding key version. |
| Rotation failure/callback exception | The scheduler swallows callback exceptions and advances/persists its timestamp after an attempt. |
| Partial mount/unmount/rotation state | `unmountAll()` ignores individual results and some rotation unmount results are ignored. Verify actual host state. |
| Misleading health/metrics | Health checks mounted state and backend presence only; container-size metric is emitted but not updated by current source. |

The operational plugin does not instantiate a KDF-enabled backend or connect
its parsed rotation configuration to the scheduler. Those capabilities require
separate caller wiring. The `OFFEN` level is intentionally unencrypted; it
must not be treated as equivalent to an encrypted level.

## Contract discrepancy

`user_storage_encrypted_api_contract.h` declares error values and a
fail-closed predicate in a different namespace from the operational plugin.
It is normative contract text, not evidence that the operational implementation
meets each guarantee. In particular, mount failure cleanup, general key/salt
zeroing, transactional rotation, per-container serialization, and callback
thread/re-entry behavior differ or are not established by the current source.
See the [source-by-source comparison](API.md).

## Trust and recovery practices

Treat host access, key custody, mount/salt paths, and rotation state as trusted
operational dependencies. Preserve diagnostics and the approved salt/key
version during incident investigation. Before restoring access after a key,
mount, or provider incident, verify data access with the retained compatible
key material and inspect mount state independently.

## Source references

- `include/user_storage_encrypted/user_storage_encrypted_api_contract.h`
- `include/user_storage_encrypted/gocryptfs_backend.hpp`
- `include/user_storage_encrypted/key_derivation_service.hpp`
- `include/user_storage_encrypted/multi_level_storage.hpp`
- `src/user_storage_encrypted/gocryptfs_backend.cpp`
- `src/user_storage_encrypted/key_derivation_service.cpp`
- `src/user_storage_encrypted/key_rotation_scheduler.cpp`
- `src/user_storage_encrypted/multi_level_storage.cpp`
