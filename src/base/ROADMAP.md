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

## In Progress 🚧
- [P] Hot-reload support for plugins without database restart (Target: Q2 2026) (Issue: #1554, PR: #2396)
- [I] Plugin dependency resolution and ordered loading (Target: Q2 2026) (Issue: #1566)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [I] Plugin marketplace manifest format (JSON schema) (Issue: #1556)
- [I] Runtime plugin capability negotiation (version ranges) (Issue: #1984)
- [!] Plugin sandboxing with resource limits (memory, CPU) (Issue: #2082)
- [I] Plugin health monitoring and automatic restart (Issue: #1570)
- [I] Signed plugin repository with key pinning (Issue: #1571)

### Long-term (6-12 months)
- [I] WASM-based plugin isolation for untrusted code (Issue: #1572)
- [I] Remote plugin loading from authenticated registry (Issue: #1562)
- [I] Plugin dependency graph visualization (Issue: #1563)
- [I] Per-plugin audit trail (load, unload, errors) (Issue: #1564)
- [I] A/B testing framework using module swapping (Issue: #1565)

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
- [P] Hot-reload support for plugins without database restart (`base/hot_reload_manager.cpp`, Target: Q2 2026) (Issue: #1554, PR: #2396)
- [ ] Plugin dependency resolution and ordered loading (Target: Q2 2026)

### Phase 3: Marketplace & Sandboxing (Status: Planned 📋)
- [ ] Plugin marketplace manifest format (JSON schema)
- [ ] Runtime plugin capability negotiation (version ranges)
- [ ] Plugin sandboxing with resource limits (memory, CPU)
- [ ] Plugin health monitoring and automatic restart
- [ ] Signed plugin repository with key pinning
- [ ] WASM-based plugin isolation for untrusted code

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1573)
- [I] Integration tests (Issue: #1574)
- [I] Performance benchmarks (Issue: #1575)
- [x] Security audit (signature verification, revocation checking)
- [x] Documentation complete
- [x] API stability guaranteed for module loading interface

## Known Issues & Limitations
- Hot-reload is supported via `HotReloadManager` (`include/themis/base/hot_reload_manager.h`); see Phase 2 above
- WASM plugin isolation is not yet implemented
- Remote plugin loading from a registry is not yet available
- Plugin dependency resolution is manual (loading order not enforced)

## Breaking Changes
- WASM plugin interface will be a new API surface (additive, non-breaking to existing plugin interface)
