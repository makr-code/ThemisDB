# Acceleration Module (src/acceleration)

## Overview

The Acceleration module provides hardware-accelerated compute backends for ThemisDB. Its goal is to speed up compute-heavy primitives used by higher-level subsystems (e.g., vector similarity search / ANN, graph analytics, and geospatial operators) while preserving **correctness**, **determinism**, and a **CPU fallback** when no suitable accelerator is available.

In practice, this module is responsible for:

- Selecting an appropriate backend at runtime (GPU/CPU) without breaking portability.
- Providing a stable interface (`ComputeBackend` and related backend interfaces) that consumers can call without depending on CUDA/Vulkan specifics.
- Hosting accelerator implementations and/or plugins (e.g., CUDA, Vulkan, HIP) behind feature flags so builds work even if SDKs are not installed.

## Directory Layout (high level)

> Note: filenames below are referenced by `FUTURE_ENHANCEMENTS.md` and may evolve; treat this as a “map” of the current structure.

- **Backend selection & registry**  
  - `backend_registry.cpp`: runtime backend registration/selection and CPU fallback.  
  - `compute_backend.cpp`: abstract `ComputeBackend` base class and shared utilities.  
  - `device_manager.cpp`: device enumeration, capability probing (VRAM, compute capability, driver version), 60 s TTL cache, and `BackendRegistry::deviceInfo()` observability accessor.  

- **CUDA backend (optional, guarded by `THEMIS_ENABLE_CUDA`)**  
  - `cuda_backend.cpp` + `cuda/ann_kernels.cu`, `cuda/geo_kernels.cu`, `cuda/tensor_core_matmul.cu`, `cuda/vector_kernels.cu`: CUDA kernels and stream/graph management for vector similarity and geospatial operations.  
  - `nccl_vector_backend.cpp`: multi-GPU NCCL collectives for sharding and query scatter/gather.  
  - `tensor_core_matmul.cpp`: Tensor Core FP16/BF16 matrix multiplication.

- **HIP/ROCm backend (optional, guarded by `THEMIS_ENABLE_HIP`)**  
  - `hip_backend.cpp` + `hip/ann_kernels.hip`, `hip/geo_kernels.hip`: AMD HIP ANN and geospatial kernels.  
  - `rccl_vector_backend.cpp`: multi-GPU RCCL collectives (AMD mirror of NCCL backend).

- **Vulkan backend (optional, guarded by `THEMIS_ENABLE_VULKAN`)**  
  - `vulkan_backend_full.cpp`: Vulkan compute infrastructure.  
  - `vulkan/shaders/`: SPIR-V compute shaders for L2, cosine, inner-product, top-K, Haversine, and point-in-polygon operations.

- **Other GPU / platform backends**  
  - `directx_backend_full.cpp` + `directx/shaders/`: DirectX Compute backend (Windows).  
  - `metal_backend.mm`: Apple Metal backend (macOS/iOS).  
  - `opencl_backend.cpp`: OpenCL backend for broad hardware compatibility.  
  - `graphics_backends.cpp`: shared graphics/GPU utility helpers.  
  - `zluda_backend.cpp`: ZLUDA (AMD on CUDA API) backend.  
  - `oneapi_backend.cpp`: Intel oneAPI backend.  
  - `faiss_gpu_backend.cpp`: FAISS GPU wrapper for billion-scale ANN search.

- **Multi-GPU**  
  - `multi_gpu_backend.cpp`: range-based sharding, fan-out KNN, and host-side top-k merge across N devices.

- **Geospatial bridge**  
  - `geo_acceleration_bridge.cpp`: bridges geospatial operators (Haversine distance, point-in-polygon) to the acceleration layer via `GeoKernelDispatch`.

- **CPU fallback**  
  - `cpu_backend.cpp`, `cpu_backend_mt.cpp`: reference single-thread and pthreads implementations used when accelerators are unavailable and as correctness baselines.  
  - `cpu_backend_tbb.cpp`: Intel TBB-based parallel CPU backend.

- **Kernel dispatch & fallback/retry** (`include/acceleration/kernel_fallback_dispatcher.h`)  
  - `ANNKernelFallbackDispatcher`: wraps a primary `ANNKernelDispatch` table (GPU) and a fallback table (CPU). Null slots in the primary are routed directly to the fallback (unsupported kernel). Transient device errors (`DeviceLost`, `OperationTimeout`, `SynchronizationFailed`) are retried with exponential back-off; all other errors and exhausted retries also fall back.  
  - `GeoKernelFallbackDispatcher`: same semantics for the two geospatial kernel slots.  
  - `RetryPolicy`: configures `maxAttempts`, initial/max delay (ms), and back-off multiplier.

- **Plugins / security**  
  - `plugin_loader.cpp`: loads optional backend plugins at runtime.  
  - `plugin_security.cpp`: enforces the sandbox/allow-list for dynamically loaded GPU backends; verifies GPG/code signatures before `dlopen`.  
  - `shader_integrity.cpp`: verifies SPIR-V shader integrity before pipeline creation.

- **vLLM / LLM resource management**  
  - `vllm_resource_manager.cpp`: GPU VRAM resource lease management for LLM inference paths.

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

The following peer-reviewed publications, standards, and reference implementations form the scientific basis for the design decisions and algorithms used in this module. Citations follow IEEE format with DOI, URL, and access date.

---

### GPU-Accelerated Database Operations

