---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-11-19"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
---

> Exact model backend not verifiable from final document text alone.

# Core Module - Build and Test Evidence

<!-- Historical validation annotation: 2026-07-28; current source registration is summarized below. -->
<!-- Issue: #5638 (Development Status 2026-07-18) -->
<!-- Links: ROADMAP.md · FUTURE_ENHANCEMENTS.md · ARCHITECTURE.md -->

## Evidence Summary

This document separates source/build-registration evidence from executable results. Historical snapshots below remain historical; only the dated validation section records the current runner outcome.

## Canonical Snapshot (from issue context, validated 2026-07-18)

- Preset: `windows-release`
- Focused target pattern: `module_core_test_*_focused.exe`
- Result: focused module binary was not found in `build-msvc-windows-release/bin`
- Status: evidence gap documented in issue #5638

## Focused Test Registration Evidence (source-verifiable, validated 2026-07-28)

- Test registration file: `tests/core/CMakeLists.txt`
- Focused target naming rule:
  - `module_core_${_stem}_focused`
- Current focused test source present:
  - `tests/core/test_core_smoke.cpp`
- Expected focused target from current source:
  - `module_core_test_core_smoke_focused`

## Historical Local Build/Test Attempt (2026-07-28)

- Command: `cmake --preset linux-release`
- Result: failed before generation
  - missing toolchain file `vcpkg/scripts/buildsystems/vcpkg.cmake`
  - Ninja build program not found
- Impact: no local focused core binary could be built in this environment during this validation pass

## Focused Test Registration Check (2026-11-19)

`tests/core/CMakeLists.txt` globs `test_*.cpp`; the eight current sources produce
these executable target names using `module_core_${_stem}_focused`:

- `module_core_test_adapter_registry_focused_focused`
- `module_core_test_adapter_signing_focused_focused`
- `module_core_test_circuit_breaker_focused_focused`
- `module_core_test_concerns_context_focused_focused`
- `module_core_test_concerns_value_boundary_focused_focused`
- `module_core_test_core_highcardinality_stress_focused`
- `module_core_test_core_smoke_focused`
- `module_core_test_plugin_loading_focused_focused`

Registration is source/build-definition evidence only; it is not evidence that a
binary was built or executed. The focused sources currently contain 12 adapter
registry cases, 11 adapter-signing cases, 9 circuit-breaker cases, 18 context
cases, and 8 plugin-loading cases. The stress source uses local stand-ins and is
not production-adapter stress evidence.

## Validation Attempt (2026-11-19)

- Base revision: `a9837d936d391dad3e5b3fc1ff94757944dae43b`; the two focused test additions in this working tree were not compiled.
- Host: Linux x86_64, GNU C++ 13.3.0, CMake Debug.
- Standard configure: `cmake --preset community-debug -B /tmp/themis-core-evidence-build`
  stopped before generation because RocksDB was not installed.
- Diagnostic configure: `cmake -S /home/runner/work/ThemisDB/ThemisDB -B /tmp/themis-core-evidence-build -DTHEMIS_ALLOW_MISSING_ROCKSDB=ON -DTHEMIS_DIAGNOSTIC_MODE=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE= -DTHEMIS_BUILD_TESTS=ON -DTHEMIS_BUILD_BENCHMARKS=OFF -DTHEMIS_EDITION=COMMUNITY`
  stopped before generation because the required `fmt` library was not installed.
- No build or CTest executable ran; no pass is claimed. Normal, invalid-input,
  degraded-state, timeout, and recovery executable evidence remains open for
  release sign-off.

## Status Assessment

- [x] Focused test target mapping verified against `tests/core/CMakeLists.txt`
- [x] Current runner configure failures and missing dependencies recorded
- [~] Fresh executable-level focused build/test evidence remains unavailable
- [ ] Production-adapter stress, sanitizer, fault-recovery, and soak evidence remains open

## Implementation Coverage (added 2026-07-28)

### New Files — Phase 1–6 Implementation

