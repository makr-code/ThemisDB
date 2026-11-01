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

# User Storage Encrypted Module

Exact model backend not verifiable from final document text alone.

The module provides file-backed user/group records organized by security
level, an optional gocryptfs backend, KDF interfaces, a standalone key
rotation scheduler, and a multi-level plugin. This page describes implemented
scope, not a production-readiness certification.

## Components

| Source/header | Responsibility |
|---|---|
| `gocryptfs_backend.cpp` / `gocryptfs_backend.hpp` | Container create, mount, unmount, mount-state, and availability operations |
| `key_derivation_service.cpp` / `key_derivation_service.hpp` | Argon2id or PBKDF2 fallback KDF APIs and salt handling |
| `key_rotation_scheduler.cpp` / `key_rotation_scheduler.hpp` | Separate scheduler thread and optional rotation-state persistence |
| `multi_level_storage.cpp` / `multi_level_storage.hpp` | Configuration, providers, per-level lifecycle, JSON user/group files, health, and metrics |
| `diagnostic_events.cpp` / `error_codes.hpp` | Diagnostic event types and default logging handler |
| `CMakeLists.txt` | Optional shared-library and Argon2 build wiring |

## Runtime configuration

`MultiLevelEncryptedStorage::initialize(const char*)` expects JSON rooted at
`multi_level_storage.levels`. Plain entries require `path`; encrypted entries
require `encrypted_dir`, `mount_point`, and nested `encryption.key_id`.
Provider settings (`vault`, `hsm`, or `mock`) are in the nested `encryption`
object. The shipped YAML example is
`plugins/user_storage_encrypted/config/storage_config.yaml.example`; convert
its content to the JSON shape expected by the runtime before passing it to
`initialize()`.

The plugin creates its backend without a KDF service. It parses rotation
configuration but does not instantiate the standalone scheduler. Both
capabilities require separate caller wiring.

## Runtime behavior and limits

- Encrypted lifecycle depends on gocryptfs and host FUSE support.
- KDF mode depends on CMake's libargon2 discovery; fallback is
  PBKDF2-HMAC-SHA256.
- Stale mount cleanup is best-effort and non-fatal.
- Health checks inspect backend presence/mount state, not provider or
  scheduler health.
- `unmountAll()` ignores per-level unmount results.
- `KeyRotationScheduler` callbacks report `(level, true, "")`; exceptions are
  swallowed and `last_check_ms` advances after an attempt. Manual
  `triggerRotation()` invokes its callback on the caller's thread and does not
  perform `MultiLevelEncryptedStorage::rotateKey()`.
- `MultiLevelEncryptedStorage::rotateKey()` is a separate provider/container
  operation; ignored unmount/rollback results mean source does not establish
  transactional rotation.
- Key cleanup is limited to some temporary buffers. The KDF's domain-separated
  `derive()` clears its temporary concatenated password, but does not wipe
  caller-owned master-key or salt inputs. See [API contract comparison](API.md).
- `container_size_bytes` is emitted but is not updated by current source.

The separately declared error contract contains stronger guarantees than
some operational paths demonstrate. See [API.md](API.md).

## Build, tests, and benchmarks

The shared library is controlled by `THEMIS_PLUGIN_USER_STORAGE_ENCRYPTED`.
Focused test registration is in `tests/user_storage_encrypted/CMakeLists.txt`.
Benchmark target registration is in `benchmarks/user_storage_encrypted/CMakeLists.txt`.
See their README files for conditional registration, skipped tests, simulated
benchmarks, and evidence limitations.

## Source references

- `include/user_storage_encrypted/`
- `src/user_storage_encrypted/`
- `tests/user_storage_encrypted/README.md`
- `benchmarks/user_storage_encrypted/README.md`
- `plugins/user_storage_encrypted/config/storage_config.yaml.example`
