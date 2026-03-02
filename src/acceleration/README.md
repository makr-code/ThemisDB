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

- **Kernel dispatch & fallback/retry** (`include/acceleration/kernel_fallback_dispatcher.h`)  
  - `ANNKernelFallbackDispatcher`: wraps a primary `ANNKernelDispatch` table (GPU) and a fallback table (CPU). Null slots in the primary are routed directly to the fallback (unsupported kernel). Transient device errors (`DeviceLost`, `OperationTimeout`, `SynchronizationFailed`) are retried with exponential back-off; all other errors and exhausted retries also fall back.  
  - `GeoKernelFallbackDispatcher`: same semantics for the two geospatial kernel slots.  
  - `RetryPolicy`: configures `maxAttempts`, initial/max delay (ms), and back-off multiplier.

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

- For a deep-dive into capability negotiation, the fallback chain, kernel-level
  fallback/retry, health monitoring, and operational troubleshooting, see:  
  - `docs/acceleration/capability_negotiation.md`  
  - `docs/acceleration/troubleshooting.md` — operational troubleshooting guide (runbooks, diagnostics, platform-specific issues)  
- For planned work items, constraints, required interfaces, and measurable performance targets, see:  
  - `src/acceleration/FUTURE_ENHANCEMENTS.md`  
- When implementing new accelerator paths:  
  - Ensure CPU/GPU parity tests exist (or are added).  
  - Prefer deterministic numerics and document tolerances where floating-point differences are expected.  
  - Keep plugin ABI stability in mind (no breaking changes before v2.0).

## 📚 Scientific Foundations

The acceleration module is grounded in the following peer-reviewed research and industry specifications. Citations are in IEEE format.

### Research Papers

1. J. Johnson, M. Douze, and H. Jégou, "Billion-scale similarity search with GPUs," *IEEE Transactions on Big Data*, vol. 7, no. 3, pp. 535–547, 2021, doi: 10.1109/TBDATA.2019.2921572. [Online]. Available: https://faiss.ai/ [Accessed: 2026-02-22]  
   — **ThemisDB application:** `src/acceleration/faiss_gpu_backend.cpp`; GPU-accelerated vector search at billion-scale via the FAISS library.

2. Y. A. Malkov and D. A. Yashunin, "Efficient and robust approximate nearest neighbor search using Hierarchical Navigable Small World graphs," *IEEE Transactions on Pattern Analysis and Machine Intelligence*, vol. 42, no. 4, pp. 824–836, Apr. 2020, doi: 10.1109/TPAMI.2018.2889473. [Online]. Available: https://ieeexplore.ieee.org/document/8613833 [Accessed: 2026-02-22]  
   — **ThemisDB application:** `src/index/hnsw_index.cpp`, GPU acceleration kernels; HNSW algorithm for high-dimensional nearest-neighbor search.

3. T. Dao, D. Y. Fu, S. Ermon, A. Rudra, and C. Ré, "FlashAttention: Fast and memory-efficient exact attention with IO-awareness," in *Proc. Advances in Neural Information Processing Systems (NeurIPS)*, 2022, pp. 16344–16359. [Online]. Available: https://arxiv.org/abs/2205.14135 [Accessed: 2026-02-22]  
   — **ThemisDB application:** Batch vector search optimization and Tensor Core kernels; IO-aware GPU kernel design principles.

4. Y. Gao, K. Xiong, X. Gao, J. Ding, and C. D. Carothers, "NVIDIA Tensor Core for machine learning and deep learning," *IEEE Micro*, vol. 40, no. 6, pp. 33–45, Nov.–Dec. 2020, doi: 10.1109/MM.2020.3037720. [Online]. Available: https://ieeexplore.ieee.org/document/9269176 [Accessed: 2026-02-22]  
   — **ThemisDB application:** `src/acceleration/cuda_backend.cpp`; Tensor Core architecture for matrix operations and mixed-precision (FP16/TF32) kernels.

5. C. Ding, A. Sharma, S. C. Suh, M. R. Amer, A. Bhattacharya, and S. Kumar, "ScaNN: Efficient vector similarity search at scale," in *Proc. 37th Int. Conf. Machine Learning (ICML)*, 2020, pp. 2589–2599. [Online]. Available: https://arxiv.org/abs/1908.10396 [Accessed: 2026-02-22]  
   — **ThemisDB application:** Hybrid CPU/GPU search and quantization support; quantization-aware ANN search for efficient production deployment.

### Specifications & API References

6. Khronos Group, "Vulkan API Specification v1.3," Khronos Registries. [Online]. Available: https://www.khronos.org/registry/vulkan/ [Accessed: 2026-02-22]  
   — **ThemisDB application:** `src/acceleration/vulkan_backend_full.cpp`; cross-platform GPU compute with deterministic performance.

7. AMD, "ROCm documentation: Software platform for GPU computing," AMD. [Online]. Available: https://rocmdocs.amd.com/ [Accessed: 2026-02-22]  
   — **ThemisDB application:** `src/acceleration/hip_backend.cpp`; HIP API, rocBLAS, and RCCL for AMD GPU and multi-GPU support.
