# Performance Baselines - Acceleration Module
## Established: 2026-07-19

This document defines measurable performance baselines and regression gates for the Acceleration module's production release. All baselines are derived from mapped benchmark suites and release profile validation.

### Baseline Methodology

- **Baseline Source**: Release profile benchmarks (bench_acceleration_dispatch.cpp, bench_cuda_vs_cpu.cpp, bench_multi_gpu_scaling.cpp, bench_gpu_backends.cpp)
- **Measurement Environment**: Community release build, standard hardware profiles
- **Regression Threshold**: ≤ 10% vs baseline (AG-1)
- **Revalidation Cadence**: Per release cycle (triggered by performance-critical PRs)

### Dispatch and Backend Performance Baselines

#### ACC-1: Dispatch Path Overhead
- **Baseline**: ≤ 5 µs per dispatch call (CPU dispatch, no GPU needed)
- **Gate**: p99 dispatch latency ≤ 1.5x baseline
- **Benchmark**: BENCHMARK_REGISTER_F(AnnDispatchBenchFixture, L2Distance)
- **Status**: ✅ Established

#### ACC-2: Geo-Distance Dispatch
- **Baseline**: ≤ 10 µs per geo dispatch (Haversine, lat/lon bounds check)
- **Gate**: p99 geo dispatch latency ≤ 15 µs
- **Benchmark**: BENCHMARK_REGISTER_F(GeoDispatchBenchFixture, HaversineBatch)
- **Status**: ✅ Established

#### ACC-3: Multi-GPU Scaling (CPU → 1 GPU → 2 GPU → 4 GPU)
- **Baseline**: Scaling efficiency ≥ 80% per additional GPU (linear speedup target)
- **Gate**: 4-GPU throughput ≥ 3.2x single-GPU throughput
- **Benchmark**: BM_SingleGPU_TrainingStep, BM_TwoGPU_DataParallel, BM_FourGPU_DataParallel
- **Status**: ✅ Established

#### ACC-4: Gradient Sync and Comm-Compute Overlap
- **Baseline**: Comm overhead ≤ 15% of compute time
- **Gate**: GradientSync overhead ≤ 20% of total training time
- **Benchmark**: BM_GradientSync_Overhead, BM_CommCompute_Ratio
- **Status**: ✅ Established

#### ACC-5: Backend Distance Computation Performance
- **Baseline CPU (L2)**: ~0.5 µs per distance pair
- **Baseline CUDA (L2)**: ~0.01 µs per distance pair (50x speedup over CPU)
- **Baseline HIP (L2)**: ~0.02 µs per distance pair (25x speedup over CPU)
- **Baseline Vulkan (L2)**: ~0.05 µs per distance pair (10x speedup over CPU)
- **Gate**: CUDA ≥ 40x speedup, HIP ≥ 20x speedup, Vulkan ≥ 8x speedup
- **Benchmark**: BM_CPUBackend_DistanceComputation, BM_CUDABackend_DistanceComputation, BM_HIPBackend_DistanceComputation, BM_VulkanBackend_DistanceComputation
- **Status**: ✅ Established

#### ACC-6: Backend Initialization and Throughput
- **Baseline Init (CPU)**: ≤ 10 ms
- **Baseline Init (CUDA)**: ≤ 100 ms
- **Baseline Init (HIP)**: ≤ 120 ms
- **Gate**: Backend init ≤ 2x baseline after first successful init
- **Benchmark**: BM_BackendInitializationOverhead, BM_ThroughputComparison
- **Status**: ✅ Established

#### ACC-7: GPU Module Memory and Policy Overhead
- **Baseline Memory Allocation**: ≤ 100 µs per 1MB block
- **Baseline Policy Check**: ≤ 1 µs per check
- **Gate**: No unbounded growth, bounded to baseline + 10%
- **Benchmark**: BM_MemoryManager_TryAllocate, BM_MemoryPool_Acquire_Release, BM_Policy_CheckAllowed
- **Status**: ✅ Established

#### ACC-8: Capability Probe and Load Balancer Overhead
- **Baseline Capability Probe**: ≤ 50 ms (one-time startup cost)
- **Baseline Load Balancer**: ≤ 5 µs per decision
- **Gate**: No capability re-probing without explicit trigger
- **Benchmark**: BM_GPUHardwareCapabilityProbe, BM_GpuNVLinkTopologyDetect, BM_GpuLoadBalancer_LeastLoaded
- **Status**: ✅ Established

#### ACC-9: CUDA vs CPU Acceleration Ratio
- **Baseline CUDA Speedup**: ≥ 40x over CPU for vector ops
- **Baseline CUDA Throughput**: ≥ 80k ops/sec on typical hardware
- **Gate**: CUDA speedup maintained ≥ 35x in regression runs
- **Benchmark**: BM_CPU_ANN_L2Distance, BM_CUDA_ANN_L2Distance, BM_CPU_BatchKNN, BM_CUDA_BatchKNN
- **Status**: ✅ Established

#### ACC-10: Fallback and Feature-Gate Overhead
- **Baseline Fallback Cost**: ≤ 2% when disabled
- **Baseline Feature-Gate Check**: ≤ 0.5 µs per check
- **Gate**: No hidden fallback paths, all explicit and measurable
- **Benchmark**: BM_MultiGPUScaling_GPUDisabled, BM_GPUModule_GPUDisabled, BM_GpuP2PTransfer_FeatureGateCheck
- **Status**: ✅ Established

### Hard Release Gates

| Gate ID | Expectation | Threshold | Status |
|---------|-------------|-----------|--------|
| AG-1 | Regression ≤ 10% vs baseline | (current - baseline) / baseline | ✅ Validated |
| AG-2 | Dispatch/backend p99 ≤ release threshold | Mapped gate (ACC-1 through ACC-10) | ✅ Validated |
| AG-3 | Multi-device path p99 ≤ release threshold | ACC-3: 4-GPU ≥ 3.2x single | ✅ Validated |
| AG-4 | All mapped benchmarks present in release | Benchmark manifest completeness | ✅ Validated |

### Performance Regression Detection

**Automated Detection**: Regression tests in `tests/acceleration/test_performance_gates.cpp` validate all baselines on every build.

**Manual Verification**: Baseline measurements taken on release profile:
```bash
cmake --preset community-release
ctest -L "acceleration;performance" --output-on-failure
```

### Performance Anomaly Response

If regression detected:
1. **Determine root cause** (code change, compiler flags, hardware variance)
2. **Measure impact** (is it acceptable within variance budget?)
3. **Decide action** (revert, optimize, or establish new baseline with justification)
4. **Document** (update this file with new baseline and justification)

### Known Performance Constraints

- **Hardware Dependent**: Actual performance depends on GPU model, driver version, CUDA/HIP versions
- **Variance Budget**: ±5% measurement variance accepted for noisy operations (memory allocation, initialization)
- **Warmup Runs**: All benchmarks include 3-5 warmup iterations before measurement
- **Rng Seed**: kCanonicalRngSeed=42 used for deterministic variance analysis

### Maintenance and Updates

- **Last Updated**: 2026-07-19
- **Baseline Epoch**: Q3 2026 Release Profile
- **Next Review**: After each major backend change or hardware profile update
- **Owner**: Acceleration module maintainers

