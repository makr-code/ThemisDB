# GPU Performance Expectations

## Scope and evidence standard

This page maps GPU workload areas to benchmark sources for developers and
release reviewers. A benchmark declaration establishes a measurement case, not
a measured result, universal performance target, or successful run. Results
must identify build options, backend, device/driver, workload, and whether data
transfer is included. CPU/stub paths do not establish physical-GPU performance.

## Current benchmark mapping

The source files below are under `benchmarks/gpu/`. The cases listed are
declared in those sources; availability of the matching CMake target depends on
the repository build configuration and required backend/toolkit.

| Area | Source | Representative cases |
|---|---|---|
| Memory manager, pool, policy, config, metrics, admin | `bench_gpu_module.cpp` | `BM_MemoryManager_TryAllocate`, `BM_MemoryManager_Deallocate`, `BM_MemoryManager_GetStats`, `BM_MemoryPool_Acquire_Release`, `BM_Policy_CheckAllowed`, `BM_Config_Validate`, `BM_Metrics_Snapshot`, `BM_AdminAPI_GetStatsJson` |
| Backend comparisons | `bench_gpu_backends.cpp` | `BM_CPUBackend_DistanceComputation`, `BM_CUDABackend_DistanceComputation`, `BM_HIPBackend_DistanceComputation`, `BM_VulkanBackend_DistanceComputation`, `BM_BackendComparison_VaryingDimensions`, `BM_ThroughputComparison` |
| Vector index | `bench_gpu_vector_index.cpp` | `BM_IndexBuild_CPU`, `BM_IndexBuild_VULKAN`, `BM_Search_CPU`, `BM_Search_VULKAN`, `BM_BatchSearch_CPU`, `BM_BatchSearch_VULKAN`, `BM_ANN_ExplicitVulkanPath` |
| Training cycles | `bench_gpu_training_cycle.cpp` | `BM_TrainingCycle_CPU_Baseline`, `BM_TrainingCycle_CUDA`, `BM_TrainingCycle_HIP`, `BM_TrainingCycle_Vulkan`, `BM_CompleteTrainingStep_CUDA` |
| Capability, topology, transfer, load balancing | `bench_gpu_hardware_capability.cpp` | `BM_GPUHardwareCapabilityProbe`, `BM_GpuP2PTransfer_CPUFallback`, `BM_GpuNVLinkTopologyDetect`, `BM_GpuNVLinkScheduleSelect`, `BM_GpuLoadBalancer_RoundRobin`, `BM_GpuLoadBalancer_LeastLoaded` |
| Erasure encoding | `bench_gpu_erasure.cpp` | `BM_CPU_Encode_1MB`, `BM_GPU_Encode_1MB`, `BM_GPU_BatchEncode_Small`, `BM_GPU_Encode_LowRedundancy`, `BM_GPU_Encode_HighRedundancy` |

Additional registered benchmark sources include
`bench_gpu_phase2_phase3_gates.cpp`, `bench_gpu_dedicated_gates.cpp`,
`bench_gpu_cpu_breakeven_category_a.cpp`,
`bench_gpu_cpu_breakeven_category_b.cpp`, and
`bench_gpu_a8_baselines.cpp`. Their names and registration can be inspected in
`benchmarks/CMakeLists.txt` and `benchmarks/gpu/CMakeLists.txt`.

## Gate and target status

The CMake files register benchmark targets, including the named phase, dedicated,
and CPU/GPU break-even cases. This document does not establish a universal
10-percent regression limit, a numeric p99 limit, or a completeness gate for all
listed cases. No such universal thresholds are declared by these GPU benchmark
registrations. Use an approved release baseline and its separately defined
thresholds when evaluating results; do not report an unrun benchmark as passing.

## Validation

For a benchmark change, verify the source case and target registration in the
current CMake files, build with the intended backend configuration, and record
the run environment and output. A target that is omitted by configuration or
cannot run without hardware is not evidence of a passing GPU measurement.
