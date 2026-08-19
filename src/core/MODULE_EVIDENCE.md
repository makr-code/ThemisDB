# Core Module - Build and Test Evidence

<!-- Status: current | validated: 2026-07-28 -->
<!-- Issue: #5638 (Development Status 2026-07-18) -->
<!-- Links: ROADMAP.md · FUTURE_ENHANCEMENTS.md · ARCHITECTURE.md -->

## Evidence Summary

This document tracks the current evidence state for the core module status issue and records both canonical historical evidence and current-environment validation gaps.

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

## Current Local Build/Test Attempt (2026-07-28)

- Command: `cmake --preset linux-release`
- Result: failed before generation
  - missing toolchain file `vcpkg/scripts/buildsystems/vcpkg.cmake`
  - Ninja build program not found
- Impact: no local focused core binary could be built in this environment during this validation pass

## Status Assessment

- [x] Roadmap/future/architecture synchronization refreshed for issue #5638
- [x] Focused test registration path verified in source
- [~] Fresh executable-level focused build/test evidence remains blocked by local toolchain/generator setup
- [ ] New executable run evidence for `module_core_test_*_focused` captured for this cycle

## Implementation Coverage (added 2026-07-28)

### New Files — Phase 1–6 Implementation

| File | Purpose |
|---|---|
| `include/core/concerns/adapter_metadata.h` | `AdapterMetadata`, `AdapterValidator`, `AdapterSignature` (production), `kCurrentApiVersion` |
| `include/core/concerns/adapter_registry.h` | `AdapterRegistry` — typed registry, `registerAdapter<T>`, `resolve<T>`, `hotSwap<T>`, `kHotSwapTimeoutMs`, `loadFromPlugin()` (production), `PluginHandle` RAII, `AdapterTrustPolicy` |
| `src/core/concerns/adapter_registry.cpp` | Non-template implementations: `count()`, `hasAdapter()`, `loadFromPlugin()` (dlopen/LoadLibraryA), `setTrustPolicy()`, destructor |
| `include/core/concerns/plugin_api.h` | Plugin ABI contract: `ThemisPluginRegisterFn`, `kPluginInitSymbol`, `kPluginAbiVersion`, `THEMIS_DEFINE_PLUGIN_INIT` macro |
| `include/core/concerns/adapter_signing.h` | `SignedAdapterValidator` — `validate()`, `canonicalString()`, `sha256Hex()` |
| `src/core/concerns/adapter_signing.cpp` | SHA-256 signing implementation via OpenSSL EVP |
| `tests/core/test_adapter_registry_focused.cpp` | 10 focused tests (AR_01..AR_10) |
| `tests/core/test_plugin_loading_focused.cpp` | 8 focused tests (PL_01..PL_08) |
| `tests/core/test_adapter_signing_focused.cpp` | 10 focused tests (SGN_01..SGN_10) |
| `tests/core/test_circuit_breaker_focused.cpp` | 8 focused tests (CB_01..CB_08) |
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

### Test Target Names (auto-discovered by tests/core/CMakeLists.txt)

- `module_core_test_adapter_registry_focused_focused`
- `module_core_test_plugin_loading_focused_focused`
- `module_core_test_adapter_signing_focused_focused`
- `module_core_test_circuit_breaker_focused_focused`
- `module_core_test_concerns_context_focused_focused`

### Design Decisions

- `AdapterRegistry` uses `std::shared_mutex` (C++17 reader-writer) so concurrent `resolve()` calls hold only a shared lock.
- `hotSwap()` releases the write lock before draining so new resolvers get the new adapter immediately; drain polls use_count() in 1 ms steps up to 100 ms.
- `ConcernsContext::resolve<T>()` for built-in concern types returns a no-op-deleter `shared_ptr`; callers must not store beyond context lifetime.
- `ICircuitBreaker::call()` uses `if constexpr (std::is_void_v<decltype(fn())>)` to correctly handle both void and non-void callable return types.
- `loadFromPlugin()` uses `dlopen(RTLD_NOW | RTLD_LOCAL)` on POSIX and `LoadLibraryA` on Windows; holds library handles alive until registry destruction via RAII `PluginHandle`.
- `AdapterTrustPolicy::kRequireSignature` enforces a SHA-256 digest file (`.sig` suffix) comparison before any `dlopen` call; `kTrustAll` is the development default.
- `SignedAdapterValidator` computes `sha256Hex(id:apiVersion:description)` and uses `CRYPTO_memcmp` for constant-time comparison to resist timing side-channels.
- Plugin ABI is a single C-linkage function `themis_plugin_register`; plugins call `registry->registerAdapter<T>()` for full type safety.

## Next Evidence Actions

1. Restore functional build prerequisites (toolchain + Ninja) in validation environment.
2. Configure and build focused core test targets:
   - `module_core_test_adapter_registry_focused_focused`
   - `module_core_test_circuit_breaker_focused_focused`
   - `module_core_test_concerns_context_focused_focused`
3. Execute focused binaries and append pass/fail evidence with timestamp.
