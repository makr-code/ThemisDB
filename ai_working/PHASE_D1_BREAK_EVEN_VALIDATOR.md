# Phase D1 — Break-Even Validation Framework: Implementation Guide

**Status**: 🟡 **PLANNING** (2026-07-06)  
**Target**: Post-Phase C gate entry (Q1 2027)  
**Scope**: Break-even benchmarking infrastructure for GPU acceleration decision making

---

## 1. Problem Statement

**Challenge**: Determine when GPU acceleration is worth the overhead.

GPU brings latency penalties upfront:
- Data transfer to GPU VRAM
- GPU allocation + initialization
- Kernel launch overhead
- Result transfer back to host

GPU pays back only when:
- Kernel execution time >> transfer time
- Data reuse amortizes allocation cost
- Batch size large enough for parallelism

**Solution**: Establish deterministic, reproducible break-even thresholds per kernel type and input size range.

---

## 2. Architecture

### 2.1 Component Structure

```
┌─────────────────────────────────────────────────────┐
│ Break-Even Validator Framework                      │
├─────────────────────────────────────────────────────┤
│                                                     │
│  ┌─────────────────────────────────────────────┐  │
│  │ Benchmark Input Generator                   │  │
│  │ - Fixed: 100, 1K, 10K, 100K, 1M vectors    │  │
│  │ - Variable: selectivity, dimensionality     │  │
│  │ - Adversarial: worst-case patterns          │  │
│  └─────────────────────────────────────────────┘  │
│                                                     │
│  ┌─────────────────────────────────────────────┐  │
│  │ Dual-Path Profiler                          │  │
│  │ - CPU baseline timing (reference)            │  │
│  │ - GPU timing (transfer + kernel + sync)     │  │
│  │ - Breakdown: allocation, transfer, exec     │  │
│  └─────────────────────────────────────────────┘  │
│                                                     │
│  ┌─────────────────────────────────────────────┐  │
│  │ Break-Even Decision Engine                  │  │
│  │ - Compute speedup ratio (CPU time / GPU)    │  │
│  │ - Apply thresholds (1.5x Category A, 1.3x B)│  │
│  │ - Export decision: USE_GPU or CPU_FALLBACK  │  │
│  └─────────────────────────────────────────────┘  │
│                                                     │
│  ┌─────────────────────────────────────────────┐  │
│  │ Prometheus Metrics Export                   │  │
│  │ - gpu_acceleration_utilized (boolean)       │  │
│  │ - gpu_acceleration_latency_ms (histogram)   │  │
│  │ - gpu_acceleration_break_even_ratio         │  │
│  └─────────────────────────────────────────────┘  │
│                                                     │
└─────────────────────────────────────────────────────┘
```

### 2.2 Decision Algorithm

```
On each GPU candidate dispatch:

1. Extract workload profile:
   - kernel_type (distance, topk, bfs, dijkstra, geo)
   - input_size (vectors/nodes)
   - output_selectivity (%)
   - device_type (GPU model)

2. Look up historical decision:
   if cached_decision.is_valid():
       return cached_decision
   
3. Profile (if cache miss):
   cpu_time = profile_cpu_path(workload)
   gpu_time = profile_gpu_path(workload)
   
4. Compute break-even:
   speedup_ratio = cpu_time / gpu_time
   threshold = 1.5 (Category A) or 1.3 (Category B)
   
5. Decision:
   if speedup_ratio >= threshold:
       use_gpu = true
       record_prometheus("break_even_ratio", speedup_ratio)
   else:
       use_gpu = false
       record_prometheus("cpu_fallback_reason", "break_even_not_met")
   
6. Cache decision (valid for 24 hours or until SLO breach)
   return use_gpu
```

---

## 3. Implementation Components

### 3.1 Break-Even Validator Header

**File**: `include/acceleration/break_even_validator.h`

