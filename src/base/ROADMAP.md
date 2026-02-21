# Base Module Roadmap

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
- [ ] Hot-reload support for plugins without database restart (Target: Q2 2026)
- [ ] Plugin dependency resolution and ordered loading (Target: Q2 2026)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [ ] Plugin marketplace manifest format (JSON schema)
- [ ] Runtime plugin capability negotiation (version ranges)
- [ ] Plugin sandboxing with resource limits (memory, CPU)
- [ ] Plugin health monitoring and automatic restart
- [ ] Signed plugin repository with key pinning

### Long-term (6-12 months)
- [ ] WASM-based plugin isolation for untrusted code
- [ ] Remote plugin loading from authenticated registry
- [ ] Plugin dependency graph visualization
- [ ] Per-plugin audit trail (load, unload, errors)
- [ ] A/B testing framework using module swapping

## Production Readiness Checklist
- [ ] Unit tests coverage > 80%
- [ ] Integration tests
- [ ] Performance benchmarks
- [x] Security audit (signature verification, revocation checking)
- [x] Documentation complete
- [x] API stability guaranteed for module loading interface

## Known Issues & Limitations
- Hot-reload is not yet supported; module changes require a restart
- WASM plugin isolation is not yet implemented
- Remote plugin loading from a registry is not yet available
- Plugin dependency resolution is manual (loading order not enforced)

## Breaking Changes
- WASM plugin interface will be a new API surface (additive, non-breaking to existing plugin interface)
