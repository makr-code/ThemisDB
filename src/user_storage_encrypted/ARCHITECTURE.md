# Architecture - User Storage Encrypted Module

<!-- Status: current | validated: 2026-08-08 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The user_storage_encrypted module combines encrypted mount lifecycle control, per-container key derivation, scheduled key rotation, and multi-tier encrypted storage coordination into a bounded plugin subsystem.

## Main Execution Planes

1. Backend plane
- gocryptfs backend initialization, availability checks, mount-state inspection, mount, and unmount behavior

2. Key-management plane
- key derivation behavior and rotation scheduling by security level

3. Tier-orchestration plane
- HOT/WARM/COLD encrypted storage coordination and lifecycle behavior

## Core Contracts

| Contract | Behavior |
|---|---|
| backend contract | explicit mount lifecycle and availability behavior |
| key contract | bounded derivation and rotation behavior |
| orchestration contract | deterministic tier coordination and storage-level transitions |

## Failure Semantics

- missing host support or backend failures surface through explicit backend errors.
- key derivation and rotation faults remain observable and non-silent.
- tier-orchestration failures remain bounded to configured storage levels.

## Sourcecode Verification (Module: user_storage_encrypted/architecture)

- Verified files:
  - src/user_storage_encrypted/gocryptfs_backend.cpp
  - src/user_storage_encrypted/key_derivation_service.cpp
  - src/user_storage_encrypted/key_rotation_scheduler.cpp
  - src/user_storage_encrypted/multi_level_storage.cpp
- Verified architecture claims:
  - backend, key-management, and orchestration plane split
  - explicit failure boundaries for backend and scheduler behavior
  - module-local ownership of encrypted storage lifecycle behavior
---

### Direct Downstream Consumers (modules that use this module)

> **Production consumer route: INTEGRATION-READY (plugin host + HTTP handler)**
> Handler header `include/server/encrypted_storage_api_handler.h` has been created
> as the production HTTP consumer route. The module is built as an opt-in plugin
> target via `THEMIS_PLUGIN_USER_STORAGE_ENCRYPTED`. Wiring
> `EncryptedStorageApiHandler` into `HttpServer` and the plugin host are the
> remaining steps.

| Module | Via | Notes |
|--------|-----|-------|
| `server` | `include/server/encrypted_storage_api_handler.h` → `EncryptedStorageApiHandler` | Planned HTTP routes: `POST /user/storage/encrypted/store`, `GET /user/storage/encrypted/retrieve/{key}`, `DELETE /user/storage/encrypted/{key}`, `GET /user/storage/encrypted/list`, `POST /user/storage/encrypted/rotate`. Gate: `THEMIS_PLUGIN_USER_STORAGE_ENCRYPTED`. Handler header implemented; `HttpServer` wiring pending. |
| `plugin_host` | CMake flag `THEMIS_PLUGIN_USER_STORAGE_ENCRYPTED` → `MultiLevelEncryptedStorage` plugin registration | Plugin loaded at startup when flag is ON; registers as `IThemisPlugin`. |
| _(tests)_ | `include/user_storage_encrypted/multi_level_storage.hpp`, `gocryptfs_backend.hpp`, etc. | `tests/test_user_storage_v03.cpp`, `tests/test_kdf_argon2_bridge.cpp` — current only verified consumers. |