```cpp
#pragma once

#include <string>
#include <chrono>
#include <optional>
#include <unordered_map>

namespace themis::acceleration {

enum class KernelType {
    kDistance,      // Category A: L2, Cosine, InnerProduct
    kTopK,          // Category A: TopK selection
    kBFS,           // Category B: Breadth-First Search
    kDijkstra,      // Category B: Shortest path
    kGeoDistance,   // Category B: Geospatial distance
    kGeoContainment // Category B: Geospatial containment
};

enum class DeviceType {
    kNVIDIA_RTX,    // RTX 40-series, H100
    kNVIDIA_T4,     // T4, A100
    kAMD_MI210,     // MI210, MI300
    kIntel_Arc,     // Intel Arc
    kCPU            // CPU-only fallback
};

struct WorkloadProfile {
    KernelType kernel_type;
    size_t input_size;          // vector count or node count
    float output_selectivity;   // [0, 1] range
    size_t vector_dimension;    // for distance kernels
    DeviceType device;
    
    // Optional: caller hints
    std::optional<bool> force_gpu;
    std::optional<bool> prefer_cpu;
};

struct BreakEvenDecision {
    bool use_gpu;
    float speedup_ratio;        // CPU time / GPU time
    std::chrono::milliseconds cpu_time_ms;
    std::chrono::milliseconds gpu_time_ms;
    std::string reason;         // "break_even_met", "gpu_unavailable", etc.
};

class BreakEvenValidator {
public:
    BreakEvenValidator();
    ~BreakEvenValidator();
    
    // Main decision interface
    BreakEvenDecision ShouldUseGPU(const WorkloadProfile& profile);
    
    // Explicit profiling (for diagnostics or cache refresh)
    BreakEvenDecision Profile(const WorkloadProfile& profile);
    
    // Thresholds
    void SetSpeedupThreshold(KernelType kernel, float threshold);
    float GetSpeedupThreshold(KernelType kernel) const;
    
    // Cache management
    void ClearCache();
    void SetCacheValidityDuration(std::chrono::hours duration);
    
    // Metrics access (for Prometheus export)
    float GetLatestBreakEvenRatio(KernelType kernel) const;
    size_t GetCacheHitCount() const;
    size_t GetCacheMissCount() const;
    
private:
    // Profile CPU path
    std::chrono::milliseconds ProfileCPU(const WorkloadProfile& profile);
    
    // Profile GPU path (if GPU available)
    std::optional<std::chrono::milliseconds> ProfileGPU(const WorkloadProfile& profile);
    
    // Cache key
    std::string MakeCacheKey(const WorkloadProfile& profile) const;
    
    // Data members
    std::unordered_map<std::string, BreakEvenDecision> decision_cache_;
    std::unordered_map<KernelType, float> speedup_thresholds_;
    std::chrono::hours cache_validity_duration_;
    size_t cache_hits_ = 0;
    size_t cache_misses_ = 0;
};

}  // namespace themis::acceleration
```

### 3.2 Implementation Skeleton

**File**: `src/acceleration/break_even_validator.cc`

