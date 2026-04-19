> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · docs/de/plugins/ -->

# Plugins Module - Future Enhancements
<!-- Links: src/plugins/README.md · src/plugins/ROADMAP.md · src/plugins/ARCHITECTURE.md · docs/de/plugins/README.md -->

## Scope

This document covers planned enhancements to ThemisDB's plugin subsystem, which is currently **Production Ready** for dynamic loading, lifecycle management, manifest validation, Ed25519 signing, dependency resolution, hot-reload, health monitoring, and Prometheus metrics. The following features are planned for future releases to further harden and extend the platform.

## Design Constraints

- Plugins must be cryptographically signed; unsigned plugins must be rejected at load time by `plugin_registry.cpp`.
- The plugin execution sandbox must prevent plugins from accessing host memory outside their granted capability set (defined in `plugin_system_edition.cpp`).
- The public Plugin API surface must remain stable across minor versions; breaking changes require a major version bump and migration shim.
- Hot-plug operations (`plugin_hot_plug_monitor.cpp`) must not cause observable latency spikes (>5 ms) on the query path.

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `IPlugin::on_load() / on_unload()` | `plugin_manager.cpp` | Lifecycle hooks; must be idempotent |
| `PluginManifest` (JSON schema v2) | `plugin_registry.cpp` | Validated against embedded JSON Schema at registration |
| `RpcServiceRegistry` | `rpc_service_registry.cpp` | Plugins that expose gRPC endpoints register here |
| `PluginMetrics::record_latency()` | `plugin_metrics.cpp` | Per-plugin P99 latency telemetry |
| `PluginHealthMonitor::heartbeat()` | `plugin_health_monitor.cpp` | Liveness probe; triggers auto-restart on failure |

## Planned Features

### [x] `PluginRegistry`: Upgrade Global Mutex to `shared_mutex`
**Priority:** Medium
**Target Version:** v1.8.0

`plugin_registry.cpp` uses a single static `std::mutex` (line 50) for all registry operations. All read operations (`getPlugin`, `listPlugins`, `isRegistered`) hold an exclusive lock, serializing concurrent reads from multiple modules.

**Implementation Notes:**
- `[x]` Replace the static `std::mutex` in `PluginRegistry::getMutex()` with a `std::shared_mutex`.
- `[x]` Upgrade `getPlugin`, `listPlugins`, `isRegistered` to `std::shared_lock`.
- `[x]` Keep `registerPlugin`, `unregisterPlugin`, `clear` on `std::unique_lock`. Added `unregisterFactory<T>` with `std::unique_lock`.

---


**Priority:** High
**Target Version:** v0.9.0

Replace the current dlopen-based loading with a WASM runtime (Wasmtime or WasmEdge) to provide memory-safe, OS-independent plugin execution. Each plugin runs in its own WASM linear memory; host functions are explicitly allowlisted via a capabilities manifest field.

**Implementation Notes:**
- `[x]` Add `wasm_plugin_loader.cpp` alongside `plugin_manager.cpp`; select loader via `PluginManifest.runtime` field (`"native"` | `"wasm"`). Runtime field added to `PluginManifest`; loader scaffold in place with `#ifdef THEMIS_WASM_SUPPORT`.
- `[x]` Expose a `WasmHostAPI` header under `include/plugins/` that maps the existing `IPlugin` vtable to WASM import functions. Created `include/plugins/wasm_host_api.h`.
- `[x]` Update `plugin_system_edition.cpp` to gate WASM support behind the Enterprise edition flag. WASM path gated by `ArePluginsSupported()` + `THEMIS_WASM_SUPPORT`.
- `[x]` `plugin_registry.cpp` must verify WASM module hash against manifest `sha256` field before instantiation. Implemented in `verifyWasmModuleHash()` in `wasm_plugin_loader.cpp`; called before any WASM instantiation.

**Performance Targets:**
- WASM plugin cold-start latency: <50 ms per plugin on warm JIT cache.
- Steady-state call overhead vs. native plugin: <3× (Wasmtime near-native tier).

---

### [x] Manifest Schema v2 with Capability Declarations
**Priority:** High
**Target Version:** v0.8.0

Extend the plugin manifest format to include fine-grained capability declarations (e.g., `network:egress`, `storage:read`, `rpc:register`). `plugin_registry.cpp` validates declared capabilities against the node's allowlist before loading.

**Implementation Notes:**
- Define JSON Schema v2 in `include/plugins/manifest_schema_v2.json`; update `plugin_registry.cpp` validation path.
- Add `CapabilityChecker` class to `plugin_system_edition.cpp` that maps capability strings to bitmask flags.
- Emit a structured audit event via `audit_logger.cpp` (utils module) on each capability grant or denial.

**Performance Targets:**
- Manifest validation time: <2 ms per plugin at registration.
- Zero runtime capability checks on the hot call path (capabilities frozen at load time).

---

### [x] Ed25519 Plugin Signing and Verification Pipeline
**Priority:** High
**Target Version:** v0.8.0

Implement end-to-end plugin signing using Ed25519 keys managed by the PKI client (`utils/pki_client.cpp`). The signing tool generates a detached `.sig` file; `plugin_registry.cpp` verifies the signature against the trusted key store before dlopen or WASM instantiation.

