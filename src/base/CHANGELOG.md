<!-- Status: current | validated: 2026-04-08 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Base Module

All notable changes to the Base module are documented here.  
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]
- Concrete WasmRuntime integration (Wasmtime or WasmEdge) — infrastructure ready, backend registration required (Target Q3 2026)

## [1.9.0] — 2026-04-08
### Added
- **Unit test coverage > 80% confirmed** (Issue #1573): 361+ tests across 13 test files — `test_module_loader.cpp` (162), `test_wasm_plugin_sandbox.cpp` (56), `test_base_interfaces.cpp` (44), `test_hot_reload_manager.cpp` (40), `test_module_sandbox.cpp` (33), `test_base_entity.cpp` (26), `test_ab_test_manager.cpp`, `test_ab_testing_framework.cpp`, `test_remote_registry_client.cpp`, `test_plugin_dependency_graph.cpp`, `test_plugin_dependency_resolver.cpp`, `test_module_hash_verifier.cpp`, `test_module_signature_verifier.cpp`, `test_module_dependency_resolver.cpp`; focused targets: `BaseEntityFocusedTests`, `BaseInterfacesFocusedTests`, `ModuleLoaderFocusedTests`, `PluginWatchdogFocusedTests`, `RemoteRegistryClientUnifiedTests`, `PluginDependencyGraphFocusedTests`, `PluginDependencyResolverFocusedTests`
- **Integration tests for hot-reload and sandbox scenarios** (Issue #1574): `tests/integration/hot_reload_manager_integration_test.cpp` (11 tests covering full reload lifecycle with ModuleLoader + callback system); `CgroupV2MemoryLimitEnforcement` fork-based OOM enforcement test (8 MiB cgroup limit → 32 MiB mmap → SIGKILL within 500 ms); `ConcurrentReadersWithReloadThread` TSAN-compatible stress test (16 reader threads + 1 reload writer, TSAN-detectable under `-DTHEMIS_ENABLE_TSAN=ON`) — all in `tests/test_module_sandbox.cpp` and `tests/test_hot_reload_manager.cpp`
- **Performance benchmarks for module load and hot-reload cycles** (Issue #1575): `benchmarks/bench_hot_reload_manager.cpp` (`RegisterUnregister`, `RegisteredModulesList`, `ReloadAttemptThroughput`, `CallbackDispatch`, `ConcurrentReloadContention`); `benchmarks/bench_plugin_hot_plug.cpp`; `benchmarks/bench_plugin_system.cpp` — all registered in `benchmarks/CMakeLists.txt`

### Changed
- ROADMAP.md: marked Issues #1573, #1574, #1575 as `[x]` complete in Planned Features and Production Readiness Checklist
- FUTURE_ENHANCEMENTS.md: marked sandbox cgroup v2 memory constraint as `[x]`; marked integration test and TSAN test items as `[x]`

## [1.8.0] — 2026-03-22
### Added
- WASM instruction fuel metering: `WasmPluginSandbox::Config::max_instructions` (total budget, 0 = unlimited) and `Config::fuel_check_interval` (units deducted per `callExport()`, default 1); `fuel_remaining_` counter is initialized at `loadFromBytes()` / `loadFromFile()` and decremented on each `callExport()` invocation; when the budget reaches zero, `callExport()` returns a structured "fuel exhausted" error without invoking the runtime, safely bounding runaway plugin execution
- `WasmPluginSandbox::remainingFuel()` — observability accessor returning the current fuel counter (`UINT64_MAX` when `max_instructions == 0`)
- 8 new unit tests for fuel metering in `tests/test_wasm_plugin_sandbox.cpp`: unlimited fuel by default, fuel initialised from config, fuel deducted per call, exhausted fuel returns structured error, fuel counted as trap in stats, infinite-loop bounded by budget, reload resets fuel, large interval clamped to zero

## [1.7.0] — 2026-03-09
### Added
- Plugin health monitoring watchdog with automatic restart: `ModuleLoader::startWatchdog()`/`stopWatchdog()`, `configureWatchdog(WatchdogConfig)`, exponential backoff, per-module restart metrics via `WatchdogModuleStats` (Issue #2373)
- `PluginWatchdogFocusedTests` standalone test target in `tests/CMakeLists.txt`
- TLS public-key pinning for remote plugin registry: `RegistryConfig::pinned_public_key` + `CURLOPT_PINNEDPUBLICKEY` in `remote_registry_client.cpp`
- WASM-based plugin isolation sandbox infrastructure (`src/base/wasm_plugin_sandbox.cpp`, `src/base/wasm_runtime_injector.cpp`): `THEMIS_REGISTER_WASM_RUNTIME` macro for concrete backend injection (Issue #1572)

## [1.6.0] — 2026-02-10
### Added
- Plugin sandboxing with memory and CPU resource limits (`src/base/module_sandbox.cpp`) (Issue #2372)
- Remote plugin loading from authenticated registry with TLS-verified download + SHA-256 integrity check (`base/remote_registry_client.cpp`)
- `ModuleDependencyResolver` with semver compatibility, topological sort (Kahn's algorithm), cycle detection, and version-mismatch reporting (Issue #1566)
- Hot-reload support for plugins without database restart (`src/base/hot_reload_manager.cpp`) (Issue #1554, PR #2396)
- Plugin dependency graph visualization: `topologicalOrder()` and DOT output (`src/base/plugin_dependency_graph.cpp`) (Issue #1563)
- Per-plugin audit trail for load, unload, and error events (`auditTrail_` in `module_loader.cpp`) (Issue #1564)
- A/B testing framework using module swapping (`src/base/ab_test_manager.cpp`) (Issue #1565)

### Changed
- `ModuleLoader` watchdog replaces manual health polling with configurable background thread

## [1.0.0] — 2024-01-01
### Added
- Secure DLL/SO/DYLIB loading across Windows, Linux, and macOS (`src/base/module_loader.cpp`)
- Digital signature verification for loaded modules (`ModuleSecurityVerifier`)
- File integrity hash validation with SHA-256 manifest check (`ModuleHashVerifier`)
- Trust levels: TRUSTED, VERIFIED, UNTRUSTED
- OCSP/CRL revocation checking for module certificates
- Development mode for unsigned modules (`ModuleSecurityPolicy::allowUnsigned`)
- Plugin lifecycle management: initialize, execute, shutdown state machine
- Interface discovery to query plugin capabilities (`getCapabilities()`)
- Automatic resource cleanup on unload (RAII handles)
- Cross-platform export/import macros (`include/themis/base/export.h`)
- Version compatibility checking via `ModuleCapabilities` struct
