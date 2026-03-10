<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# Themis Core Framework Module Roadmap

## Current Status
v1.7.0 (in progress) – Build info and license validation live in `src/utils/`; module loading in `src/base/`. The `src/themis/` directory now contains `wire_protocol_server.cpp` (Phase 3 deliverable: `themis::wire` namespace, header at `include/themis/network/wire_protocol_server.hpp`), `module_dependency_resolver.cpp`, and `edition_manager.cpp` (Issue: #2469, dynamic feature-flag override API included). The `src/network/wire_protocol_server.cpp` (`themis::network` namespace) is a separate, higher-level server implementation that is retained for backward compatibility; both coexist in the monolithic build during the v1.7.0 migration window.

## Completed ✅
- [x] Public header interfaces defined (`include/themis/`)
- [x] Build information API headers (`build_info.h`)
- [x] Edition management headers
- [x] License validation headers
- [x] Module loader headers
- [x] Wire protocol server headers
- [x] `getBuildConfiguration()` – aggregate build metadata at runtime (Issue: #2311)
- [x] `isModuleCompiledIn()` – runtime module availability check (Issue: #2470)
- [x] SHA-256 hash verification for loaded modules (Issue: #2471)
- [x] Module dependency resolution and load-order management (Issue: #2474)
- [x] `edition_manager.cpp` – Community / Enterprise / Cloud edition feature gating with dynamic override API (Issue: #2469)

## In Progress 🚧
- [~] `license_info.cpp` – implemented in `src/utils/`; pending migration to `src/themis/` (Target: Q2 2026, v1.7.0)
- [~] `module_loader.cpp` – implemented in `src/base/`; pending migration to `src/themis/` (Target: Q3 2026, v1.7.0)

## Planned Features 📋

### Short-term (Next 3-6 months)
See "In Progress" section above for `license_info.cpp` and `module_loader.cpp` migration status.

### Long-term (6-12 months)
- [I] Full modularization of monolithic build (split into loadable `.so` / `.dll` modules) (Issue: #2472)
- [I] Dynamic feature flag gating per edition at runtime (Issue: #2317)

## Implementation Phases

### Phase 1: Public Header Interfaces (Status: Completed ✅)
- [x] Public header interfaces defined (`include/themis/`)
- [x] `build_info.h` – build information API headers
- [x] Edition management headers
- [x] License validation headers
- [x] Module loader headers
- [x] Wire protocol server headers

### Phase 2: Core Implementation Files (Status: Implemented in monolithic location ✅)
- [x] `build_info.cpp` – build metadata collection and formatting (`src/utils/build_info.cpp`)
- [x] `license_info.cpp` – embedded license validation and Ed25519 signature verification (`src/utils/license_info.cpp`)
- [x] `module_loader.cpp` – secure shared-library loading with hash/signature checks (`src/base/module_loader.cpp`)

### Phase 3: Wire Protocol & Edition Manager (Status: Completed ✅)
- [x] `wire_protocol_server.cpp` – move wire protocol implementation from `src/server/` (`src/themis/wire_protocol_server.cpp`, namespace `themis::wire`)
- [x] `edition_manager.cpp` – Community / Enterprise / Cloud edition feature gating (Issue: #2469)
- [x] `getBuildConfiguration()` – aggregate build metadata at runtime
- [x] `isModuleCompiledIn()` – runtime module availability check
- [x] SHA-256 hash verification for loaded modules

### Phase 4: Full Modularization & Signature Verification (Status: Planned 📋)
- [ ] Full modularization of monolithic build (split into loadable `.so` / `.dll` modules)
- [x] Authenticode (Windows) and GPG (Linux) signature verification for modules
- [x] Zone.Identifier / quarantine detection (Windows)
- [ ] Dynamic feature flag gating per edition at runtime
- [x] Module dependency resolution and load-order management

## Production Readiness Checklist
- [x] Unit tests coverage > 80% (tests/test_themis_wire_protocol_server.cpp; CTest: ThemisWireProtocolV1Tests)
- [x] Unit tests for edition_manager (tests/test_edition_manager.cpp; CTest: EditionManagerTests)
- [x] Unit tests for dynamic feature-flag overrides (tests/test_dynamic_feature_flags.cpp; CTest: DynamicFeatureFlagTests)
- [x] Unit tests for runtime license gate (tests/test_runtime_license_gate.cpp; CTest: RuntimeLicenseGateTests)
- [x] Unit tests for module hash verifier (tests/test_module_hash_verifier.cpp; CTest: ModuleHashVerifierFocusedTests)
- [x] Unit tests for module signature verifier (tests/test_module_signature_verifier.cpp; CTest: ModuleSignatureVerifierFocusedTests)
- [x] Build system: edition_manager.cpp, module_hash_verifier.cpp, module_signature_verifier.cpp registered in cmake/CMakeLists.txt and cmake/ModularBuild.cmake
- [?] Integration tests (module load, license validation, build info)
- [?] Performance benchmarks (module load time, license check overhead)
- [?] Security audit (signature verification, constant-time license comparison)
- [x] Documentation complete (ARCHITECTURE.md, README.md, ROADMAP.md, FUTURE_ENHANCEMENTS.md, Known Issues section)
- [x] API stability guaranteed (public header include/themis/network/wire_protocol_server.hpp frozen for v1.x)

## Known Issues & Limitations
- The `src/themis/` directory contains `wire_protocol_server.cpp` (Phase 3 complete), `module_dependency_resolver.cpp`, `edition_manager.cpp`, and the SHA-256 module hash verifier. `license_info.cpp` and `module_loader.cpp` remain in `src/utils/` and `src/base/` respectively, pending migration (see Planned Features).
- `WireProtocolServer::sessions_` is never pruned when a session disconnects; the
  map grows monotonically and `active_sessions()` never decreases. Fixing this
  requires adding a disconnect-callback member to `WireProtocolSession`, which
  would change the frozen v1.x public header ABI.
- `WireProtocolServer` members (`sessions_`, `running_`, `total_connections_`) are
  not protected by a mutex. Thread-safety depends on the caller using a
  single-threaded `io_context` or providing external synchronisation. Fixing this
  requires adding a `std::mutex` member to the frozen header.
- `WireProtocolSession::write_buffer_` is shared across `send_error`, `send_ok`, and
  `async_write_response`. Concurrent calls from multiple threads are unsafe; within
  a single-threaded `io_context` event loop the design is correct.
- LZ4 compress/decompress stubs return empty vectors; full implementation deferred
  until the LZ4 dependency is unconditionally available across all build targets.
- `OpCode::PING` and `OpCode::PONG` share the same wire value (`0xFE`) in the
  frozen header; this is a pre-existing design decision.
- Modularization is blocked on the v1.7.0 architectural refactor.
- Platform-specific module loading (Windows LoadLibrary, Linux dlopen) is planned
  but not yet implemented here.

## Breaking Changes
- `module_dependency_resolver.cpp` is already present; all remaining APIs are new.
- Public header interfaces (`include/themis/`) are frozen for v1.x to prevent downstream breakage.
