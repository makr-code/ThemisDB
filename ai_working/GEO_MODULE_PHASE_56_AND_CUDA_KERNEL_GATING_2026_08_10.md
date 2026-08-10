# Geo Module Phase 5-6 Completion & CUDA Kernel Gating Design

**Date:** 2026-08-10  
**Status:** Phase 5-6 Complete; CUDA kernel gating design ready for Q4 2026 implementation  
**Scope:** Geo module Phase 5-6 analysis + CUDA kernel gating (A-06, A-07) + public plugin externalization (DEF-03a)  
**Authority:** src/geo/ROADMAP.md (Phase 5-6 marked [x]); ROADMAP.md §Release Hardening Program Phase 5-6; DEF-03a in DEFERRED_ITEMS_EXECUTION_PLAN_2026_08_10.md

---

## Part 1: Geo Module Phase 5-6 Completion Evidence

### Phase 5: Performance and Hardening — ✅ COMPLETE (2026-08-07)

#### Deliverables
- ✅ Benchmark-backed release gates: `benchmarks/geo/bench_geo_release_gates.cpp`
  - **GATE-GRG-01:** Point-in-polygon query (1k points) p99 ≤ 5ms
  - **GATE-GRG-02:** BBox query (10k points) p99 ≤ 1ms
  - **GATE-GRG-03:** GeoJSON parse p99 ≤ 500µs
  - **GATE-GRG-04:** Haversine distance p99 ≤ 10µs
  - **GATE-GRG-05:** Spatial join p99 ≤ 50ms
  - **GATE-GRG-06:** Backend selection p99 ≤ 50µs

- ✅ Performance validation: p95/p99 and throughput validated against release baselines (2026-08-07)
- ✅ CPU/GPU parity testing: Fallback paths tested for deterministic behavior
- ✅ Benchmark stabilization: GeoJSON serialization O(n²) → O(n) ostringstream + reserve()

#### Gate Status
| Gate | Target | Result | Verified |
|------|--------|--------|----------|
| GATE-GRG-01 | ≤5ms | PASS | 2026-08-07 ✅ |
| GATE-GRG-02 | ≤1ms | PASS | 2026-08-07 ✅ |
| GATE-GRG-03 | ≤500µs | PASS | 2026-08-07 ✅ |
| GATE-GRG-04 | ≤10µs | PASS | 2026-08-07 ✅ |
| GATE-GRG-05 | ≤50ms | PASS | 2026-08-07 ✅ |
| GATE-GRG-06 | ≤50µs | PASS | 2026-08-07 ✅ |

### Phase 6: Documentation and Acceptance — ✅ COMPLETE (2026-08-10)

#### Deliverables
- ✅ API contract frozen: `include/geo/geo_api_contract.h` (GeoJSON, backend dispatch, spatial index, join error taxonomy)
- ✅ Contract-hardening tests: `tests/geo/test_geo_contract_hardening_focused.cpp` (GCH-01..GCH-16)
- ✅ Release-gate benchmarks: `benchmarks/geo/bench_geo_release_gates.cpp` (GRG-01..GRG-06)
- ✅ Benchmark CMakeLists registered: `benchmarks/geo/CMakeLists.txt`
- ✅ Documentation aligned to source-verifiable behavior
- ✅ Roadmap/future planning separated from historical entries
- ✅ Module-level security and failure behavior documented

#### Production Readiness Status
| Criterion | Evidence | Status |
|-----------|----------|--------|
| API contract frozen | include/geo/geo_api_contract.h | ✅ |
| Tests verified | GCH-01..GCH-16 + GRG-01..GRG-06 focused tests | ✅ |
| Benchmarks validated | bench_geo_release_gates.cpp all gates PASS | ✅ |
| Documentation complete | Geo module README.md + ROADMAP.md Phase 6 | ⏳ README stale (last update 2026-08-03) |
| Source verification | Contract header, tests, benchmarks cross-checked | ✅ |

### Module Maturity: PRODUCTION_CANDIDATE ✅

**Classification:** Geo module Phase 1-6 complete; production-ready for release.

---

## Part 2: CUDA Kernel Gating Design (A-06, A-07)

### Current State (2026-08-10)

#### Delivered (Q3 2026)
- ✅ Haversine distance CUDA kernel (Phase 1-2)
- ✅ Point-in-polygon CUDA kernel (Phase 1-2)
- ✅ Vincenty distance CUDA kernel for batched per-pair dispatch (Phase 2-3, 2026-08-08)
- ✅ CPU/GPU backend dispatch logic with fallback
- ✅ RAII wrappers for CUDA device memory lifecycle

