# Phase D1 Implementation Complete — Break-Even Validator Framework

**Date**: 2026-07-06  
**Status**: ✅ **IMPLEMENTATION COMPLETE** (awaiting integration & tests)  
**Scope**: Break-even decision framework for GPU acceleration  
**Target**: Phase D Optional GPU Acceleration entry criteria  

---

## 1. Deliverables Summary

### 1.1 Header File: `include/acceleration/break_even_validator.h`
- **Lines**: 453
- **API**: 12 public methods + 2 internal helpers
- **Enums**: KernelType (6 values), DeviceType (5 values)
- **Structs**: WorkloadProfile, BreakEvenDecision, CacheEntry (private)
- **Thread-Safety**: Full mutex protection for all mutable state
- **Documentation**: Comprehensive Doxygen comments on all public APIs

**Key Classes/Types**:
```cpp
enum class KernelType {
    kDistance, kTopK,                      // Category A
    kBFS, kDijkstra, kGeoDistance, kGeoContainment  // Category B
};

enum class DeviceType {
    kNVIDIA_RTX, kNVIDIA_T4, kAMD_MI210, kIntel_Arc, kCPU
};

struct WorkloadProfile {
    KernelType kernel_type;
    size_t input_size;
    float output_selectivity;
    size_t vector_dimension;
    DeviceType device;
    std::optional<bool> force_gpu;
    std::optional<bool> prefer_cpu;
};

struct BreakEvenDecision {
    bool use_gpu;
    float speedup_ratio;
    std::chrono::milliseconds cpu_time_ms;
    std::chrono::milliseconds gpu_time_ms;
    std::string reason;
    bool from_cache;
};

class BreakEvenValidator {
    BreakEvenDecision ShouldUseGPU(const WorkloadProfile& profile);
    BreakEvenDecision Profile(const WorkloadProfile& profile);
    void SetSpeedupThreshold(KernelType kernel, float threshold);
    float GetSpeedupThreshold(KernelType kernel) const;
    void ClearCache();
    void SetCacheValidityDuration(std::chrono::hours duration);
    float GetLatestBreakEvenRatio(KernelType kernel) const;
    size_t GetCacheHitCount() const;
    size_t GetCacheMissCount() const;
    size_t GetCacheSize() const;
    // ... + private helpers
};
```

### 1.2 Implementation File: `src/acceleration/break_even_validator.cc`
- **Lines**: 399
- **Key Sections**:
  - BreakEvenValidator constructor (threshold initialization)
  - ShouldUseGPU() method (cache + override logic)
  - Profile() method (CPU/GPU profiling delegation)
  - Cache management (hit/miss tracking, expiry checking)
  - String conversion helpers (KernelType, DeviceType)
  - Thread-safe locking via std::mutex

**Implementation Details**:
1. **Default Thresholds** (constructor):
   - Category A (Distance, TopK): 1.5x speedup required
   - Category B (Graph): 1.3x speedup required

2. **Decision Algorithm** (ShouldUseGPU):
   - Check caller overrides (force_gpu, prefer_cpu)
   - Query cache with profile key
   - On cache miss: call Profile() + cache result
   - Return decision with from_cache flag

3. **Profiling** (Profile):
   - Call ProfileCPU() for reference timing
   - Call ProfileGPU() for GPU path timing (if available)
   - Compute speedup_ratio = cpu_time / gpu_time
   - Compare against threshold
   - Cache decision with timestamp
   - Track latest speedup ratios for metrics

4. **Placeholder Implementations** (TODO items):
   - ProfileCPU(): Returns mock timings based on input_size
   - ProfileGPU(): Returns mock timings or nullopt if "too small"

### 1.3 Test Suite: `tests/gpu/test_break_even_validation.cpp`
- **Lines**: 509
- **Test Count**: 45+ test cases
- **Coverage**:
  - Category A kernels (Distance, TopK)
  - Category B kernels (BFS, Dijkstra, Geo)
  - Caching (hit/miss, consistency, clear, statistics)
  - Thresholds (default, custom, minimum)
  - Caller overrides (force_gpu, prefer_cpu)
  - Device types (RTX, T4, MI210, Arc, CPU)
  - Metrics (latest speedup ratio, cache statistics)
  - String conversions (KernelType, DeviceType)
  - Concurrency (multi-threaded access, race conditions)
  - Integration (realistic workloads)

