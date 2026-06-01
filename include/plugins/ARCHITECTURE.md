> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/plugins/ARCHITECTURE.md -->

# Plugins Module — Public Header Architecture

**Module Path:** `include/plugins/`
**Implementation:** `../../src/plugins/`
**Canonical architecture doc:** [`../../src/plugins/ARCHITECTURE.md`](../../src/plugins/ARCHITECTURE.md)

---

## 1. Overview

`include/plugins/` defines the **public plugin lifecycle, registry, security, OCI distribution, and WASM runtime API contract** for ThemisDB. The 20 headers cover the core plugin interfaces and API, plugin manager, registry, dependency resolution, health monitoring, hot-plug, metrics, self-healing, RPC plugin, OCI registry/manifest signing, signed plugin repository, HuggingFace ingestion plugin, image analysis and generation interfaces, audio backend, and WebAssembly component model and host API.

For runtime composition — plugin sandboxing, OCI layer pull/push mechanics, WASM instantiation, and self-healing loop internals — see:
→ [`../../src/plugins/ARCHITECTURE.md`](../../src/plugins/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Core Plugin Contract

| Header | Public Type | Purpose |
|--------|------------|---------|
| `plugin_interface.h` | `IPlugin` | Base plugin interface every plugin must implement |
| `plugin_api.h` | `PluginAPI` | Host-side API surface exposed to plugins |
| `plugin_manager.h` | `PluginManager` | Plugin load/unload lifecycle and dispatch |
| `plugin_registry.h` | `PluginRegistry` | Plugin discovery and capability-based lookup |
| `plugin_dependency_resolver.h` | `PluginDependencyResolver` | Dependency-graph resolution and load ordering |

### 2.2 Health, Monitoring, and Self-Healing

| Header | Public Type | Purpose |
|--------|------------|---------|
| `plugin_health_monitor.h` | `PluginHealthMonitor` | Liveness and readiness checks for loaded plugins |
| `plugin_hot_plug_monitor.h` | `PluginHotPlugMonitor` | Hot-plug insertion and removal detection |
| `plugin_metrics.h` | `PluginMetrics` | Per-plugin Prometheus-compatible metrics |
| `self_healing_plugin.h` | `SelfHealingPlugin` | Self-healing retry and recovery contract |

### 2.3 Security and Distribution

| Header | Public Type | Purpose |
|--------|------------|---------|
| `oci_registry_client.h` | `OCIRegistryClient` | OCI-registry pull/push client |
| `oci_manifest_signing.h` | `OCIManifestSigning` | OCI manifest cosign/sigstore signing and verification |
| `signed_plugin_repository.h` | `SignedPluginRepository` | Repository of signature-verified plugin artefacts |

### 2.4 Domain Plugin Interfaces

| Header | Public Type | Purpose |
|--------|------------|---------|
| `rpc_plugin_interface.h` | `IRPCPlugin` | RPC endpoint extension plugin interface |
| `huggingface_ingestion_plugin.h` | `HuggingFaceIngestionPlugin` | HuggingFace Hub dataset ingestion plugin |
| `image_analysis_interface.h` | `IImageAnalysisPlugin` | Image analysis plugin contract |
| `image_analysis_manager.h` | `ImageAnalysisManager` | Image-analysis plugin lifecycle and dispatch |
| `image_generation_interface.h` | `IImageGenerationPlugin` | Image generation plugin contract |
| `audio_backend_interface.h` | `IAudioBackend` | Audio backend plugin contract |

### 2.5 WebAssembly Runtime

| Header | Public Type | Purpose |
|--------|------------|---------|
| `wasm_component_model.h` | `WASMComponentModel` | WebAssembly Component Model instantiation and binding |
| `wasm_host_api.h` | `WASMHostAPI` | Host-function API exposed to WASM components |

---

## 3. Namespace Layout

| Namespace | Scope |
|-----------|-------|
| `themis::plugins` | All plugin lifecycle, registry, security, and WASM types |

---

## 4. Public Contract Notes

- `IPlugin` in `plugin_interface.h` is the mandatory base contract; all plugins must implement it.
- `PluginAPI` defines the stable host-side surface exposed to loaded plugins; breaking changes require major-version bumps.
- OCI and signing headers define stable distribution and verification contracts; cosign/sigstore verification must fail closed for unsigned or untrusted artefacts.
- Domain plugin interfaces (`IImageAnalysisPlugin`, `IImageGenerationPlugin`, `IAudioBackend`, `IRPCPlugin`) are public extension points for embedders.
- WASM headers define the Component Model and host-function contracts; sandboxing and memory isolation remain internal.
- Health, hot-plug, and self-healing headers expose stable monitoring and recovery contracts for orchestration consumers.
