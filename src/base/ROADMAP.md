> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# Base Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->
<!-- validated: 2026-04-06 | status: current | evidence: source-code audit commit 0091524 -->

## Current Status
Production-ready for module loading, signature verification, and plugin lifecycle management across Windows, Linux, and macOS.

## Completed ✅
- [x] Secure DLL/shared library loading (Windows DLL, Linux SO, macOS DYLIB) — Evidence: `src/base/module_loader.cpp` `dlopen`/`LoadLibrary` paths
- [x] Digital signature verification for loaded modules — Evidence: `ModuleSecurityVerifier` in `module_loader.cpp`
- [x] File integrity hash validation — Evidence: `ModuleHashVerifier::loadManifest()`, `src/base/module_loader.cpp` line 372 (SHA-256 manifest check Issue #2471)
- [x] Trust levels: TRUSTED, VERIFIED, UNTRUSTED — Evidence: `TrustLevel` enum in `include/themis/base/module_loader.h`
- [x] Revocation checking for certificates — Evidence: OCSP/CRL code paths in `module_loader.cpp`
- [x] Development mode to allow unsigned modules — Evidence: `ModuleSecurityPolicy::allowUnsigned` in `module_loader.h`
- [x] Plugin lifecycle management (initialize, execute, shutdown) — Evidence: lifecycle state machine in `module_loader.cpp`
- [x] Interface discovery to query plugin capabilities — Evidence: `getCapabilities()` in `module_loader.cpp`
- [x] Automatic resource cleanup on unload — Evidence: `unloadModule()` + RAII handles in `module_loader.cpp`
- [x] Cross-platform export/import macros — Evidence: `include/themis/base/export.h`
- [x] Version compatibility checking — Evidence: version fields in `ModuleCapabilities` struct
- [x] Plugin sandboxing with resource limits (memory, CPU) (Issue: #2372) — Evidence: `src/base/module_sandbox.cpp`, `include/themis/base/module_sandbox.h`
- [x] Plugin health monitoring and automatic restart (Issue: #2373) — Watchdog background thread (`ModuleLoader::startWatchdog/stopWatchdog`) performs periodic health checks on all loaded modules and automatically restarts failed plugins with configurable exponential backoff; `WatchdogConfig` and `WatchdogModuleStats` expose full per-module restart metrics; `PluginWatchdogFocusedTests` in `tests/CMakeLists.txt`
- [~] WASM-based plugin isolation for untrusted code (Issue: #1572) — **Partial**: full sandbox infrastructure implemented in `src/base/wasm_plugin_sandbox.cpp`; requires injection of a concrete `WasmRuntime` (e.g. Wasmtime, WasmEdge) before `callExport()` is operational; see Missing Implementations report
- [x] Hot-reload support for plugins without database restart (Issue: #1554, PR: #2396) — Evidence: `src/base/hot_reload_manager.cpp`
- [x] Plugin dependency graph visualization (Issue: #1563) — Evidence: `src/base/plugin_dependency_graph.cpp`, `topologicalOrder()` + DOT output
- [x] Per-plugin audit trail (load, unload, errors) (Issue: #1564) — Evidence: `auditTrail_` in `module_loader.cpp`
- [x] A/B testing framework using module swapping (Issue: #1565) — Evidence: `src/base/ab_test_manager.cpp`
- [x] Remote plugin loading from authenticated registry (`base/remote_registry_client.cpp`) — Evidence: TLS-verified download + SHA-256 integrity check in `remote_registry_client.cpp`
- [x] Plugin dependency resolution and ordered loading (Issue: #1566) — `ModuleDependencyResolver` fully implemented in `module_loader.cpp`: `registerModule`, `resolve`, `resolveFor`, `isVersionCompatible` (semver), topological sort (Kahn's algorithm), cycle detection, missing-dependency and version-mismatch reporting
- [x] TLS public-key pinning for remote plugin registry — `RegistryConfig::pinned_public_key` field added; `CURLOPT_PINNEDPUBLICKEY` applied in both `httpGet` and `httpGetBinary` code paths in `remote_registry_client.cpp`

## In Progress 🚧
*(No open work items — all Phase 1 and Phase 2 features are complete)*

## Planned Features 📋

### Short-term (Next 3-6 months)
- [x] Unit test coverage > 80% (Target: Q2 2026) (Issue: #1573) — `test_base_entity.cpp`, `test_base_interfaces.cpp`, `test_hot_reload_manager.cpp`, `test_module_sandbox.cpp`, `test_module_sandbox_wasm_injection.cpp` registered as focused CTest targets (2026-04-21)
- [x] Integration tests for hot-reload and sandbox scenarios (Target: Q2 2026) (Issue: #1574) — `HotReloadManagerFocusedTests`, `ModuleSandboxFocusedTests`, `ModuleSandboxWasmInjectionFocusedTests` registered in `tests/CMakeLists.txt` (2026-04-21)
- [ ] Performance benchmarks for module load and hot-reload cycles (Target: Q2 2026) (Issue: #1575) — `benchmarks/bench_module_load_hot_reload.cpp` exists; benchmark mapping entry pending
- [x] Automatic plugin restart after health-check failure (Issue: #2373) — implemented via `ModuleLoader` watchdog: `startWatchdog()`, `stopWatchdog()`, `configureWatchdog(WatchdogConfig)`, `getWatchdogStats()`, `getAllWatchdogStats()`, `resetWatchdogStats()`

### Long-term (6-12 months)
- [ ] Concrete WasmRuntime integration (Wasmtime or WasmEdge) (Target: Q3 2026) — `ModuleSandbox::Config::enable_wasm_isolation` and `WasmRuntimeInjector` injection path are ready (v1.8.0); register a concrete backend via `THEMIS_REGISTER_WASM_RUNTIME` macro
- [x] TLS public-key pinning for remote plugin registry — implemented via `RegistryConfig::pinned_public_key` + `CURLOPT_PINNEDPUBLICKEY`; Ed25519 application-layer key pinning is handled separately by `SignedPluginRepository` in the plugins module

## Implementation Phases

### Phase 1: Secure Plugin Foundation (Status: Completed ✅)
- [x] Secure DLL/SO/DYLIB loading across Windows, Linux, macOS (`base/module_loader.cpp`)
- [x] Digital signature verification for loaded modules
- [x] File integrity hash validation
- [x] Trust levels: TRUSTED, VERIFIED, UNTRUSTED
- [x] Revocation checking for certificates
- [x] Development mode to allow unsigned modules
- [x] Plugin lifecycle management: initialize, execute, shutdown (`base/module_loader.cpp`)
- [x] Interface discovery to query plugin capabilities
- [x] Automatic resource cleanup on unload
- [x] Cross-platform export/import macros and version compatibility checking

### Phase 2: Dynamic Loading & Dependency Management (Status: Completed ✅)
- [x] Hot-reload support for plugins without database restart (`base/hot_reload_manager.cpp`) (Issue: #1554, PR: #2396)
- [x] Per-plugin audit trail: load, unload, errors (`base/module_loader.cpp`) (Issue: #1564)
- [x] Plugin dependency graph visualization (`base/plugin_dependency_graph.cpp`) (Issue: #1563)
- [x] Remote plugin loading from authenticated registry (`base/remote_registry_client.cpp`)
- [x] A/B testing framework via module swapping (`base/ab_test_manager.cpp`) (Issue: #1565)
- [x] Plugin dependency resolution and ordered loading (Issue: #1566) — `ModuleDependencyResolver` in `module_loader.cpp`

### Phase 3: Marketplace & Sandboxing (Status: In Progress 🚧 — partially complete)
- [x] Plugin marketplace manifest format (JSON schema) — Evidence: `setHashManifest()` in `module_loader.cpp`
- [x] Runtime plugin capability negotiation (version ranges) — `ModuleDependencyResolver::isVersionCompatible()` + `topologicalSort()` enforce version constraints during load-order resolution; higher-level runtime negotiation via `PluginCapabilityNegotiator` in `plugins` module (Issue: #1984)
- [x] Plugin sandboxing with resource limits (memory, CPU) — Evidence: `module_sandbox.cpp`
- [x] Plugin health monitoring and automatic restart — Watchdog background thread (`startWatchdog/stopWatchdog`) with configurable exponential backoff, `WatchdogConfig`/`WatchdogModuleStats`, `PluginWatchdogFocusedTests` (Issue: #2373)
- [x] Signed plugin repository with key pinning — TLS SPKI pinning via `RegistryConfig::pinned_public_key` + `CURLOPT_PINNEDPUBLICKEY`; Ed25519 application-layer key pinning in `SignedPluginRepository` (`plugins/signed_plugin_repository.h`)
- [~] WASM-based plugin isolation for untrusted code — **Partial**: infrastructure complete; WASM runtime injection into `ModuleSandbox` implemented (v1.8.0, Issue: #1572); concrete backend (Wasmtime/WasmEdge) registration still required for production execution

### Phase 4: Fuel Metering & Observability (Status: Completed ✅)
- [x] WASM instruction fuel metering — `WasmPluginSandbox::Config::max_instructions` (total budget, 0 = unlimited) and `fuel_check_interval` (units deducted per `callExport()`) added; `fuel_remaining_` counter initialized at load and decremented per call; fuel-exhausted calls return structured error without invoking runtime; `remainingFuel()` exposes current budget; 8 unit tests in `tests/test_wasm_plugin_sandbox.cpp` (v1.8.0)

## Production Readiness Checklist
- [x] Unit tests coverage > 80% (Issue: #1573) — `test_base_entity.cpp` (383 LOC), `test_base_interfaces.cpp` (678 LOC), `test_hot_reload_manager.cpp` (509 LOC), `test_module_sandbox.cpp` (521 LOC), `test_module_sandbox_wasm_injection.cpp` (489 LOC); focused standalone targets: `BaseEntityFocusedTests`, `BaseInterfacesFocusedTests`, `HotReloadManagerFocusedTests`, `ModuleSandboxFocusedTests`, `ModuleSandboxWasmInjectionFocusedTests`
- [x] Integration tests (Issue: #1574) — hot-reload, sandbox, and WASM injection integration tests registered in `tests/CMakeLists.txt` (2026-04-21)
- [I] Performance benchmarks (Issue: #1575) — `benchmarks/bench_module_load_hot_reload.cpp` exists; benchmark mapping entry pending
- [x] Security audit (signature verification, revocation checking)
- [x] Documentation complete — validated 2026-03-09
- [x] API stability guaranteed for module loading interface

## Known Issues & Limitations
- WASM plugin isolation (`WasmPluginSandbox`) requires injection of a concrete WASM runtime (Wasmtime, WasmEdge, etc.) for full execution support (Issue: #1572)
- Automatic plugin restart after health-check failure is implemented via `ModuleLoader` watchdog thread (Issue: #2373)
- Performance benchmarks (`bench_module_load_hot_reload.cpp`) exist but benchmark mapping entry is pending (Issue: #1575)

## Breaking Changes
- WASM plugin interface will be a new API surface (additive, non-breaking to existing plugin interface)

## Latente Symbole (Unused-Functions-Audit)

_Stand: 2026-04-20 – Quelle: [`src/UNUSED_FUNCTIONS_REPORT.md`](../UNUSED_FUNCTIONS_REPORT.md)_

### 🟡 UNGENUTZT (kein Test, kein externer Aufrufer)

- `configToJson` – Serialisiert ABTestConfig in JSON für REST-API-Antworten
- `configFromJson` – Deserialisiert ABTestConfig aus JSON (HTTP-Body-Parsing)
  > **Aktion:** Für jedes Symbol entscheiden: (1) Verdrahten, (2) Testen oder (3) als CANDIDATE_FOR_REMOVAL einplanen.

