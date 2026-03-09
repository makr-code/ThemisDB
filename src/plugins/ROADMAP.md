# Plugins Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
v1.x – Core plugin infrastructure production-ready. Dynamic loading, manifest validation, Ed25519 signing, dependency resolution, hot-reload, health monitoring, capability negotiation, Prometheus metrics, and OCI registry client are all implemented.

## Completed ✅
- [x] Dynamic plugin loader (shared library loading)
- [x] Plugin lifecycle management (load, initialize, unload)
- [x] Plugin API implementation and versioning
- [x] Plugin manifest validation (JSON Schema v2)
- [x] Plugin signing and signature verification (Ed25519)
- [x] Secure plugin execution sandbox
- [x] Plugin signer tool (`tools/plugin_signer/`)
- [x] Runtime plugin capability negotiation with version ranges (`PluginCapabilityNegotiator`, Issue: #1984)
- [x] Plugin hot-reload without server restart (`plugin_hot_plug_monitor.cpp`, `PluginManager::reloadPlugin()`)
- [x] Plugin dependency resolution (plugin A requires plugin B) (`PluginDependencyResolver`, Issue: #2427)
- [x] Plugin health monitoring and automatic restart on crash (`plugin_health_monitor.cpp`)
- [x] Plugin metrics / Prometheus integration (`plugin_metrics.cpp`)
- [x] Remote plugin loading from OCI registries (`oci_registry_client.cpp`, Issue: #2224)

## In Progress 🚧
- [?] Plugin marketplace / registry integration (Target: Q3 2026)
- [?] Plugin configuration schema validation (JSON Schema per-plugin config)
- [?] Per-plugin resource quotas (CPU time, memory)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [?] Plugin API versioning with compatibility matrix
- [?] First-party importer plugins (MySQL, SQLite, MongoDB)

### Long-term (6-12 months)
- [?] WebAssembly (WASM) plugin runtime for sandboxed execution
- [?] Plugin capability permissions model (fine-grained access control)
- [?] Plugin SDK (C++, Python, Go bindings)
- [?] Community plugin repository with security scanning

## Implementation Phases

### Phase 1: Core Plugin Infrastructure (Status: Completed ✅)
- [x] Dynamic plugin loader – shared library (.so/.dll) loading via dlopen/LoadLibrary
- [x] Plugin lifecycle management (load, initialize, unload) with RAII guards
- [x] Plugin API versioning and compatibility negotiation
- [x] Runtime plugin capability negotiation with version ranges (`PluginCapabilityNegotiator`, `PluginVersionRange`, `PluginCapabilityRequirement` in `include/plugins/plugin_interface.h`)
- [x] `PluginManager::negotiateCapabilities()` entry point in `src/plugins/plugin_manager.cpp`
- [x] Plugin manifest validation (JSON Schema enforcement)
- [x] Ed25519 plugin signing and signature verification (`tools/plugin_signer/`)
- [x] Secure plugin execution sandbox (capability isolation)
- [x] Basic per-plugin resource accounting

### Phase 2: Signing Hardening & Permissions (Status: Completed ✅)
- [x] Ed25519 manifest signing workflow with key-rotation support (`signed_plugin_repository.cpp`)
- [?] Capability-based permission model (fine-grained access control per plugin) (Target: Q3 2026)
- [x] Plugin dependency resolution (plugin A requires plugin B) (`PluginDependencyResolver`)
- [x] Plugin hot-reload without server restart (`PluginManager::reloadPlugin()`, atomic swap with rollback)

### Phase 3: WASM Sandbox & Ecosystem (Status: In Progress 🚧)
- [?] WebAssembly (WASM) plugin runtime via Wasmtime for sandbox isolation (Target: Q3 2027)
- [x] Plugin metrics dashboard (call latency, error rate per plugin — `plugin_metrics.cpp`, `PluginMetricsCollector`, `grafana/dashboards/plugins.json`)
- [x] Remote plugin loading from OCI registries (`oci_registry_client.cpp`)
- [?] Plugin SDK with C++, Python, and Go bindings (Target: Q4 2027)
- [?] Community plugin repository with automated security scanning (Target: Q4 2027)

## Production Readiness Checklist
- [x] Unit tests for core plugin manager operations
- [x] Integration tests (lifecycle, hot-reload, health monitoring)
- [?] Unit test coverage > 80% across all plugin subsystems
- [?] Performance benchmarks (plugin call overhead)
- [?] Security audit (signature enforcement, sandbox escape prevention)
- [x] Documentation complete (README, ARCHITECTURE, ROADMAP, FUTURE_ENHANCEMENTS)
- [x] API stability guaranteed (v1.x stable, breaking changes require major version bump)
- [x] Prometheus metrics integration (`PluginMetricsCollector`, `plugin_health_score` gauge, Grafana dashboard)

## Known Issues & Limitations
- Plugin execution is in-process; a crash in a native plugin can affect the server (WASM isolation planned).
- WASM sandbox isolation is planned for v0.9.0 but not yet implemented.
- Capability-based permission model enforcement is load-time only; runtime escalation is not yet blocked programmatically.

## Breaking Changes
- Plugin API version 1.x is stable; v2.0 will add new hook points with backward compatibility.
- Manifest format may gain new required fields in v1.5.0.