**Test Organization**:
```cpp
class BreakEvenValidatorTest : public ::testing::Test {
    BreakEvenValidator validator_;
    WorkloadProfile MakeProfile(...);  // Helper
};

// Test categories:
- Category A: Distance/TopK (input size: small/medium/large)
- Category B: BFS/Dijkstra/Geo (graph size and selectivity)
- Caching: hit rate, consistency, expiry
- Thresholds: defaults, customization, bounds
- Overrides: force_gpu, prefer_cpu precedence
- Devices: NVIDIA/AMD/Intel/CPU
- Metrics: speedup ratio tracking
- Concurrency: 10 threads accessing simultaneously
- Integration: realistic (768-dim BERT, 100K-node graphs)
```

---

## 2. Architecture & Design

### 2.1 Decision Flow

```
On ShouldUseGPU(profile) call:
┌─────────────────────────────────────────┐
│ Check caller overrides                  │
│ - force_gpu? → return GPU=true          │
│ - prefer_cpu? → return GPU=false        │
└────────────────────────┬────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────┐
│ Query decision cache                    │
│ Key = KernelType_InputSize_Selectivity  │
│ Check expiry (24h default)              │
└────────────────────────┬────────────────┘
                         │
         ┌───────────────┴────────────────┐
         │ (Cache hit)           (Cache miss)
         ▼                              ▼
    ┌─────────────┐         ┌──────────────────────┐
    │ Return      │         │ Profile both paths   │
    │ cached      │         │ - ProfileCPU()       │
    │ decision    │         │ - ProfileGPU()       │
    │ from_cache= │         │ Compute speedup      │
    │ true        │         │ Compare vs threshold │
    └─────────────┘         │ Cache result         │
                            │ Return decision      │
                            │ from_cache=false     │
                            └──────────────────────┘
```

### 2.2 Cache Design

```cpp
decision_cache_: std::unordered_map<std::string, CacheEntry>
  Key: WorkloadProfile::ToCacheKey()
       Format: "kernel_size_dim_selectivity"
       Example: "distance_1000000_128_1.00"

  Value: CacheEntry {
    decision: BreakEvenDecision { use_gpu, speedup_ratio, ... }
    timestamp: steady_clock::now()
  }

  Expiry: timestamp + cache_validity_duration_
          Default: 24 hours
          Configurable via SetCacheValidityDuration()
```

### 2.3 Thread Safety

```cpp
All mutable state protected by std::mutex:
  - decision_cache_
  - speedup_thresholds_
  - latest_speedup_ratios_
  - cache_hits_ / cache_misses_

Lock scope: std::lock_guard<std::mutex> lock(mutex_);
  - Minimal: only for data access, not profiling
  - Profiling (ProfileCPU/ProfileGPU) runs without lock

Pattern: Read-modify-write operations atomic within lock scope
```

---

## 3. Integration Points (TODO)

### 3.1 Query Planner Integration

**File**: `src/evaluation/query_planner.cc` (lines ~1400)

```cpp
// In query planning when considering GPU acceleration:

bool QueryPlanner::ShouldUseGPUKernel(const KernelCandidate& candidate) {
    auto profile = WorkloadProfile{
        .kernel_type = candidate.kernel_type,
        .input_size = candidate.input_cardinality,
        .output_selectivity = candidate.estimated_selectivity,
        .device = accelerator_device_,
    };
    auto decision = break_even_validator_.ShouldUseGPU(profile);
    return decision.use_gpu;
}
```

### 3.2 Acceleration Layer Integration

**File**: `src/acceleration/compute_graph.cc` (lines ~800)

```cpp
// In acceleration kernel dispatch:

DispatchResult DispatchKernel(const KernelRequest& request) {
    auto profile = WorkloadProfile::FromRequest(request);
    auto decision = break_even_validator_.ShouldUseGPU(profile);
    
    if (decision.use_gpu) {
        return DispatchToGPU(request);
    } else {
        return DispatchToCPU(request);
    }
}
```

### 3.3 Prometheus Metrics Export

**File**: To be added (metrics integration layer)

```cpp
// Export decision metrics:
RecordHistogram("gpu_acceleration_break_even_ratio",
    speedup_ratio,
    {{"kernel", KernelTypeToString(kernel)}});

RecordGauge("gpu_acceleration_utilized",
    use_gpu ? 1.0 : 0.0,
    {{"kernel", KernelTypeToString(kernel)}});

RecordCounter("gpu_acceleration_fallback_count", 1,
    {{"reason", decision.reason}});
```

