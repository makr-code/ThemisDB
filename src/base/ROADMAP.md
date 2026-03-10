# Base Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->
<!-- validated: 2026-03-09 | status: current | evidence: source-code audit commit 0091524 -->

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
- [~] Plugin health monitoring and automatic restart (Issue: #2373) — **Partial**: health checks on module load implemented (`module_loader.cpp` lines 452–1178, `healthChecks_` map); **automatic restart** on failure is not yet implemented; see Missing Implementations report
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
- [ ] Unit test coverage > 80% (Target: Q2 2026) (Issue: #1573)
- [ ] Integration tests for hot-reload and sandbox scenarios (Target: Q2 2026) (Issue: #1574)
- [ ] Performance benchmarks for module load and hot-reload cycles (Target: Q2 2026) (Issue: #1575)
- [ ] Automatic plugin restart after health-check failure (Target: Q2 2026) (Issue: #2373) — prerequisite: health monitoring partially implemented; restart loop not implemented

### Long-term (6-12 months)
- [ ] Concrete WasmRuntime integration (Wasmtime or WasmEdge) (Target: Q3 2026) — prerequisite: WasmPluginSandbox infrastructure complete; runtime injection required
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
- [~] Plugin health monitoring and automatic restart — **Partial**: health checks implemented; automatic restart not yet implemented (Issue: #2373)
- [x] Signed plugin repository with key pinning — TLS SPKI pinning via `RegistryConfig::pinned_public_key` + `CURLOPT_PINNEDPUBLICKEY`; Ed25519 application-layer key pinning in `SignedPluginRepository` (`plugins/signed_plugin_repository.h`)
- [~] WASM-based plugin isolation for untrusted code — **Partial**: infrastructure complete; requires WasmRuntime injection (Issue: #1572)

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1573)
- [I] Integration tests (Issue: #1574)
- [I] Performance benchmarks (Issue: #1575)
- [x] Security audit (signature verification, revocation checking)
- [x] Documentation complete — validated 2026-03-09
- [x] API stability guaranteed for module loading interface

## Known Issues & Limitations
- WASM plugin isolation (`WasmPluginSandbox`) requires injection of a concrete WASM runtime (Wasmtime, WasmEdge, etc.) for full execution support (Issue: #1572)
- Automatic plugin restart after health-check failure is not implemented; health checks run at module-load time only (Issue: #2373)
- Unit test coverage, integration tests, and performance benchmarks are still open (Issues: #1573, #1574, #1575)

## Breaking Changes
- WASM plugin interface will be a new API surface (additive, non-breaking to existing plugin interface)
