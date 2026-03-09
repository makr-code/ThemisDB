# Plugins Module

Plugin system infrastructure for ThemisDB.

## Module Purpose

Implements the plugin system infrastructure for ThemisDB, providing dynamic plugin loading, secure plugin execution with manifest validation and signing, and plugin lifecycle management.

## Subsystem Scope

**In scope:** Dynamic shared library loading, plugin manifest validation (JSON Schema v2), Ed25519 signing/verification, plugin lifecycle (register/initialize/execute/shutdown), capability-based permissions, dependency resolution, hot-reload, health monitoring, Prometheus metrics, OCI registry client.

**Out of scope:** Plugin business logic (in individual plugin packages), WASM sandboxing (planned for v0.9.0), plugin SDK bindings for Python/Go (planned).

## Relevant Interfaces

- `plugin_manager.cpp` — lifecycle orchestrator: load, initialize, hot-reload, shutdown
- `plugin_registry.cpp` — central registry of loaded plugins and their metadata
- `plugin_hot_plug_monitor.cpp` — directory watcher for hot-plug install/update
- `plugin_health_monitor.cpp` — periodic health checks and auto-unload of failing plugins
- `plugin_metrics.cpp` — per-plugin call counts, latency, error rates (Prometheus)
- `signed_plugin_repository.cpp` — Ed25519 signing and signature verification
- `plugin_system_edition.cpp` — edition-aware plugin limits and capability gates

## Current Delivery Status

**Maturity:** 🟢 Production Ready — Dynamic loading, manifest validation, Ed25519 signing, dependency resolution, hot-reload, health monitoring, and capability negotiation are all implemented and tested.

## Components

- Plugin manager (lifecycle: load, initialize, hot-reload, shutdown)
- Plugin registry (central catalog with type-indexed lookup)
- Plugin health monitor (liveness probes, auto-restart on failure)
- Plugin hot-plug monitor (filesystem watcher for zero-downtime updates)
- Plugin metrics (Prometheus-compatible per-plugin telemetry)
- Plugin security (Ed25519 signing/verification, manifest schema v2)
- Plugin dependency resolver (topological sort, cycle detection)
- OCI registry client (pull plugins from OCI/Docker registries)

## Features

- Dynamic plugin loading (dlopen/LoadLibrary, platform-native)
- Secure plugin execution (Ed25519 signature verification, capability-based permissions)
- Plugin manifest validation (JSON Schema v2)
- Plugin signing and verification (Ed25519, key rotation support)
- Dependency resolution (Kahn's algorithm, circular dependency detection)
- Hot-reload without server restart (atomic swap with rollback on failure)
- Plugin health monitoring and automatic restart on repeated failure
- Prometheus metrics per plugin (call count, P50/P95/P99 latency, error rate)
- Runtime capability negotiation with version-range constraints
- OCI registry integration for remote plugin distribution

## Documentation

For plugin documentation, see:
- [Plugin Security](../../docs/plugins/PLUGIN_SECURITY.md)
- [Plugin Migration](../../docs/plugins/PLUGIN_MIGRATION.md)
- [Manifest Signatures](../../docs/plugins/MANIFEST_SIGNATURES.md)
- [Plugin Signer Tool](../../tools/plugin_signer/)

## Scientific References

1. Gamma, E., Helm, R., Johnson, R., & Vlissides, J. (1994). **Design Patterns: Elements of Reusable Object-Oriented Software**. Addison-Wesley. ISBN: 978-0-201-63361-0

2. Fowler, M. (2002). **Patterns of Enterprise Application Architecture**. Addison-Wesley. ISBN: 978-0-321-12742-6

3. Herzfeld, C. (1989). **Plugin Architectures and Extensible Applications**. *ACM SIGPLAN Notices*, 24(4), 57–65.

4. Szyperski, C. (2002). **Component Software: Beyond Object-Oriented Programming (2nd ed.)**. Addison-Wesley. ISBN: 978-0-201-74572-6
