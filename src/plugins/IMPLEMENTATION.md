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

# Plugin Runtime Implementation

## Purpose and scope

This guide is for maintainers modifying runtime behavior in `src/plugins`.
Public contracts are declared under `include/plugins`; this page records the
implementation sequence and invariants to preserve.

## Source ownership

| Source | Responsibility |
|---|---|
| `plugin_manager.cpp` | Manifest discovery, validation, native library lifecycle, dependencies, capability checks, reload, and diagnostics. |
| `plugin_registry.cpp` | Shared-mutex-protected storage for typed plugin factories. |
| `plugin_system_edition.cpp` | Edition and visibility policy helpers. |
| `signed_plugin_repository.cpp` | In-process pinned-key Ed25519 catalog verification. |
| `oci_registry_client.cpp` | OCI reference parsing, registry requests, layer download, digest verification, and cache output. |
| `wasm_plugin_loader.cpp` | Optional WASM hash validation and runtime-specific host bridge. |
| `plugin_hot_plug_monitor.cpp` | Filesystem events mapped to manager scan/load/reload/unload operations. |
| `plugin_health_monitor.cpp` | Periodic checks, recovery attempts, event callbacks, and health metrics. |
| `plugin_metrics.cpp` | In-memory lifecycle/call/error statistics and metrics-sink collection. |
| `rpc_service_registry.cpp` | RPC service pointer registration and lookup. |

## Native load flow

For a manifest-discovered plugin, preserve this order:

1. Scan the directory and reject unsafe plugin names before using them to derive
   plugin paths.
2. Read and parse `plugin.json`; require a non-empty `name` and `version` and a
   `binary` object.
3. Resolve dependencies and enforce edition/license and manifest constraints.
4. Apply the build-mode-specific manifest hash check and plugin-binary security
   policy before constructing the plugin.
5. Open the shared library, resolve the `createPlugin` entry point, construct
   the object, and initialize it with the requested JSON configuration.
6. Store the loaded entry and capability snapshot only after successful
   initialization. On failure, destroy any created object, close the library,
   and leave no partially active entry.

`loadPluginFromPath()` is a separate path: it verifies the binary, opens the
library, resolves its exports, and initializes with the supplied JSON, but it
does not parse `plugin.json` or apply manifest edition/license restrictions.
`loadPluginFromOci()` verifies the downloaded layer digest and delegates to
that path. Keep this distinction explicit in callers and security reviews.

`IThemisPlugin` is the minimum interface: stable name/version/type/capability
queries, `initialize()`, `shutdown()`, and `getInstance()`. The library also
exports `destroyPlugin` so the manager can destroy the instance through the
plugin's own ABI boundary.

## Reload and cleanup

Reload keeps the old instance available while the replacement is checked,
created, initialized, and (for `IStatefulPlugin`) given saved state. A failed
replacement returns an error and preserves the old instance. Successful
replacement swaps the entry under manager synchronization, notifies listeners,
then shuts down and releases the old library. Verify dependent-plugin rules
before changing reload order.

Every new path must handle a null factory result, missing factory/destroy
symbol, failed initialization, invalid manifest or binary, dependency errors,
and shutdown/unload failure without exposing an active half-entry. Lifecycle
state transitions are checked under the entry's state synchronization.

## Security implementation boundary

`PluginManager::verifyManifestSignature()` currently compares the first line of
`plugin.json.sig` to the SHA-256 hash of the manifest in production builds; it
does not perform asymmetric signature verification. The separate
`SignedPluginRepository` uses Ed25519 with pinned keys. Keep these mechanisms
distinct in code and docs. The built-in WASM loader checks an expected SHA-256
after first consulting any registered `WasmPluginLoadFn`; the callback owns
its own validation policy. A hash detects content mismatch but does not
authenticate a publisher by itself. Full details are in [`TRUST.md`](./TRUST.md).

## Validation

Add or update the closest focused test under `tests/plugins/` for every changed
invariant. Relevant suites include lifecycle state machine, error handling,
registry concurrency, manifest edition gates, signed repository, OCI client,
WASM loading, health monitoring, and hot-plug. Run the exact test target
available in the active CMake configuration; filenames alone do not prove a
test is registered or passing.

For benchmark-sensitive changes, use the mapped release-profile cases and
baseline rules in [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md).
Update [`API.md`](./API.md), [`MODELING.md`](./MODELING.md), and the operator
guidance when the observable behavior changes.
