> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — User Encrypted Storage Plugin

All notable changes to this module are documented here.
Format: [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), newest first.

---

## [Unreleased]

- Key escrow: encrypted key backup to ThemisDB secrets store
- Integration tests: create → mount → write → unmount → re-mount → verify

---

## [0.2.0] — 2026-03-25

### Added

- **`MultiLevelEncryptedStorage::reconcileStaleMounts()`**:
  Called from `initialize()` before `initializeLevel()`. Scans `/proc/mounts`
  for orphaned FUSE mounts whose mount point matches a configured encrypted
  level. Unmounts via `fusermount -u`; falls back to `umount` on failure.
  Non-fatal: failures are silently ignored so initialization can continue.
- **5 `StaleMountReconciliationTest` tests** in `test_user_storage_features.cpp`.

---

## [0.1.0] — 2026-03-24

### Added

- **Stdin-based key delivery** (FUTURE_ENHANCEMENTS §1):
  `GocryptfsBackend` now delivers key material to gocryptfs via a stdin pipe
  (`-passfile /dev/stdin`) instead of a `/tmp` password file.
  New private methods: `executeCommandWithStdin()` sets up a pipe pair, forks
  gocryptfs with stdin wired to the read end, and writes the hex-encoded key
  through the write end before reading stdout; `deliverKeyViaStdin()` performs
  the write and calls `explicit_bzero` to clear the buffer.  Key material never
  touches the filesystem.

- **`KeyDerivationService` / `Argon2idKeyDerivationService`**
  (`key_derivation_service.hpp/.cpp`) (FUTURE_ENHANCEMENTS §2):
  New header `include/user_storage_encrypted/key_derivation_service.hpp`
  defines the abstract `KeyDerivationService` interface and the concrete
  `Argon2idKeyDerivationService` implementation.  Parameters follow OWASP
  recommendations (m=65536, t=3, p=4, output=32 bytes).  Per-container 16-byte
  salt stored in `{encrypted_dir}/.themis_kdf_salt`.  Salt generation reads
  from `/dev/urandom`.  `GocryptfsBackend` gains a `KeyDerivationService*`
  constructor parameter and an internal `resolveKey()` helper.

- **Key rotation persistence** (FUTURE_ENHANCEMENTS §3):
  New `IRotationStore` interface in `key_rotation_scheduler.hpp` with
  `get(key, out)` / `put(key, value)` primitives.
  `KeyRotationScheduler::initialize()` accepts an optional
  `std::shared_ptr<IRotationStore>`.  On `scheduleRotation()`, persisted
  `last_check_ms` / `interval_days` values are loaded from the store.
  After each successful callback invocation the updated state is written
  back under key `user_storage:rotation_state:{level}` as JSON.
  `shutdown()` now uses a `std::condition_variable` to interrupt the
  background sleep immediately.

- **`const_cast` removal**:
  `createPasswordFile()` now returns `Result<std::string>` (path) instead of
  using `const_cast` on a reference parameter.
  `getBackendVersion()` calls `executeCommandSafe()` directly without casting.

- **Tests** (`plugins/user_storage_encrypted/tests/test_user_storage_features.cpp`):
  20 unit / integration tests — 4 stdin delivery (AC-SD), 10 Argon2id KDF (AC-KDF),
  6 persistence (AC-PRS), 2 GocryptfsBackend (AC-GCF).

- **CI** (`.github/workflows/user-storage-encrypted-ci.yml`):
  Builds and runs `test_user_storage_features` on Ubuntu 22.04 / GCC-12 and
  Ubuntu 24.04 / GCC-14 with `libargon2-dev`.

### Fixed

- `include/user_storage_encrypted/security_level.hpp`: added missing
  `#include <stdexcept>` (pre-existing omission that caused compilation
  failures in clean build environments).

---

## [0.0.1] — 2026-03-22

### Added

- **`GocryptfsBackend`** (`gocryptfs_backend.cpp/.hpp`):
  Manages gocryptfs FUSE container lifecycle. Operations: `initialize()`,
  `checkAvailability()` (validates gocryptfs binary and `/dev/fuse`),
  `createContainer()`, `mountContainer()`, `unmountContainer()`, `isMounted()`,
  `getBackendVersion()`. Uses `fork/execvp` via `executeCommandSafe()` for all
  subprocess invocations — no `system()` or `popen()`. Key material written to
  a `mkstemp()` temp file with `fchmod(0600)` and immediately `unlink()`ed after
  use. Linux mount detection reads `/proc/mounts`; macOS falls back to `mount`
  command output.

- **`KeyRotationScheduler`** (`key_rotation_scheduler.cpp/.hpp`):
  Schedules automatic cryptographic key rotation per `SecurityLevel`. Maintains a
  `map<SecurityLevel, RotationSchedule>` with per-level `interval_days`,
  `auto_rotate` flag, and `RotationCallback`. Background `schedulerLoop()` thread
  wakes every `check_interval_seconds` (default 3600 s) to fire due callbacks.
  `shutdown()` sets `running_=false` and joins the thread. Supports
  `triggerRotation(level)` for manual rotation.

- **`MultiLevelEncryptedStorage`** (`multi_level_storage.cpp`):
  Manages encrypted storage across security classification levels (`SecurityLevel`
  enum: OFFEN, VS-NFD, GEHEIM, STRENG-GEHEIM), each with an independent
  `GocryptfsBackend` instance and its own key and mount point.

- **Security model**:
  All subprocess calls use explicit `std::vector<std::string>` argument lists
  passed to `execvp()` — shell metacharacter injection is impossible by construction.