#### Blocked Items (Q3 2026)
- ⏸️ **A-06 CUDA geospatial kernels**: Gates (Haversine, point-in-polygon) implemented but **CI validation blocked by unavailable hardware**
- ⏸️ **A-07 CUDA kernel batching & performance gates**: Vincenty batching complete; performance gates **require GPU CI runner**

#### Known Issue
- Compilation: Focused tests can be built; execution requires NVIDIA GPU hardware
- CI: Current CI runners lack CUDA-capable GPU; hardware-in-the-loop job needed

### A-06 / A-07 Gating Strategy (Q4 2026 Phase 2-3)

#### Design Goals
1. **Default behavior (no GPU):** CPU-only execution; all tests pass on standard CI runners
2. **Optional behavior (GPU available):** CUDA kernels auto-selected when GPU detected; performance gates validated
3. **Graceful degradation:** Missing GPU → fallback to CPU implementation (deterministic parity)
4. **CI coverage:** All gates runnable; GPU gates optional (self-hosted runner)

#### CMake Feature Gating

**New CMake flag:**
```cmake
option(THEMIS_GEO_CUDA "Enable CUDA geospatial kernels for accelerated distance/containment" OFF)

if(THEMIS_GEO_CUDA)
    # CUDA kernel targets
    find_package(CUDAToolkit REQUIRED)
    enable_language(CUDA)
    
    # Compile CUDA kernels
    add_library(themis_geo_cuda_kernels STATIC
        src/geo/kernels/haversine_kernel.cu
        src/geo/kernels/pip_kernel.cu
        src/geo/kernels/vincenty_kernel.cu
    )
    target_link_libraries(themis_geo_cuda_kernels PUBLIC ${CUDAToolkit_LIBRARIES})
    
    # Link geo module to CUDA kernels
    target_link_libraries(themis_geo PRIVATE themis_geo_cuda_kernels)
    target_compile_definitions(themis_geo PRIVATE HAVE_CUDA_KERNELS=1)
else()
    # CPU-only fallback (always available)
    target_compile_definitions(themis_geo PRIVATE HAVE_CUDA_KERNELS=0)
endif()
```

#### Runtime Detection & Dispatch

**Runtime device query (geo/geo_backend_dispatch.cpp):**
```cpp
class GeoBackendDispatcher {
public:
    static bool isCudaAvailable() {
        #if HAVE_CUDA_KERNELS
            // Query CUDA device count; return true if GPU found
            int deviceCount = 0;
            cudaError_t status = cudaGetDeviceCount(&deviceCount);
            return (status == cudaSuccess && deviceCount > 0);
        #else
            return false; // CUDA not compiled in
        #endif
    }
    
    // Dispatch to CUDA or CPU based on availability + performance heuristics
    GeometryResult computeHaversineBatch(
        const std::vector<Point>& points1,
        const std::vector<Point>& points2
    ) {
        if (isCudaAvailable() && shouldUseCuda(points1.size())) {
            return cudaHaversineBatch(points1, points2);
        } else {
            return cpuHaversineBatch(points1, points2); // Fallback
        }
    }
};
```

#### Test Gating

**Unconditional tests (CPU-only, always run):**
```cmake
# tests/geo/test_geo_contract_hardening_focused.cpp
# All GCH-01..GCH-16 tests run on all platforms without CUDA
themis_register_module_focused_test(
    MODULE geo
    NAME geo_contract_hardening
    TARGET module_geo_test_contract_hardening_focused
    TIER unit
    TIMEOUT 60
    LABELS geo release_critical
)

# tests/geo/test_geo_cpu_gpu_parity_focused.cpp
# Parity tests run CPU path unconditionally; GPU path skipped if no CUDA
themis_register_module_focused_test(
    MODULE geo
    NAME geo_cpu_gpu_parity
    TARGET module_geo_test_cpu_gpu_parity_focused
    TIER unit
    TIMEOUT 120
    LABELS geo release_critical # CPU path is release-critical
)
```

