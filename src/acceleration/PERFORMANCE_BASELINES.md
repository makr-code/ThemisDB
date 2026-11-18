---
Author: "ThemisDB Team"
Created: "2026-11-16"
Last Updated: "2026-11-16"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Performance Baselines - Acceleration Module

> **Evidence status (2026-11-16):** `benchmarks/baselines/acceleration/baseline.json` exists, but its own metadata describes representative 2-core CI values and uses `commit: initial` with a fixed timestamp; it is not an approved measured release baseline. No release run manifest or reproducible hardware-profile results were found. Treat the numeric values below as candidate targets only. `test_acceleration_performance_gates.cpp` times proxy calculations and `bench_acceleration_dedicated_gates.cpp` benchmarks local stubs; neither measures production backend selection or fallback latency.

This document records candidate performance targets and the evidence required before they can become release gates.

### Baseline Methodology

- **Candidate Source**: Production-path benchmarks listed in `src/acceleration/PERFORMANCE_EXPECTATIONS.md`
- **Existing CPU artifact**: `benchmarks/baselines/acceleration/baseline.json` is representative sample data, not a release measurement
- **Measurement Environment**: Must be recorded per run (binary/build flags, CPU/GPU model, driver/runtime, workload dimensions, warmup/repetition settings)
- **Regression Threshold**: Candidate ≤ 10% vs an approved profile-specific baseline (AG-1); not currently validated
- **Revalidation Cadence**: Per release cycle and after performance-critical changes

### Candidate Dispatch and Backend Targets

#### ACC-1: Dispatch Path Overhead
- **Baseline**: ≤ 5 µs per dispatch call (CPU dispatch, no GPU needed)
- **Gate**: p99 dispatch latency ≤ 1.5x baseline
- **Benchmark**: BENCHMARK_REGISTER_F(AnnDispatchBenchFixture, L2Distance)
- **Status**: NOT VERIFIED

#### ACC-2: Geo-Distance Dispatch
- **Baseline**: ≤ 10 µs per geo dispatch (Haversine, lat/lon bounds check)
- **Gate**: p99 geo dispatch latency ≤ 15 µs
- **Benchmark**: BENCHMARK_REGISTER_F(GeoDispatchBenchFixture, HaversineBatch)
- **Status**: NOT VERIFIED

#### ACC-3: Multi-GPU Scaling (CPU → 1 GPU → 2 GPU → 4 GPU)
- **Baseline**: Scaling efficiency ≥ 80% per additional GPU (linear speedup target)
- **Gate**: 4-GPU throughput ≥ 3.2x single-GPU throughput
- **Benchmark**: BM_SingleGPU_TrainingStep, BM_TwoGPU_DataParallel, BM_FourGPU_DataParallel
- **Status**: NOT VERIFIED

#### ACC-4: Gradient Sync and Comm-Compute Overlap
- **Baseline**: Comm overhead ≤ 15% of compute time
- **Gate**: GradientSync overhead ≤ 20% of total training time
- **Benchmark**: BM_GradientSync_Overhead, BM_CommCompute_Ratio
- **Status**: NOT VERIFIED

#### ACC-5: Backend Distance Computation Performance
- **Baseline CPU (L2)**: ~0.5 µs per distance pair
- **Baseline CUDA (L2)**: ~0.01 µs per distance pair (50x speedup over CPU)
- **Baseline HIP (L2)**: ~0.02 µs per distance pair (25x speedup over CPU)
- **Baseline Vulkan (L2)**: ~0.05 µs per distance pair (10x speedup over CPU)
- **Gate**: CUDA ≥ 40x speedup, HIP ≥ 20x speedup, Vulkan ≥ 8x speedup
- **Benchmark**: BM_CPUBackend_DistanceComputation, BM_CUDABackend_DistanceComputation, BM_HIPBackend_DistanceComputation, BM_VulkanBackend_DistanceComputation
- **Status**: NOT VERIFIED

#### ACC-6: Backend Initialization and Throughput
- **Baseline Init (CPU)**: ≤ 10 ms
- **Baseline Init (CUDA)**: ≤ 100 ms
- **Baseline Init (HIP)**: ≤ 120 ms
- **Gate**: Backend init ≤ 2x baseline after first successful init
- **Benchmark**: BM_BackendInitializationOverhead, BM_ThroughputComparison
- **Status**: NOT VERIFIED

