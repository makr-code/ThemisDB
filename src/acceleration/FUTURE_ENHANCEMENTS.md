# Acceleration Module - Future Enhancements
<!-- Status: current | validated: 2026-03-09 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · docs/de/acceleration/README.md -->

## Scope

This document covers implementation-specific future enhancements for the Acceleration module (`src/acceleration/`), focusing on GPU and hardware-accelerated compute backends including CUDA (`cuda_backend.cpp`), Vulkan (`vulkan_backend_full.cpp`), HIP (`hip_backend.cpp`), and the backend registry (`backend_registry.cpp`). Enhancements to higher-level query planning or AQL execution are out of scope. CPU fallback paths in `cpu_backend.cpp` and `cpu_backend_mt.cpp` are included only where they affect GPU parity or benchmarking.

## Design Constraints

- `[ ]` Hardware portability: enhancements must not break `BackendRegistry` fallback to `CPUVectorBackend`/`CPUGraphBackend`/`CPUGeoBackend` when no GPU is present.
- `[ ]` Plugin ABI stability: `plugin_loader.cpp` and `plugin_security.cpp` define a versioned plugin contract; GPU backend plugins must not alter that ABI before v2.0.
- `[ ]` Memory budget: GPU device memory is finite and shared; all backends must honour the per-operation memory cap exposed via `ComputeBackend::memoryBudgetBytes()`.
- `[ ]` CUDA/Vulkan SDK optionality: the build must succeed without CUDA or Vulkan SDKs installed (`#ifdef THEMIS_ENABLE_CUDA` / `THEMIS_ENABLE_VULKAN` guards must remain in all GPU paths).

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `BackendRegistry::instance()` | AQL executor, vector index | Singleton; must remain thread-safe after multi-GPU registration |
| `ComputeBackend::batchSimilaritySearch()` | `faiss_gpu_backend.cpp`, `nccl_vector_backend.cpp` | Signature frozen for v1.x |
| `PluginLoader::loadPlugin()` | Dynamic GPU backends (`zluda_backend.cpp`, `oneapi_backend.cpp`) | Plugin security sandbox enforced by `plugin_security.cpp` |
| `VLLMResourceManager` | LLM inference paths calling into acceleration | Resource lease API must not regress under concurrent GPU workloads |

## Planned Features

### CUDA Kernel Completion for Vector Similarity Search
**Priority:** High
**Target Version:** v1.7.0

`cuda_backend.cpp` currently declares 13 stub kernel launch functions (`launchL2DistanceKernel`, `launchCosineDistanceKernel`, `launchTopKKernel`, …). All kernels must be fully implemented using cuBLAS batched GEMM for L2/cosine distance and a CUB-based top-k selection pass.

**Implementation Notes:**
- `[ ]` Implement `.cu` kernel files alongside `cuda_backend.cpp`; link via `CMakeLists.txt` `target_sources` under `THEMIS_ENABLE_CUDA`.
- `[ ]` Use `cudaStream_t` per-query for async overlap; expose stream pool in `CUDAVectorBackend::streamPool_`.
- `[ ]` Cosine distance: fuse L2-norm and dot-product into a single tiled kernel to avoid a second pass over device memory.
- `[ ]` Top-k (k ≤ 1024): use CUB `DeviceSegmentedSort`; for k > 1024 fall back to thrust `partial_sort`.
- `[ ]` Add `CUDA_ARCH` compile-time guard: require sm_70+ (Tensor Core availability); emit warning for sm_60.

**Performance Targets:**
- 1M × 128-dim float32 L2 search in < 8 ms on RTX 3090 (single GPU).
- Throughput ≥ 10× CPU AVX2 baseline measured by `benchmarks/vector_bench.cpp`.
- GPU memory footprint < 2 GB for 10M 128-dim vectors.

**API Sketch:**
```cpp
// cuda_backend.cpp — completed signature (currently stub)
std::vector<SearchResult> CUDAVectorBackend::batchSimilaritySearch(
    const float* queries,   // host pointer, [numQueries × dim]
    size_t numQueries,
    size_t dim,
    DistanceMetric metric,
    size_t topK,
    const SearchOptions& opts) override;
```

---

### Vulkan Compute Shader Pipeline for Cross-Platform GPU
**Priority:** High
**Target Version:** v1.7.0

