### Context

This issue implements the roadmap item 'Kernel Block-Dimension Occupancy Tuning' for the acceleration domain. It is sourced from the consolidated roadmap under 🟢 Low Priority — Future (v1.9.0+) and targets milestone v1.9.0.

Primary detail section: Kernel Block-Dimension Occupancy Tuning

### Goal

Deliver the scoped changes for Kernel Block-Dimension Occupancy Tuning in src/acceleration/ and complete the linked detail section in a release-ready state for v1.9.0.

### Detailed Scope

### Kernel Block-Dimension Occupancy Tuning
**Priority:** Medium
**Target Version:** v1.9.0

Multiple CUDA and HIP kernel launchers use hard-coded block dimensions that are not tuned for actual device occupancy:
- `cuda/ann_kernels.cu:366`: `constexpr int kThreadsPerBlock = 256;`
- `cuda/vector_kernels.cu:359`: `int threadsPerBlock = 256;`
- `cuda/geo_kernels.cu:151,181`: `constexpr int kBlockSize = 256;`
- `cuda/graph_kernels.cu:248`: `static constexpr int kBFSBlockDim = 256;`
- `hip/ann_kernels.hip:367`: `constexpr int kThreadsPerBlock = 256;`
- `hip/geo_kernels.hip:154,184`: `constexpr int kBlockSize = 256;`
- `hip_backend.cpp:602`: `int threadsPerBlock = 256;`

A fixed block size of 256 is a reasonable default for NVIDIA sm_86 and AMD RDNA2, but may underperform on GPUs with 64-thread wavefronts (AMD GCN2) or on sm_90 (Hopper) where 128-thread blocks better utilize the warp scheduler.

**Implementation Notes:**
- `[ ]` Replace hard-coded `threadsPerBlock = 256` in `cuda/vector_kernels.cu:359` and `hip_backend.cpp:602` with a runtime call to `cudaOccupancyMaxPotentialBlockSize()` / `hipOccupancyMaxPotentialBlockSize()` at `initialize()` time; store the result in the backend's `Impl` struct and pass it to all kernel launches.
- `[ ]` For `constexpr` block sizes in `.cu`/`.hip` files (`ann_kernels.cu`, `geo_kernels.cu`, `graph_kernels.cu`), expose a launch wrapper that accepts `threadsPerBlock` as a parameter and is called from the backend with the occupancy-tuned value rather than hard-coding the constant at the launch site.
- `[ ]` For AMD GCN targets (wavefront = 64): default to 64 threads when `hipGetDeviceProperties().warpSize == 64` to avoid half-occupancy.
- `[ ]` Vulkan `l2_distance.comp` hard-codes `layout(local_size_x = 16, local_size_y = 16)`: expose this as a specialization constant (`layout(constant_id = 0) const uint LOCAL_SIZE_X = 16`) so the `VulkanVectorBackend` can inject the optimal value for the target device via `VkSpecializationInfo` at pipeline creation time.
- `[ ]` Add a micro-benchmark (`benchmarks/kernel_block_size_bench.cpp`) that sweeps block sizes 64/128/256/512 for each kernel and reports achieved occupancy.

**Performance Targets:**
- ≥ 5% throughput improvement on AMD RDNA2 (wavefront=32) vs. 256-thread baseline.
- No regression on NVIDIA sm_86/sm_89 (Ampere/Ada).

---

### Acceptance Criteria

- [ ] `cuda/ann_kernels.cu:366`: `constexpr int kThreadsPerBlock = 256;`
- [ ] `cuda/vector_kernels.cu:359`: `int threadsPerBlock = 256;`
- [ ] `cuda/geo_kernels.cu:151,181`: `constexpr int kBlockSize = 256;`
- [ ] `cuda/graph_kernels.cu:248`: `static constexpr int kBFSBlockDim = 256;`
- [ ] `hip/ann_kernels.hip:367`: `constexpr int kThreadsPerBlock = 256;`
- [ ] `hip/geo_kernels.hip:154,184`: `constexpr int kBlockSize = 256;`
- [ ] `hip_backend.cpp:602`: `int threadsPerBlock = 256;`
- [ ] Replace hard-coded `threadsPerBlock = 256` in `cuda/vector_kernels.cu:359` and `hip_backend.cpp:602` with a runtime call to `cudaOccupancyMaxPotentialBlockSize()` / `hipOccupancyMaxPotentialBlockSize()` at `initialize()` time; store the result in the backend's `Impl` struct and pass it to all kernel launches.
- [ ] For `constexpr` block sizes in `.cu`/`.hip` files (`ann_kernels.cu`, `geo_kernels.cu`, `graph_kernels.cu`), expose a launch wrapper that accepts `threadsPerBlock` as a parameter and is called from the backend with the occupancy-tuned value rather than hard-coding the constant at the launch site.
- [ ] For AMD GCN targets (wavefront = 64): default to 64 threads when `hipGetDeviceProperties().warpSize == 64` to avoid half-occupancy.
- [ ] Vulkan `l2_distance.comp` hard-codes `layout(local_size_x = 16, local_size_y = 16)`: expose this as a specialization constant (`layout(constant_id = 0) const uint LOCAL_SIZE_X = 16`) so the `VulkanVectorBackend` can inject the optimal value for the target device via `VkSpecializationInfo` at pipeline creation time.
- [ ] Add a micro-benchmark (`benchmarks/kernel_block_size_bench.cpp`) that sweeps block sizes 64/128/256/512 for each kernel and reports achieved occupancy.
- [ ] ≥ 5% throughput improvement on AMD RDNA2 (wavefront=32) vs. 256-thread baseline.
- [ ] No regression on NVIDIA sm_86/sm_89 (Ampere/Ada).

### Relationships

- Roadmap row: #234 (🟢 Low Priority — Future (v1.9.0+))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/acceleration/FUTURE_ENHANCEMENTS.md#kernel-block-dimension-occupancy-tuning
- Source key: roadmap:234:acceleration:v1.9.0:kernel-block-dimension-occupancy-tuning

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:234:acceleration:v1.9.0:kernel-block-dimension-occupancy-tuning -->
<!-- roadmap-ref: row=234;module=acceleration;target=v1.9.0 -->
<!-- roadmap-detail: src/acceleration/FUTURE_ENHANCEMENTS.md#kernel-block-dimension-occupancy-tuning -->
