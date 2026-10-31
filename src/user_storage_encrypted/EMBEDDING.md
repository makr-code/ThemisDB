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

# Embedding Guide for user_storage_encrypted

Exact model backend not verifiable from final document text alone.

The public declarations are under `include/user_storage_encrypted/`; the
implementation and build definition are under `src/user_storage_encrypted/`.
The generated manifest template and example configuration are under
`plugins/user_storage_encrypted/`.

## Build and plugin boundary

`THEMIS_PLUGIN_USER_STORAGE_ENCRYPTED` controls the shared
`themis_user_storage_encrypted` library and is defined with default `ON`,
subject to edition configuration. CMake detects libargon2 and defines
`THEMIS_HAS_ARGON2` when found; otherwise the KDF uses the PBKDF2-HMAC-SHA256
fallback. The plugin manifest template sets `auto_load` to `false`; building
the target does not establish that the host loads it.

`MultiLevelEncryptedStorage` implements `IThemisPlugin` and returns
`user_storage_encrypted` from `getName()`. Build-time and host registration
are separate concerns. Optional server handler compilation/linking is
conditional; this guide makes no claim that HTTP routes are wired or active.

## Runtime configuration

The JSON input to `initialize(const char*)` has a
`multi_level_storage.levels` array (not a top-level `levels` array). Plain
levels use `path`; encrypted levels require `encrypted_dir`, `mount_point`,
and `encryption.key_id`, with provider options in the nested `encryption`
object. See `plugins/user_storage_encrypted/config/storage_config.yaml.example`.

Initialization parses and validates configuration, attempts stale-mount
cleanup, checks backend availability for encrypted levels, obtains key
providers, creates a container when needed, and mounts it. The plugin
constructs the backend without a KDF service. Although the backend supports
direct KDF-enabled construction, the current multi-level configuration path
does not enable it.

Rotation fields are parsed but the plugin does not create or start
`KeyRotationScheduler`; scheduler integration must be performed separately.

## Host dependencies and operational limits

Encrypted mount behavior depends on gocryptfs and host FUSE support. Vault
provider configuration uses `vault_addr` and the process `VAULT_TOKEN`;
HSM provider configuration requires an HSM library path. The current health
check tests backend initialization and mounted state, not ongoing provider or
scheduler health. Metrics include an emitted container-size gauge that current
source does not update.

Do not treat the separate `user_storage_encrypted_api_contract.h` comments as
proof that every operation is atomic or that cleanup failures are returned;
see [API.md](API.md).

## Build references

- `src/user_storage_encrypted/CMakeLists.txt`
- `cmake/features/PluginFeatures.cmake`
- `plugins/user_storage_encrypted/plugin.json.in`
- `plugins/user_storage_encrypted/config/storage_config.yaml.example`
- `include/user_storage_encrypted/multi_level_storage.hpp`
- `src/user_storage_encrypted/multi_level_storage.cpp`
