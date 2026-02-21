# Plugins Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
v1.x – Core plugin infrastructure implemented. Dynamic loading, manifest validation, and plugin signing are in place; ecosystem of first-party plugins is growing.

## Completed ✅
- [x] Dynamic plugin loader (shared library loading)
- [x] Plugin lifecycle management (load, initialize, unload)
- [x] Plugin API implementation and versioning
- [x] Plugin manifest validation
- [x] Plugin signing and signature verification
- [x] Secure plugin execution sandbox
- [x] Plugin signer tool (`tools/plugin_signer/`)

## In Progress 🚧
- [ ] Plugin hot-reload without server restart (Target: Q2 2026)
- [ ] Plugin dependency resolution (plugin A requires plugin B) (Target: Q2 2026)
- [ ] Plugin marketplace / registry integration (Target: Q3 2026)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [ ] Plugin configuration schema validation (JSON Schema)
- [ ] Per-plugin resource quotas (CPU time, memory)
- [ ] Plugin health monitoring and automatic restart on crash
- [ ] Plugin API versioning with compatibility matrix
- [ ] First-party importer plugins (MySQL, SQLite, MongoDB)

### Long-term (6-12 months)
- [ ] WebAssembly (WASM) plugin runtime for sandboxed execution
- [ ] Remote plugin loading from OCI registries
- [ ] Plugin capability permissions model (fine-grained access control)
- [ ] Plugin SDK (C++, Python, Go bindings)
- [ ] Community plugin repository with security scanning

## Implementation Phases

### Phase 1: Core Plugin Infrastructure (Status: Completed ✅)
- [x] Dynamic plugin loader – shared library (.so/.dll) loading via dlopen/LoadLibrary
- [x] Plugin lifecycle management (load, initialize, unload) with RAII guards
- [x] Plugin API versioning and compatibility negotiation
- [x] Plugin manifest validation (JSON Schema enforcement)
- [x] Ed25519 plugin signing and signature verification (`tools/plugin_signer/`)
- [x] Secure plugin execution sandbox (capability isolation)
- [x] Basic per-plugin resource accounting

### Phase 2: Signing Hardening & Permissions (Status: In Progress 🚧)
- [~] Ed25519 manifest signing workflow (`plugins/manifest_signer.cpp`) with key-rotation support (Target: Q2 2026)
- [~] Capability-based permission model (fine-grained access control per plugin) (Target: Q2 2026)
- [ ] Plugin dependency resolution (plugin A requires plugin B) (Target: Q2 2026)
- [ ] Plugin hot-reload without server restart (Target: Q3 2026)

### Phase 3: WASM Sandbox & Ecosystem (Status: Planned 📋)
- [ ] WebAssembly (WASM) plugin runtime via Wasmtime for sandbox isolation
- [ ] Plugin metrics dashboard (call latency, error rate per plugin)
- [ ] Remote plugin loading from OCI registries
- [ ] Plugin SDK with C++, Python, and Go bindings
- [ ] Community plugin repository with automated security scanning

## Production Readiness Checklist
- [ ] Unit tests coverage > 80%
- [ ] Integration tests (load, sign, verify, unload lifecycle)
- [ ] Performance benchmarks (plugin call overhead)
- [ ] Security audit (signature enforcement, sandbox escape prevention)
- [ ] Documentation complete
- [ ] API stability guaranteed

## Known Issues & Limitations
- Hot-reload is not yet supported; plugin updates require server restart.
- Plugin execution is in-process; a crash in a plugin can affect the server.
- WASM sandbox isolation is planned but not yet implemented.

## Breaking Changes
- Plugin API version 1.x is stable; v2.0 will add new hook points with backward compatibility.
- Manifest format may gain new required fields in v1.5.0.