**Implementation Notes:**
- Add `plugin_signer.cpp` (offline signing tool) and `plugin_verifier.cpp` (runtime verification) under `src/plugins/`.
- Integrate with `utils/pki_client.cpp` for certificate chain validation and key rotation events.
- Store the plugin public-key fingerprint in the manifest; `plugin_registry.cpp` cross-checks fingerprint against the node's trusted-keys store.
- Hot-plug monitor (`plugin_hot_plug_monitor.cpp`) must re-verify signature on each reload event.

**Performance Targets:**
- Ed25519 signature verification: <1 ms per plugin on a modern CPU.
- Key rotation propagation to all nodes: <30 s via gossip (integrates with `sharding/gossip_protocol.cpp`).

---

### [x] Plugin Dependency Resolution and Ordered Loading
**Priority:** Medium
**Target Version:** v0.9.0

Support `dependencies` field in plugin manifests to declare inter-plugin dependencies. `plugin_manager.cpp` performs topological sort to determine load order and validates that required API versions are satisfied before loading a dependent plugin.

**Implementation Notes:**
- Implement `PluginDependencyGraph` in `plugin_manager.cpp` using Kahn's algorithm for cycle detection.
- Dependency resolution errors must surface as structured log entries via `utils/logger.cpp` with plugin name, required version, and found version.
- Circular dependencies must be detected and rejected with an explicit error code; no partial loading.

**Performance Targets:**
- Dependency resolution for 100-plugin graph: <10 ms.
- Memory overhead of dependency graph: <1 MB for 500 registered plugins.

---

### [x] Plugin Metrics Dashboard Integration
**Priority:** Medium
**Target Version:** v0.9.0

**Status:** Implemented. `PluginMetricsCollector::collect(IMetrics&)` publishes per-plugin counters, latency gauges, and memory metrics to any `IMetrics` sink (e.g. `PrometheusMetricsAdapter`). `PluginHealthMonitor::attachMetrics(IMetrics*)` emits `plugin_health_score` after every health check. The Grafana dashboard template is at `grafana/dashboards/plugins.json`.

**Implementation Notes:**
- `PluginMetricsCollector` (in `plugin_metrics.h` / `plugin_metrics.cpp`) reads `PluginMetrics::getAllStats()` and emits labelled gauges via `IMetrics::setGauge()`.
- `PluginHealthMonitor::attachMetrics(IMetrics*)` wires the monitor to a Prometheus sink; call once at startup with a `PrometheusMetricsAdapter`.
- `PluginHealthMonitor::computeHealthScore()` maps `PluginHealthStatus` + error rate to a 0–1 score; emitted as `plugin_health_score{plugin=…}`.
- Grafana dashboard (`grafana/dashboards/plugins.json`) provides panels for health score, call latency (avg/P95/P99), error count, memory, reload count, and load time.

**Performance Targets:**
- Metrics scrape overhead: <0.5 ms per registered plugin.
- Memory overhead of per-plugin histogram: <4 KB per plugin (use fixed-width HDR histogram buckets).

---

## Test Strategy

| Test Type | Coverage Target | Notes |
|-----------|----------------|-------|
| Unit | >85% new code | Cover `PluginDependencyGraph`, `CapabilityChecker`, manifest schema v2 validation paths |
| Integration | All lifecycle transitions | Load → start → hot-reload → stop → unload; native and WASM runtimes |
| Security | 100% signing paths | Tampered manifest, missing signature, expired key, capability escalation attempt |
| Performance | P99 < budgets above | Benchmark cold-start and hot-call latency under 50-concurrent-plugin load |

## Performance Targets

| Metric | Current | Target | Method |
|--------|---------|--------|--------|
| Plugin cold-start (native dlopen) | ~80 ms | <20 ms | Profile `plugin_manager.cpp` init path with perf |
| Plugin cold-start (WASM) | N/A | <50 ms | Wasmtime AOT cache benchmark |
| Hot-reload latency impact on query path | ~12 ms spike | <5 ms | Latency histogram diff during `plugin_hot_plug_monitor` reload |
| Signature verification time | N/A | <1 ms | OpenSSL EVP_DigestVerify microbenchmark |
| Memory per idle loaded plugin | ~2 MB | <512 KB | Heap profiler on minimal test plugin |

## Security / Reliability

- [x] All plugins must carry a valid Ed25519 signature verified before any code is executed; the system must fail closed on verification errors. — **Implemented:** `PluginManager::verifyPlugin()` (in `plugin_manager.cpp`) enforces signature in production (NDEBUG) mode; load is rejected on failure (`ERR_PLUGIN_INVALID_SIGNATURE`). `plugin_registry.cpp` is the registry entry point and delegates to the same pipeline.
- [~] Capability declarations in the manifest are the sole authorization mechanism; plugins must not be able to acquire capabilities at runtime beyond what was declared at registration. — **Partial:** capabilities are validated at load time via `PluginCapabilityNegotiator`; programmatic runtime escalation block is planned.
- [ ] WASM linear-memory isolation prevents a malicious plugin from reading host process memory; native plugins run in a separate subprocess with seccomp-bpf filter (Linux) or App Sandbox (macOS). — **Planned** (WASM sandbox not yet implemented).
- [?] Clarify policy for plugin author identity (org-level vs. individual key) before v0.8.0 GA.
- [x] Plugin hot-plug must hold a read lock during signature re-verification to prevent TOCTOU between verification and dlopen/wasm instantiation. — **Implemented:** `PluginManager::reloadPlugin()` Phase 2 verifies the signature (`verifyPlugin()`) before the atomic swap under mutex, so the old plugin continues running while verification proceeds.
