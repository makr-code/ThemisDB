# PERFORMANCE_EXPECTATIONS - src/gpu

<!-- Status: current | validated: 2026-08-17 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · PRODUCTION_REQUIREMENTS.md · SECURITY.md -->

## Scope

- Module: src/gpu
- This file defines measurable GPU module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_gpu_module.cpp
  - benchmarks/bench_gpu_backends.cpp
  - benchmarks/bench_gpu_vector_index.cpp
  - benchmarks/bench_gpu_training_cycle.cpp
  - benchmarks/bench_gpu_hardware_capability.cpp
  - benchmarks/bench_gpu_erasure.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| GPUP-1 | memory manager control-path operations remain bounded | BM_MemoryManager_TryAllocate, BM_MemoryManager_Deallocate, BM_MemoryManager_GetStats |
| GPUP-2 | memory pool and concurrent allocation behavior remain stable under load | BM_MemoryPool_Acquire_Release, BM_MemoryManager_ConcurrentAlloc |
| GPUP-3 | policy, config, metrics, and admin control-plane checks remain bounded | BM_Policy_CheckAllowed, BM_Policy_CheckDenied, BM_Config_Validate, BM_Config_SimulateAllocation, BM_Metrics_RecordAllocSuccess, BM_Metrics_RecordFallback, BM_Metrics_Snapshot, BM_AdminAPI_GetStatsJson, BM_AdminAPI_SimulateJson |
| GPUP-4 | backend distance-computation and throughput comparison remain bounded | BM_CPUBackend_DistanceComputation, BM_CUDABackend_DistanceComputation, BM_HIPBackend_DistanceComputation, BM_VulkanBackend_DistanceComputation, BM_BackendComparison_VaryingDimensions, BM_ThroughputComparison |
| GPUP-5 | vector index build/search and batch search paths remain bounded | BM_IndexBuild_CPU, BM_IndexBuild_VULKAN, BM_Search_CPU, BM_Search_VULKAN, BM_BatchSearch_CPU, BM_BatchSearch_VULKAN, BM_ANN_ExplicitVulkanPath |
| GPUP-6 | training cycle and end-to-end training step execution remain bounded | BM_TrainingCycle_CPU_Baseline, BM_TrainingCycle_CUDA, BM_TrainingCycle_HIP, BM_TrainingCycle_Vulkan, BM_CompleteTrainingStep_CUDA |
| GPUP-7 | hardware capability and topology-related helper paths remain bounded | BM_GPUHardwareCapabilityProbe, BM_GpuP2PTransfer_CPUFallback, BM_GpuNVLinkTopologyDetect, BM_GpuNVLinkScheduleSelect, BM_GpuLoadBalancer_RoundRobin, BM_GpuLoadBalancer_LeastLoaded |
| GPUP-8 | GPU erasure encode paths remain bounded across sizes and redundancy modes | BM_CPU_Encode_1MB, BM_CPU_Encode_10MB, BM_CPU_Encode_100MB, BM_GPU_Encode_1MB, BM_GPU_Encode_10MB, BM_GPU_Encode_100MB, BM_GPU_BatchEncode_Small, BM_GPU_BatchEncode_Medium, BM_GPU_Encode_LowRedundancy, BM_GPU_Encode_HighRedundancy |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| GPUG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| GPUG-2 | GPU hot-path p99 <= release threshold | p99 from mapped GPU benchmark cases |
| GPUG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded as additional GPU benchmark scenarios are introduced.

## Sourcecode Verification (Module: gpu/performance)

- Verified benchmark sources:
  - benchmarks/bench_gpu_module.cpp
  - benchmarks/bench_gpu_backends.cpp
  - benchmarks/bench_gpu_vector_index.cpp
  - benchmarks/bench_gpu_training_cycle.cpp
  - benchmarks/bench_gpu_hardware_capability.cpp
  - benchmarks/bench_gpu_erasure.cpp
- Verified mapping surfaces:
  - memory manager/pool/concurrency, control-plane checks, and admin/metrics paths
  - backend comparison and throughput paths
  - vector index and training cycle paths
  - hardware capability, topology/load-balancer, and erasure encoding paths
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.