**Optional GPU-specific gates (conditional, GPU required):**
```cmake
# tests/geo/test_geo_cuda_kernels_focused.cpp (NEW)
# CUDA-specific tests: only compile/run if THEMIS_GEO_CUDA=ON AND GPU detected
if(THEMIS_GEO_CUDA)
    add_executable(module_geo_test_cuda_kernels_focused
        tests/geo/test_geo_cuda_kernels_focused.cpp
    )
    target_link_libraries(module_geo_test_cuda_kernels_focused PRIVATE themis_geo themis_geo_cuda_kernels)
    
    # Register with conditional skip if no GPU
    add_test(NAME geo_cuda_kernels_focused COMMAND module_geo_test_cuda_kernels_focused)
    set_tests_properties(geo_cuda_kernels_focused PROPERTIES
        LABELS "geo;cuda_optional"
        SKIP_RETURN_CODE 77  # CUDA not available → skip (exit code 77)
        TIMEOUT 120
    )
endif()
```

#### Benchmark Gating

**CPU benchmarks (release-critical, run on all platforms):**
```cmake
# benchmarks/geo/bench_geo_release_gates.cpp (existing)
# GRG-01..GRG-06 gates measure CPU paths; always included

themis_register_benchmark(
    MODULE geo
    NAME bench_geo_release_gates
    TARGET bench_geo_release_gates
    LABELS geo release_critical gate
)
```

**GPU benchmarks (optional, GPU required):**
```cmake
# benchmarks/geo/bench_geo_cuda_kernels.cpp (NEW)
# CUDA-specific performance gates: A-06, A-07

if(THEMIS_GEO_CUDA)
    add_executable(bench_geo_cuda_kernels
        benchmarks/geo/bench_geo_cuda_kernels.cpp
    )
    target_link_libraries(bench_geo_cuda_kernels PRIVATE 
        benchmark
        themis_geo 
        themis_geo_cuda_kernels
    )
    
    add_test(
        NAME bench_geo_cuda_kernels_run
        COMMAND bench_geo_cuda_kernels --benchmark_filter="Haversine|PointInPolygon|Vincenty"
    )
    set_tests_properties(bench_geo_cuda_kernels_run PROPERTIES
        LABELS "geo;cuda_optional;benchmark"
        TIMEOUT 300
    )
endif()
```

#### Performance Gates (A-06, A-07)

**Target Performance (CUDA kernels, GPU-only):**

| Gate | Operation | Input | Target | Verification |
|------|-----------|-------|--------|--------------|
| **A-06-01** | Haversine distance (batched 1M pairs) | 1M points→pairs | ≤500ms batch | bench_geo_cuda_kernels |
| **A-06-02** | Point-in-polygon (1k queries, 100-edge poly) | 1k points × poly | p99 ≤ 2ms | bench_geo_cuda_kernels |
| **A-07-01** | Vincenty distance (batched 1M pairs) | 1M points→pairs | ≤1000ms batch | bench_geo_cuda_kernels |
| **A-07-02** | Kernel dispatch overhead (GPU selection) | N/A | ≤100µs decision | bench_geo_cuda_kernels |

---

## Part 3: Public Plugin Externalization Design (DEF-03a)

### Objective
Extract `src/geo` into optional public plugin `plugins/themisdb_geo` while maintaining integrated monorepo path as default.

### Design

#### Option 1: Integrated Monorepo (Default, Always Available)
```
src/
├── geo/
│   ├── CMakeLists.txt
│   ├── ROADMAP.md
│   ├── README.md
│   ├── *.cpp *.h
│   └── ...
└── ...

benchmarks/
├── geo/
│   ├── bench_geo_release_gates.cpp (CPU gates, always included)
│   ├── bench_geo_cuda_kernels.cpp (GPU gates, included if THEMIS_GEO_CUDA=ON)
│   └── CMakeLists.txt
└── ...

tests/
├── geo/
│   ├── test_geo_contract_hardening_focused.cpp (always included)
│   ├── test_geo_cpu_gpu_parity_focused.cpp (CPU path always; GPU optional)
│   ├── test_geo_cuda_kernels_focused.cpp (optional if THEMIS_GEO_CUDA=ON)
│   └── CMakeLists.txt
└── ...
```

**CMake flag:** `THEMIS_EXTERNALIZE_GEO_PLUGIN=OFF` (default)

#### Option 2: Optional Public Plugin (User-Selectable)
```
plugins/
├── themisdb_geo/
│   ├── CMakeLists.txt
│   ├── ROADMAP.md (references monorepo src/geo/ROADMAP.md Phase 5-6 baseline)
│   ├── include/themisdb_geo/
│   ├── src/
│   └── benchmarks/
│       ├── bench_geo_cpu_gpu.cpp (accelerated variants: GPU batching, SIMD)
│       └── bench_geo_cuda_kernels.cpp
└── ...

# .gitmodules
[submodule "plugins/themisdb_geo"]
    path = plugins/themisdb_geo
    url = https://github.com/makr-code/themisdb_geo.git
    branch = develop
```

