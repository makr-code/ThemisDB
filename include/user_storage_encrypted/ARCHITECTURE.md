---
Author: ThemisDB Contributors
Created: 2026-06-01
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
---

# Public Header Architecture - user_storage_encrypted

Exact model backend not verifiable from final document text alone.

The public operational declarations are under `include/user_storage_encrypted/`
in namespace `themis::plugins::user_storage`. Runtime implementation details
are described in
[`../../src/user_storage_encrypted/ARCHITECTURE.md`](../../src/user_storage_encrypted/ARCHITECTURE.md).

## Interface groups

| Header | Public types | Purpose |
|---|---|---|
| `encryption_backend_interface.hpp` | `EncryptionBackendInterface`, `Result<T>` | Backend lifecycle interface and result wrapper |
| `gocryptfs_backend.hpp` | `GocryptfsBackend` | gocryptfs backend implementation |
| `multi_level_storage.hpp` | `MultiLevelEncryptedStorage`, `LevelConfig`, `StorageMetrics` | Plugin lifecycle, per-level storage operations, health, and metrics |
| `irotation_store.hpp` | `IRotationStore`, `NullRotationStore`, `FileRotationStore` | Rotation-state persistence interface and implementations |
| `key_derivation_service.hpp` | `IKeyDerivationService`, `KeyDerivationService`, `Argon2idKeyDerivationService` | KDF interfaces and implementation |
| `key_rotation_scheduler.hpp` | `KeyRotationScheduler` | Separate rotation scheduling API |
| `security_level.hpp` | `SecurityLevel` | Four security-level values and string conversion |
| `user_models.hpp` | `User`, `Group`, `HealthStatus` | Stored data and health result types |
| `error_codes.hpp` | `ErrorCode`, `DiagnosticEvent` | Operational diagnostic types |

`user_storage_encrypted_api_contract.h` is a separate header using namespace
`themis::user_storage_encrypted`; it is not the namespace that contains the
operational plugin classes.

## Build boundary

`src/user_storage_encrypted/CMakeLists.txt` builds a shared library when
`THEMIS_PLUGIN_USER_STORAGE_ENCRYPTED` is enabled. This is not a header-only
API. The module implementation and CMake target are the evidence for build
and runtime integration; this document does not promise ABI stability beyond
the repository's versioning policy.

## References

- [Header inventory](README.md)
- [Header roadmap](ROADMAP.md)
- [Runtime architecture](../../src/user_storage_encrypted/ARCHITECTURE.md)
- [API and contract discrepancy](../../src/user_storage_encrypted/API.md)
