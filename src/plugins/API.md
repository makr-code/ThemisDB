---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Plugins API Reference

## Purpose and scope

This page is a developer reference to interfaces declared under
`include/plugins`. Use the declarations as the exact source for signatures and
the relevant implementation for runtime semantics. The module has separate
manifest-backed lifecycle and typed factory-registry APIs.

## Core plugin and lifecycle

`IThemisPlugin` declares `getName()`, `getVersion()`, `getType()`,
`getCapabilities()`, `initialize(const char*)`, `shutdown()`, and
`getInstance()`. `IStatefulPlugin` is optional and supplies `saveState()` and
`restoreState()` for reload. `PluginManager` owns loaded instances and exposes:

| Operation | Return | Failure/edge behavior |
|---|---|---|
| `scanPluginDirectory(path)` | `Result<size_t>` | Reports discovery count or an error; unsafe names and malformed manifests are rejected. |
| `loadPlugin(name)` | `Result<IThemisPlugin*>` | Manifest-backed path: resolves dependencies and applies manifest, edition/license, and binary checks before construction and initialization. |
| `loadPluginFromPath(path, config)` | `Result<IThemisPlugin*>` | Applies the binary verifier and shared-library/export/initialization checks. It does not read `plugin.json` or apply its edition/license restrictions. |
| `loadPluginFromOci(ref, cache_dir, auth_token)` | `Result<IThemisPlugin*>` | Parses and pulls a plugin layer, verifies its digest, then delegates to path loading; it does not apply `plugin.json` edition/license restrictions. |
| `unloadPlugin(name)` / `unloadAllPlugins()` | `Result<void>` | Reports unknown plugin or lifecycle/unload failure. |
| `getPlugin(name)` | `Result<IThemisPlugin*>` | Returns a manager-owned pointer only for a loaded plugin; do not retain it across unload/reload. |
| `reloadPlugin(name)` | `Result<void>` | On replacement failure, the old instance remains active. |
| `negotiateCapabilities(name, requirements)` | `PluginNegotiationResult` | Missing/unloaded plugins fail negotiation; unknown capability names are unsatisfied. |
| `checkCapabilityEscalation(name)` | `Result<void>` | Marks the loaded plugin restricted when live capabilities exceed the frozen snapshot. |

The manager also supports listing, `autoLoadPlugins()`, hot-plug controls,
reload listeners, diagnostics, and attaching a non-owning health monitor. See
[`plugin_manager.h`](../../include/plugins/plugin_manager.h) for parameter
contracts and ownership notes.

## Manifest and lifecycle contract

The runtime parser requires non-empty `name` and `version` plus a `binary`
object. It reads platform binary paths and optional `dependencies`,
`capabilities`, `auto_load`, `allowed_editions`, and `license_feature`.
Absent `auto_load` is false; an empty edition allowlist permits all editions;
an empty license feature requires no feature gate. Plugin names are restricted
to alphanumeric characters, `_`, and `-`.

`plugins_api_contract.h` declares `PluginsError` values in the 8200 range and
the `PluginState` contract vocabulary. Manager lifecycle tracking uses the
separate `PluginLifecycleState` enum in `plugin_interface.h`; see
[`MODELING.md`](./MODELING.md) before relying on state labels.

## Other public surfaces

- `PluginRegistry::registerFactory<T>()`, `create<T>()`, `listPlugins<T>()`,
  `hasPlugin<T>()`, and `unregisterFactory<T>()` provide typed factory lookup.
  `create<T>()` throws `std::runtime_error` for a missing registration or
  detected type mismatch. Registering the same name for the same interface
  replaces the previous factory.
- `SignedPluginRepository` supports pinned key management, signature
  verification, catalog entry insertion and lookup, and canonical manifest
  serialization. It verifies Ed25519 signatures against active pinned keys.
- `OciReference::parse()` parses a registry/name/tag/digest reference.
  `OciRegistryClient::fetchManifest()` and `pullPluginBinary()` return `Result`
  values; the latter accepts only the plugin binary layer media type and checks
  the layer SHA-256 digest before placing it in the cache.
- `PluginHealthMonitor` registers non-owning plugin pointers, runs periodic or
  immediate health checks, and can trigger configured recovery. The plugin
  pointer must outlive its monitoring registration.
- `PluginMetrics` records load/reload/call/error and memory values.
  `PluginMetricsCollector::collect()` emits a point-in-time snapshot to an
  `IMetrics` sink; it does not itself run a scrape loop.
- `loadWasmPlugin()` requires an expected hash and an enabled runtime path.
  Without `THEMIS_WASM_SUPPORT`, the build does not provide the active runtime
  bridge.

Detailed schemas and behavioral contracts are in
[`plugin_interface.h`](../../include/plugins/plugin_interface.h),
[`plugins_api_contract.h`](../../include/plugins/plugins_api_contract.h),
[`signed_plugin_repository.h`](../../include/plugins/signed_plugin_repository.h),
and [`oci_registry_client.h`](../../include/plugins/oci_registry_client.h).
Signature and trust distinctions are documented in [`TRUST.md`](./TRUST.md).
