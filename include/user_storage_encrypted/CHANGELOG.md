<!-- Status: current | validated: 2026-03-22 -->

# User Storage Encrypted — Include Changelog

> Public header changes only. For implementation changes see [`../../src/user_storage_encrypted/CHANGELOG.md`](../../src/user_storage_encrypted/CHANGELOG.md).

## [Unreleased]

## [0.0.1] — 2026-03-22

### Added
- `encryption_backend_interface.hpp` — `IEncryptionBackend` abstract interface
- `gocryptfs_backend.hpp` — `GocryptfsBackend` concrete implementation header
- `key_rotation_scheduler.hpp` — `KeyRotationScheduler` public API
- `multi_level_storage.hpp` — `MultiLevelEncryptedStorage` with HOT/WARM/COLD tier support
- `security_level.hpp` — `SecurityLevel` enum (STANDARD, HIGH, MAXIMUM)
- `user_models.hpp` — user storage metadata types

### Notes
- First release. All headers are initial drafts; API may change in 0.1.x.
