# User Storage Encrypted Plugin – Changelog

All notable changes to this plugin are documented in this file.  
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

---

## [0.2.0] – 2026-03-24

### Added

- **Startup stale mount reconciliation** (`MultiLevelEncryptedStorage::reconcileStaleMounts()`).
  - Scans `/proc/mounts` on Linux for any FUSE mount point that is a child of the configured
    base path but is **not** among the currently configured mount points.
  - Each stale mount is unmounted via `fusermount -u`; on failure the call falls back to
    `umount`.  Errors are logged to stderr and are **never fatal** – startup continues.
  - Called automatically from `initialize()` before `initializeLevel()` for every distinct
    base-path parent of the configured mount points.
  - 5 new unit tests (`StaleMountReconciliationTest`) in
    `plugins/user_storage_encrypted/tests/test_user_storage_features.cpp`.

---

## [0.1.0] – 2026-03-22

### Added

- **Stdin key delivery** (`GocryptfsBackend::executeCommandWithStdin()` +
  `deliverKeyViaStdin()`): the gocryptfs passphrase is passed via a pipe
  (`-passfile /dev/stdin`) instead of a temporary file; the write buffer is
  zeroed with `explicit_bzero` after use.  4 unit tests (`GocryptfsStdinTest`).

- **Argon2id KDF** (`Argon2idKeyDerivationService`, libargon2):
  per-container key derivation with parameters m=65536/t=3/p=4; the
  per-container salt is stored in `{encrypted_dir}/.themis_kdf_salt` and
  loaded automatically on subsequent mounts.  10 unit tests
  (`Argon2idKdfTest`, `Argon2idPerformanceTest`).

- **Key rotation persistence** (`IRotationStore` interface):
  `KeyRotationScheduler::initialize()` accepts an optional `IRotationStore`;
  `last_check_ms` and `interval_days` are persisted after every rotation
  callback and restored on the next call to `initialize()`.  6 unit tests
  (`KeyRotationPersistenceTest`).

- CI workflow `.github/workflows/user-storage-encrypted-ci.yml`.

---

*See also: [`ROADMAP.md`](ROADMAP.md) · [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)*
