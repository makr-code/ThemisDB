# Base Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
Production-ready for module loading, signature verification, and plugin lifecycle management across Windows, Linux, and macOS.

## Completed ✅
- [x] Secure DLL/shared library loading (Windows DLL, Linux SO, macOS DYLIB)
- [x] Digital signature verification for loaded modules
- [x] File integrity hash validation
- [x] Trust levels: TRUSTED, VERIFIED, UNTRUSTED
- [x] Revocation checking for certificates
- [x] Development mode to allow unsigned modules
- [x] Plugin lifecycle management (initialize, execute, shutdown)
- [x] Interface discovery to query plugin capabilities
- [x] Automatic resource cleanup on unload
- [x] Cross-platform export/import macros
- [x] Version compatibility checking
- [x] Plugin sandboxing with resource limits (memory, CPU) (Issue: #2372)
- [x] Plugin health monitoring and automatic restart (Issue: #2373)
- [x] WASM-based plugin isolation for untrusted code (Issue: #1572)
- [x] Hot-reload support for plugins without database restart (Issue: #1554, PR: #2396)
- [x] Plugin dependency graph visualization (Issue: #1563)
- [x] Per-plugin audit trail (load, unload, errors) (Issue: #1564)
- [x] A/B testing framework using module swapping (Issue: #1565)
- [x] Remote plugin loading from authenticated registry (`base/remote_registry_client.cpp`)

## In Progress 🚧
- [I] Plugin dependency resolution and ordered loading (Target: Q2 2026) (Issue: #1566)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [ ] Unit test coverage > 80% (Target: Q2 2026) (Issue: #1573)
- [ ] Integration tests for hot-reload and sandbox scenarios (Target: Q2 2026) (Issue: #1574)
- [ ] Performance benchmarks for module load and hot-reload cycles (Target: Q2 2026) (Issue: #1575)

### Long-term (6-12 months)

## Implementation Phases

### Phase 1: Secure Plugin Foundation (Status: Completed ✅)
- [x] Secure DLL/SO/DYLIB loading across Windows, Linux, macOS (`base/module_loader.cpp`)
- [x] Digital signature verification for loaded modules
- [x] File integrity hash validation
- [x] Trust levels: TRUSTED, VERIFIED, UNTRUSTED
- [x] Revocation checking for certificates
- [x] Development mode to allow unsigned modules
- [x] Plugin lifecycle management: initialize, execute, shutdown (`base/plugin_lifecycle.cpp`)
- [x] Interface discovery to query plugin capabilities
- [x] Automatic resource cleanup on unload
- [x] Cross-platform export/import macros and version compatibility checking

### Phase 2: Dynamic Loading & Dependency Management (Status: In Progress 🚧)
- [x] Hot-reload support for plugins without database restart (`base/hot_reload_manager.cpp`) (Issue: #1554, PR: #2396)
- [x] Per-plugin audit trail: load, unload, errors (`base/module_loader.cpp`) (Issue: #1564)
- [x] Plugin dependency graph visualization (`base/plugin_dependency_graph.cpp`) (Issue: #1563)
- [x] Remote plugin loading from authenticated registry (`base/remote_registry_client.cpp`)
- [x] A/B testing framework via module swapping (`base/ab_test_manager.cpp`) (Issue: #1565)
- [I] Plugin dependency resolution and ordered loading (Target: Q2 2026) (Issue: #1566)

### Phase 3: Marketplace & Sandboxing (Status: Completed ✅)
- [x] Plugin marketplace manifest format (JSON schema)
- [x] Runtime plugin capability negotiation (version ranges)
- [x] Plugin sandboxing with resource limits (memory, CPU)
- [x] Plugin health monitoring and automatic restart
- [x] Signed plugin repository with key pinning
- [x] WASM-based plugin isolation for untrusted code

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1573)
- [I] Integration tests (Issue: #1574)
- [I] Performance benchmarks (Issue: #1575)
- [x] Security audit (signature verification, revocation checking)
- [x] Documentation complete
- [x] API stability guaranteed for module loading interface

## Known Issues & Limitations
- Plugin dependency resolution is not yet enforced: loading order for plugins with declared dependencies must currently be managed manually (Issue: #1566)
- WASM plugin isolation (`WasmPluginSandbox`) requires injection of a concrete WASM runtime (Wasmtime, WasmEdge, etc.) for full execution support
- Unit test coverage, integration tests, and performance benchmarks are still open (Issues: #1573, #1574, #1575)

## Breaking Changes
- WASM plugin interface will be a new API surface (additive, non-breaking to existing plugin interface)