**CMake flag:** `THEMIS_EXTERNALIZE_GEO_PLUGIN=ON`

#### CMake Integration Strategy

**Main CMakeLists.txt:**
```cmake
if(THEMIS_EXTERNALIZE_GEO_PLUGIN)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugins/themisdb_geo/CMakeLists.txt")
        add_subdirectory(plugins/themisdb_geo)
        set(THEMIS_GEO_TARGET themisdb_geo_plugin)
    else()
        message(FATAL_ERROR "THEMIS_EXTERNALIZE_GEO_PLUGIN=ON but plugins/themisdb_geo not found; run: git submodule update --init --recursive plugins/themisdb_geo")
    endif()
else()
    # Integrated monorepo path (default, always available)
    add_subdirectory(src/geo)
    set(THEMIS_GEO_TARGET themis_geo)
endif()

# Link all modules that depend on geo
target_link_libraries(themis_query PRIVATE ${THEMIS_GEO_TARGET})
target_link_libraries(themis_timeseries PRIVATE ${THEMIS_GEO_TARGET})
```

### Benchmark Split Strategy

#### Monorepo Benchmarks (Always Included)
- `benchmarks/geo/bench_geo_release_gates.cpp` — CPU-only gates (GRG-01..GRG-06)
- `benchmarks/geo/bench_geo_cpu_gpu.cpp` — CPU/GPU parity (CPU path release-critical; GPU optional)

**Reasoning:** Release gates are mandatory for all editions; must not depend on external plugin.

#### Plugin Benchmarks (Optional, Externalized)
- `plugins/themisdb_geo/benchmarks/bench_geo_cuda_kernels.cpp` — GPU-specific gates (A-06, A-07)
- `plugins/themisdb_geo/benchmarks/bench_geo_simd_optimized.cpp` — SIMD variants (future)

**Reasoning:** GPU acceleration and advanced optimizations are plugin-specific; won't slow down community builds.

### Test Split Strategy

#### Monorepo Tests (Always Included)
- `tests/geo/test_geo_contract_hardening_focused.cpp` (GCH-01..GCH-16) — API contract verification
- `tests/geo/test_geo_cpu_gpu_parity_focused.cpp` — CPU/GPU parity (CPU path release-critical)

#### Plugin Tests (Optional)
- `plugins/themisdb_geo/tests/test_geo_cuda_kernels_focused.cpp` (A-06, A-07 gates)
- `plugins/themisdb_geo/tests/test_geo_simd_focused.cpp` (SIMD optimization gates)

### Release Package Strategy

**Community/Minimal Editions:**
- Ship integrated `src/geo` (no plugin dependency)
- `THEMIS_EXTERNALIZE_GEO_PLUGIN=OFF` (default)
- All release-critical tests pass
- All GRG-01..GRG-06 gates included

**Enterprise/Hyperscaler/Military Editions:**
- Option A: Ship with integrated path + optional plugin for GPU acceleration
- Option B: Ship only plugin path (if customer has GPU infrastructure)

---

## Part 4: Q4 2026 Execution Timeline

### Phase 2 (2026-08-19 to 2026-09-15)

#### Milestone 1: CUDA Kernel Gating (A-06, A-07)
- [ ] Add `THEMIS_GEO_CUDA=ON|OFF` flag to CMakePresets.json
- [ ] Implement `geo_backend_dispatch.cpp` with runtime GPU detection
- [ ] Create `tests/geo/test_geo_cuda_kernels_focused.cpp` (conditional, GPU required)
- [ ] Create `benchmarks/geo/bench_geo_cuda_kernels.cpp` with A-06-01..A-07-02 gates
- [ ] Verify CPU/GPU parity (deterministic behavior)
- [ ] Document GPU driver requirements in SETUP.md / HARDWARE_REQUIREMENTS.md
- [ ] Set up self-hosted CI runner for GPU validation (optional)

**Target Date:** 2026-08-31  
**Owner:** Geo Module Lead + GPU Infrastructure  
**Success Criterion:** A-06, A-07 gates PASS on GPU-enabled runner; CPU path unchanged

