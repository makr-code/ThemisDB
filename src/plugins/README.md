# Plugins Module
<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: src/plugins/README.md · src/plugins/ARCHITECTURE.md · src/plugins/ROADMAP.md · src/plugins/FUTURE_ENHANCEMENTS.md · include/plugins/FUTURE_ENHANCEMENTS.md · docs/de/plugins/README.md -->

Plugin system infrastructure for ThemisDB.

## Module Purpose

Implements the plugin system infrastructure for ThemisDB, providing dynamic plugin loading, secure plugin execution with manifest validation and Ed25519 signing, plugin lifecycle management, hot-reload, dependency resolution, health monitoring, and per-plugin metrics.

## Subsystem Scope

**In scope:** Dynamic shared library loading, plugin manifest validation, Ed25519 signing/verification, plugin lifecycle (register/initialize/execute/shutdown), capability-based permissions, hot-reload without server restart, dependency resolution and topological load ordering, plugin health monitoring and self-healing, per-plugin Prometheus-compatible metrics, OCI registry integration, RPC service registration.

**Out of scope:** Plugin business logic (in individual plugin packages), WASM sandboxing (planned), community plugin marketplace (planned).

## Relevant Interfaces

| File / Component | Role |
|---|---|
| `plugin_manager.cpp` | Core lifecycle management: load, unload, hot-reload, capability negotiation, autoLoad |
| `plugin_registry.cpp` | Plugin registration, manifest validation, Ed25519 signature verification |
| `plugin_system_edition.cpp` | Edition-gated feature flags (CORE / Professional / Enterprise) |
| `plugin_metrics.cpp` | Per-plugin metrics: call count, latency, error rate |
| `plugin_health_monitor.cpp` | Liveness probing, automatic restart on consecutive failures |
| `plugin_hot_plug_monitor.cpp` | Directory watcher: detects new/updated plugins and triggers reload |
| `signed_plugin_repository.cpp` | Signed plugin repository: pinned-key store, remote manifest fetch |
| `oci_registry_client.cpp` | Remote plugin loading from OCI (Docker Hub-compatible) registries |
| `rpc_service_registry.cpp` | Registry for plugins that expose gRPC/RPC service endpoints |
| `huggingface_ingestion_plugin.cpp` | First-party plugin: HuggingFace model ingestion |

## Header Interfaces (`include/plugins/`)

| Header | Role |
|---|---|
| `plugin_interface.h` | `IThemisPlugin`, `IStatefulPlugin`, `ISelfHealingPlugin`, `PluginCapabilityNegotiator` |
| `plugin_manager.h` | `PluginManager` — main entry point for host code |
| `plugin_registry.h` | `PluginRegistry` — registration, validation, verification |
| `plugin_api.h` | Public plugin API types and version constants |
| `plugin_dependency_resolver.h` | `PluginDependencyResolver` — header-only topological sort / cycle detection |
| `plugin_health_monitor.h` | `PluginHealthMonitor` — liveness probe API |
| `plugin_hot_plug_monitor.h` | `PluginHotPlugMonitor` — filesystem watch API |
| `plugin_metrics.h` | `PluginMetrics` — per-plugin telemetry |
| `signed_plugin_repository.h` | `SignedPluginRepository` — key store and entry management |
| `oci_registry_client.h` | `OciRegistryClient` — remote registry fetch |
| `rpc_plugin_interface.h` | `RpcPlugin`, `RpcServiceRegistry` — RPC plugin base types |
| `self_healing_plugin.h` | `ISelfHealingPlugin` — heartbeat and auto-restart contract |
| `manifest_schema_v2.json` | JSON Schema v2 for capability-aware manifests |
| `image_analysis_interface.h` | `IImageAnalysisPlugin` — image analysis plugin base |
| `image_analysis_manager.h` | `ImageAnalysisManager` — multi-backend plugin manager |
| `huggingface_ingestion_plugin.h` | `HuggingFaceIngestionPlugin` — HuggingFace first-party plugin |

## Current Delivery Status

**Maturity:** 🟢 Production-Ready — Core plugin loading, manifest validation, Ed25519 signing, hot-reload, dependency resolution, health monitoring, metrics, OCI registry, and RPC integration are all implemented. WASM sandbox isolation and community marketplace are planned.

## Documentation

For plugin documentation, see:
- [ARCHITECTURE (src/plugins)](ARCHITECTURE.md) — detailed architecture guide
- [ROADMAP (src/plugins)](ROADMAP.md) — development roadmap, verified against source
- [Secondary Docs (docs/de/plugins)](../../docs/de/plugins/README.md) — German-language overview
- [Plugin Migration Guide](../../docs/de/plugins/PLUGIN_MIGRATION.md)
- [Manifest Signatures](../../docs/de/plugins/MANIFEST_SIGNATURES.md)
- [Hot-Reload Guide](../../docs/de/plugins/HOT_RELOAD_GUIDE.md)
- [Dependency Resolver Usage](../../docs/de/plugins/DEPENDENCY_RESOLVER_USAGE.md)

## Scientific References

1. Gamma, E., Helm, R., Johnson, R., & Vlissides, J. (1994). **Design Patterns: Elements of Reusable Object-Oriented Software**. Addison-Wesley. ISBN: 978-0-201-63361-0

2. Fowler, M. (2002). **Patterns of Enterprise Application Architecture**. Addison-Wesley. ISBN: 978-0-321-12742-6

3. Herzfeld, C. (1989). **Plugin Architectures and Extensible Applications**. *ACM SIGPLAN Notices*, 24(4), 57–65.

4. Szyperski, C. (2002). **Component Software: Beyond Object-Oriented Programming (2nd ed.)**. Addison-Wesley. ISBN: 978-0-201-74572-6

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.