---

## 4. Acceptance Criteria Checklist

### Phase D1 Entry Requirements
- [x] BreakEvenValidator API complete and documented
- [x] WorkloadProfile enum types cover all kernel categories
- [x] Cache management with TTL and hit/miss tracking
- [x] Thread-safe mutex protection for concurrent access
- [x] String conversion helpers for logging/metrics
- [x] 45+ test cases covering all kernel types and sizes
- [x] Threshold customization per kernel type
- [x] Caller override support (force_gpu, prefer_cpu)
- [x] Device type variations (NVIDIA, AMD, Intel, CPU)
- [ ] Integration with query planner (pending)
- [ ] Integration with acceleration dispatcher (pending)
- [ ] Prometheus metrics export (pending)
- [ ] ProfileCPU() implementation (currently placeholder)
- [ ] ProfileGPU() implementation (currently placeholder)

### Placeholder Implementations (TODO)
These methods return mock data for placeholder profiling:

1. **ProfileCPU()** — needs CPU reference kernel delegation
   - Call to cpu_reference_kernels::L2Distance(), TopK(), etc.
   - Measure with std::chrono::high_resolution_clock
   - Return median of multiple runs
   - Handle CPU fallback paths

2. **ProfileGPU()** — needs GPU kernel profiling
   - Check GPU availability and select best device
   - Allocate GPU memory (CUDA cudaMalloc, HIP hipMalloc)
   - Transfer input to GPU (cudaMemcpy, hipMemcpy)
   - Launch kernel and measure
   - Transfer results back
   - Return total time or nullopt if unavailable

---

## 5. Next Steps (Phase D1.5 - Integration & Real Profiling)

### 5.1 Immediate
1. [ ] Implement ProfileCPU() with actual CPU reference kernels
2. [ ] Implement ProfileGPU() with CUDA/HIP timing
3. [ ] Integrate with query planner
4. [ ] Integrate with acceleration dispatcher
5. [ ] Add Prometheus metrics export

### 5.2 Validation
1. [ ] Build and link tests (ensure no compilation errors)
2. [ ] Run test suite (target: 45/45 passing)
3. [ ] Profile on representative hardware:
   - NVIDIA RTX 4090 (high-end)
   - NVIDIA T4 (inference)
   - AMD MI210 (if available)
4. [ ] Establish break-even baselines and publish

### 5.3 Phase D2 Readiness
1. [ ] Confirm Phase D1 tests stable and repeatable
2. [ ] Measure cache hit rate on realistic workloads (target ≥90%)
3. [ ] Verify decision latency <10ms on cache hit, <100ms on miss
4. [ ] Prepare for Phase D2 CUDA error handling hardening

---

## 6. Risk Mitigation

| Risk | Mitigation |
|------|-----------|
| ProfileCPU/GPU placeholder data unrealistic | Implement with actual kernels early; baseline on representative data |
| Cache key collisions or instability | Use stable, deterministic key format; add unit tests |
| GPU unavailability edge cases | ProfileGPU() returns nullopt; fallback to CPU gracefully |
| Thread safety regressions | Mutex test suite with 10+ concurrent threads |
| Threshold values optimistic | Collect empirical baseline on multiple GPU types; adjust thresholds |

---

## 7. Files Delivered

| File | Size | Purpose | Status |
|------|------|---------|--------|
| `include/acceleration/break_even_validator.h` | 453 lines | API + types | ✅ Complete |
| `src/acceleration/break_even_validator.cc` | 399 lines | Implementation | ✅ Complete (placeholders for profiling) |
| `tests/gpu/test_break_even_validation.cpp` | 509 lines | 45+ test cases | ✅ Complete |
| `ai_working/PHASE_D1_BREAK_EVEN_VALIDATOR.md` | Design doc | Reference | ✅ Complete |

---

## 8. References

- **PHASE_D_PREPARATION_PLAN.md** — Phase D overall strategy
- **TARGET_ARCHITECTURE.md** — Four-layer hybrid architecture
- **KERNEL_CLASSIFICATION_REVIEW.md** — Category A/B/C kernel safety
- **HYBRID_RETRIEVAL_ROLLOUT_PLAN.md** — Rollout phases A-D

---

**Document Version**: 1.0  
**Last Updated**: 2026-07-06  
**Next Phase**: D1.5 Integration & Real Profiling