| File | Purpose |
|---|---|
| `include/core/concerns/adapter_metadata.h` | `AdapterMetadata`, `AdapterValidator`, `AdapterSignature` digest record, `kCurrentApiVersion` |
| `include/core/concerns/adapter_registry.h` | `AdapterRegistry` — typed registry, `registerAdapter<T>`, `resolve<T>`, `hotSwap<T>`, `kHotSwapTimeoutMs`, `loadFromPlugin()` (production), `PluginHandle` RAII, `AdapterTrustPolicy` |
| `src/core/concerns/adapter_registry.cpp` | Non-template implementations: `count()`, `hasAdapter()`, `loadFromPlugin()` (dlopen/LoadLibraryA), `setTrustPolicy()`, destructor |
| `include/core/concerns/plugin_api.h` | Plugin ABI contract: `ThemisPluginRegisterFn`, `kPluginInitSymbol`, `kPluginAbiVersion`, `THEMIS_DEFINE_PLUGIN_INIT` macro |
| `include/core/concerns/adapter_signing.h` | `SignedAdapterValidator` — `validate()`, `canonicalString()`, `sha256Hex()` |
| `src/core/concerns/adapter_signing.cpp` | SHA-256 metadata-digest validation via OpenSSL EVP |
| `tests/core/test_adapter_registry_focused.cpp` | 12 focused tests (AR_01..AR_12) |
| `tests/core/test_plugin_loading_focused.cpp` | 8 focused tests (PL_01..PL_08) |
| `tests/core/test_adapter_signing_focused.cpp` | 11 focused tests (SGN_01..SGN_10, including SGN_04b) |
| `tests/core/test_circuit_breaker_focused.cpp` | 9 focused tests (CB_01..CB_09) |
| `tests/core/test_concerns_context_focused.cpp` | 18 focused tests (CCT_01..CCT_18) |

### Modified Files — Phase 1–6 + Plugin/Signing Implementation

| File | Change Summary |
|---|---|
| `include/core/concerns/concerns_context.h` | Added `#include adapter_registry.h`, `resolve<T>()` template, `registry()` accessors, `registry_` member, constructor initialiser |
| `include/core/concerns/i_circuit_breaker.h` | Added `call(fn, fallback)` template with void/non-void branch via `if constexpr` |
| `cmake/CMakeLists.txt` | Added `adapter_registry.cpp` + `adapter_signing.cpp` to `THEMIS_CORE_SOURCES`; added `${CMAKE_DL_LIBS}` link on non-Windows |
| `cmake/ModularBuild.cmake` | Added `adapter_registry.cpp` + `adapter_signing.cpp` to core concerns sources |
| `src/core/ROADMAP.md` | All Q4 2026 plugin loading + signing items marked `[x]` (2026-07-28) |
| `src/core/MODULE_EVIDENCE.md` | This section updated |

### Key Test Target Names (auto-discovered by tests/core/CMakeLists.txt)

- `module_core_test_adapter_registry_focused_focused`
- `module_core_test_plugin_loading_focused_focused`
- `module_core_test_adapter_signing_focused_focused`
- `module_core_test_circuit_breaker_focused_focused`
- `module_core_test_concerns_context_focused_focused`

### Design Decisions

- `AdapterRegistry` uses `std::shared_mutex` (C++17 reader-writer) so concurrent `resolve()` calls hold only a shared lock.
- `hotSwap()` releases the write lock before draining so new resolvers get the new adapter immediately; drain polls use_count() in 1 ms steps up to 100 ms.
- `ConcernsContext::resolve<T>()` for built-in concern types returns an owning
  `shared_ptr` snapshot; the context stores its adapters in shared pointers.
- `ICircuitBreaker::call()` uses `if constexpr (std::is_void_v<decltype(fn())>)` to correctly handle both void and non-void callable return types.
- `loadFromPlugin()` uses `dlopen(RTLD_NOW | RTLD_LOCAL)` on POSIX and `LoadLibraryA` on Windows; holds library handles alive until registry destruction via RAII `PluginHandle`.
- `AdapterTrustPolicy::kRequireSignature` enforces a SHA-256 digest file (`.sig` suffix) comparison before any `dlopen` call; `kTrustAll` is the development default.
- `SignedAdapterValidator` computes `sha256Hex(id:apiVersion:description)` and uses `CRYPTO_memcmp` for constant-time digest comparison; this does not authenticate a publisher.
- Plugin ABI is a single C-linkage function `themis_plugin_register`; plugins call `registry->registerAdapter<T>()` for full type safety.

## Next Evidence Actions

1. Restore the required system dependencies (including RocksDB and fmt) in the validation environment.
2. Configure and build focused core test targets:
   - `module_core_test_adapter_registry_focused_focused`
   - `module_core_test_circuit_breaker_focused_focused`
   - `module_core_test_concerns_context_focused_focused`
3. Execute the focused binaries, including the registry timeout and circuit-breaker exception cases, and append pass/fail evidence with revision and timestamp.