`vulkan_backend_full.cpp` is production-ready infrastructure (quality score 94, 0 stubs). SPIR-V compute shaders for vector distance and geospatial operators are **implemented** in `vulkan/shaders/`. Remaining work is performance tuning and MoltenVK compatibility hardening for Apple Silicon.

**Status:**
- `[x]` `shaders/l2_distance.comp`, `shaders/cosine_distance.comp`, `shaders/inner_product_distance.comp`, `shaders/batch_search.comp`, `shaders/topk_selection.comp`, `shaders/haversine_distance.comp`, `shaders/point_in_polygon.comp` — all implemented
- `[x]` Push constants used for `numVectors`, `dim`, `topK` (no per-query UBO re-allocation)
- `[ ]` MoltenVK path: disable `VK_KHR_buffer_device_address` if not available; add capability probe in `VulkanBackend::initialize()`
- `[ ]` Implement double-buffering of staging buffers to overlap host→device DMA with shader dispatch
- `[ ]` Benchmark workgroup size tuning on Mali-G710 and RDNA2

**Performance Targets:**
- 500K × 128-dim cosine search in < 20 ms on Apple M2 Pro via MoltenVK.
- < 5% throughput regression versus CUDA path on AMD RX 7800 XT.

---

### Multi-GPU Sharding for Large Embedding Datasets
**Priority:** Medium
**Target Version:** v1.9.0

`nccl_vector_backend.cpp` and `rccl_vector_backend.cpp` stub NCCL/RCCL collective operations. Implement a sharding strategy in `BackendRegistry` that partitions an embedding index across N GPUs and scatters queries using NCCL `ncclBcast` + `ncclAllGather`.

**Implementation Notes:**
- `[x]` Introduce `MultiGPUVectorBackend` in a new file `multi_gpu_backend.cpp`; register it in `BackendRegistry` when `cudaGetDeviceCount() > 1`.
- `[x]` Shard by contiguous vector-ID ranges; store shard metadata in a `std::vector<ShardDescriptor>` on the host.
- `[~]` Use `ncclGroupStart` / `ncclGroupEnd` to batch cross-GPU transfers. (NCCL/RCCL backends initialized; actual group-call wiring is v2.5+ pending real CUDA kernels)
- `[x]` RCCL mirror: `rccl_vector_backend.cpp` must expose the same `IVectorBackend` interface; `BackendRegistry` selects NCCL vs RCCL at runtime via `cudaGetDeviceProperties`.
- `[x]` Graceful degradation: if NCCL init fails, fall back to single-GPU or CPU backend.

**Performance Targets:**
- 100M × 128-dim index distributed across 4× A100 80GB; query latency < 15 ms @ 99th percentile for k=100.
- Linear scaling efficiency ≥ 75% from 1→4 GPUs.

---

### CUDA Graph Capture for Recurring Query Workloads
**Priority:** Medium
**Target Version:** v1.8.0

For workloads that repeatedly execute the same ANN kernel shape (same `dim`, `numQueries`, `topK`), CUDA Graph capture eliminates kernel-launch overhead and CPU-side stream synchronisation. Add a `CUDAGraphCache` within `CUDAVectorBackend` that captures and replays graphs keyed on `{dim, numQueries, topK, metric}`.

**Implementation Notes:**
- `[x]` Add `CUDAGraphCache` struct to `cuda_backend.h`/`cuda_backend.cpp`; keyed by a `QueryShape` tuple (`numQueries`, `numVectors`, `dim`, `topK`, `metric`), value is a `CUDAGraphEntry` owning a `cudaGraph_t` + `cudaGraphExec_t` pair plus pre-allocated device buffers.
- `[x]` On cache miss: record a graph with `cudaStreamBeginCapture` / `cudaStreamEndCapture` on a temporary non-blocking capture stream; instantiate via `cudaGraphInstantiate` (CUDA 11/12 API variant guarded by `CUDART_VERSION`).
- `[x]` On cache hit: copy new input data into the entry's pre-allocated device buffers via `cudaMemcpyAsync` on the main stream, then replay with `cudaGraphLaunch`. Device-pointer addresses remain constant (pre-allocated at capture time) so no node-parameter update is required on every replay.
- `[x]` LRU evict graphs when cache exceeds 32 entries to bound device memory usage (`CUDAGraphCache::evictLRU` traverses all entries in O(n) — acceptable since n ≤ 32).
- `[x]` Variable-shape batches: callers with variable-length batches are directed to use `batchKnnSearch()` instead; documented in the `batchKnnSearchWithGraph()` method comment.