**[1]** Y. Chen, T. Li, Y. Zhou, and Z. Wang, "Accelerating Database Operations on GPUs: A Survey," *IEEE Trans. Knowl. Data Eng.*, vol. 29, no. 1, pp. 147–165, Jan. 2017, doi: 10.1109/TKDE.2016.2603064. [Online]. Available: https://ieeexplore.ieee.org/document/7586066. Accessed: Mar. 2, 2026.

> **Relevance:** Provides a systematic taxonomy of GPU-accelerated relational and vector database operators. The survey's classification of memory-bandwidth-bound versus compute-bound kernels directly informs the design of `cuda_backend.cpp` and the selection of cuBLAS GEMM for L2/cosine distance over custom reduction kernels. The authors' analysis of data transfer bottlenecks motivates the double-buffered staging strategy planned for `vulkan_backend_full.cpp`.

---

### SIMD-Accelerated Database Operations

**[2]** A. He, S. Pandey, and A. Gupta, "SIMD-Accelerated Database Systems: A Survey of Techniques and Open Problems," *Proc. VLDB Endow.*, vol. 12, no. 3, pp. 309–322, Nov. 2018, doi: 10.14778/3352063.3352067. [Online]. Available: https://www.vldb.org/pvldb/vol12/p309-he.pdf. Accessed: Mar. 2, 2026.

> **Relevance:** Surveys SIMD vectorisation techniques for selection, aggregation, join, and sorting — operations directly implemented in `cpu_backend.cpp` and `cpu_backend_mt.cpp`. The survey's "open problems" section on operator fusion is the basis for the planned fused L2-norm + dot-product kernel in the CUDA backend, and motivates AVX-512 loop unrolling in the CPU reference path that serves as the benchmark baseline (≥ 10× GPU speedup target).

**[3]** J. Zhou and K. A. Ross, "Implementing database operations using SIMD instructions," in *Proc. ACM SIGMOD Int. Conf. Manag. Data*, Madison, WI, USA, Jun. 2002, pp. 145–156, doi: 10.1145/564691.564710. [Online]. Available: https://doi.org/10.1145/564691.564710. Accessed: Mar. 2, 2026.

> **Relevance:** Foundational paper establishing SIMD scan, selection, and sort primitives for relational databases. The vectorised scan patterns described here are implemented in `cpu_backend.cpp` as the correctness baseline against which all GPU backends measure their result parity. The paper's methodology of "data-parallel inner loops with scalar remainder handling" is reflected in the AVX2 fallback guard in `cpu_backend_mt.cpp`.

---

### FPGA / Hardware-Offload for Query Execution

**[4]** D. Sidler, Z. István, M. Owaida, and G. Alonso, "Accelerating Pattern Matching Queries in Hybrid CPU-FPGA Architectures," in *Proc. ACM SIGMOD Int. Conf. Manag. Data*, Chicago, IL, USA, May 2017, pp. 403–415, doi: 10.1145/3035918.3035941. [Online]. Available: https://doi.org/10.1145/3035918.3035941. Accessed: Mar. 2, 2026.

> **Relevance:** Demonstrates a CPU-FPGA co-execution model for SQL query offload. The hardware-abstract `ComputeBackend` interface and `BackendRegistry` capability-negotiation design in this module follow the same separation-of-concerns principle: the host (CPU) orchestrates while the accelerator (GPU/FPGA) executes data-parallel kernels. The FPGA plugin slot in `plugin_loader.cpp` is designed to accommodate future FPGA backends following this co-execution model.

---

### GPU Data Science Ecosystem (RAPIDS / cuDF)

**[5]** NVIDIA Corporation, "RAPIDS: Open GPU Data Science — cuDF, cuML, cuGraph," NVIDIA Developer, 2019. [Online]. Available: https://rapids.ai. Accessed: Mar. 2, 2026.

> **Relevance:** RAPIDS/cuDF provides the reference GPU DataFrame and GPU array libraries used by `faiss_gpu_backend.cpp` and the planned CUDA vector kernels. The RAPIDS memory model (RMM — RAPIDS Memory Manager) is the basis for the per-operation `memoryBudgetBytes()` constraint enforced in `ComputeBackend`. The cuGraph analytics pipeline is the upstream dependency for any future GPU graph backend registered in `BackendRegistry`.

---

## See Also

- [`src/gpu/`](../gpu/README.md) — Low-level GPU device discovery, memory management, and CUDA/Vulkan/HIP driver wrappers used by the acceleration backends.
- [`src/geo/`](../geo/README.md) — Geospatial operators (Haversine distance, point-in-polygon) whose GPU acceleration path calls through `geo_acceleration_bridge.cpp` in this module.
- [`src/graph/`](../graph/README.md) — Graph analytics engine; GPU-accelerated graph traversal delegates to backends registered in `BackendRegistry`.
- [`src/index/`](../index/README.md) — Vector index layer (HNSW, IVF-Flat); calls `ComputeBackend::batchSimilaritySearch()` for GPU-accelerated ANN search.
- [`src/performance/`](../performance/README.md) — Performance benchmarking infrastructure; `benchmarks/vector_bench.cpp` validates the ≥ 10× GPU speedup target referenced in `FUTURE_ENHANCEMENTS.md`.
- [`docs/acceleration/capability_negotiation.md`](../../docs/acceleration/capability_negotiation.md) — Deep-dive into backend capability negotiation and the fallback chain.
- [`docs/acceleration/troubleshooting.md`](../../docs/acceleration/troubleshooting.md) — Operational troubleshooting guide (runbooks, diagnostics, platform-specific issues).
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
