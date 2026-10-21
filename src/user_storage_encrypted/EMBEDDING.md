---
Author: ThemisDB Contributors
Created: 2026-10-21
Last Updated: 2026-10-21
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: unknown
Human_Review: true
Human_Author: ThemisDB Contributors
---

# Embedding Guide for user_storage_encrypted

Exact model backend not verifiable from final document text alone.

This module is embedded into ThemisDB as an optional plugin-like component under `src/user_storage_encrypted` with its public API in `include/user_storage_encrypted`. The actual build wiring is visible in `src/user_storage_encrypted/CMakeLists.txt` and the plugin metadata is generated through the build system.

## Module boundary

The module owns the encrypted container lifecycle and its policy-aware storage orchestration. It is not a general-purpose storage engine; it is a bounded feature that integrates with the wider ThemisDB runtime through plugin registration and configuration.

The key ownership split is:

- public API: `include/user_storage_encrypted/`
- implementation: `src/user_storage_encrypted/`
- plugin build: `src/user_storage_encrypted/CMakeLists.txt`
- plugin config example: `plugins/user_storage_encrypted/config/storage_config.yaml.example`

## Build-time embedding

The module is conditionally enabled via `THEMIS_PLUGIN_USER_STORAGE_ENCRYPTED` in `src/user_storage_encrypted/CMakeLists.txt`.

When enabled, the build creates a shared library named `themis_user_storage_encrypted` and installs it under the plugin directory. The CMake file also detects `libargon2`, exposes `THEMIS_HAS_ARGON2`, and falls back to PBKDF2 if Argon2 is unavailable. This is a direct runtime configuration pitfall: the module is built in two compile-time modes, and both are considered supported in the implementation comments.

## Runtime embedding

The class `MultiLevelEncryptedStorage` implements `IThemisPlugin` and exposes the plugin name `user_storage_encrypted`. In practice the module is intended to be loaded as an optional plugin when the build flag is on, and the plugin registration path is part of the host/build integration rather than the module's own source files.

The plugin is designed to manage:

- encrypted storage level configuration
- security-specific per-level key providers
- periodic key rotation callbacks
- health and metrics reporting

## Data-flow embedding

The current implementation uses a structured runtime flow:

1. Configuration is loaded into `LevelConfig` entries.
2. `validateConfiguration()` checks structural assumptions.
3. `initializeLevel()` starts each configured level.
4. options, mount points, and provider configs are bound at runtime.
5. operational health is exposed through `getMetricsText()` and `checkHealth()`.

This means the module expects to be embedded in a host that can provide configuration and lifecycle control, but it does not assume the host will do all the encrypted storage enforcement itself.

## Host expectations

The module assumes the embedding environment provides:

- a valid `gocryptfs` binary
- kernel or OS support for FUSE / mount operations
- filesystem permissions to create mount points and encrypted directories
- a configuration source for level metadata and rotation intervals

If those assumptions fail, the module surfaces them explicitly rather than masking them. This is consistent with the fail-closed contract in `user_storage_encrypted_api_contract.h`.

## Operational embedding considerations

When embedding this module in a larger service, treat the following as requirements:

- enable the plugin only when host support and permissions are confirmed
- validate mount points before startup
- use explicit key provider configuration rather than relying on default-insecure state
- monitor rotation schedules and stale mount reconciliation
- treat backend unavailability as a system-level signal, not a soft warning

## Limitations of the current embedding model

The source does not show a more general plugin registry abstraction inside this module itself; the module is designed as a local subsystem boundary and relies on the host runtime for broader orchestration and lifecycle management. The embedding is therefore intentionally narrow and production-oriented, but not a full application server integration layer.
