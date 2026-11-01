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

> Exact model backend not verifiable from final document text alone.

# Public plugin headers

This directory declares plugin-facing contracts. The declarations are
authoritative for signatures; runtime behavior is implemented in
[`../../src/plugins/`](../../src/plugins/). See
[`../../src/plugins/API.md`](../../src/plugins/API.md) for a concise behavioral
reference and
[`ARCHITECTURE.md`](ARCHITECTURE.md) for header groups and dataflow.

## Core interfaces

- [`plugin_interface.h`](plugin_interface.h): `IThemisPlugin`, optional
  `IStatefulPlugin`, `PluginType`, capabilities, manifest parsing, lifecycle
  state, and `THEMIS_PLUGIN_IMPL(PluginClass)` exports for `createPlugin` and
  `destroyPlugin`.
- [`plugin_manager.h`](plugin_manager.h): `PluginManager` discovery,
  manifest-backed loading, explicit-path and OCI loading, unload/reload,
  capability negotiation, health-monitor attachment, and metrics access.
- [`plugin_registry.h`](plugin_registry.h): separate type-indexed factory
  registration and creation; it does not own dynamic-library lifecycle.
- [`plugin_dependency_resolver.h`](plugin_dependency_resolver.h): dependency
  graph/order helpers used by manifest-backed manager loading.
- [`plugins_api_contract.h`](plugins_api_contract.h): error vocabulary and
  contract types. Runtime behavior and `PluginManager` return types must be
  checked in their respective declarations and implementations.

## Supporting APIs

| Area | Headers | Boundary |
|---|---|---|
| Monitoring | `plugin_health_monitor.h`, `plugin_hot_plug_monitor.h`, `plugin_metrics.h`, `self_healing_plugin.h` | Optional health/recovery, filesystem event handling, metrics collection, and self-healing interfaces. |
| Distribution and trust | `oci_registry_client.h`, `signed_plugin_repository.h`, `oci_manifest_signing.h` | OCI reference/manifest/layer retrieval, pinned-key catalog API, and declared OCI signing helpers. These are distinct mechanisms; see [`../../src/plugins/TRUST.md`](../../src/plugins/TRUST.md). |
| Domain extensions | `rpc_plugin_interface.h`, `image_analysis_interface.h`, `image_analysis_manager.h`, `image_generation_interface.h`, `audio_backend_interface.h`, `huggingface_ingestion_plugin.h` | Domain-specific interfaces. A declaration does not prove a corresponding implementation is present or wired into `PluginManager`. |
| WASM | `wasm_host_api.h`, `wasm_component_model.h` | Host bridge and component-model declarations. Runtime availability depends on build configuration and runtime integration; this API does not establish a general sandbox guarantee. |
| Other | `plugin_api.h`, `edition_policy_plugin.h`, `tesseract_ocr_plugin.h`, `yolov8_onnx_plugin.h` | Typed access/policy and example or domain-specific plugin declarations. Verify each contract in its header. |

## Loading and trust boundary

For manifest-backed loading, `scanPluginDirectory(path)` discovers manifests
and `loadPlugin(name)` uses the registered manifest, resolves declared
dependencies, checks applicable edition/license policy and plugin-binary
policy, then creates and initializes the shared-library plugin. A plugin
library exports the factory/destructor pair supplied by
`THEMIS_PLUGIN_IMPL`.

`loadPluginFromPath(path, config)` and `loadPluginFromOci(reference, cache_dir,
auth_token)` are separate entry points: the explicit-path loader does not parse
`plugin.json`; the OCI path verifies the downloaded layer digest and delegates
to path loading. Neither applies manifest edition/license fields. Consult
[`../../src/plugins/TRUST.md`](../../src/plugins/TRUST.md) before treating a
digest or manifest `.sig` hash as publisher authentication.

The typed `PluginRegistry` is independent of the manager's manifest-backed
instance map. `SignedPluginRepository` is also a separate catalog API; its
pinned-key Ed25519 verification is not automatically implied by a manager load.

## Configuration and build boundaries

- Manifest fields include required `name`, `version`, and `binary`, with
  optional `dependencies`, `capabilities`, `auto_load`, `allowed_editions`,
  `license_feature`, and visibility-related fields. `auto_load` defaults to
  false. Consult `plugin_interface.h` for parser and schema details.
- `HotPlugConfig` defaults to disabled; its automatic load/reload/unload
  switches default to true when monitoring is enabled.
- `HealthMonitorConfig` defaults and lifetime requirements are declared in
  `plugin_health_monitor.h`.
- `OciRegistryClient` defaults its request timeout to 30 seconds; authentication
  may be anonymous when no registry credentials are supplied. Do not infer a
  separate connect-timeout default from the interface.
- `THEMIS_WASM_SUPPORT` controls the WASM bridge declarations/implementation.
  A registered `WasmPluginLoadFn` can be used by the loader path; built-in
  Wasmtime/WasmEdge code is additionally guarded by
  `THEMIS_WASM_WASMTIME` / `THEMIS_WASM_WASMEDGE`. No generic sandbox guarantee
  is established by these flags. The callback is application-supplied and its
  verification/isolation behavior is not guaranteed by this module.

The public headers do not prescribe a project-wide plugin install directory,
credential source, or production environment-variable set. Treat those as
deployment-level decisions, not header guarantees.