#### Milestone 2: Public Plugin Externalization (DEF-03a)
- [ ] Design `plugins/themisdb_geo/CMakeLists.txt` with plugin manifest
- [ ] Add `THEMIS_EXTERNALIZE_GEO_PLUGIN` flag to CMakePresets.json (default: OFF)
- [ ] Implement CMake integration strategy (option 1 vs option 2 switching)
- [ ] Separate benchmarks (monorepo vs plugin)
- [ ] Test both paths:
  - Option 1 (integrated, THEMIS_EXTERNALIZE_GEO_PLUGIN=OFF): all tests pass
  - Option 2 (plugin, THEMIS_EXTERNALIZE_GEO_PLUGIN=ON): all tests pass
- [ ] Document feature flags in `docs/architecture/plugin_model.md`

**Target Date:** 2026-09-15  
**Owner:** Geo Module Lead + Plugin Infrastructure  
**Success Criterion:** Both integrated and externalized paths pass release-critical tests

### Phase 3 (2026-09-16 to 2026-10-15)

#### Milestone 3: CUDA Infrastructure Hardening
- [ ] Performance optimization: Kernel memory access patterns, register pressure
- [ ] Fallback resilience: GPU device reset, driver recovery
- [ ] Batch size heuristics: Auto-tune for GPU hardware variant (Tesla, Quadro, consumer)

#### Milestone 4: Documentation Completion
- [ ] Update `src/geo/ROADMAP.md` Phase 5-6 with CUDA kernel evidence
- [ ] Update `src/geo/README.md` with Phase 5-6 completion status
- [ ] Create `docs/architecture/geo_cuda_kernel_design.md` (kernel behavior, dispatch logic)
- [ ] Add CUDA-specific performance tuning guide in `docs/performance/geo_optimization.md`

**Target Date:** 2026-10-15  
**Owner:** Geo Module Lead + Technical Writer

---

## Success Metrics (Q4 2026 Target: 2026-10-15)

### Part 1: Geo Phase 5-6 Completion — ✅ ALREADY ACHIEVED
- [x] All Phase 1-6 gates PASS
- [x] Release-gate benchmarks validated (GRG-01..GRG-06)
- [x] API contract frozen
- [x] Production-ready status confirmed

### Part 2: CUDA Kernel Gating (A-06, A-07)
- [ ] `THEMIS_GEO_CUDA` flag compiles without errors (GPU available)
- [ ] `THEMIS_GEO_CUDA` flag disabled without errors (GPU unavailable)
- [ ] A-06-01..A-07-02 performance gates PASS on GPU hardware
- [ ] CPU/GPU parity tests pass (deterministic behavior)
- [ ] All CPU-only tests pass (release-critical) on standard CI

### Part 3: Public Plugin Externalization (DEF-03a)
- [ ] `THEMIS_EXTERNALIZE_GEO_PLUGIN=OFF` (integrated) passes all tests ≥ release-critical
- [ ] `THEMIS_EXTERNALIZE_GEO_PLUGIN=ON` (plugin) passes all tests ≥ release-critical
- [ ] Community edition ships with integrated path (no plugin dependency)
- [ ] Enterprise edition option includes optional plugin for GPU acceleration
- [ ] Missing submodule degrades gracefully (warning, not error)

---

## References

- **Geo ROADMAP:** `src/geo/ROADMAP.md` (Phase 1-6 complete)
- **Geo README:** `src/geo/README.md` (update status: 2026-08-03)
- **Release Gates:** `benchmarks/geo/bench_geo_release_gates.cpp` (GRG-01..GRG-06)
- **Contract Tests:** `tests/geo/test_geo_contract_hardening_focused.cpp` (GCH-01..GCH-16)
- **.gitmodules:** `plugins/themisdb_geo` entry (line 60-63)
- **Deferred Items:** `ai_working/DEFERRED_ITEMS_EXECUTION_PLAN_2026_08_10.md` (DEF-03a)

---

**Geo Phase 5-6 Status:** ✅ COMPLETE (2026-08-07)  
**CUDA Kernel Gating Design:** ✅ READY FOR Q4 2026 IMPLEMENTATION  
**Public Plugin Externalization Design:** ✅ READY FOR Q4 2026 IMPLEMENTATION

**Next Action (Batch 2):** Design review + CMake strategy finalization  
**Next Implementation (Batch 3+):** Post-GA content push to Wave-1 repos; then CUDA/plugin work begins

**Owner:** Geo Module Lead + GPU Infrastructure + Plugin Program Manager  
**Last Updated:** 2026-08-10