**Performance Targets:**
- ≥ 30% reduction in end-to-end ANN query latency for fixed-shape repeated queries (benchmarked via `benchmarks/vector_bench.cpp`).
- Zero CUDA API error rate under 10-thread concurrent graph replay (validated by `tests/test_cuda_graph_capture.cpp`).

---

### Runtime Device Capability Negotiation
**Priority:** High
**Target Version:** v1.7.0

`BackendRegistry` currently selects backends at startup without probing device capabilities (compute capability, available VRAM, driver version). Add a `DeviceCapabilityProbe` that queries all visible GPUs and ranks them, allowing `BackendRegistry::selectBestBackend()` to make an informed choice.

**Implementation Notes:**
- `[x]` Create `device_capability_probe.cpp` / `.h`; expose `DeviceInfo` struct with `computeCapabilityMajor`, `computeCapabilityMinor`, `totalMemoryBytes`, `driverVersion`, `backendType`. — implemented as `device_manager.h` / `device_manager.cpp`; `DeviceCapabilityInfo` struct in `compute_backend.h`
- `[x]` Probe order: CUDA → HIP → Vulkan → Metal → OpenCL → CPU. — delegated to `themis::gpu::DeviceDiscovery::Enumerate()` which follows this order
- `[x]` Cache probe results for 60 s; re-probe on explicit `BackendRegistry::refresh()` call. — `DeviceManager::probeDevices()` caches for `kCacheTTL = 60 s`; `DeviceManager::refresh()` forces re-probe; `BackendRegistry::initializeRuntime()` calls `DeviceManager::refresh()`
- `[x]` Emit structured log line via `utils/logger.h` listing selected backend and device name on startup. — `DeviceManager::logDeviceInfo()` emits structured output; called from `BackendRegistry::initializeRuntime()` (consistent with existing std::cout pattern in backend_registry.cpp)
- `[x]` Expose probe results via `BackendRegistry::deviceInfo()` for observability. — `BackendRegistry::deviceInfo()` returns the `DeviceCapabilityInfo` snapshot captured at `initializeRuntime()` time

**Performance Targets:**
- Probe completes in < 50 ms on a system with 4 GPUs.
- Zero false-positive backend selection failures in CI matrix covering CUDA 11.8, CUDA 12.x, ROCm 5.7, Vulkan 1.3.

## Test Strategy

| Test Type | Coverage Target | Notes |
|-----------|----------------|-------|
| Unit | >80% new code | Mock `cudaMemcpy` / Vulkan dispatch via dependency-injected function pointers; test `DeviceCapabilityProbe` with mock device list |
| Integration | All backends registered and falling back to CPU when SDK absent | Run in CI with `THEMIS_ENABLE_CUDA=OFF` and `THEMIS_ENABLE_VULKAN=OFF` to validate CPU fallback path |
| Performance | Vector bench regression ≤ 5% | `benchmarks/vector_bench.cpp`; run on GPU runner; alert if p99 regresses |

## Performance Targets

| Metric | Current | Target | Method |
|--------|---------|--------|--------|
| L2 search 1M×128 (CUDA) | N/A (stub) | < 8 ms | `benchmarks/vector_bench.cpp` on RTX 3090 |
| Cosine search 500K×128 (Vulkan/MoltenVK) | N/A (stub) | < 20 ms | Manual bench on M2 Pro |
| Multi-GPU scale-out efficiency | N/A | ≥ 75% (1→4× A100) | `benchmarks/multi_gpu_bench.cpp` |
| CUDA Graph replay latency reduction | Baseline | ≥ 30% | `benchmarks/vector_bench.cpp` fixed-shape mode |
| Device probe latency (4-GPU system) | N/A | < 50 ms | `tests/acceleration/device_probe_test.cpp` |

## Security / Reliability

