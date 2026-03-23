<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — User Encrypted Storage Plugin

All notable changes to this module are documented here.  
Format: [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), newest first.

---

## [Unreleased]

- `gocryptfs_backend.cpp` hardening to Production-Ready
- Unit and integration tests for all three components
- Secure in-memory key delivery via stdin pipe (replacing `/tmp` password files)
- Key derivation function (KDF) integration for key stretching
- Remove deprecated `executeCommand()` wrapper

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
