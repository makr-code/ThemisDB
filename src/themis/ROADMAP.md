<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# Themis Core Framework Module Roadmap

## Current Status
v1.8.0 – Wire Protocol V2 delivered (Phase 5): `V2Server` + `V2Session` with HTTP/2-style multiplexing, server push, flow control, priority/dependency management (RFC 7540 §6.3/§5.3.1 compliant), and LZ4/Zstd compression implemented in `src/network/wire_protocol_v2.cpp`; public header at `include/themis/network/wire_protocol_v2.hpp`; 52 focused tests (WireProtocolV2FocusedTests); CI: `.github/workflows/wire-protocol-v2-ci.yml`. Previously (v1.7.0): module loading migrated to `src/themis/`; the directory now contains eleven implementation files: `build_info.cpp`, `wire_protocol_server.cpp` (Phase 3 deliverable: `themis::wire` namespace), `module_dependency_resolver.cpp`, `edition_manager.cpp` (Issue: #2469), `module_hash_verifier.cpp` (Issue: #2471), `module_signature_verifier.cpp` (Issue: #2473), `module_loader.cpp` (platform-independent core), `module_loader_win32.cpp` (Windows: Zone.Identifier, Authenticode), `module_loader_linux.cpp` (Linux: GPG, xattr, ELF metadata), `module_security.cpp` (ModuleSecurityVerifier), and `license_info.cpp`. The `src/network/wire_protocol_server.cpp` (`themis::network` namespace) is retained for backward compatibility.

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
- [x] `build_info.cpp` – migrated to `src/themis/` (zero stubs; replaces `src/utils/build_info.cpp` in both monolithic and modular builds)
- [x] `license_info.cpp` – migrated to `src/themis/` (zero stubs; replaces former utils-location implementation in both monolithic and modular builds)
- [x] `module_hash_verifier.cpp` – SHA-256 manifest verification registered in cmake/CMakeLists.txt and cmake/ModularBuild.cmake (Issue: #2471)
- [x] `module_signature_verifier.cpp` – Authenticode/GPG signature verification registered in cmake/CMakeLists.txt and cmake/ModularBuild.cmake (Issue: #2473)
- [x] `module_dependency_resolver.cpp` – focused CTest target added (ModuleDependencyResolverFocusedTests) covering topological sort, version compat, cycle detection (Issue: #2474)
- [x] Build Reproducibility: `getReproducibilityInfo()`, `exportBuildManifest()`, `verifyBuildManifest()` implemented in `src/themis/build_info.cpp`; CMake captures git HEAD, branch, dirty flag, build host/user at configure time (v1.7.0)
- [x] Wire Protocol V2: `V2Server` + `V2Session` with HTTP/2-style multiplexing, server push, flow control, priority/dependency management (RFC 7540 §6.3/§5.3.1 compliant: stream_id=0 → GOAWAY, self-dependency → RST_STREAM), LZ4/Zstd compression; public header `include/themis/network/wire_protocol_v2.hpp`; 52 focused tests (WireProtocolV2FocusedTests); CI: `.github/workflows/wire-protocol-v2-ci.yml` (v1.8.0)

## In Progress 🚧
- [x] `license_info.cpp` – migrated to `src/themis/` (zero stubs; replaces former utils-location implementation in both monolithic and modular builds) (Target: Q2 2026, v1.7.0)
- [x] `module_loader.cpp` – migrated to `src/themis/` with platform-specific split (module_loader_win32.cpp, module_loader_linux.cpp, module_security.cpp) (Issue: #module-loader-implementation)

## Planned Features 📋

### Short-term (Next 3-6 months)
All migration work is complete (v1.7.0): `license_info.cpp` and `module_loader.cpp` are in `src/themis/`.

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

### Phase 2: Core Implementation Files (Status: Implemented ✅)
- [x] `build_info.cpp` – build metadata collection and formatting (`src/themis/build_info.cpp`; migrated from `src/utils/`)
- [x] `license_info.cpp` – embedded license validation and Ed25519 signature verification (`src/themis/license_info.cpp`)
- [x] `module_loader.cpp` – secure shared-library loading with hash/signature checks (`src/themis/module_loader.cpp`, `src/themis/module_loader_win32.cpp`, `src/themis/module_loader_linux.cpp`, `src/themis/module_security.cpp`)

### Phase 3: Wire Protocol & Edition Manager (Status: Completed ✅)
- [x] `wire_protocol_server.cpp` – move wire protocol implementation from `src/server/` (`src/themis/wire_protocol_server.cpp`, namespace `themis::wire`)
- [x] `edition_manager.cpp` – Community / Enterprise / Cloud edition feature gating (Issue: #2469)
- [x] `module_hash_verifier.cpp` – SHA-256 module integrity verification registered in build system (Issue: #2471)
- [x] `module_signature_verifier.cpp` – Authenticode/GPG signature verification registered in build system (Issue: #2473)
- [x] `getBuildConfiguration()` – aggregate build metadata at runtime
- [x] `isModuleCompiledIn()` – runtime module availability check
- [x] SHA-256 hash verification for loaded modules

### Phase 4: Full Modularization & Signature Verification (Status: Planned 📋)
- [ ] Full modularization of monolithic build (split into loadable `.so` / `.dll` modules)
- [x] Authenticode (Windows) and GPG (Linux) signature verification for modules
- [x] Zone.Identifier / quarantine detection (Windows)
- [ ] Dynamic feature flag gating per edition at runtime
- [x] Module dependency resolution and load-order management

### Phase 5: Wire Protocol V2 (Status: Completed ✅)
- [x] `include/themis/network/wire_protocol_v2.hpp` – public header with full V2 protocol types, frame layout, stream state machine, and `V2Server`/`V2Session` API
- [x] `src/network/wire_protocol_v2.cpp` – `V2Server` + `V2SessionImpl` (PIMPL) with HTTP/2-style multiplexing, server push (`PUSH_PROMISE`), flow control (`WINDOW_UPDATE`), priority/dependency management (`PRIORITY` frame handler + `set_stream_priority()`, RFC 7540 §6.3/§5.3.1 guards), LZ4/Zstd compression
- [x] `tests/test_wire_protocol_v2.cpp` – 47 unit tests (WireProtocolV2FocusedTests) covering all acceptance criteria
- [x] `.github/workflows/wire-protocol-v2-ci.yml` – CI workflow (ubuntu-22.04 + ubuntu-24.04, GCC-12 + GCC-14)

## Production Readiness Checklist
- [x] Unit tests coverage > 80% (tests/test_themis_wire_protocol_server.cpp; CTest: ThemisWireProtocolV1Tests)
- [x] Unit tests for edition_manager (tests/test_edition_manager.cpp; CTest: EditionManagerTests)
- [x] Unit tests for dynamic feature-flag overrides (tests/test_dynamic_feature_flags.cpp; CTest: DynamicFeatureFlagTests)
- [x] Unit tests for runtime license gate (tests/test_runtime_license_gate.cpp; CTest: RuntimeLicenseGateTests)
- [x] Unit tests for module hash verifier (tests/test_module_hash_verifier.cpp; CTest: ModuleHashVerifierFocusedTests)
- [x] Unit tests for module signature verifier (tests/test_module_signature_verifier.cpp; CTest: ModuleSignatureVerifierFocusedTests)
- [x] Unit tests for module dependency resolver (tests/test_module_dependency_resolver.cpp; CTest: ModuleDependencyResolverFocusedTests)
- [x] `module_loader.cpp` / `module_loader_win32.cpp` / `module_loader_linux.cpp` / `module_security.cpp` – migrated to `src/themis/`, registered in cmake/MiscellaneousFeatures.cmake and cmake/ModularBuild.cmake; CTest target ModuleLoaderFocusedTests added
- [x] Unit tests for module loader core (tests/test_module_loader.cpp; CTest: ModuleLoaderFocusedTests)
- [x] Unit tests for build info / reproducibility (tests/test_build_info.cpp; CTest: BuildInfoTests) – covers getBuildConfiguration(), getReproducibilityInfo(), exportBuildManifest(), verifyBuildManifest()
- [x] Unit tests for Wire Protocol V2 (tests/test_wire_protocol_v2.cpp; CTest: WireProtocolV2FocusedTests) – 52 tests covering multiplexing, server push, flow control, priority/dependency management, RFC 7540 compliance guards, compression flags, state machine; CI: wire-protocol-v2-ci.yml
- [x] Integration tests (license lifecycle × edition gate, module hash verifier + build-info cross-component, dynamic override lifecycle, full gate stack; tests/test_themis_integration.cpp; CTest: ThemisIntegrationTests)
- [?] Performance benchmarks (module load time, license check overhead)
- [?] Security audit (signature verification, constant-time license comparison)
- [x] Documentation complete (ARCHITECTURE.md, README.md, ROADMAP.md, FUTURE_ENHANCEMENTS.md, Known Issues section)
- [x] API stability guaranteed (public header include/themis/network/wire_protocol_server.hpp frozen for v1.x)

## Known Issues & Limitations
- The `src/themis/` directory now contains all eleven implementation files: `build_info.cpp`, `wire_protocol_server.cpp`, `module_dependency_resolver.cpp`, `edition_manager.cpp`, `module_hash_verifier.cpp`, `module_signature_verifier.cpp`, `module_loader.cpp`, `module_loader_win32.cpp`, `module_loader_linux.cpp`, `module_security.cpp`, and `license_info.cpp`.
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
- LZ4 compress/decompress are fully implemented in the network module (v1.7.0+). The earlier note about empty-vector stubs is no longer applicable; see `src/network/ROADMAP.md` for network-layer compression status.
- `OpCode::PING` and `OpCode::PONG` share the same wire value (`0xFE`) in the
  frozen header; this is a pre-existing design decision.
- Modularization is blocked on the v1.7.0 architectural refactor.
- Platform-specific module loading (Windows `LoadLibrary`/`FreeLibrary`/`GetProcAddress` and Linux `dlopen`/`dlclose`/`dlsym`) is implemented in `src/themis/module_loader.cpp`, `module_loader_win32.cpp`, and `module_loader_linux.cpp`.

## Breaking Changes
- `module_dependency_resolver.cpp` is already present; all remaining APIs are new.
- Public header interfaces (`include/themis/`) are frozen for v1.x to prevent downstream breakage.
