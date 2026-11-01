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

# API Reference for user_storage_encrypted

Exact model backend not verifiable from final document text alone.

This reference describes the implemented C++ interfaces. The runtime plugin API
and the separate frozen value contract are not the same namespace or error
mechanism; see the discrepancy note below.

## Public C++ namespaces and types

The operational API in `include/user_storage_encrypted/*.hpp` is in
`themis::plugins::user_storage`. It includes:

- `SecurityLevel` and `securityLevelToString()` /
  `stringToSecurityLevel(const std::string&)`. The parser accepts exactly
  `offen`, `vs-nfd`, `geheim`, and `streng-geheim`, and throws
  `std::invalid_argument` for other strings.
- `Result<T>` / `Result<void>` and `EncryptionBackendInterface`.
- `GocryptfsBackend`, which implements `initialize(const std::string&)`,
  `createContainer(const std::string&, const std::string&,
  const std::vector<uint8_t>&)`, `mountContainer(...)`,
  `unmountContainer(const std::string&)`, `isMounted(const std::string&)`,
  `checkAvailability()`, `getBackendName() const`, and
  `getBackendVersion() const`.
- `IKeyDerivationService` (`deriveKey`, `generateSalt`,
  `loadOrCreateSalt`) and `KeyDerivationService` (`derive`, sized
  `generateSalt`). `Argon2idKeyDerivationService` implements both interfaces.
- `IRotationStore`, `NullRotationStore`, and `FileRotationStore`.
- `KeyRotationScheduler`, with `initialize(int,
  std::shared_ptr<IRotationStore>)`, `scheduleRotation(SecurityLevel, int,
  bool, RotationCallback)`, `cancelRotation`, `isRotationDue`,
  `getNextRotationTime`, `triggerRotation`, and `shutdown`.
- `MultiLevelEncryptedStorage`, an `IThemisPlugin` implementation. Its plugin
  lifecycle is `bool initialize(const char*)` and `void shutdown()`. It also
  exposes user/group CRUD, mount/unmount, key rotation, health, and metrics
  methods in `multi_level_storage.hpp`.

All fallible operational methods return `Result<T>` except the plugin
`initialize(const char*)`, which returns `bool`; check results before accessing
`Result<T>::value()`.

## Configuration and runtime wiring

`MultiLevelEncryptedStorage::initialize()` expects JSON with a
`multi_level_storage.levels` array. Each level uses `name` and `encrypted`.
Plain levels require `path`; encrypted levels require `encrypted_dir`,
`mount_point`, and `encryption.key_id`. The `encryption` object is parsed for
`backend`, `key_provider`, Vault fields, and HSM fields; providers implemented
by `getKeyProvider()` are `vault`, `hsm`, and `mock`. Although `backend` is
parsed, the current orchestrator constructs `GocryptfsBackend` directly rather
than selecting an implementation from that field. A Vault address and HSM
library path are required for their respective providers.

Each encrypted level may contain a `rotation` object, which is parsed into that
level's `LevelConfig`. The `MultiLevelEncryptedStorage` implementation does not
attach or start a `KeyRotationScheduler`, however, so parsed rotation options
do not schedule or execute rotations. Use of `KeyRotationScheduler` and
automatic rotation therefore requires separate caller wiring. Likewise, the
plugin constructs `GocryptfsBackend` without a KDF service; KDF-enabled backend
use is available through direct construction but is not configured by the
multi-level plugin.

The backend sends gocryptfs key material through a stdin pipe on supported
POSIX builds. The backend returns an explicit unsupported-platform error on
Windows in its command/stdin paths. On Linux, availability checks probe
`gocryptfs` and `/dev/fuse`. On macOS, the source probes a macFUSE/osxfuse
package identifier with `pkgutil` and may log a warning if it is absent; the
check still returns success and does not verify a complete mount lifecycle.
Do not treat this probe as a broad macOS support or compatibility guarantee.

## KDF behavior

When CMake finds libargon2, the build defines `THEMIS_HAS_ARGON2` and the
`deriveKey()` path uses Argon2id. Otherwise it uses the built-in
PBKDF2-HMAC-SHA256 fallback. The `derive()` overload combines the master key,
user ID, and container ID before derivation. The backend stores a raw salt file
at `<encrypted_dir>/.themis_kdf_salt` when a KDF service is supplied. Preserve
the salt and the matching key/provider version needed to reopen data.

`Argon2idKeyDerivationService` accepts `Argon2idParams`, but the two APIs do
not use those settings identically. Without an injected `deriveKey` function,
the Argon2id `deriveKey()` path uses fixed `kTimeCost`, `kMemoryCost`,
`kParallelism`, and `kKeyLength` values; its PBKDF2 fallback uses fixed
`kKeyLength` and 310,000 rounds. Both ignore constructor `params_`. The
domain-separated `derive()` path uses `params_` for Argon2id. With the PBKDF2
fallback, it passes `params_.output_len`, but the fallback uses its fixed
310,000-round count rather than `params_.iterations`.
Evidence: `include/user_storage_encrypted/key_derivation_service.hpp` and
`src/user_storage_encrypted/key_derivation_service.cpp`.

Do not treat these implementations as interchangeable for existing data:
there is no migration or compatibility guarantee in the implementation.
The exact compatibility of any pre-existing key material must be established
from its originating configuration.

