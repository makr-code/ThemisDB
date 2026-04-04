<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Plugins Module

All notable changes to public headers are documented here.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
Implementation details in `../../src/plugins/CHANGELOG.md`.

## [1.3.0] — 2026-01

### Added
- `oci_registry_client.h` — `OciRegistryClient` for OCI registry push/pull of plugin artifacts.
- `plugin_hot_plug_monitor.h` — `PluginHotPlugMonitor` inotify/FSEvents-based hot-reload.
- `plugin_dependency_resolver.h` — `PluginDependencyResolver` topological dependency resolution and `PluginCapabilityNegotiator`.
- `plugin_health_monitor.h` — `PluginHealthMonitor` liveness/readiness probes.
- `plugin_metrics.h` — `PluginMetrics` Prometheus counter/histogram per plugin.

## [1.2.0] — 2025-09

### Added
- `signed_plugin_repository.h` — `SignedPluginRepository` Ed25519 signature verification.
- `wasm_host_api.h` — `WasmHostApi` WASM sandbox with restricted host-call surface.
- `self_healing_plugin.h` — `SelfHealingPlugin` crash recovery and restart policy.

## [1.1.0] — 2025-06

### Added
- `image_analysis_interface.h` — `IImageAnalysisPlugin` abstract interface.
- `image_analysis_manager.h` — `ImageAnalysisManager` plugin instance lifecycle.
- `rpc_plugin_interface.h` — `IRpcPlugin` RPC transport interface.
- `huggingface_ingestion_plugin.h` — Hugging Face dataset ingestion plugin.

## [1.0.0] — 2025-01

### Added
- `plugin_interface.h` — `IThemisPlugin` core plugin interface.
- `plugin_manager.h` — `PluginManager` central registry.
- `plugin_registry.h` — Thread-safe plugin catalogue.
- `plugin_api.h` — C ABI macros (`THEMIS_PLUGIN_EXPORT`, version helpers).