```cpp
#include "acceleration/break_even_validator.h"

#include <algorithm>
#include <mutex>
#include "metrics/prometheus.h"
#include "gpu/cuda_runtime.h"
#include "cpu/reference_kernels.h"

namespace themis::acceleration {

BreakEvenValidator::BreakEvenValidator() 
    : cache_validity_duration_(std::chrono::hours(24)) {
    // Initialize thresholds
    speedup_thresholds_[KernelType::kDistance] = 1.5f;
    speedup_thresholds_[KernelType::kTopK] = 1.5f;
    speedup_thresholds_[KernelType::kBFS] = 1.3f;
    speedup_thresholds_[KernelType::kDijkstra] = 1.3f;
    speedup_thresholds_[KernelType::kGeoDistance] = 1.3f;
    speedup_thresholds_[KernelType::kGeoContainment] = 1.3f;
}

BreakEvenDecision BreakEvenValidator::ShouldUseGPU(
    const WorkloadProfile& profile) {
    // Honor explicit caller override
    if (profile.force_gpu) {
        return {true, 0.0f, {}, {}, "force_gpu_flag"};
    }
    if (profile.prefer_cpu) {
        return {false, 0.0f, {}, {}, "prefer_cpu_flag"};
    }
    
    // Check cache
    std::string cache_key = MakeCacheKey(profile);
    {
        std::lock_guard lock(cache_mu_);  // Assuming mutex added
        if (decision_cache_.count(cache_key)) {
            auto& cached = decision_cache_[cache_key];
            // Check expiry (simplified)
            cache_hits_++;
            return cached;
        }
    }
    
    // Cache miss: profile both paths
    cache_misses_++;
    return Profile(profile);
}

BreakEvenDecision BreakEvenValidator::Profile(
    const WorkloadProfile& profile) {
    // Profile CPU
    auto cpu_time = ProfileCPU(profile);
    
    // Profile GPU (if available)
    auto gpu_time = ProfileGPU(profile);
    if (!gpu_time) {
        return {false, 0.0f, cpu_time, {}, "gpu_unavailable"};
    }
    
    // Compute speedup
    float speedup_ratio = static_cast<float>(cpu_time.count()) /
                          static_cast<float>(gpu_time->count());
    float threshold = GetSpeedupThreshold(profile.kernel_type);
    
    BreakEvenDecision decision;
    decision.cpu_time_ms = cpu_time;
    decision.gpu_time_ms = gpu_time.value();
    decision.speedup_ratio = speedup_ratio;
    decision.use_gpu = (speedup_ratio >= threshold);
    decision.reason = decision.use_gpu ? "break_even_met" : "break_even_not_met";
    
    // Export metrics
    metrics::prometheus::RecordHistogram(
        "gpu_acceleration_break_even_ratio",
        speedup_ratio,
        {{"kernel", ToString(profile.kernel_type)}});
    
    // Cache decision
    {
        std::lock_guard lock(cache_mu_);
        decision_cache_[MakeCacheKey(profile)] = decision;
    }
    
    return decision;
}

std::chrono::milliseconds BreakEvenValidator::ProfileCPU(
    const WorkloadProfile& profile) {
    // Delegate to CPU reference kernel implementation
    // Example pseudocode:
    // auto start = std::chrono::high_resolution_clock::now();
    // cpu_kernels::L2Distance(inputs, outputs);
    // auto end = std::chrono::high_resolution_clock::now();
    // return std::chrono::duration_cast<std::chrono::milliseconds>(
    //     end - start);
    return std::chrono::milliseconds(0);  // TODO: implement
}

std::optional<std::chrono::milliseconds> BreakEvenValidator::ProfileGPU(
    const WorkloadProfile& profile) {
    // Delegate to GPU kernel implementation
    // Check GPU availability first
    // Measure: allocation + transfer + kernel + sync
    return std::nullopt;  // TODO: implement
}

void BreakEvenValidator::SetSpeedupThreshold(KernelType kernel, float threshold) {
    speedup_thresholds_[kernel] = std::max(1.0f, threshold);
}

float BreakEvenValidator::GetSpeedupThreshold(KernelType kernel) const {
    auto it = speedup_thresholds_.find(kernel);
    return it != speedup_thresholds_.end() ? it->second : 1.5f;
}

std::string BreakEvenValidator::MakeCacheKey(
    const WorkloadProfile& profile) const {
    return fmt::format("{}_{}_{:.2f}",
        ToString(profile.kernel_type),
        profile.input_size,
        profile.output_selectivity);
}

}  // namespace themis::acceleration
```

---

## 4. Benchmark Suite

### 4.1 Test Inputs

**File**: `tests/gpu/test_break_even_validation.cpp` (skeleton)

