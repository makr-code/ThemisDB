# PERFORMANCE_EXPECTATIONS - src/acceleration

## Scope
- Module: src/acceleration
- This file defines measurable Acceleration module performance expectations for release gating.

## Benchmark Reference
- Relevant benchmark files:
  - benchmarks/bench_acceleration_dispatch.cpp
  - benchmarks/bench_cuda_vs_cpu.cpp
  - benchmarks/bench_multi_gpu_scaling.cpp
  - benchmarks/bench_gpu_backends.cpp
  - benchmarks/bench_gpu_module.cpp
  - benchmarks/bench_gpu_hardware_capability.cpp

## Specific Expectations
| Target ID | Expectation | Benchmark case |
|---|---|---|
| ACC-1 | dispatch-path overhead remains within release baseline budget | BENCHMARK_REGISTER_F(AnnDispatchBenchFixture, L2Distance), BENCHMARK_REGISTER_F(AnnDispatchBenchFixture, CosineDistance), BENCHMARK_REGISTER_F(AnnDispatchBenchFixture, TopK) |
| ACC-2 | geo-dispatch overhead remains bounded | BENCHMARK_REGISTER_F(GeoDispatchBenchFixture, HaversineBatch) |
| ACC-3 | single-vs-multi-GPU scaling remains bounded | BM_SingleGPU_TrainingStep, BM_TwoGPU_DataParallel, BM_FourGPU_DataParallel |
| ACC-4 | gradient-sync and comm-compute overhead remains bounded | BM_GradientSync_Overhead, BM_CommCompute_Ratio |
| ACC-5 | backend distance-computation performance remains within release baseline budget | BM_CPUBackend_DistanceComputation, BM_CUDABackend_DistanceComputation, BM_HIPBackend_DistanceComputation, BM_VulkanBackend_DistanceComputation |
| ACC-6 | backend init and throughput behavior remains bounded | BM_BackendInitializationOverhead, BM_ThroughputComparison |
| ACC-7 | gpu-module memory and policy path overhead remains bounded | BM_MemoryManager_TryAllocate, BM_MemoryPool_Acquire_Release, BM_Policy_CheckAllowed, BM_Config_Validate |
| ACC-8 | capability-probe and load-balancer overhead remains bounded | BM_GPUHardwareCapabilityProbe, BM_GpuNVLinkTopologyDetect, BM_GpuLoadBalancer_LeastLoaded |
| ACC-9 | CUDA-vs-CPU acceleration ratio remains within release baseline budget | BM_CPU_ANN_L2Distance, BM_CUDA_ANN_L2Distance, BM_CPU_BatchKNN, BM_CUDA_BatchKNN |
| ACC-10 | fallback and capability-gate behavior remains bounded in disabled/guarded paths | BM_MultiGPUScaling_GPUDisabled, BM_GPUModule_GPUDisabled, BM_GpuP2PTransfer_FeatureGateCheck |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| AG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| AG-2 | dispatch and backend path p99 <= release threshold | p99 from mapped bench_acceleration_dispatch and bench_gpu_backends cases |
| AG-3 | multi-device path p99 <= release threshold | p99 from mapped bench_multi_gpu_scaling cases |
| AG-4 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation
- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- For proxy-only targets, keep follow-up benchmark hardening explicitly tracked.

## Sourcecode Verification (Module: acceleration/performance)

- Verified benchmark sources:
  - benchmarks/bench_acceleration_dispatch.cpp
  - benchmarks/bench_cuda_vs_cpu.cpp
  - benchmarks/bench_multi_gpu_scaling.cpp
  - benchmarks/bench_gpu_backends.cpp
  - benchmarks/bench_gpu_module.cpp
  - benchmarks/bench_gpu_hardware_capability.cpp
- Verified mapping surfaces:
  - dispatch and geo-dispatch benchmarks
  - backend and gpu-module performance benchmarks
  - multi-GPU scaling and overhead benchmarks
  - capability and fallback/feature-gate benchmarks
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.
