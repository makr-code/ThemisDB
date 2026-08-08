# ThemisDB User Storage Encrypted Module

<!-- Status: current | validated: 2026-08-08 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The user_storage_encrypted module provides encrypted user-storage behavior for ThemisDB through a gocryptfs-backed mount backend, key derivation behavior, key rotation scheduling, and multi-tier encrypted storage orchestration.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| gocryptfs_backend.cpp | gocryptfs-backed container lifecycle behavior |
| key_derivation_service.cpp | per-container key derivation behavior |
| key_rotation_scheduler.cpp | scheduled rotation behavior by security level |
| multi_level_storage.cpp | HOT/WARM/COLD storage orchestration behavior |
| CMakeLists.txt | module build and dependency wiring |

## Scope

In scope:
- encrypted container initialization, mount, unmount, and mount-state behavior
- key derivation and rotation scheduling behavior
- multi-level encrypted storage orchestration behavior

Out of scope:
- non-encrypted generic storage behavior owned by other modules
- OS-level FUSE implementation details outside module boundaries

## Runtime Behavior and Limits

- mount lifecycle behavior depends on gocryptfs and host FUSE availability.
- key material handling remains inside module-controlled execution paths.
- tier orchestration remains bounded to configured HOT/WARM/COLD storage behavior.
- host-environment failures surface as explicit backend or scheduling errors.

## Sourcecode Verification (Module: user_storage_encrypted/readme)

- Verified files:
  - src/user_storage_encrypted/gocryptfs_backend.cpp
  - src/user_storage_encrypted/key_derivation_service.cpp
  - src/user_storage_encrypted/key_rotation_scheduler.cpp
  - src/user_storage_encrypted/multi_level_storage.cpp
  - src/user_storage_encrypted/CMakeLists.txt
- Verified behavior surfaces:
  - encrypted mount lifecycle
  - key derivation and rotation scheduling
  - multi-level storage orchestration
- Verified benchmark/test anchors:
  - benchmarks/bench_user_storage_mount_latency.cpp
  - tests/test_user_storage_v03.cpp