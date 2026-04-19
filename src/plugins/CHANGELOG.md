> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Plugins Module

All notable changes to the Plugins module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

- WASM sandbox via Wasmtime for in-process native plugin isolation
- Marketplace integration for community plugin discovery and installation
- Per-plugin resource quotas (CPU, memory, I/O)
- SDK bindings for plugin authors (C, Python, Rust)
- Community plugin repository scanning and trust scoring

## [1.3.0] — 2026-03-12

### Added
- OCI registry client (`oci_registry_client.cpp`) for remote plugin loading with signed manifest verification
- Plugin hot-reload without server restart (`PluginManager::reloadPlugin()`) with atomic swap and rollback on failure
- Plugin capability negotiation with semantic version ranges (`PluginCapabilityNegotiator`)
- Plugin dependency resolution engine (`PluginDependencyResolver`) with cycle detection
- Health monitoring with automatic restart on crash (`plugin_health_monitor.cpp`)
- Prometheus metrics integration (`plugin_metrics.cpp`) with Grafana dashboard
- Plugin system edition management (`plugin_system_edition.cpp`) for community/enterprise tier differentiation
- Runtime capability escalation blocking: `PluginManager::checkCapabilityEscalation()` compares current capabilities against snapshot frozen at load time; any new capability flag triggers `ERR_PLUGIN_CAPABILITY_ESCALATION` and marks plugin as `RESTRICTED` (implemented 2026-04-09)

### Changed
- Plugin lifecycle now fully RAII-managed (load → init → unload with exception safety)
- Manifest validation upgraded to JSON Schema v2

### Fixed
- Race condition in hot-plug monitor during concurrent load/unload sequences
- Stale capability entries not cleaned up on plugin unload

## [1.2.0] — 2025-06-01

### Added
- Ed25519 signing and signature verification enforced at load time (`signed_plugin_repository.cpp`)
- Secure sandbox baseline with capability isolation at load time
- Plugin hot-plug file system monitor (`plugin_hot_plug_monitor.cpp`)
- RPC service registry integration (`rpc_service_registry.cpp`)

### Changed
- Plugin registry (`plugin_registry.cpp`) refactored to support multi-edition layouts
- Improved error reporting on manifest validation failure

## [1.1.0] — 2024-09-01

### Added
- Plugin manifest validation using JSON Schema v2
- Dynamic plugin loader using `dlopen`/`LoadLibrary` with platform abstraction
- Plugin lifecycle management (load/init/unload) with RAII guards
- HuggingFace ingestion plugin (`huggingface_ingestion_plugin.cpp`) as reference implementation

### Fixed
- Plugin manager crash on malformed manifest during batch load

## [1.0.0] — 2024-01-01

### Added
- Initial plugin infrastructure with `plugin_manager.cpp` and `plugin_registry.cpp`
- Core plugin interface (`IPlugin`) with version and capability declaration
- Basic dynamic library loading for Linux and Windows
