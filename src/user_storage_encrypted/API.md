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

# API Reference for user_storage_encrypted

Exact model backend not verifiable from final document text alone.

This page summarizes the public interface surface and the operational contract implemented by the module. The source of truth is `include/user_storage_encrypted/*.hpp` and the frozen contract in `include/user_storage_encrypted/user_storage_encrypted_api_contract.h`.

## 1. Security-level model

`SecurityLevel` is the primary classification surface.

```cpp
namespace themis::plugins::user_storage {
    enum class SecurityLevel {
        OFFEN,
        VS_NFD,
        GEHEIM,
        STRENG_GEHEIM
    };
}
```

The associated conversion helpers are `securityLevelToString()` and `stringToSecurityLevel()`. The conversion is intentionally strict; invalid strings are treated as failures rather than silently coerced defaults.

## 2. Result and backend interfaces

`Result<T>` and `EncryptionBackendInterface` define the generic error-propagation contract used across backend interactions.

The backend interface models:

- initialize(config_json)
- createContainer(...) / mountContainer(...) / unmountContainer(...)
- isMounted(...)
- checkAvailability()
- backend identity and version queries

The implementation uses `Result<void>` and related typed results to propagate failures such as backend unavailability, invalid path, or permission-related issues. `user_storage_encrypted_api_contract.h` defines the canonical error taxonomy and fail-closed contract.

## 3. gocryptfs backend contract

`GocryptfsBackend` is the concrete backend implementation that wraps the gocryptfs binary. It accepts a `KeyDerivationService*` in the constructor so that the backend can use a derived per-container key instead of passing raw secrets to the OS process.

The backend implementation expects:

- absolute or safe relative mount paths only
- reject invalid characters or traversal attempts
- explicit key validation before executing external commands
- host support for gocryptfs and FUSE to be verified before mount activities

If those checks fail, the caller receives an explicit error and mount operations do not continue silently.

## 4. Key derivation API

`Argon2idKeyDerivationService` is the default KDF provider for this module. The service exposes:

- `deriveKey(master_key, salt)`
- `generateSalt()`
- `loadOrCreateSalt(salt_file_path)`
- runtime override support via `setDeriveKeyFn()`

The service is safe by default: it rejects empty or invalid input and falls back to PBKDF2 when `libargon2` is unavailable. The code comments explicitly call out that older non-Argon2 key material is not compatible with the current fallback path.

## 5. Rotation scheduler API

`KeyRotationScheduler` manages rotation state and triggers callbacks when intervals expire.

Core operations:

- `initialize(check_interval_seconds, store)`
- `scheduleRotation(level, interval_days, auto_rotate, callback)`
- `cancelRotation(level)`
- `isRotationDue(level, last_rotation_ms)`
- `getNextRotationTime(level)`
- `triggerRotation(level)`
- `shutdown()`

The implementation persists schedule state in a rotation store and keeps rotation checks behind a dedicated scheduler thread. It also exposes `IRotationStore` / `FileRotationStore` so rotation state can be persisted without coupling the scheduler to a specific storage backend.

## 6. Multi-level orchestration API

`MultiLevelEncryptedStorage` is the orchestration entry point. It manages user and group storage, level configuration, mount state, key rotation, and runtime health.

Key APIs:

- `initialize(config_json)`
- `shutdown()`
- `mountAll()`, `unmountAll()`
- `mountLevel(level)`, `unmountLevel(level)`
- `rotateKey(level)`
- `checkHealth()`, `checkLevelHealth(level)`
- `getMetricsText()`
- `recordKeyRotation(level)`

It is intentionally fail-closed: when a backend or key path is unavailable, the API does not continue with a misleading success result.

## 7. Error taxonomy

The module's canonical error set is defined in `UserStorageEncryptedError` in `user_storage_encrypted_api_contract.h`.

Key values:

- `kMountFailed`
- `kUnmountFailed`
- `kKeyDerivationFailed`
- `kRotationFailed`
- `kBackendUnavailable`
- `kInvalidPath`
- `kPermissionDenied`
- `kInternalError`

The module also defines `isUserStorageEncryptedFailClosed()` to centralize the rule that backend unavailability, key derivation failure, and internal errors all require fail-closed denial.

## 8. Contract expectations for callers

Callers must assume:

- storage operations may fail if gocryptfs or FUSE is unavailable
- mount paths must be safe and explicitly validated
- rotation state may need operator attention if persistence or callback logic fails
- the module is not designed for silent fallback from encrypted storage to plaintext storage

The public surface is therefore narrow and explicit: it favors clear failure signals over permissive recovery behavior.
