> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# Plugins Module Roadmap

## Current Status
v1.3.0 — Phases 1–4 complete and production-ready (Phase 5 planned):

- **Dynamic loading** — cross-platform dlopen/LoadLibrary-based plugin loading with platform abstraction
- **Manifest validation** — JSON Schema v2 validation with capability declarations, dependency fields, runtime field (`"native"` | `"wasm"`), and SHA-256 hash pinning
- **Ed25519 signing** — end-to-end plugin signing pipeline; load rejected unconditionally on signature failure
- **Capability-based permissions** — `PluginCapabilityNegotiator` enforces declared capabilities at load time; undeclared capabilities denied
- **Hot-reload** — `PluginHotPlugMonitor` (inotify/FSEvents) detects plugin file changes and triggers reload with atomic swap and rollback on failure
- **Dependency resolution** — topological sort (Kahn's algorithm) for ordered loading with cycle detection
- **Health monitoring** — `PluginHealthMonitor` runs periodic liveness probes and triggers auto-restart on consecutive failures
- **Prometheus metrics** — per-plugin call count, P99 latency, error rate, memory; Grafana dashboard at `grafana/dashboards/plugins.json`
- **OCI registry** — `OciRegistryClient` supports remote plugin fetch with signed manifest verification and key rotation
- **RPC service registry** — `RpcServiceRegistry` allows plugins to expose gRPC/RPC endpoints
- **WASM foundation** — `WasmPluginLoader` validates SHA-256, checks Enterprise edition gate; runtime instantiation placeholder ready for Wasmtime/WasmEdge linkage

## Completed ✅
- [x] `PluginManager` — singleton lifecycle orchestrator: scan, load, init, unload, hot-reload with rollback
- [x] `PluginRegistry` — central registry with `std::shared_mutex` (upgraded from `std::mutex`): read-concurrent, write-exclusive
- [x] `PluginHotPlugMonitor` — directory watcher with TOCTOU-safe reload (signature re-verified before atomic swap)
- [x] `PluginHealthMonitor` — liveness probe loop, auto-restart on `failure_threshold` consecutive failures, `attachMetrics()` integration
- [x] `PluginMetrics` / `PluginMetricsCollector` — atomic per-plugin counters, P95/P99 latency histogram, `IMetrics` sink integration
- [x] `PluginDependencyResolver` — header-only topological sort with cycle detection; `dependency` field in manifest
- [x] `SignedPluginRepository` — Ed25519 verification, pinned key store, manifest + binary signature verification
- [x] `OciRegistryClient` — OCI-compatible remote plugin fetch with signed manifest and key rotation
- [x] `RpcServiceRegistry` — plugin-exposed gRPC endpoint registration
- [x] `HuggingFaceIngestionPlugin` — first-party reference plugin with full lifecycle integration
- [x] `WasmPluginLoader` — SHA-256 hash verification, Enterprise edition gate, host-function C ABI stubs (Target: v0.9.0)
- [x] `WasmHostAPI` — `IThemisPlugin` bridge, runtime selector (`WasmPluginRuntime` enum), `wasm_host_api.h` public header
- [x] `plugin_system_edition.cpp` — Community / Professional / Enterprise capability tiers; WASM runtime gated behind Enterprise
- [x] JSON Schema v2 manifest (`manifest_schema_v2.json`) — capability declarations, dependency versioning, runtime and sha256 fields
- [x] `plugin_registry.cpp` — `unregisterFactory<T>` with `std::unique_lock`; WASM module hash verified via `verifyWasmModuleHash()` before instantiation
- [x] Grafana dashboard (`grafana/dashboards/plugins.json`) — health score, latency avg/P95/P99, error count, memory, reload count, load time panels
- [x] `PluginCapabilityNegotiator` — semantic version-range matching; `PluginManager::negotiateCapabilities()` exposed
- [x] 14 focused test targets registered in `tests/CMakeLists.txt` (see Production Readiness Checklist)

## In Progress 🚧
- [~] Per-plugin resource quotas (Target: Q2 2027)

## Planned Features 📋

### Short-term (Next 3–6 months)
- [x] Runtime capability escalation blocking (Target: Q4 2026 → implemented 2026-04-09)
  - Files: `include/plugins/plugin_manager.h`, `src/plugins/plugin_manager.cpp`, `include/utils/error_registry.h`
  - Implementation: `PluginManager::checkCapabilityEscalation()` compares current `getCapabilities()` against `frozen_capabilities` snapshot captured at load time; any new true flag triggers `ERR_PLUGIN_CAPABILITY_ESCALATION` and marks plugin as `RESTRICTED`
  - `PluginManager::isPluginRestricted()` allows callers to query restriction state
  - Tests: `tests/test_plugin_capability_escalation.cpp` — `CapabilityEscalationBlockedTests`, `CapabilityEscalationLogicTests`, `CapabilityEscalationApiTests`

- [ ] Per-plugin resource quotas (Target: Q2 2027)
  - Files: `include/plugins/plugin_interface.h`, `src/plugins/plugin_manager.cpp`, `src/plugins/plugin_metrics.cpp`
  - Implementation: `ResourceQuota{max_cpu_ms_per_call, max_memory_bytes, max_io_ops_per_sec}` declared in manifest; enforced via `PluginMetrics` counters + `cgroups` v2 (Linux) or Job Objects (Windows) when THEMIS_ENABLE_PLUGIN_QUOTAS=ON
  - Error cases: quota exceeded → call aborted, `ERR_PLUGIN_QUOTA_EXCEEDED` returned; `plugin_health_monitor` records violation
  - Tests: `tests/test_plugin_metrics.cpp` (quota enforcement path)
  - Perf: quota check overhead < 1 µs per plugin call (single atomic read)

### Long-term (6–18 months)
- [ ] WASM sandbox via Wasmtime (Target: Q3 2027)
  - Files: `src/plugins/wasm_plugin_loader.cpp`, `include/plugins/wasm_host_api.h`, `src/plugins/plugin_system_edition.cpp`
  - Implementation: replace `loadWasmPlugin()` TODO block with Wasmtime C API calls: `wasmtime_engine_new()` → `wasmtime_module_new()` → `wasmtime_instance_new()`; register host-function imports via `wasmtime_linker_define()`; hook `WasmHostAPI` vtable to WASM export dispatch; cold-start latency target < 50 ms with JIT warm cache
  - Constraints: gated behind `THEMIS_WASM_SUPPORT` compile flag and Enterprise edition runtime check; WASM linear-memory isolation prevents host-memory reads; SHA-256 hash verified before any `wasmtime_module_new()` call
  - Error cases: invalid WASM binary → `ERR_WASM_VALIDATION_FAILED`; hash mismatch → `ERR_WASM_HASH_MISMATCH`; runtime missing → build error (not a runtime fallback)
  - Tests: `tests/test_wasm_plugin_sandbox.cpp` — full lifecycle under Wasmtime; hash mismatch rejection; memory isolation smoke test
  - Perf: steady-state call overhead ≤ 3× vs. native (Wasmtime near-native tier); cold-start < 50 ms on JIT warm cache

- [ ] SDK bindings for plugin authors (Target: Q3 2027)
  - Languages: C (stable ABI), Python (`ctypes` wrapper), Rust (`themis-plugin` crate)
  - Files: new `sdk/c/`, `sdk/python/`, `sdk/rust/` under `plugins/` root
  - Implementation: C SDK exposes `themis_plugin_register()`, `themis_plugin_emit_metric()`, `themis_plugin_log()` via a versioned header; Python and Rust wrappers auto-generated from C header
  - Tests: per-SDK smoke tests under `sdk/*/tests/`

- [ ] Community plugin repository scanning and trust scoring (Target: Q4 2027)
  - Files: `src/plugins/signed_plugin_repository.cpp`, new trust-scorer implementation file (planned)
  - Implementation: `PluginTrustScorer` queries community registry for known CVEs, maintainer reputation score, and download count; trust score (0–1) stored in registry metadata; plugins below configurable threshold rejected or flagged
  - Security: registry communication over mTLS; trust scores re-evaluated on every OCI manifest update

- [ ] Marketplace integration (Target: Q4 2027)
  - Files: new marketplace client source/header pair (planned)
  - Implementation: REST client for community plugin discovery, rating, and one-click installation; integrates with `OciRegistryClient` for download and `SignedPluginRepository` for trust verification

## Implementation Phases

### Phase 1: Core Plugin Loading (Status: Completed ✅)
- [x] Cross-platform `dlopen`/`LoadLibrary` abstraction (`plugin_manager.cpp`)
- [x] `IThemisPlugin` base interface with lifecycle hooks (`plugin_interface.h`)
- [x] Plugin manifest (JSON) parsing and schema validation
- [x] `PluginRegistry` — central lookup; RAII guards on load/unload
- [x] `HuggingFaceIngestionPlugin` as first-party reference implementation
- [x] Basic capability declarations and edition-gated feature flags

### Phase 2: Security & Signing (Status: Completed ✅)
- [x] Ed25519 signing pipeline (`signed_plugin_repository.cpp`)
- [x] JSON Schema v2 manifest with capability declarations (`manifest_schema_v2.json`)
- [x] WASM SHA-256 hash verification (`wasm_plugin_loader.cpp` → `verifyWasmModuleHash()`)
- [x] `PluginCapabilityNegotiator` — semantic version-range matching
- [x] OCI registry client with signed manifest verification (`oci_registry_client.cpp`)
- [x] `plugin_system_edition.cpp` — Community / Professional / Enterprise tier gates

### Phase 3: Advanced Lifecycle Features (Status: Completed ✅)
- [x] `PluginHotPlugMonitor` — directory watcher, TOCTOU-safe reload, atomic swap with rollback
- [x] `PluginDependencyResolver` — topological sort, Kahn's algorithm, cycle detection
- [x] `PluginHealthMonitor` — liveness probe loop, auto-restart on failure threshold
- [x] `PluginMetrics` / `PluginMetricsCollector` — atomic counters, P99 latency histogram, `IMetrics` sink
- [x] `RpcServiceRegistry` — plugins expose gRPC endpoints
- [x] `WasmHostAPI` + `WasmPluginLoader` scaffold (Enterprise edition gate, SHA-256 verified)

### Phase 4: Production Hardening (Status: Completed ✅)
- [x] `PluginRegistry` upgraded from `std::mutex` to `std::shared_mutex` (reads concurrent, writes exclusive)
- [x] `PluginManager::reloadPlugin()` two-phase: verify signature before atomic swap; old plugin serves during verification
- [x] `PluginManager::negotiateCapabilities()` — public API for capability version-range negotiation
- [x] Grafana dashboard (`grafana/dashboards/plugins.json`) for all plugin telemetry
- [x] 13 focused test targets covering all major components (see Production Readiness Checklist)
- [x] `AUDIT.md`, `SECURITY.md`, `ARCHITECTURE.md`, `CHANGELOG.md`, `FUTURE_ENHANCEMENTS.md` documentation complete

### Phase 5: Sandbox & SDK (Status: In Progress 🚧)
- [ ] WASM Wasmtime runtime integration (replaces `loadWasmPlugin()` TODO block) (Target: Q3 2027)
- [x] Runtime capability escalation blocking (implemented 2026-04-09)
- [ ] Per-plugin resource quotas via cgroups v2 / Job Objects (Target: Q2 2027)
- [ ] C / Python / Rust plugin author SDKs (Target: Q3 2027)
- [ ] Community repository scanning and trust scoring (Target: Q4 2027)
- [ ] Marketplace integration (Target: Q4 2027)

## Production Readiness Checklist
- [x] Unit tests coverage > 80% — 14 focused test targets in `tests/CMakeLists.txt`
- [x] `PluginManagerFocusedTests` — `tests/test_plugin_manager.cpp`: singleton, scanPluginDirectory, autoLoad, hot-plug enable/disable, negotiateCapabilities
- [x] `PluginLifecycleFocusedTests` — `tests/test_plugin_lifecycle.cpp`: load/initialize/unload, hot-reload with rollback, dependency-ordered loading, RAII guards
- [x] `GenericPluginRegistryFocusedTests` — `tests/test_generic_plugin_registry.cpp`: registerFactory, create, listPlugins, hasPlugin, unregisterFactory, clearRegistry, concurrent reads under shared_mutex
- [x] `PluginHealthMonitorFocusedTests` — `tests/test_plugin_health_monitor.cpp`: liveness probe, auto-restart, failure threshold, attachMetrics
- [x] `PluginHotPlugFocusedTests` — `tests/test_plugin_hot_plug.cpp`: directory watch, TOCTOU-safe reload, event callback, enable/disable
- [x] `PluginHotReloadEnhancedFocusedTests` — `tests/test_plugin_hot_reload_enhanced.cpp`: atomic swap, rollback on bad plugin, reload listener phases
- [x] `PluginDependencyGraphFocusedTests` — `tests/test_plugin_dependency_graph.cpp`: topological sort, cycle detection, version compatibility
- [x] `PluginDependencyResolverFocusedTests` — `tests/test_plugin_dependency_resolver.cpp`: resolver API, missing dependency error, ordered load result
- [x] `PluginMetricsFocusedTests` — `tests/test_plugin_metrics.cpp`: per-plugin counters, latency recording, getAllStats
- [x] `PluginMetricsIntegrationFocusedTests` — `tests/test_plugin_metrics_integration.cpp`: IMetrics sink export, Prometheus label format
- [x] `PluginCapabilityEscalationTests` — `tests/test_plugin_capability_escalation.cpp`: checkCapabilityEscalation, isPluginRestricted, ERR_PLUGIN_CAPABILITY_ESCALATION, individual flag detection, error-code value contract
- [x] `PluginSecurityAuditFocusedTests` — `tests/test_plugin_security_audit.cpp`: signature enforcement, capability denial audit events
- [x] `PluginSecurityImplementationFocusedTests` — `tests/test_plugin_security_implementation.cpp`: Ed25519 verify, hash mismatch rejection, manifest tamper detection
- [x] `PluginCapabilityNegotiationTests` — `tests/test_plugin_capability_negotiation.cpp`: version-range matching, semantic version parse, negotiateCapabilities API
- [x] Security audit complete — `src/plugins/SECURITY.md`: Ed25519 mandatory, capability isolation at load, OCI signed manifest
- [x] Documentation complete — `src/plugins/ARCHITECTURE.md`, `src/plugins/AUDIT.md`, `src/plugins/FUTURE_ENHANCEMENTS.md`, `src/plugins/CHANGELOG.md`
- [x] Build system — all 10 `src/plugins/*.cpp` registered in `cmake/CMakeLists.txt`; `wasm_plugin_loader.cpp` gated behind `THEMIS_WASM_SUPPORT`
- [x] API stability — `IThemisPlugin` vtable and `PluginManifest` struct stable from v1.x; `plugin_api.h` version constants maintained
- [x] Thread safety — `PluginRegistry` uses `std::shared_mutex`; `PluginManager` uses `std::mutex`; `PluginMetrics` uses atomic counters; `PluginHotPlugMonitor` and `PluginHealthMonitor` run on dedicated background threads
- [x] Grafana dashboard — `grafana/dashboards/plugins.json` covering health score, latency, error count, memory, reload count, load time

## Known Issues & Limitations
- Runtime capability escalation is now blocked programmatically via `PluginManager::checkCapabilityEscalation()`. This is an explicit check; for continuous enforcement on the hot call path, a future enhancement could wrap every `getCapabilities()` invocation.
- Native plugins run in-process; a crash or memory corruption propagates to the host until WASM sandbox isolation is complete (Target: Q3 2027, OI-01 in AUDIT.md).

## Breaking Changes
- `IThemisPlugin` vtable is stable from v1.x; any new pure virtual method would be a breaking change requiring a major version bump.
- `PluginManifest` struct is stable from v1.x; new optional fields are backward compatible; removing or renaming existing fields requires a major version bump.
- `PluginRegistry::clearRegistry()` is marked testing-only; calling it in production is unsupported.