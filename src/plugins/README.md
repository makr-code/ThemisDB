# Plugins Module

Plugin system infrastructure for ThemisDB.

**Last Updated:** March 2026

## Module Purpose

Implements the plugin system infrastructure for ThemisDB, providing dynamic plugin loading, secure plugin execution with manifest validation and Ed25519 signature verification, plugin lifecycle management, hot-plug monitoring, health monitoring, dependency resolution, and OCI registry integration.

## Subsystem Scope

**In scope:** Dynamic shared library loading (`dlopen`/`LoadLibrary`), plugin manifest validation and marketplace manifest schema (v2), Ed25519 signing/verification (`SignedPluginRepository`), plugin lifecycle (register/initialize/execute/shutdown), capability-based negotiation (`PluginCapabilityNegotiator`), plugin dependency resolution, plugin hot-plug file watching, plugin health monitoring with self-healing, metrics collection, OCI registry client, HuggingFace ingestion plugin, RPC plugin/service registry.

**Out of scope:** Plugin business logic (in individual plugin packages), WASM sandboxing (planned — see FUTURE_ENHANCEMENTS.md).

## Source Files

### Headers (`include/plugins/`)

| Header | Description |
|--------|-------------|
| `plugin_interface.h` | Core plugin types: `IThemisPlugin`, `PluginManifest`, `MarketplaceManifest`, `PluginCapabilityNegotiator`, `PluginVersionRange`, `ManifestSchemaValidator` |
| `plugin_manager.h` | `PluginManager`: load/unload/reload/negotiate/scan; hot-plug and OCI integration |
| `plugin_api.h` | Top-level API helpers (`PluginAPI`) — header-only |
| `plugin_registry.h` | Plugin registration catalog |
| `plugin_metrics.h` | Per-plugin call/error/latency metrics |
| `plugin_health_monitor.h` | Crash detection and automatic restart (`PluginHealthMonitor`) |
| `plugin_hot_plug_monitor.h` | File-system watcher for hot-plug events (`PluginHotPlugMonitor`) |
| `plugin_dependency_resolver.h` | DAG-based dependency resolution — header-only |
| `signed_plugin_repository.h` | Ed25519-signed plugin catalog with key-pinning (`SignedPluginRepository`) |
| `oci_registry_client.h` | OCI registry pull client (`OciRegistryClient`) |
| `rpc_plugin_interface.h` | RPC plugin interface |
| `image_analysis_interface.h` | Multi-backend image-analysis plugin interface |
| `image_analysis_manager.h` | Image-analysis plugin manager |
| `self_healing_plugin.h` | Self-healing plugin helpers |
| `huggingface_ingestion_plugin.h` | HuggingFace dataset ingestion plugin |
| `manifest_schema_v2.json` | JSON Schema for marketplace manifest validation |

### Sources (`src/plugins/`)

| Source | Description |
|--------|-------------|
| `plugin_manager.cpp` | `PluginManager` implementation: dlopen/LoadLibrary, lifecycle, `verifyPlugin()` (Ed25519, NDEBUG-guarded), `reloadPlugin()` (Phase-2 atomic TOCTOU-safe swap), `negotiateCapabilities()` |
| `plugin_registry.cpp` | Plugin registration catalog implementation |
| `plugin_metrics.cpp` | Plugin metrics collection |
| `plugin_health_monitor.cpp` | Health monitoring and auto-restart |
| `plugin_hot_plug_monitor.cpp` | File-system watcher for directory hot-plug |
| `signed_plugin_repository.cpp` | Ed25519 signature verification with key-pinning |
| `oci_registry_client.cpp` | OCI registry manifest fetch and layer pull |
| `rpc_service_registry.cpp` | RPC service registry |
| `huggingface_ingestion_plugin.cpp` | HuggingFace ingestion plugin implementation |
| `plugin_system_edition.cpp` | Edition-gated plugin system initialisation |

## Current Delivery Status

**Maturity:** 🟡 Beta — Core plugin loading, manifest validation, Ed25519 signature verification, capability negotiation, dependency resolution, health monitoring, hot-plug monitoring, and OCI registry integration are all implemented and production-ready. WASM sandbox isolation is planned (see FUTURE_ENHANCEMENTS.md).

## Features

- Dynamic plugin loading (`.so`/`.dll` via `dlopen`/`LoadLibrary`)
- Plugin manifest validation (JSON Schema v2, `ManifestSchemaValidator`)
- Ed25519 plugin signature verification (`SignedPluginRepository`, key-pinning)
- Plugin lifecycle management (load/initialize/unload with RAII guards)
- Capability-based negotiation (`PluginCapabilityNegotiator`, version ranges)
- Plugin dependency resolution (DAG, cycle detection)
- Hot-plug file-system monitoring (`PluginHotPlugMonitor`)
- Health monitoring and auto-restart (`PluginHealthMonitor`)
- Plugin metrics (call latency, error rate per plugin)
- OCI registry integration (remote plugin pull)
- HuggingFace ingestion plugin
- RPC plugin/service registry

## Documentation

For plugin documentation, see:
- [Plugin Migration](../../docs/de/plugins/PLUGIN_MIGRATION.md)
- [Manifest Signatures](../../docs/de/plugins/MANIFEST_SIGNATURES.md)
- [RPC Plugin Architecture](../../docs/de/plugins/RPC_PLUGIN_ARCHITECTURE.md)
- [Plugin Signer Tool](../../tools/plugin_signer/)
- [Missing Implementations Report](../../docs/de/plugins/missing-implementations.md)

## Scientific References

1. Gamma, E., Helm, R., Johnson, R., & Vlissides, J. (1994). **Design Patterns: Elements of Reusable Object-Oriented Software**. Addison-Wesley. ISBN: 978-0-201-63361-0

2. Fowler, M. (2002). **Patterns of Enterprise Application Architecture**. Addison-Wesley. ISBN: 978-0-321-12742-6

3. Herzfeld, C. (1989). **Plugin Architectures and Extensible Applications**. *ACM SIGPLAN Notices*, 24(4), 57–65.

4. Szyperski, C. (2002). **Component Software: Beyond Object-Oriented Programming (2nd ed.)**. Addison-Wesley. ISBN: 978-0-201-74572-6
