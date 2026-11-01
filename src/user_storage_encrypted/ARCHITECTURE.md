---
Author: ThemisDB Contributors
Created: 2026-08-08
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
---

# Architecture - User Storage Encrypted Module

Exact model backend not verifiable from final document text alone.

## Runtime components

`MultiLevelEncryptedStorage` parses and validates level configuration, chooses
a key provider, constructs a `GocryptfsBackend`, and initializes/mounts each
configured encrypted level. Plain levels use their configured filesystem path.
User and group records are JSON files beneath the selected level's `users/` and
`groups/` directories.

The backend validates container/mount paths, manages gocryptfs lifecycle
commands, and sends key bytes to the child process through stdin on supported
POSIX builds. `Argon2idKeyDerivationService` can be supplied when directly
constructing a backend; the multi-level plugin currently constructs the
backend without a KDF service.

`KeyRotationScheduler` is a separate API with one automatic polling thread and
an optional rotation-state store. `triggerRotation()` is a manual callback
invocation on the caller's thread; it does not call
`MultiLevelEncryptedStorage::rotateKey()`. The orchestrator parses rotation
configuration but does not instantiate the scheduler or wire its callbacks.
Its `rotateKey(level)` method is a separate explicit provider/container
operation, and the implementation does not establish transactional rotation.
See [API contract comparison](API.md) for the frozen-header guarantees and
source-by-source discrepancies.

## Configuration and dataflow

The implemented configuration root is `multi_level_storage`, containing a
`levels` array. Plain levels require `name`, `encrypted: false`, and `path`.
Encrypted levels require `name`, `encrypted: true`, `encrypted_dir`,
`mount_point`, and `encryption.key_id`; provider settings are read from the
nested `encryption` object. The shipped example is
`plugins/user_storage_encrypted/config/storage_config.yaml.example`.

At startup the plugin parses and validates configuration, attempts stale-mount
reconciliation, then initializes configured levels. Encrypted level
initialization probes gocryptfs/FUSE, resolves a provider, creates a container
when its gocryptfs config file is absent, and mounts it. The startup
reconciliation code is Linux-specific; failed cleanup is non-fatal and does
not prove that the resulting mount state is healthy.

## Health, errors, and metrics

`checkLevelHealth()` verifies that an encrypted level has an initialized
backend and appears mounted; it does not re-check provider health, key
retrieval, or scheduler state. `getMetricsText()` emits mount, rotation, and
container-size metric families, but the current implementation does not update
the container-size gauge. The scheduler callback contract also does not
propagate callback exceptions as failures.

There are two distinct error surfaces: operational methods generally return
message-bearing `Result<T>` values, while
`user_storage_encrypted_api_contract.h` declares a separate error enum and
predicate in `themis::user_storage_encrypted`. Some guarantees in that
contract's comments are not demonstrated by current implementation behavior;
see [API.md](API.md).

## Build and optional integration

`THEMIS_PLUGIN_USER_STORAGE_ENCRYPTED` controls the shared-library target
`themis_user_storage_encrypted`; the option is enabled by default where the
edition does not disable it. The target installs its public headers and a
generated plugin manifest. The manifest declares `auto_load: false`; enabling
the build option alone does not establish that a host loads the plugin.
Optional HTTP handler compilation/linking is conditional on the HTTP-server
feature and plugin target; this document does not claim that HTTP routes are
registered or served.

Focused module tests are defined under `tests/user_storage_encrypted/`.
Benchmark targets are defined under `benchmarks/user_storage_encrypted/`;
their actual integration and limitations are documented in the corresponding
README files.