#### ACC-7: GPU Module Memory and Policy Overhead
- **Baseline Memory Allocation**: ≤ 100 µs per 1MB block
- **Baseline Policy Check**: ≤ 1 µs per check
- **Gate**: No unbounded growth, bounded to baseline + 10%
- **Benchmark**: BM_MemoryManager_TryAllocate, BM_MemoryPool_Acquire_Release, BM_Policy_CheckAllowed
- **Status**: NOT VERIFIED

#### ACC-8: Capability Probe and Load Balancer Overhead
- **Baseline Capability Probe**: ≤ 50 ms (one-time startup cost)
- **Baseline Load Balancer**: ≤ 5 µs per decision
- **Gate**: No capability re-probing without explicit trigger
- **Benchmark**: BM_GPUHardwareCapabilityProbe, BM_GpuNVLinkTopologyDetect, BM_GpuLoadBalancer_LeastLoaded
- **Status**: NOT VERIFIED

#### ACC-9: CUDA vs CPU Acceleration Ratio
- **Baseline CUDA Speedup**: ≥ 40x over CPU for vector ops
- **Baseline CUDA Throughput**: ≥ 80k ops/sec on typical hardware
- **Gate**: CUDA speedup maintained ≥ 35x in regression runs
- **Benchmark**: BM_CPU_ANN_L2Distance, BM_CUDA_ANN_L2Distance, BM_CPU_BatchKNN, BM_CUDA_BatchKNN
- **Status**: NOT VERIFIED

#### ACC-10: Fallback and Feature-Gate Overhead
- **Baseline Fallback Cost**: ≤ 2% when disabled
- **Baseline Feature-Gate Check**: ≤ 0.5 µs per check
- **Gate**: No hidden fallback paths, all explicit and measurable
- **Benchmark**: BM_MultiGPUScaling_GPUDisabled, BM_GPUModule_GPUDisabled, BM_GpuP2PTransfer_FeatureGateCheck
- **Status**: NOT VERIFIED

### Hard Release Gates

| Gate ID | Expectation | Threshold | Status |
|---------|-------------|-----------|--------|
| AG-1 | Regression ≤ 10% vs approved profile baseline | (current - baseline) / baseline | PENDING baseline artifact |
| AG-2 | Dispatch/backend p99 ≤ approved profile threshold | Production-path dispatch and backend benchmark output | PENDING production-path evidence |
| AG-3 | Multi-device p95/p99 and scaling within approved envelope | Real multi-device workload on named topology | PENDING hardware evidence |
| AG-4 | Every required benchmark is present and completed | Versioned run manifest linked to raw benchmark output | PENDING manifest |

### Performance Regression Detection

**Automated Detection**: The current unit tests do not validate these baselines. Release detection requires comparing raw output from the production-path benchmarks against a versioned, profile-specific baseline and preserving the run manifest.

**Manual Verification**: Run the production-path benchmarks listed in `PERFORMANCE_EXPECTATIONS.md` on a named release profile. Preserve their raw output, benchmark arguments, build flags, host and driver/runtime details, and a complete manifest; the unit-test CTest command is not performance evidence.

### Performance Anomaly Response

If regression detected:
1. **Determine root cause** (code change, compiler flags, hardware variance)
2. **Measure impact** (is it acceptable within variance budget?)
3. **Decide action** (revert, optimize, or establish new baseline with justification)
4. **Document** (update this file with new baseline and justification)

### Known Performance Constraints

- **Hardware Dependent**: Actual performance depends on GPU model, driver version, CUDA/HIP versions
- **Variance Budget**: Not established; define it per approved hardware profile before interpreting regressions
- **Warmup Runs**: Record benchmark-specific warmup and repetition settings in each release manifest
- **Rng Seed**: Record benchmark-specific seeds when applicable; no module-wide seed is established

### Maintenance and Updates

- **Last Updated**: 2026-11-16
- **Baseline Epoch**: Not established; Q3 2026 values remain unverified candidate targets
- **Next Review**: After each major backend change or hardware profile update
- **Owner**: Acceleration module maintainers