- `[ ]` `plugin_security.cpp` sandbox must be applied to all dynamically loaded GPU backends (`zluda_backend.cpp`, `oneapi_backend.cpp`); verify symbol allow-list before `dlopen`.
- `[ ]` GPU memory allocated via `cudaMalloc` / `vkAllocateMemory` must be zeroed before exposing to query results to prevent information leakage between tenants.
- `[ ]` `vllm_resource_manager.cpp` lease acquisition must be wrapped in a timeout (default 500 ms) to prevent deadlock when a GPU backend hangs during kernel execution.

## 📚 Scientific Foundations

All planned features in this document are grounded in the following peer-reviewed research and industry specifications (IEEE format):

1. J. Johnson, M. Douze, and H. Jégou, "Billion-scale similarity search with GPUs," *IEEE Transactions on Big Data*, vol. 7, no. 3, pp. 535–547, 2021, doi: 10.1109/TBDATA.2019.2921572. [Online]. Available: https://faiss.ai/ [Accessed: 2026-02-22]  
   — Informs the FAISS GPU backend (`faiss_gpu_backend.cpp`) and GPU vector indexing roadmap.

2. Y. A. Malkov and D. A. Yashunin, "Efficient and robust approximate nearest neighbor search using Hierarchical Navigable Small World graphs," *IEEE Transactions on Pattern Analysis and Machine Intelligence*, vol. 42, no. 4, pp. 824–836, Apr. 2020, doi: 10.1109/TPAMI.2018.2889473. [Online]. Available: https://ieeexplore.ieee.org/document/8613833 [Accessed: 2026-02-22]  
   — Informs GPU-accelerated HNSW kernel design (`src/index/hnsw_index.cpp`, acceleration kernels).

3. T. Dao, D. Y. Fu, S. Ermon, A. Rudra, and C. Ré, "FlashAttention: Fast and memory-efficient exact attention with IO-awareness," in *Proc. Advances in Neural Information Processing Systems (NeurIPS)*, 2022, pp. 16344–16359. [Online]. Available: https://arxiv.org/abs/2205.14135 [Accessed: 2026-02-22]  
   — Informs IO-aware tiled kernel design for batch vector search and Tensor Core optimizations.

4. Y. Gao, K. Xiong, X. Gao, J. Ding, and C. D. Carothers, "NVIDIA Tensor Core for machine learning and deep learning," *IEEE Micro*, vol. 40, no. 6, pp. 33–45, Nov.–Dec. 2020, doi: 10.1109/MM.2020.3037720. [Online]. Available: https://ieeexplore.ieee.org/document/9269176 [Accessed: 2026-02-22]  
   — Informs FP16/TF32 mixed-precision kernels in `cuda_backend.cpp`.

5. C. Ding, A. Sharma, S. C. Suh, M. R. Amer, A. Bhattacharya, and S. Kumar, "ScaNN: Efficient vector similarity search at scale," in *Proc. 37th Int. Conf. Machine Learning (ICML)*, 2020, pp. 2589–2599. [Online]. Available: https://arxiv.org/abs/1908.10396 [Accessed: 2026-02-22]  
   — Informs quantization-aware ANN search and hybrid CPU/GPU search strategies.

6. Khronos Group, "Vulkan API Specification v1.3," Khronos Registries. [Online]. Available: https://www.khronos.org/registry/vulkan/ [Accessed: 2026-02-22]  
   — Informs Vulkan compute shader pipeline and cross-platform GPU support (`vulkan_backend_full.cpp`).

7. AMD, "ROCm documentation: Software platform for GPU computing," AMD. [Online]. Available: https://rocmdocs.amd.com/ [Accessed: 2026-02-22]  
   — Informs HIP API usage, rocBLAS, and RCCL multi-GPU collectives (`hip_backend.cpp`, `rccl_vector_backend.cpp`).

## See Also

- [`src/gpu/`](../gpu/README.md) — Low-level GPU device discovery and driver wrappers used by the acceleration backends.
- [`src/geo/`](../geo/README.md) — Geospatial operators whose GPU path calls through `geo_acceleration_bridge.cpp`.
- [`src/graph/`](../graph/README.md) — Graph analytics engine; GPU-accelerated traversal delegates to backends registered here.
- [`src/index/`](../index/README.md) — Vector index layer; calls `ComputeBackend::batchSimilaritySearch()` for GPU ANN search.
- [`src/performance/`](../performance/README.md) — Benchmarking infrastructure validating the ≥ 10× GPU speedup targets.
