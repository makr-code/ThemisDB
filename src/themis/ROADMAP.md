<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# Themis Core Framework Module Roadmap

## Current Status
v1.6.x – Implementation directory is currently empty pending the v1.7.0 modularization effort. Core functionality (build info, edition management, license validation, module loading, wire protocol) currently lives in `src/core/`, `src/security/`, and `src/server/`.

## Completed ✅
- [x] Public header interfaces defined (`include/themis/`)
- [x] Build information API headers (`build_info.h`)
- [x] Edition management headers
- [x] License validation headers
- [x] Module loader headers
- [x] Wire protocol server headers

## In Progress 🚧
- [?] `build_info.cpp` – build metadata collection and formatting (Target: Q2 2026, v1.7.0)
- [?] `license_info.cpp` – embedded license validation and Ed25519 signature verification (Target: Q2 2026, v1.7.0)
- [?] `module_loader.cpp` – secure shared-library loading with hash/signature checks (Target: Q3 2026, v1.7.0)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [!] `wire_protocol_server.cpp` – move wire protocol implementation from `src/server/` (Issue: #2468)
- [I] `edition_manager.cpp` – Community / Enterprise / Cloud edition feature gating (Issue: #2469)
- [I] `getBuildConfiguration()` – aggregate build metadata at runtime (Issue: #2311)
- [I] `isModuleCompiledIn()` – runtime module availability check (Issue: #2470)
- [!] SHA-256 hash verification for loaded modules (Issue: #2471)

### Long-term (6-12 months)
- [I] Full modularization of monolithic build (split into loadable `.so` / `.dll` modules) (Issue: #2472)
- [!] Authenticode (Windows) and GPG (Linux) signature verification for modules (Issue: #2473)
- [I] Zone.Identifier / quarantine detection (Windows) (Issue: #2316)
- [I] Dynamic feature flag gating per edition at runtime (Issue: #2317)
- [I] Module dependency resolution and load-order management (Issue: #2474)

## Implementation Phases

### Phase 1: Public Header Interfaces (Status: Completed ✅)
- [x] Public header interfaces defined (`include/themis/`)
- [x] `build_info.h` – build information API headers
- [x] Edition management headers
- [x] License validation headers
- [x] Module loader headers
- [x] Wire protocol server headers

### Phase 2: Core Implementation Files (Status: In Progress 🚧)
- [?] `build_info.cpp` – build metadata collection and formatting (v1.7.0)
- [?] `license_info.cpp` – embedded license validation and Ed25519 signature verification (v1.7.0)
- [?] `module_loader.cpp` – secure shared-library loading with hash/signature checks (v1.7.0)

### Phase 3: Wire Protocol & Edition Manager (Status: Planned 📋)
- [ ] `wire_protocol_server.cpp` – move wire protocol implementation from `src/server/`
- [ ] `edition_manager.cpp` – Community / Enterprise / Cloud edition feature gating
- [ ] `getBuildConfiguration()` – aggregate build metadata at runtime
- [ ] `isModuleCompiledIn()` – runtime module availability check
- [ ] SHA-256 hash verification for loaded modules

### Phase 4: Full Modularization & Signature Verification (Status: Planned 📋)
- [ ] Full modularization of monolithic build (split into loadable `.so` / `.dll` modules)
- [ ] Authenticode (Windows) and GPG (Linux) signature verification for modules
- [ ] Zone.Identifier / quarantine detection (Windows)
- [ ] Dynamic feature flag gating per edition at runtime
- [ ] Module dependency resolution and load-order management

## Production Readiness Checklist
- [?] Unit tests coverage > 80%
- [?] Integration tests (module load, license validation, build info)
- [?] Performance benchmarks (module load time, license check overhead)
- [?] Security audit (signature verification, constant-time license comparison)
- [?] Documentation complete
- [?] API stability guaranteed

## Known Issues & Limitations
- The `src/themis/` directory is currently empty; the monolithic build distributes this logic elsewhere.
- Modularization is blocked on the v1.7.0 architectural refactor.
- Platform-specific module loading (Windows LoadLibrary, Linux dlopen) is planned but not yet implemented here.

## Breaking Changes
- No existing code in this directory; all APIs are new.
- Public header interfaces (`include/themis/`) are frozen for v1.x to prevent downstream breakage.
