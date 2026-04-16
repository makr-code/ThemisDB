<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Plugins Module Roadmap

## Current Status

v1.3.0 — production. Hot-reload, Ed25519 signing, OCI distribution, WASM sandbox, dependency resolution, and health monitoring are operational.

## Completed

- [x] Core `IThemisPlugin` interface and C ABI macros
- [x] `PluginManager` central registry and `PluginRegistry` catalogue
- [x] Ed25519 signature verification (`SignedPluginRepository`)
- [x] WASM sandbox with restricted host-call surface
- [x] Self-healing crash recovery (`SelfHealingPlugin`)
- [x] Image analysis plugin interface + manager
- [x] RPC plugin transport interface
- [x] HuggingFace ingestion plugin
- [x] OCI registry push/pull (`OciRegistryClient`)
- [x] Hot-reload (`PluginHotPlugMonitor`)
- [x] Topological dependency resolution (`PluginDependencyResolver`)
- [x] Health monitoring (`PluginHealthMonitor`)
- [x] Per-plugin Prometheus metrics (`PluginMetrics`)

## Implementation Phases

### Phase 1 — Core API ✅
- [x] `IThemisPlugin` interface design
- [x] `plugin_api.h` C ABI macros and version negotiation

### Phase 2 — Registry & Lifecycle ✅
- [x] `PluginManager` load/unload/list
- [x] `PluginRegistry` thread-safe catalogue

### Phase 3 — Security ✅
- [x] Ed25519 signing and verification
- [x] WASM sandbox host-call allowlist

### Phase 4 — Resilience ✅
- [x] `SelfHealingPlugin` crash recovery
- [x] `PluginHealthMonitor` probes
- [x] `PluginHotPlugMonitor` hot-reload

### Phase 5 — Distribution & Observability ✅
- [x] `OciRegistryClient` OCI distribution
- [x] `PluginMetrics` Prometheus integration
- [x] `PluginDependencyResolver` topo-sort

### Phase 6 — Future Enhancements (Planned)
- [x] WASM Component Model support (Target: Q3 2026)
- [ ] Plugin sandboxing via Linux namespaces (Target: Q4 2026)
- [x] Signed OCI manifests with Sigstore/Cosign (Target: Q3 2026)

## Production Readiness Checklist

- [x] All plugins verified to implement `IThemisPlugin` before load
- [x] Ed25519 verification tested with known-bad signatures
- [x] WASM sandbox fuzz-tested against malformed bytecode
- [x] WASM Component Model upgrade (Target: Q3 2026)
