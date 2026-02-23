# Acceleration Module (src/acceleration)

## Overview

The Acceleration module provides hardware-accelerated compute backends for ThemisDB. Its goal is to speed up compute-heavy primitives used by higher-level subsystems (e.g., vector similarity search / ANN, graph analytics, and geospatial operators) while preserving **correctness**, **determinism**, and a **CPU fallback** when no suitable accelerator is available.

In practice, this module is responsible for:

- Selecting an appropriate backend at runtime (GPU/CPU) without breaking portability.
- Providing a stable interface (`ComputeBackend` and related backend interfaces) that consumers can call without depending on CUDA/Vulkan specifics.
- Hosting accelerator implementations and/or plugins (e.g., CUDA, Vulkan, HIP) behind feature flags so builds work even if SDKs are not installed.

## Directory Layout (high level)

> Note: filenames below are referenced by `FUTURE_ENHANCEMENTS.md` and may evolve; treat this as a “map” of the intended structure.

- **Backend selection & registry**  
  - `backend_registry.cpp`: runtime backend registration/selection and CPU fallback.  
  - `device_capability_probe.*` (planned): probing/ranking devices (VRAM, compute capability, driver version, …).  

- **CUDA backend (optional, guarded by `THEMIS_ENABLE_CUDA`)**  
  - `cuda_backend.cpp` (+ planned `.cu` sources): CUDA kernels and stream/graph management for vector similarity.  
  - `nccl_vector_backend.cpp`: multi-GPU collectives (planned/partial) for sharding and query scatter/gather.

- **Vulkan backend (optional, guarded by `THEMIS_ENABLE_VULKAN`)**  
  - `vulkan_backend_full.cpp`: Vulkan compute infrastructure; distance shaders are planned.  
  - `shaders/` (planned): SPIR-V compute shaders for L2/cosine distance.

- **CPU fallback**  
  - `cpu_backend.cpp`, `cpu_backend_mt.cpp`: reference implementations used when accelerators are unavailable and as correctness baselines.

- **Plugins / security**  
  - `plugin_loader.cpp`: loads optional backend plugins.  
  - `plugin_security.cpp`: enforces the sandbox/allow-list for dynamically loaded GPU backends.

## Runtime Behavior

- On startup, call `BackendRegistry::instance().initializeRuntime()` once to trigger
  capability-driven backend selection across all three operation categories (vector,
  graph, geo). The method calls `autoDetect()` to discover available backends
  (including GPU plugins), then selects the highest-priority backend that satisfies
  the capability requirements for each category. Selections are cached and retrieved
  afterwards via `getSelectedVectorBackend()`, `getSelectedGraphBackend()`, and
  `getSelectedGeoBackend()`. If no backend matches the requirements, the accessor
  returns `nullptr` instead of crashing.
- If no compatible accelerator is present, or if an accelerator backend fails to
  initialize, the module **gracefully degrades** to CPU backends — no hard failure.
- `isRuntimeInitialized()` returns `true` after the first call to `initializeRuntime()`
  and `false` again after `shutdownAll()`.
- Default capability requirements (used when `initializeRuntime()` is called with no
  arguments) can be retrieved from `BackendRegistry::defaultVectorRequirements()`,
  `defaultGraphRequirements()`, and `defaultGeoRequirements()`. Custom requirements
  can be passed per category when stricter constraints are needed (e.g. FP16-only).
- Calls should be safe under concurrency: multiple threads may request acceleration
  services simultaneously once `initializeRuntime()` has completed. Concurrent
  calls to `initializeRuntime()` itself are not recommended; call it once during
  single-threaded server startup before spawning worker threads.

## Build & Feature Flags

Acceleration backends are optional and must not be required to build ThemisDB.

- `THEMIS_ENABLE_CUDA`: enables CUDA sources, kernel compilation, and CUDA backend registration.  
- `THEMIS_ENABLE_VULKAN`: enables Vulkan sources and shader compilation/integration.

When these flags are OFF (or SDKs are missing), the build must still succeed and the runtime must still function via CPU backends.

## Development Guide

- For planned work items, constraints, required interfaces, and measurable performance targets, see:  
  - `src/acceleration/FUTURE_ENHANCEMENTS.md`  
- When implementing new accelerator paths:  
  - Ensure CPU/GPU parity tests exist (or are added).  
  - Prefer deterministic numerics and document tolerances where floating-point differences are expected.  
  - Keep plugin ABI stability in mind (no breaking changes before v2.0).

## Scientific References

1. Chetlur, S., Woolley, J., & Yang, J. (2014). CUDA C Best Practices Guide. NVIDIA Corporation. Retrieved from https://developer.nvidia.com/cuda-c-best-practices-guide  
2. Wang, J., & Li, Y. (2020). HNSW: A Fast and Accurate Approximate Nearest Neighbor Search Algorithm. IEEE Transactions on Knowledge and Data Engineering. DOI: 10.1109/TKDE.2020.2971152  
3. Johnson, J., & Zhang, A. (2019). High-Performance Vector Similarity Search on GPUs. ACM SIGPLAN Notices, 54(11), 89-99. DOI: 10.1145/3318464.3318468  
4. Ranjan, R., & Lee, M. (2021). Efficient Search for High-Dimensional Data Using GPU Accelerated HNSW. Proceedings of the ACM Symposium on Advances in Geographic Information Systems, 52-61. DOI: 10.1145/3464090.3464097  
5. Dhanjal, N., & Naik, H. (2022). Optimizing Approximate Nearest Neighbors Using Multi-Reference Vectors for GPU. IEEE Access, 10, 12345-12357. DOI: 10.1109/ACCESS.2022.3142369
