> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Base Module

All notable changes to the Base module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]
- Unit test coverage > 80% (Issue #1573, Target Q2 2026)
- Integration tests for hot-reload and sandbox scenarios (Issue #1574, Target Q2 2026)
- Performance benchmarks for module load and hot-reload cycles (Issue #1575, Target Q2 2026)
- Concrete WasmRuntime integration (Wasmtime or WasmEdge) — infrastructure ready, backend registration required (Target Q3 2026)

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
