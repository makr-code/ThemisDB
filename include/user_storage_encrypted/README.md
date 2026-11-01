---
Author: ThemisDB Contributors
Created: 2026-05-13
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
---

# user_storage_encrypted public headers

Exact model backend not verifiable from final document text alone.

The operational C++ API is in namespace `themis::plugins::user_storage`.
The separate frozen contract header uses `themis::user_storage_encrypted`;
these are distinct interfaces. See the [implementation API notes](../../src/user_storage_encrypted/API.md)
for current runtime behavior and the contract/implementation discrepancy.

## Header map

| Header | Public declarations |
|---|---|
| `encryption_backend_interface.hpp` | `Result<T>`, `EncryptionBackendInterface` |
| `gocryptfs_backend.hpp` | `GocryptfsBackend`, including public command/stdin helpers |
| `key_derivation_service.hpp` | `IKeyDerivationService`, `KeyDerivationService`, `Argon2idParams`, `Argon2idKeyDerivationService` |
| `irotation_store.hpp` | `IRotationStore`, `NullRotationStore`, `FileRotationStore` |
| `key_rotation_scheduler.hpp` | `KeyRotationScheduler` |
| `multi_level_storage.hpp` | `LevelConfig`, `StorageMetrics`, `MultiLevelEncryptedStorage` |
| `security_level.hpp` | `SecurityLevel` and string conversion helpers |
| `user_models.hpp` | `User`, `Group`, `HealthStatus` |
| `command_timeout_manager.hpp` | `CommandTimeoutManager` |
| `error_codes.hpp` | Operational `ErrorCode`, `DiagnosticEvent`, event hooks |
| `pipe_guard.hpp` | POSIX `PipeGuard` |
| `timed_file_operation.hpp` | POSIX timed file-descriptor I/O types |
| `user_storage_encrypted_api_contract.h` | Separate error-value contract and fail-closed predicate |

`MultiLevelEncryptedStorage` implements `IThemisPlugin` and has
`bool initialize(const char*)`, `void shutdown()`, user/group CRUD, mount,
unmount, rotation, health, and metrics methods. The interface and exact
signatures are declared in `multi_level_storage.hpp`.

## KDF and rotation interfaces

`Argon2idKeyDerivationService` implements two APIs:

- `IKeyDerivationService::deriveKey(master_key, salt)` returns
  `Result<std::vector<uint8_t>>`; `generateSalt()` and
  `loadOrCreateSalt(path)` also return `Result`.
- `KeyDerivationService::derive(master_key, user_id, container_id, salt)`
  returns a byte vector and may throw; `generateSalt(size_t)` returns a byte
  vector and may throw.

The compiled KDF path depends on CMake's libargon2 detection. Without
libargon2 the implementation uses its PBKDF2-HMAC-SHA256 fallback.
`Argon2idParams` affects the domain-separated `derive()` API when Argon2id is
available; built-in `deriveKey()` uses fixed constants instead. In the PBKDF2
fallback, the `derive()` output length is configurable through `params_`, but
its iteration count remains fixed at 310,000 rounds. See the [API details](../../src/user_storage_encrypted/API.md).

`KeyRotationScheduler::initialize()` accepts an optional
`std::shared_ptr<IRotationStore>`. Its behavior is independent of
`MultiLevelEncryptedStorage` unless the caller explicitly wires them together.

## Runtime configuration shape

`MultiLevelEncryptedStorage::initialize(const char*)` receives JSON rooted at
`multi_level_storage.levels`. Each array item uses `name` and `encrypted`;
unencrypted items use `path`, while encrypted items use `encrypted_dir`,
`mount_point`, and a nested `encryption` object. `backend` belongs inside
`encryption`, not at the level root; it is parsed but does not select an
alternative backend in the current orchestrator, which constructs
`GocryptfsBackend`. Optional `rotation` settings are nested inside each
encrypted level; parsing them does not wire or start the standalone scheduler.

```json
{
  "multi_level_storage": {
    "levels": [
      {
        "name": "offen",
        "encrypted": false,
        "path": "/var/lib/themisdb/offen"
      },
      {
        "name": "vs-nfd",
        "encrypted": true,
        "encrypted_dir": "/var/lib/themisdb/encrypted/vs-nfd",
        "mount_point": "/var/lib/themisdb/mnt/vs-nfd",
        "encryption": {
          "backend": "gocryptfs",
          "key_id": "user_storage_vs_nfd",
          "key_provider": "mock"
        },
        "rotation": {
          "enabled": true,
          "interval_days": 90,
          "auto_rotate": true
        }
      }
    ]
  }
}
```

The shipped `.yaml.example` is illustrative YAML and is not passed directly to
`initialize()`; the runtime parser expects JSON. See the [runtime API notes](../../src/user_storage_encrypted/API.md).

## Build

`src/user_storage_encrypted/CMakeLists.txt` defines the
`themis_user_storage_encrypted` shared library when
`THEMIS_PLUGIN_USER_STORAGE_ENCRYPTED` is enabled. The option is defined with
default `ON`, subject to edition configuration. The target installs public
headers; consumers should use the repository's supported target/link
configuration rather than treating this directory as a header-only library.

## Documentation

- [Public-header architecture](ARCHITECTURE.md)
- [Header roadmap](ROADMAP.md)
- [Header future enhancements](FUTURE_ENHANCEMENTS.md)
- [Implementation API reference](../../src/user_storage_encrypted/API.md)
- [Module architecture](../../src/user_storage_encrypted/ARCHITECTURE.md)
- [Module security notes](../../src/user_storage_encrypted/SECURITY.md)