## Rotation and observability behavior

`KeyRotationScheduler` has one dedicated polling thread and optional
`IRotationStore` persistence. In that loop, only schedules with `auto_rotate`
enabled and an elapsed interval invoke their callback. The callback receives
`(level, true, "")`; exceptions are swallowed, then `last_check_ms` is advanced
and persisted. This is not a callback success/retry protocol.

The helper APIs are not storage-rotation operations:

- `isRotationDue(level, last_rotation_ms)` only compares the supplied timestamp
  with the configured interval.
- `getNextRotationTime(level)` returns `last_check_ms + interval`.
- `triggerRotation(level)` synchronously invokes the registered callback
  regardless of `auto_rotate` or whether the interval is due. It sets
  `last_check_ms` first, passes `(level, true, "")`, swallows callback
  exceptions, and persists the timestamp. When called directly, that callback
  runs on the caller's thread, not necessarily the scheduler thread.

The scheduler holds its mutex while invoking callbacks. The contract's
non-reentrancy restriction is a caller obligation, not a runtime guard;
re-entering scheduler methods from a callback can deadlock. Do not equate any
of these scheduler helpers or callbacks with
`MultiLevelEncryptedStorage::rotateKey()`.

`rotateKey(level)` is a separate, explicit orchestrator operation. It asks the
provider to rotate first, retrieves the new key, creates and mounts a new
container, copies data while both containers are mounted, attempts to unmount
both, renames the old directory to a backup and the new directory into place,
then attempts to remount. Some unmount results and rollback results are
ignored. A remount failure returns an error after the new directory has been
installed; there is no code rollback on that path, and the old directory
remains as backup. After successful remount, backup deletion is best-effort;
failure only logs a warning. The unmount/rename/remount sequence does not
provide zero-downtime service, and the source does not establish a
transactional rotation or guarantee that the old key remains active on every
failure path. The orchestrator does not wire its parsed rotation configuration
to `KeyRotationScheduler`.

`getMetricsText()` emits these families:

- `user_storage_mounts_active`
- `user_storage_mount_operations_total{operation="mount"|"unmount"}`
- `user_storage_key_rotations_total` (no level label in the emitted text)
- `user_storage_container_size_bytes`

The source updates mount counters on successful mount/unmount and increments
the rotation counter on `rotateKey()` or `recordKeyRotation()`. No current
source update was found for `container_size_bytes`, so do not interpret that
gauge as a live size measurement. `checkLevelHealth()` checks configuration,
backend presence, and mounted state; it does not probe the key provider or
scheduler.

## Frozen contract versus implementation

`include/user_storage_encrypted/user_storage_encrypted_api_contract.h` calls
its statements normative for v1.x. It declares a separate
`themis::user_storage_encrypted` error/value contract; it is not the namespace
or result type used by the operational plugin. The following table compares
those normative statements with the implementation. Contract wording is not
runtime evidence.

| Contract statement | Current implementation evidence |
|---|---|
| Mount is atomic; partial mounts are cleaned and return `kMountFailed` | `GocryptfsBackend::mountContainer()` returns an operational `Result` error if the child command fails; it does not attempt an unmount/rollback on that path. No per-container mutex is present in the backend. |
| Unmount is attempted on error paths; failure returns `kUnmountFailed` and alerts an operator | `unmountContainer()` returns an operational `Result` error on command failure. `MultiLevelEncryptedStorage::unmountAll()` discards per-level results and returns success; the frozen error enum and operator-alert guarantee are not used by this path. |
| Passphrase and salt are zeroed after derivation/handoff | `KeyDerivationRequest` is not consumed by the operational KDF API. `derive()` clears its temporary concatenated password buffer, but not the caller's master-key or salt inputs. The byte-vector stdin helper clears one temporary hex string on its write paths; it does not clear the input key, the stream's internal buffer, or other key copies. The string stdin overload does not clear its caller-owned input. This does not establish the contract's general zeroing guarantee. |
| Rotation is transactional and leaves the old key active on failure | `MultiLevelEncryptedStorage::rotateKey()` performs provider rotation, copy, unmount, sequential renames, and remount; several unmount/rollback outcomes are ignored. Remount failure returns after the new directory is installed, with no code rollback and the old directory retained as backup. The contract guarantee is not established. |
| Backend absence fails closed for all storage operations | `initializeLevel()` propagates `checkAvailability()` failure as an operational `Result` error. The frozen error enum and `isUserStorageEncryptedFailClosed()` predicate are not used, and this check alone does not establish a module-wide fail-closed guarantee for every state or operation. |
| Per-container operations are serialized; KDF is stateless/thread-safe | The backend has no per-container operation mutex. The KDF has a mutable process-wide injection callback; its function object is copied under a mutex but invoked after releasing it, so callback thread safety is caller-dependent. |
| The scheduler uses one thread; callbacks must not re-enter backend/scheduler | The automatic polling loop uses one scheduler thread, but `triggerRotation()` invokes its callback synchronously on the caller's thread. The scheduler mutex remains held during callback execution; re-entry is not blocked safely and can deadlock on scheduler calls. |

These are API-contract discrepancies requiring owner review. Do not describe the
contract's guarantees as observed runtime behavior until implementation and
contract are reconciled.