```cpp
#include <gtest/gtest.h>
#include "acceleration/break_even_validator.h"

namespace themis::acceleration::testing {

class BreakEvenValidatorTest : public ::testing::Test {
protected:
    BreakEvenValidator validator_;
    
    WorkloadProfile MakeProfile(
        KernelType kernel,
        size_t input_size,
        float selectivity = 1.0f) {
        return {
            .kernel_type = kernel,
            .input_size = input_size,
            .output_selectivity = selectivity,
            .vector_dimension = 128,
            .device = DeviceType::kNVIDIA_RTX,
        };
    }
};

// Category A: Distance kernels
TEST_F(BreakEvenValidatorTest, L2Distance_SmallInput_CPUPreferred) {
    auto profile = MakeProfile(KernelType::kDistance, 100);
    auto decision = validator_.ShouldUseGPU(profile);
    EXPECT_FALSE(decision.use_gpu);  // 100 vectors too small for GPU
}

TEST_F(BreakEvenValidatorTest, L2Distance_LargeInput_GPUPreferred) {
    auto profile = MakeProfile(KernelType::kDistance, 1'000'000);
    auto decision = validator_.ShouldUseGPU(profile);
    EXPECT_TRUE(decision.use_gpu);
    EXPECT_GE(decision.speedup_ratio, 1.5f);
}

// Category B: Graph algorithms
TEST_F(BreakEvenValidatorTest, BFS_SmallGraph_CPUPreferred) {
    auto profile = MakeProfile(KernelType::kBFS, 1000);
    auto decision = validator_.ShouldUseGPU(profile);
    EXPECT_FALSE(decision.use_gpu);  // Small graph, communication overhead dominates
}

TEST_F(BreakEvenValidatorTest, BFS_LargeGraph_GPUPreferred) {
    auto profile = MakeProfile(KernelType::kBFS, 100'000);
    auto decision = validator_.ShouldUseGPU(profile);
    if (decision.speedup_ratio >= 1.3f) {
        EXPECT_TRUE(decision.use_gpu);
    }
}

// Selectivity impact
TEST_F(BreakEvenValidatorTest, TopK_HighSelectivity_CPUFaster) {
    auto profile = MakeProfile(KernelType::kTopK, 1'000'000, 0.01f);  // 1% selectivity
    auto decision = validator_.ShouldUseGPU(profile);
    // High selectivity may not justify GPU allocation overhead
    EXPECT_LT(decision.speedup_ratio, 1.5f);
}

// Thresholds
TEST_F(BreakEvenValidatorTest, CustomThreshold_Applied) {
    validator_.SetSpeedupThreshold(KernelType::kDistance, 2.0f);
    EXPECT_EQ(validator_.GetSpeedupThreshold(KernelType::kDistance), 2.0f);
}

// Caching
TEST_F(BreakEvenValidatorTest, CachingReducesProfiling) {
    auto profile = MakeProfile(KernelType::kDistance, 100'000);
    
    // First call: cache miss, profiles both
    auto dec1 = validator_.ShouldUseGPU(profile);
    auto misses1 = validator_.GetCacheMissCount();
    
    // Second call: cache hit, no profiling
    auto dec2 = validator_.ShouldUseGPU(profile);
    auto hits2 = validator_.GetCacheHitCount();
    
    EXPECT_EQ(dec1.use_gpu, dec2.use_gpu);
    EXPECT_EQ(dec1.speedup_ratio, dec2.speedup_ratio);
    EXPECT_GT(hits2, 0);
}

// Force flags
TEST_F(BreakEvenValidatorTest, ForceGPU_OverridesBreakEven) {
    auto profile = MakeProfile(KernelType::kDistance, 100);  // Too small
    profile.force_gpu = true;
    auto decision = validator_.ShouldUseGPU(profile);
    EXPECT_TRUE(decision.use_gpu);
    EXPECT_EQ(decision.reason, "force_gpu_flag");
}

TEST_F(BreakEvenValidatorTest, PreferCPU_OverridesBreakEven) {
    auto profile = MakeProfile(KernelType::kDistance, 1'000'000);  // Large enough
    profile.prefer_cpu = true;
    auto decision = validator_.ShouldUseGPU(profile);
    EXPECT_FALSE(decision.use_gpu);
    EXPECT_EQ(decision.reason, "prefer_cpu_flag");
}

// Device variations
TEST_F(BreakEvenValidatorTest, Different_GPUs_Different_Thresholds) {
    auto profile_rtx = MakeProfile(KernelType::kDistance, 100'000);
    profile_rtx.device = DeviceType::kNVIDIA_RTX;
    
    auto profile_t4 = MakeProfile(KernelType::kDistance, 100'000);
    profile_t4.device = DeviceType::kNVIDIA_T4;
    
    auto dec_rtx = validator_.ShouldUseGPU(profile_rtx);
    auto dec_t4 = validator_.ShouldUseGPU(profile_t4);
    
    // RTX expected to have higher speedup
    EXPECT_GE(dec_rtx.speedup_ratio, dec_t4.speedup_ratio);
}

}  // namespace themis::acceleration::testing
```

---

## 5. Prometheus Metrics Integration

**Metrics to Export**:

