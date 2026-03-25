<!-- Status: current | validated: 2026-03-25 -->
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

## [0.1.0] — 2026-03-24

### Added

- **Stdin key delivery** (`gocryptfs_backend.cpp`):
  `executeCommandWithStdin()` forks a child, wires a pipe to its stdin, and
  writes the key hex string through it — no temp file on disk.
  `deliverKeyViaStdin()` calls `executeCommandWithStdin()` then immediately
  calls `explicit_bzero()` on the in-memory key string.
  `createContainer()` and `mountContainer()` now use `-passfile /dev/stdin`
  and `deliverKeyViaStdin()`.

- **Argon2id KDF** (`key_derivation_service.hpp/.cpp`):
  `IKeyDerivationService` interface + `Argon2idKeyDerivationService` implementation.
  Parameters: m=65536 KiB, t=3 iterations, p=4 lanes, 32-byte output.
  `deriveKey()` is deterministic for the same (master_key, salt) pair.
  `generateSalt()` uses `getrandom()` (Linux) / `/dev/urandom` (POSIX fallback).
  `loadOrCreateSalt()` reads or creates a salt file at the given path (e.g.
  `.themis_kdf_salt`), permissions 0600.

- **`IRotationStore` persistence** (`irotation_store.hpp`):
  `IRotationStore` interface (save/load), `NullRotationStore` (no-op),
  `FileRotationStore` (JSON file, thread-safe). `KeyRotationScheduler` gains
  `setRotationStore()` and `triggerRotation()`; `scheduleRotation()` loads
  the persisted `last_check_ms` so rotation intervals survive restarts.

- **`const_cast` removal**:
  `createPasswordFile()` now returns `Result<std::string>` (path) instead of
  using `const_cast` on a reference parameter.
  `getBackendVersion()` calls `executeCommandSafe()` directly without casting.

- **Tests** (`plugins/user_storage_encrypted/tests/test_user_storage_features.cpp`):
  20 tests — 4 stdin delivery (AC-SD), 10 Argon2id KDF (AC-KDF), 6 persistence
  (AC-PRS), 2 GocryptfsBackend (AC-GCF).

- **CI** (`.github/workflows/user-storage-encrypted-ci.yml`):
  Builds and runs `test_user_storage_features` on Ubuntu 22.04 / GCC-12 and
  Ubuntu 24.04 / GCC-14 with `libargon2-dev`.

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
  Orchestrates HOT, WARM, and COLD storage tiers, each backed by an independent
  `GocryptfsBackend` instance with its own key and mount point. Provides a unified
  API for routing data to the appropriate tier based on access frequency and age.

- **Security model**:
  All subprocess calls use explicit `std::vector<std::string>` argument lists
  passed to `execvp()` — shell metacharacter injection is impossible by construction.
  Temporary password files are created with `mkstemp()`, `fchmod(0600)` before
  the first write, and `unlink()`ed immediately after the subprocess reads them.