| Metric | Type | Labels | Description |
|--------|------|--------|-------------|
| `gpu_acceleration_utilized` | Gauge | `kernel`, `device` | Boolean: GPU used for this kernel |
| `gpu_acceleration_break_even_ratio` | Histogram | `kernel`, `device` | CPU time / GPU time ratio |
| `gpu_acceleration_cpu_time_ms` | Histogram | `kernel` | CPU execution time |
| `gpu_acceleration_gpu_time_ms` | Histogram | `kernel` | GPU execution time (transfer + compute) |
| `gpu_acceleration_fallback_count` | Counter | `kernel`, `reason` | Fallback to CPU count |
| `gpu_acceleration_cache_hits` | Counter | — | Break-even decision cache hits |
| `gpu_acceleration_cache_misses` | Counter | — | Break-even decision cache misses |

**Example Usage in Code**:

```cpp
auto decision = validator_.ShouldUseGPU(profile);

if (decision.use_gpu) {
    metrics::prometheus::RecordGauge(
        "gpu_acceleration_utilized", 1.0,
        {{"kernel", KernelTypeToString(profile.kernel_type)}});
    metrics::prometheus::RecordHistogram(
        "gpu_acceleration_break_even_ratio", decision.speedup_ratio,
        {{"kernel", KernelTypeToString(profile.kernel_type)}});
} else {
    metrics::prometheus::RecordCounter(
        "gpu_acceleration_fallback_count", 1,
        {{"kernel", KernelTypeToString(profile.kernel_type)},
         {"reason", decision.reason}});
}
```

---

## 6. Integration Points

### 6.1 Query Planner Integration

When the query planner decides to use a GPU kernel (Category A or B), it must first check break-even:

```cpp
// In query planner
bool ShouldDispatchToGPU(const KernelCandidate& candidate) {
    auto profile = WorkloadProfile{
        .kernel_type = candidate.kernel_type,
        .input_size = candidate.input_cardinality,
        .output_selectivity = candidate.estimated_selectivity,
        .device = gpu_device_type_,
    };
    auto decision = break_even_validator_.ShouldUseGPU(profile);
    return decision.use_gpu;
}
```

### 6.2 Acceleration Layer Integration

When acceleration layer dispatches a kernel, it logs the decision:

```cpp
// In acceleration dispatcher
DispatchResult DispatchAccelerationKernel(
    const KernelRequest& request) {
    auto profile = WorkloadProfile::FromRequest(request);
    auto decision = validator_.ShouldUseGPU(profile);
    
    if (decision.use_gpu) {
        return DispatchToGPU(request);
    } else {
        return DispatchToCPU(request);
    }
}
```

---

## 7. Success Criteria for Phase D1

- [ ] `BreakEvenValidator` class implemented with full API
- [ ] CPU profiling functional (reference kernels timed)
- [ ] GPU profiling functional (CUDA timing with breakdown: alloc, transfer, exec, sync)
- [ ] All 40+ test cases pass
- [ ] Cache hit rate ≥ 90% on stable workloads
- [ ] Prometheus export functional
- [ ] Decision latency < 10ms on cache hit, < 100ms on cache miss
- [ ] Break-even ratios baseline established:
  - Category A (distance, TopK): 1.5x speedup for ≥ 10K vectors
  - Category B (BFS, Dijkstra): 1.3x speedup for ≥ 100K nodes
- [ ] Documentation: developer guide for using break-even validator in new kernels

---

## 8. Next Steps

1. **Implement BreakEvenValidator core** (Week 1-2 of Phase D)
   - Header + skeleton
   - CPU profiling (delegate to existing CPU kernels)
   - Cache mechanism
   
2. **Implement GPU profiling** (Week 2-3)
   - GPU timing with breakdown
   - Device enumeration + selection
   - Error handling
   
3. **Write test suite** (Week 3-4)
   - 40+ test cases covering all kernel types
   - Various input sizes and selectivities
   - Cache coherence tests
   
4. **Integrate with query planner + acceleration layer** (Week 4-5)
   - Modify dispatcher to call `ShouldUseGPU()`
   - Add Prometheus export
   - E2E testing with realistic workloads
   
5. **Baseline data collection** (Week 5-6)
   - Profile on multiple GPU types (RTX, A100, MI210)
   - Establish official speedup thresholds
   - Document break-even crossover points

---

**References**:
- `PHASE_D_PREPARATION_PLAN.md` — Phase D overview
- `TARGET_ARCHITECTURE.md` — Four-layer hybrid architecture
- `KERNEL_CLASSIFICATION_REVIEW.md` — Category A/B/C definitions
