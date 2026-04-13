<!-- Status: current | validated: 2026-06-09 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · docs/de/acceleration/README.md -->

# Acceleration Module - Future Enhancements

## Scope

The Acceleration module (`src/acceleration/`) provides hardware-accelerated compute backends for vector similarity search, graph traversal, geospatial computation, and matrix operations. It encompasses CUDA (`cuda_backend.cpp`, `cuda/`), HIP/ROCm (`hip_backend.cpp`, `hip/`), Vulkan (`vulkan_backend_full.cpp`, `vulkan/shaders/`), DirectX 12 (`directx_backend_full.cpp`, `directx/shaders/`), Metal (`metal_backend.mm`), OpenCL (`opencl_backend.cpp`), OneAPI (`oneapi_backend.cpp`), multi-GPU collectives via NCCL/RCCL (`nccl_vector_backend.cpp`, `rccl_vector_backend.cpp`), FAISS GPU indexing (`faiss_gpu_backend.cpp`), and the `BackendRegistry` (`backend_registry.cpp`) that selects the best available backend at runtime. The `plugin_loader.cpp` / `plugin_security.cpp` subsystem extends the registry with dynamically loaded GPU backends. Enhancements to AQL execution planning or higher-level query routing are out of scope; CPU fallback paths are included only where they affect GPU parity or benchmarking.

## Design Constraints

- `[ ]` Hardware portability: all enhancements must preserve `BackendRegistry`'s fallback to `CPUVectorBackend`/`CPUGraphBackend`/`CPUGeoBackend`/`CPUMatrixBackend` when no GPU is present; verified by CI runs with `THEMIS_ENABLE_CUDA=OFF THEMIS_ENABLE_VULKAN=OFF`.
- `[ ]` Plugin ABI stability: `plugin_loader.cpp` and `plugin_security.cpp` define a versioned plugin contract; GPU backend plugins must not alter that ABI before v2.0.
- `[ ]` Memory budget: GPU device memory is finite and shared; all backends must honour the per-operation memory cap exposed via `BackendCapabilities::maxMemoryBytes` and the `VLLMResourceManager::Config::max_gpu_vram_mb` limit.
- `[ ]` CUDA/Vulkan/HIP/DirectX SDK optionality: the build must succeed without any GPU SDK installed (`#ifdef THEMIS_ENABLE_CUDA` / `THEMIS_ENABLE_VULKAN` / `THEMIS_ENABLE_HIP` / `THEMIS_ENABLE_DIRECTX` guards must remain in all GPU paths).
- `[ ]` `IComputeBackend` interface must never throw; all errors are returned via `ErrorContext` or empty-result sentinel values; the interface contract is documented in `compute_backend.h`.
- `[ ]` `BackendRegistry` is a process-wide singleton; all public methods must be safe to call from multiple threads after `initializeRuntime()` returns.

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `BackendRegistry::instance()` | AQL executor, vector index | Singleton; must remain thread-safe after multi-GPU registration |
| `IVectorBackend::batchKnnSearch()` | `faiss_gpu_backend.cpp`, `nccl_vector_backend.cpp`, `multi_gpu_backend.cpp` | Signature frozen for v1.x |
| `PluginLoader::loadPlugin()` | Dynamic GPU backends (`zluda_backend.cpp`, `oneapi_backend.cpp`) | Plugin security sandbox enforced by `plugin_security.cpp` |
| `VLLMResourceManager::canUseGPU()` | Acceleration paths sharing GPU with vLLM inference | Must not block indefinitely; lease timeout required |
| `NCCLVectorBackend::mergeTopK()` | `multi_gpu_backend.cpp` distributed ANN search | Distributed path unimplemented; see Planned Features |
| `IAsyncComputeDispatch` | Future async search pipeline | Defined in `include/acceleration/FUTURE_ENHANCEMENTS.md`; not yet implemented in src |

## Planned Features

### CUDA Kernel Completion for Vector Similarity Search
**Priority:** High
**Target Version:** v1.7.0

`cuda_backend.cpp` declares stub kernel launch functions (`launchL2DistanceKernel`, `launchCosineDistanceKernel`, `launchTopKKernel`, …). The core kernels have been implemented in `cuda/ann_kernels.cu` and `cuda/vector_kernels.cu`; the remaining work is wiring the HNSW index layer to call these CUDA kernels instead of the CPU fallback. cuBLAS batched GEMM is the target for L2/cosine distance; CUB `DeviceSegmentedSort` is the target for top-k selection \[6\].

**Implementation Notes:**
- `[~]` `.cu` kernel files (`cuda/ann_kernels.cu`, `cuda/vector_kernels.cu`) are implemented; HNSW graph traversal wiring into `CUDAVectorBackend` is still pending.
- `[x]` Cosine distance: fuse L2-norm and dot-product into a single tiled kernel to avoid a second pass over device memory (IO-aware pattern per FlashAttention \[3\]).
- `[x]` Top-k (k ≤ 1024): use CUB `DeviceSegmentedSort` \[6\]; for k > 1024 fall back to `thrust::partial_sort`.
- `[x]` Add `CUDA_ARCH` compile-time guard: require sm_70+ (Tensor Core availability); emit warning for sm_60.

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

### NCCL/RCCL Distributed `mergeTopK` Implementation
**Priority:** High
**Target Version:** v1.9.0
**Status:** ✅ Implemented

`nccl_vector_backend.cpp` and `rccl_vector_backend.cpp` now implement the distributed multi-rank `mergeTopK()` path using `ncclAllGather`/`rcclAllGather` + host-side `std::partial_sort` + `ncclBcast`/`rcclBcast`.

**Implementation Notes:**
- `[x]` NCCL/RCCL communicator initialized; single-rank copy path implemented.
- `[x]` Multi-rank gather in `NCCLVectorBackend::mergeTopK()`: uses `ncclAllGather` inside `ncclGroupStart`/`ncclGroupEnd` to collect per-GPU top-K (indices + distances) from all ranks; host-side `std::partial_sort` selects global top-k; `ncclBcast` from root broadcasts result.
- `[x]` Mirror identical fix in `RCCLVectorBackend::mergeTopK()`: uses `rcclAllGather` + `rcclBcast` inside `rcclGroupStart`/`rcclGroupEnd`.
- `[x]` `ncclGroupStart()` / `ncclGroupEnd()` bracket pipelining both AllGather calls and both Bcast calls.
- `[x]` `(void)root; (void)stream;` suppression lines removed.
- `[x]` Tests added to `tests/test_collective_backends.cpp` validating single-rank copy correctness and k > localK rejection.
- `[x]` Integration test `tests/acceleration/test_nccl_merge_topk.cpp` added: 13 CPU-side merge-algorithm simulation tests (worldSize ∈ {2, 4, 8}, k ∈ {10, 100, 256}) plus 6 NCCL single-rank device tests (skipped without hardware) and 6 RCCL single-rank device tests (skipped without hardware); registered in `tests/CMakeLists.txt` as `NCCLMergeTopKFocusedTests`.

**Performance Targets:**
- 100M × 128-dim index distributed across 4× A100 80 GB; p99 query latency < 15 ms for k=100.
- `mergeTopK` overhead < 500 µs for worldSize=4, k=100 on NVLink-3 interconnect.
- Linear scaling efficiency ≥ 75% from 1→4 GPUs measured by `benchmarks/multi_gpu_bench.cpp`.

---

### Plugin Security: CRL and OCSP Certificate Revocation Checking
**Priority:** High
**Target Version:** v1.8.0

`plugin_security.cpp` contains two complete stub methods for certificate revocation:
- `PluginSecurityVerifier::checkCRL()` (line 598): extracts CRL distribution points but never fetches or validates the CRL — the body is a comment block listing the 4 required steps; when revocation checking is configured it fail-safes to `false` (line 636) and warns `"actual CRL checking not implemented"`.
- `PluginSecurityVerifier::checkOCSP()` (line 654): identical structure — OCSP responder URLs are extracted but no OCSP request is built or sent; also fail-safes to `false` (line 691).
- `EnhancedPluginSecurityVerifier::verifyCertificateChain()` (line 1036–1037) emits `THEMIS_WARN("Revocation checking configured but not yet implemented")`.

This means any GPU plugin with a revoked code-signing certificate will pass security validation when `requireRevocationCheck = false` (the default), and will fail to load with an opaque error when `requireRevocationCheck = true`, with no actionable diagnostic.

**Implementation Notes:**
- `[ ]` Implement `checkCRL()` in `plugin_security.cpp`: (1) for each CRL distribution point URL, perform an HTTP GET using `libcurl` or `WinHttp`; (2) parse the DER-encoded CRL with OpenSSL `d2i_X509_CRL`; (3) verify the CRL signature against the issuer certificate; (4) call `X509_CRL_get0_by_cert()` to check the target certificate's serial number; (5) validate CRL `thisUpdate` / `nextUpdate` timestamps. Honour a configurable timeout (default 5 s) to prevent hangs on unreachable endpoints.
- `[ ]` Implement `checkOCSP()` in `plugin_security.cpp`: (1) build an OCSP request with `OCSP_REQUEST_new()` + `OCSP_request_add0_id()`; (2) POST to each OCSP responder URL via HTTP; (3) parse the response with `OCSP_response_status()` and `OCSP_resp_find_status()`; (4) verify the responder's signature; (5) check `thisUpdate` / `nextUpdate` bounds.
- `[ ]` Implement PE certificate table extraction in `EnhancedPluginSecurityVerifier::extractSigningCertificate()` (line 1092): parse the PE optional header's DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY], extract the WIN_CERTIFICATE structure, and return the embedded PKCS#7 blob as a DER byte string. Currently the method detects the PE magic bytes but returns without extracting the cert (comment: *"extraction not fully implemented"*).
- `[ ]` Cache CRL/OCSP results per certificate serial number with a configurable TTL (default: CRL `nextUpdate`, OCSP 1 h) to avoid per-load network round trips.
- `[ ]` Add unit tests with a mock HTTP server (or pre-fetched fixtures) for both CRL and OCSP paths; cover revoked-cert, unknown-cert, network-timeout, and signature-invalid cases.

**Security Note:** Until this is implemented, plugin revocation is not enforced even when `requireRevocationCheck = true`. The fail-safe behaviour (returning `false`) prevents revoked plugins from loading only when the calling code actually checks the `checkCRL()`/`checkOCSP()` return value; `verifyCertificateChain()` currently bypasses both checks with a warning.

---

### Plugin Security: PE Certificate Table Extraction
**Priority:** Medium
**Target Version:** v1.8.0
**Status:** ✅ Implemented

`EnhancedPluginSecurityVerifier::extractEmbeddedCertificate()` in `plugin_security.cpp` iterates the full PE certificate table and returns the first PKCS#7 DER blob. The Linux ELF path parses the `.note.gnu.signature` section or falls back to a sidecar `.sig` file.

**Implementation Notes:**
- `[x]` Parse PE optional-header data directories: seek to `e_lfanew + 0x18 + offsetof(OptionalHeader, DataDirectory[4])`, read the `VirtualAddress` and `Size` fields for the Security directory (entry 4, `IMAGE_DIRECTORY_ENTRY_SECURITY`).
- `[x]` Map the certificate table: for each `WIN_CERTIFICATE` record in the table, check `wCertificateType == WIN_CERT_TYPE_PKCS_SIGNED_DATA` and extract `bCertificate[dwLength - offsetof(WIN_CERTIFICATE, bCertificate)]` as a DER blob.
- `[x]` For ELF plugins on Linux: look for a `.note.gnu.signature` section or a sidecar `plugin.so.sig` file; fall back to returning an empty string (unsigned) rather than leaving the code path unreachable.
- `[x]` Return the first valid PKCS#7 DER blob; log a warning if multiple certificates are present.
- `[x]` Add a fixture-based unit test with a pre-signed PE test binary to validate extraction end-to-end.

---

### VLLMResourceManager: OS-Level CPU and RAM Monitoring
**Priority:** Medium
**Target Version:** v1.8.0
**Status:** ✅ Implemented

`VLLMResourceManager::getStats()` now returns real OS-level CPU and RAM metrics.

**Implementation Notes:**
- `[x]` Linux CPU monitoring: reads `/proc/stat` on two 100 ms-apart snapshots and computes `(total - idle) / total * 100.0`.
- `[x]` Linux RAM monitoring: parses `/proc/meminfo` fields `MemTotal` and `MemAvailable`; computes `ram_used_mb = (MemTotal - MemAvailable) / 1024`.
- `[x]` Windows CPU monitoring: calls `GetSystemTimes()` with 100 ms delta; computes `(1 - idle/total) * 100.0`.
- `[x]` Windows RAM monitoring: calls `GlobalMemoryStatusEx()` and reads `dwMemoryLoad` and `ullTotalPhys - ullAvailPhys`.
- `[x]` Gated behind `#ifdef __linux__` / `#ifdef _WIN32`; macOS/unknown returns `0.0` (safe fallback).
- `[x]` Tests added in `tests/test_vllm_resource_stats.cpp`: `cpu_utilization ∈ [0, 100]`, `ram_used_mb > 0`, uninitialised guard returns zeros.

**Performance Targets:**
- Each `getStats()` call completes in < 2 ms (single `/proc/stat` + `/proc/meminfo` read on Linux).

---

### VLLMResourceManager: Multi-GPU NVML Monitoring (Beyond GPU 0)
**Priority:** Medium
**Target Version:** v1.8.0
**Status:** ✅ Implemented

`VLLMResourceManager::initializeNVML()` previously hard-coded `nvmlDeviceGetHandleByIndex(0, &device)`.

**Implementation Notes:**
- `[x]` Added `gpu_device_index` field (default `0`) to `VLLMResourceManager::Config`; `initializeNVML()` now calls `nvmlDeviceGetHandleByIndex(config_.gpu_device_index, &device)`.
- `[x]` Added `gpu_device_indices` vector override to `Config`; when non-empty, `initializeNVML()` opens handles for all listed devices and stores them in `nvml_devices_`.
- `[x]` `queryGPUUtilization()` returns the **maximum** utilization across all monitored devices; a single busy GPU blocks new ThemisDB work.
- `[x]` `shutdownNVML()` clears `nvml_devices_` before calling `nvmlShutdown()`, ensuring all device handles are released first.
- `[x]` 5 tests in `test_vllm_resource_stats.cpp` validating config fields, multi-device init without CUDA, and single non-zero device index.
- `[x]` `canUseGPU()` fixed to iterate all `nvml_devices_` (not just the primary alias `nvml_device_`) in the async task, ensuring max-utilisation semantics apply to the busy-check as well as `queryGPUUtilization()`.
- `[x]` `setGpuUtilizationProviderForTesting()` injection seam added to `VLLMResourceManager`; `canUseGPU()` and `queryGPUUtilization()` call the provider instead of NVML when set.
- `[x]` 8 mock-provider tests added to `test_vllm_resource_stats.cpp` (CI-only, no GPU hardware required):
  - `MockProvider_CanUseGPU_ReturnsFalse_At90Percent` — configured device busy
  - `MockProvider_CanUseGPU_ReturnsTrue_WhenIdle` — configured device idle
  - `MockProvider_CanUseGPU_ReturnsFalse_WhenConfiguredDeviceAt90_Gpu0Idle` — **core acceptance-criterion test**: device 2 busy, GPU 0 idle
  - `MockProvider_CanUseGPU_ReturnsFalse_WhenAnyMonitoredDeviceBusy` — multi-device max semantics
  - `MockProvider_QueryGPUUtilization_ReflectsProvider` — getStats() propagates provider value
  - `MockProvider_CanUseGPU_ReturnsFalse_WhenNullopt` — nullopt treated as busy
  - `MockProvider_CanUseGPU_At79Percent_AllowsUse` — boundary below threshold
  - `MockProvider_CanUseGPU_At80Percent_Blocks` — boundary at threshold
- `[x]` CI workflow added: `.github/workflows/02-feature-modules_acceleration_vllm-multi-gpu-nvml-monitoring-ci.yml`

---

### CUDA HNSW Kernel: Remove Silent `k > kMaxK` Clamping
**Priority:** Medium
**Target Version:** v1.8.0
**Status:** ✅ Fully implemented (Issue #132)

`cuda/cuda_hnsw_kernels.cu` previously defined `static constexpr uint32_t kMaxK = 256u` and silently truncated results when k > 256 was requested.

**Implementation Notes:**
- `[x]` kMaxK increased from 256 → 512 → **1024** in `cuda/cuda_hnsw_kernels.cu`.
- `[x]` Silent clamp replaced with an explicit `bool* h_overflow` output flag in `launchHnswSearchKernel`; overflow is set and the kernel is NOT launched when `k > kMaxK` so the caller can take corrective action.
- `[x]` Result buffers (`res_dist`, `res_id`) moved from fixed-size local arrays to dynamically allocated shared memory via `extern __shared__`; block size is computed at launch time as `min(128, 48KB / (k * 8))` to respect SM shared-memory limits.
- `[x]` `entry_node` parameter added to the kernel to support multi-pass searches from non-zero starting nodes.
- `[x]` `computeThreadsPerBlock(k)` helper added to compute the optimal block dimension for a given k.
- `[x]` For k > 1024 (extreme re-ranking): multi-pass strategy implemented in `CudaHnswTraversalEngine::batchSearch()` — runs `ceil(k / kMaxK)` GPU passes from diverse entry nodes, merges results on host using `std::partial_sort`, deduplicates by node ID.
- `[x]` Debug guard: `__trap()` fired in debug builds (`!NDEBUG`) if `k > kMaxK` reaches the launcher, ensuring callers do not inadvertently rely on overflow behavior.
- `[x]` Release builds: overflow condition propagated as `AccelerationErrorCode::InvalidInputShape` via `setError()` in `CUDAVectorBackend::annBatchSearch()` / `batchKnnSearch()`; makes `getHealthStatus()` return `BackendHealthStatus::makeDegraded()` automatically.
- `[x]` `kHnswSinglePassMaxK = 1024u` constant added to `cuda_backend.cpp` for consistent threshold checks.
- `[x]` Test: `tests/test_cuda_hnsw_large_k.cpp` with k=257, k=512, k=1024, k=1025 (multi-pass), health-degraded, sort-order, multi-query tests (7 test cases total).
- `[x]` CI workflow: `.github/workflows/02-feature-modules_acceleration_cuda-hnsw-large-k-ci.yml` triggers on changes to kernel / traversal / backend / test files.

**Performance Targets:**
- k=256: no regression vs. prior implementation (same block size, same shared memory layout).
- k=1024 with dynamic shared memory: block size reduced to 4 threads/block; total SM usage ≤ 32 KB; functional on all SM 2.0+ devices.
- k > 1024: multi-pass strategy returns correct result count at the cost of increased latency (documented trade-off; no RTX 3090 target for extreme-k path).

---

### CUDA HNSW Kernel: Visited Array Memory Scaling
**Priority:** Medium
**Target Version:** v1.9.0
**Status:** ✅ Production Ready

`cuda/cuda_hnsw_kernels.cu` previously allocated `num_queries × num_nodes × sizeof(uint8_t)` bytes per kernel launch — 5 GB for 512 queries × 10M nodes.

**Implementation Notes:**
- `[x]` Switched from `uint8_t` per-node to 1-bit-per-node bitset: allocation is now `ceil(num_nodes / 8)` bytes per query (10M nodes → 1.25 MB per query, 512 queries → 640 MB — 8× reduction).
- `[x]` Kernel updated to use bitset read (`visited[nb >> 3] & (1u << (nb & 7u))`) and write (`visited[nb >> 3] |= (1u << (nb & 7u))`) operations.
- `[x]` Initialisation loop reduced from `num_nodes` to `ceil(num_nodes/8)` iterations.
- `[x]` Replace per-invocation `cudaMalloc` / `cudaFree` with a persistent pre-allocated pool owned by `CudaHnswTraversalEngine::Impl::d_visited_pool`; allocated once in `buildIndex()` at `maxBatchSize × ceil(numNodes/8)` bytes; eliminates per-launch allocation overhead.
- `[x]` Chunked batch processing for graphs where bitset pool cannot cover all queries: `batchSearch()` splits `numQueries` into sub-batches of at most `pool_capacity` queries, processes them serially, and concatenates results on the host.
- `[x]` Exposed `CudaHnswTraversalEngine::setMaxBatchSize(size_t n)` and `CUDAVectorBackend::setMaxBatchSize(size_t n)` so callers can tune pool allocation.
- `[x]` Pool allocation failure surfaces as `BackendHealthStatus::makeDegraded()` via `setError()` in `CUDAVectorBackend::buildHnswAnnIndex()`.
- `[x]` Pool size is clamped to 90% of `BackendCapabilities::maxMemoryBytes` during `buildHnswAnnIndex()`.

**Performance Targets:**
- Pool allocation must not exceed `BackendCapabilities::maxMemoryBytes` at construction time.
- Per-query `cudaMalloc`/`cudaFree` round trips eliminated; visited-pool reuse reduces HNSW launch overhead by ≥ 15% for repeated fixed-batch queries.

---

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
- `[x]` Replace hard-coded `threadsPerBlock = 256` in `cuda/vector_kernels.cu:359` and `hip_backend.cpp:602` with a runtime call to `cudaOccupancyMaxPotentialBlockSize()` / `hipOccupancyMaxPotentialBlockSize()` at `initialize()` time; store the result in the backend's `Impl` struct and pass it to all kernel launches.
- `[x]` For `constexpr` block sizes in `.cu`/`.hip` files (`ann_kernels.cu`, `geo_kernels.cu`, `graph_kernels.cu`), expose a launch wrapper that accepts `threadsPerBlock` as a parameter and is called from the backend with the occupancy-tuned value rather than hard-coding the constant at the launch site.
- `[x]` For AMD GCN targets (wavefront = 64): default to 64 threads when `hipGetDeviceProperties().warpSize == 64` to avoid half-occupancy.
- `[x]` Vulkan `l2_distance.comp` hard-codes `layout(local_size_x = 16, local_size_y = 16)`: expose this as a specialization constant (`layout(constant_id = 0) const uint LOCAL_SIZE_X = 16`) so the `VulkanVectorBackend` can inject the optimal value for the target device via `VkSpecializationInfo` at pipeline creation time. Also `batch_search.comp` `local_size_x = 256` is now a specialization constant.
- `[x]` Add a micro-benchmark (`benchmarks/kernel_block_size_bench.cpp`) that sweeps block sizes 64/128/256/512 for each kernel and reports achieved occupancy.

**Performance Targets:**
- ≥ 5% throughput improvement on AMD RDNA2 (wavefront=32) vs. 256-thread baseline.
- No regression on NVIDIA sm_86/sm_89 (Ampere/Ada).

---

### BackendRegistry: Replace `std::cout` with Structured Logger
**Priority:** Low
**Target Version:** v1.8.0

`backend_registry.cpp` uses `std::cout` for all diagnostic output (lines 136, 143, 167, 311, 335, 340, 359, 417, 438, 442) despite the codebase providing a structured logger via `utils/logger.h` (`THEMIS_INFO`, `THEMIS_WARN`, `THEMIS_ERROR`, `THEMIS_DEBUG` macros). The inconsistency means backend-selection events are invisible when the calling application redirects or suppresses `std::cout`, and they cannot be structured-logged (JSON, syslog) by the logging framework.

**Implementation Notes:**
- `[x]` Replace all `std::cout << "Registered backend: ..."` (line 136) with `THEMIS_INFO("Registered backend: {} (type={})", backend->name(), static_cast<int>(backend->type()))`.
- `[x]` Replace all `std::cout <<` calls in `autoDetect()`, `initializeRuntime()`, `shutdownAll()`, `loadPlugins()`, `loadPlugin()` with the appropriate severity-level macro (`THEMIS_INFO` for status, `THEMIS_WARN` for degraded paths, `THEMIS_DEBUG` for verbose capability dumps).
- `[x]` The `logSelection` lambda in `initializeRuntime()` (line 435) already uses `std::cout`; convert it to `THEMIS_INFO` / `THEMIS_WARN`.
- `[x]` Ensure `utils/logger.h` is already included in `backend_registry.cpp` (it is used for `THEMIS_ERROR` on line 180 but `#include "utils/logger.h"` is already present).

---

### BackendRegistry: Thread-Safe Read Access After Initialization
**Priority:** Medium
**Target Version:** v1.8.0
**Status:** ✅ Implemented

`BackendRegistry` is now thread-safe. All mutable state is protected by `mutable std::shared_mutex registryMutex_`.

**Implementation Notes:**
- `[x]` Added `mutable std::shared_mutex registryMutex_` to `BackendRegistry` in `compute_backend.h`; `<shared_mutex>` and `<atomic>` included.
- `[x]` Exclusive lock (`std::unique_lock`) held in `registerBackend()`, `shutdownAll()`, and the write phase of `initializeRuntime()`.
- `[x]` Shared lock (`std::shared_lock`) held in all `getBackend*()`, `selectBackendFor*()`, `getBestBackend*()`, `getAvailableBackends()`, `deviceInfo()`, `getSelected*Backend()` methods.
- `[x]` `runtimeInitialized_` converted to `std::atomic<bool>`; read with `memory_order_acquire`, written with `memory_order_release`.
- `[x]` `selectTyped<T>()` documented with "callers must hold at least a shared lock" comment.
- `[x]` Thread-safety tests added to `test_backend_registry_startup.cpp`: 16-thread concurrent `getBestVectorBackend`, readers + `getAvailableBackends` writer, `isRuntimeInitialized` concurrency.
- `[x]` Dedicated thread-safety test file added at `tests/acceleration/test_backend_registry_thread_safety.cpp`: 16 reader threads calling `getBestVectorBackend()` concurrently while a background writer calls `registerBackend()` with a lightweight in-process stub (avoids plugin scanning noise); verifying no crashes. For data-race detection run locally with `-fsanitize=thread`.

---

### BackendRegistry: O(n²) Backend Selection Index
**Priority:** Low
**Target Version:** v1.9.0
**Status:** ✅ Implemented

The `selectTyped<T>()` helper in `backend_registry.cpp:223–233` iterates the entire `kFallbackOrder` vector (13 entries) and for each entry scans all registered backends in `backends_`. In the current implementation with ~15 backends this is negligible, but it is called for every query that needs backend selection (`selectVectorBackendFor`, `selectGraphBackendFor`, `selectGeoBackendFor`, `selectMatrixBackendFor`, `getBestVectorBackend`, etc.). More importantly, the nested loop requires O(|kFallbackOrder| × |backends_|) `dynamic_cast` calls per selection.

**Implementation Notes:**
- `[x]` At the end of `initializeRuntime()`, build a `std::unordered_map<BackendType, IComputeBackend*>` index from `backends_`; replace the nested loop in `selectTyped<T>()` with a single map lookup per priority level. — `typeIndex_` (`unordered_map<BackendType, RegisteredBackend>`) is populated in `registerBackend()` and used by `selectTyped<T>()` for O(|kFallbackOrder|) typed selection; `getBackend()` also uses the map for O(1) lookup.
- `[x]` Pre-compute and cache `getBestVectorBackend()` / `getBestGraphBackend()` / `getBestGeoBackend()` results into `selectedVectorBackend_` etc. as is already partially done; ensure `getBackend(type)` also uses the map. — `getBestVectorBackend/GraphBackend/GeoBackend/MatrixBackend()` iterate `kFallbackOrder` and look up `typeIndex_` for O(|kFallbackOrder|) with no dynamic_cast; `getBackend()` uses `typeIndex_` for O(1).
- `[x]` Avoid `dynamic_cast` in the hot path: store typed pointers (`IVectorBackend*`, `IGraphBackend*`, `IGeoBackend*`) alongside the `IComputeBackend*` in a `RegisteredBackend` struct at `registerBackend()` time (one `dynamic_cast` per registration, not per query). — `RegisteredBackend` struct in `compute_backend.h` holds `base`, `vectorPtr`, `graphPtr`, `geoPtr`, `matrixPtr`; all casts done once in `registerBackend()`.

---

### TensorCore Matmul: INT8 Quantized Precision Path
**Priority:** Medium
**Target Version:** v1.9.0
**Status:** ✅ Implemented

**Implementation Notes:**
- `[x]` Added `MatrixPrecision::INT8 = 3` to the `MatrixPrecision` enum in `kernel_invocation.h`.
- `[x]` Added `INT8` case in `dispatchMatmul()` (`tensor_core_matmul.cpp`) that dispatches to `launchINT8MatmulKernel()`.
- `[x]` Implemented `launchINT8MatmulKernel()` in `cuda/tensor_core_matmul.cu` using `cublasGemmEx` with `CUDA_R_8I` inputs, `CUDA_R_32I` accumulator, and `CUBLAS_GEMM_DEFAULT_TENSOR_OP`; includes runtime SM 7.5+ guard (returns 1 on older hardware).
- `[x]` Updated `CUDAMatrixBackend::getCapabilities()` to advertise `PrecisionMode::INT8` only when `sm >= 75` (Turing+).
- `[x]` `quantize()` / `dequantize()` FP32↔INT8 helpers added to `tensor_core_matmul.h` / `tensor_core_matmul.cpp`; symmetric per-tensor quantisation with clamp and round, guard for null pointers / non-positive scale.

**Performance Targets:**
- INT8 matmul throughput ≥ 2× FP16 throughput on RTX 3090 (sm_86) for 4096×4096 matrices.

---

### FAISS GPU Backend: HNSW and ScalarQuantizer Index Types
**Priority:** Medium
**Target Version:** v1.9.0
**Status:** ✅ IMPLEMENTED

`faiss_gpu_backend.cpp` now implements all six index types. `IVF_SQ8` uses
`GpuIndexIVFScalarQuantizer` with `QT_8bit` for higher recall than PQ at
equivalent memory. `HNSW_FLAT` uses CPU-side `faiss::IndexHNSWFlat` which
exposes the same `IVectorBackend` interface and is preferred for
low-latency single-query search. All switch statements include `default:`
branches that set `lastError_` via `setError()`. Input validation guards
(null pointers, zero sizes, empty paths, negative dimension) added to all
public methods. `getCapabilities()` now advertises `FP32 | INT8` precisions
and `L2 | INNER_PRODUCT` metric bits. Tests in `tests/test_faiss_gpu_backend.cpp`
(25 GPU tests + 15 validation + 10 structural).

**Resolved checklist:**
- `[x]` Add `IndexType::IVF_SQ8` — `GpuIndexIVFScalarQuantizer` with `QT_8bit`
- `[x]` Add `IndexType::HNSW_FLAT` — CPU-side `faiss::IndexHNSWFlat` + `hnswM` config field
- `[x]` Add `default:` branches with `setError()` in all switch statements
- `[x]` Update `getCapabilities()` with `INT8` precision flag and metric bitmask
- `[x]` Input validation in `search()`, `addVectors()`, `trainIndex()`,
        `computeDistances()`, `batchKnnSearch()`, `initializeIndex()`, `saveIndex()`, `loadIndex()`
- `[x]` Introduce `setError()` helper; replace bare `std::cerr` error paths
- `[x]` Add 50 unit + integration tests in `tests/test_faiss_gpu_backend.cpp`

---

### OpenGL Compute Shader Backend: Complete 5 Remaining Stubs
**Priority:** Low
**Target Version:** v2.0.0

`graphics_backends.cpp` header comment (line 14) and status banner (line 23) explicitly note *"OpenGL stub remaining"* and records *"Stubs: 5 (OpenGL only)"*. Specifically, `OpenGLVectorBackend::batchKnnSearch()`, `batchBFS()`, `batchShortestPaths()`, `batchPointInPolygon()`, and `batchHaversineDistance()` all return `{}` (lines 1781–1823). The Vulkan equivalents of all five operations are fully implemented in SPIR-V; the OpenGL path is intended as a fallback for platforms with OpenGL 4.3+ but no Vulkan ICD.

**Implementation Notes:**
- `[ ]` Implement `OpenGLVectorBackend::batchKnnSearch()` using the existing EGL + compute-shader infrastructure from `computeDistances()` (line 1766): dispatch the L2/cosine GLSL shader, read back distances, perform top-K on CPU with `std::nth_element`, return `std::vector<std::vector<std::pair<uint32_t,float>>>`.
- `[ ]` Implement `batchBFS()` and `batchShortestPaths()` using GLSL compute shaders equivalent to the Vulkan `batch_search.comp` and `topk_selection.comp`; store adjacency list in an SSBO.
- `[ ]` Implement `batchPointInPolygon()` and `batchHaversineDistance()` porting the Vulkan `point_in_polygon.comp` and `haversine_distance.comp` shaders to GLSL 4.30 (minimal changes — both shaders are already GLSL-compatible).
- `[ ]` Update the status banner comment to *"Stubs: 0"* and maturity to `🟢 PRODUCTION-READY` once all five are complete.
- `[ ]` Mark `OpenGLVectorBackend::getCapabilities().supportsAsync = false` remains accurate; document it in the header.

---

### Vulkan Compute Shader Pipeline for Cross-Platform GPU
**Priority:** High
**Target Version:** v1.7.0
**Status:** ✅ IMPLEMENTED

`vulkan_backend_full.cpp` is PRODUCTION-READY (0 stubs). All SPIR-V compute shaders for vector distance and geospatial operations are implemented in `vulkan/shaders/`: `l2_distance.comp`, `cosine_distance.comp`, `inner_product_distance.comp`, `batch_search.comp`, `topk_selection.comp`, `haversine_distance.comp`, `point_in_polygon.comp`. The LoRA shaders (`matmul.comp`, `elementwise.comp`, `gradient.comp`, etc.) are also complete.

**Remaining Hardening:**
- `[x]` MoltenVK path: verify `VK_KHR_buffer_device_address` capability probe on Apple M-series.
- `[x]` Benchmark on Mali-G710 and RDNA2 to validate workgroup size (256 threads for `batch_search.comp`, 16×16 for `l2_distance.comp`) occupancy targets; expose as SPIR-V specialization constants (see Kernel Block-Dimension Occupancy Tuning above).
- `[x]` Double-buffer staging buffers to overlap host→device DMA with shader dispatch.

**Performance Targets:**
- 500K × 128-dim cosine search in < 20 ms on Apple M2 Pro via MoltenVK.
- < 5% throughput regression versus CUDA path on AMD RX 7800 XT.

---

### Multi-GPU Sharding for Large Embedding Datasets
**Priority:** Medium
**Target Version:** v1.9.0
**Status:** ✅ Implemented

`MultiGPUVectorBackend` in `multi_gpu_backend.cpp` implements range-based sharding across N GPUs with NCCL/RCCL collective operations for distributed top-k merge, falling back to host-side merge when collectives are unavailable. Registered in `BackendRegistry::autoDetect()` when `detectGPUCount() >= 2`.

**Implementation Notes:**
- `[x]` Introduce `MultiGPUVectorBackend` in `multi_gpu_backend.cpp`; register it in `BackendRegistry` when `cudaGetDeviceCount() > 1`.
- `[x]` Shard by contiguous vector-ID ranges; store shard metadata in a `std::vector<ShardDescriptor>` on the host.
- `[x]` Use `ncclGroupStart` / `ncclGroupEnd` to batch cross-GPU transfers. Both `NCCLVectorBackend::mergeTopK()` and `RCCLVectorBackend::mergeTopK()` bracket the `AllGather` pair and the `Bcast` pair inside `ncclGroupStart`/`ncclGroupEnd` (and `rcclGroupStart`/`rcclGroupEnd`) respectively.
- `[x]` RCCL mirror: `rccl_vector_backend.cpp` exposes the same `IVectorBackend` interface; `BackendRegistry` selects NCCL vs RCCL at runtime via `cudaGetDeviceProperties`.
- `[x]` Graceful degradation: if NCCL init fails, fall back to single-GPU or CPU backend.
- `[x]` Integration tests in `tests/acceleration/test_nccl_merge_topk.cpp` (registered as `NCCLMergeTopKFocusedTests`): 13 CPU-side merge simulation tests + 6 NCCL single-rank device tests + 6 RCCL single-rank device tests.

**Performance Targets:**
- 100M × 128-dim index distributed across 4× A100 80GB; query latency < 15 ms @ 99th percentile for k=100.
- Linear scaling efficiency ≥ 75% from 1→4 GPUs.

---

### CUDA Graph Capture for Recurring Query Workloads
**Priority:** Medium
**Target Version:** v1.8.0
**Status:** ✅ IMPLEMENTED

For workloads that repeatedly execute the same ANN kernel shape (same `dim`, `numQueries`, `topK`), CUDA Graph capture eliminates kernel-launch overhead and CPU-side stream synchronisation \[6\]. `CUDAGraphCache` is implemented in `cuda_backend.h`/`cuda_backend.cpp` and captures/replays graphs keyed on `{dim, numQueries, topK, metric}`.

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
**Status:** ✅ IMPLEMENTED

`BackendRegistry` selects backends at startup by probing device capabilities (compute capability, available VRAM, driver version) through `DeviceManager`.

**Implementation Notes:**
- `[x]` Create `device_capability_probe.cpp` / `.h`; expose `DeviceInfo` struct with `computeCapabilityMajor`, `computeCapabilityMinor`, `totalMemoryBytes`, `driverVersion`, `backendType`. — implemented as `device_manager.h` / `device_manager.cpp`; `DeviceCapabilityInfo` struct in `compute_backend.h`
- `[x]` Probe order: CUDA → HIP → Vulkan → Metal → OpenCL → CPU. — delegated to `themis::gpu::DeviceDiscovery::Enumerate()` which follows this order
- `[x]` Cache probe results for 60 s; re-probe on explicit `BackendRegistry::refresh()` call. — `DeviceManager::probeDevices()` caches for `kCacheTTL = 60 s`; `DeviceManager::refresh()` forces re-probe; `BackendRegistry::initializeRuntime()` calls `DeviceManager::refresh()`
- `[x]` Emit structured log line via `utils/logger.h` listing selected backend and device name on startup. — `DeviceManager::logDeviceInfo()` emits structured output; called from `BackendRegistry::initializeRuntime()`
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
| Thread-Safety | `BackendRegistry` concurrent access | 16-thread contention test; concurrent `getBestVectorBackend()` + `registerBackend()` writer (lightweight stub, no plugin I/O) — see BackendRegistry thread-safety feature above; run locally with `-fsanitize=thread` for data-race detection |
| Security | Plugin revocation (CRL/OCSP) | Fixture-based test with mock HTTP server; cover revoked, unknown, timeout, and invalid-signature paths |
| Edge-Cases | `kMaxK` overflow, k > 256 HNSW clamping | `tests/acceleration/test_cuda_hnsw_large_k.cpp`; assert result count == requested k for k ∈ {257, 512, 1024} |

## Performance Targets

| Metric | Current | Target | Method |
|--------|---------|--------|--------|
| L2 search 1M×128 (CUDA) | N/A (stub) | < 8 ms | `benchmarks/vector_bench.cpp` on RTX 3090 |
| Cosine search 500K×128 (Vulkan/MoltenVK) | < 20 ms ✅ | < 20 ms | Manual bench on M2 Pro |
| Multi-GPU scale-out efficiency | N/A | ≥ 75% (1→4× A100) | `benchmarks/multi_gpu_bench.cpp` |
| CUDA Graph replay latency reduction | ≥ 30% ✅ | ≥ 30% | `benchmarks/vector_bench.cpp` fixed-shape mode |
| Device probe latency (4-GPU system) | < 50 ms ✅ | < 50 ms | `tests/acceleration/device_probe_test.cpp` |
| NCCL `mergeTopK` overhead (worldSize=4, k=100) | N/A (stub) | < 500 µs | `benchmarks/multi_gpu_bench.cpp` |
| INT8 matmul throughput vs FP16 | N/A (not implemented) | ≥ 2× on sm_86 | `benchmarks/tensor_core_bench.cpp` |
| `getStats()` call latency (Linux) | 0 ms (returns 0) | < 2 ms | `tests/acceleration/test_vllm_resource_stats.cpp` |
| Block-dim occupancy gain (RDNA2) | baseline (256 fixed) | ≥ 5% throughput gain | `benchmarks/kernel_block_size_bench.cpp` |

## Security / Reliability

- `[ ]` **CRL/OCSP revocation not enforced**: `plugin_security.cpp` methods `checkCRL()` (line 598) and `checkOCSP()` (line 654) are stubs that warn but do not perform network revocation checks; `EnhancedPluginSecurityVerifier::verifyCertificateChain()` (line 1036) emits `THEMIS_WARN` and bypasses both checks. Until fixed, plugins with revoked code-signing certificates pass validation when `requireRevocationCheck = false`. See **Plugin Security: CRL and OCSP** feature above.
- `[ ]` **PE certificate extraction incomplete**: `EnhancedPluginSecurityVerifier::extractSigningCertificate()` (`plugin_security.cpp:1092`) detects the PE magic bytes but does not parse the certificate table — `verifyAuthenticodeSignature()` receives an empty cert string on Windows plugins. See **Plugin Security: PE Certificate Table Extraction** above.
- `[ ]` `plugin_security.cpp` sandbox must be applied to all dynamically loaded GPU backends (`zluda_backend.cpp`, `oneapi_backend.cpp`); verify symbol allow-list before `dlopen`.
- `[ ]` GPU memory allocated via `cudaMalloc` / `vkAllocateMemory` must be zeroed before exposing to query results to prevent information leakage between tenants.
- `[x]` `vllm_resource_manager.cpp` `canUseGPU()`: wrapped `queryGPUUtilization()` with `std::async` + `wait_for(500ms)`; returns `false` on timeout (safe CPU fallback). NVML hang no longer blocks the caller.
- `[x]` `BackendRegistry` shared mutable state (`backends_`, `selectedVectorBackend_`) now protected by `std::shared_mutex registryMutex_` — data race fixed. See **BackendRegistry: Thread-Safe Read Access** above.

## 📚 Scientific Foundations

All planned features in this document are grounded in the following peer-reviewed research and industry specifications (IEEE format):

1. J. Johnson, M. Douze, and H. Jégou, "Billion-scale similarity search with GPUs," *IEEE Transactions on Big Data*, vol. 7, no. 3, pp. 535–547, 2021, doi: 10.1109/TBDATA.2019.2921572. [Online]. Available: https://faiss.ai/ [Accessed: 2026-02-22]  
   — Informs the FAISS GPU backend (`faiss_gpu_backend.cpp`) and GPU vector indexing roadmap.

2. Y. A. Malkov and D. A. Yashunin, "Efficient and robust approximate nearest neighbor search using Hierarchical Navigable Small World graphs," *IEEE Transactions on Pattern Analysis and Machine Intelligence*, vol. 42, no. 4, pp. 824–836, Apr. 2020, doi: 10.1109/TPAMI.2018.2889473. [Online]. Available: https://ieeexplore.ieee.org/document/8613833 [Accessed: 2026-02-22]  
   — Informs GPU-accelerated HNSW kernel design (`cuda/cuda_hnsw_kernels.cu`) and the `kMaxK` clamping issue.

3. T. Dao, D. Y. Fu, S. Ermon, A. Rudra, and C. Ré, "FlashAttention: Fast and memory-efficient exact attention with IO-awareness," in *Proc. Advances in Neural Information Processing Systems (NeurIPS)*, 2022, pp. 16344–16359. [Online]. Available: https://arxiv.org/abs/2205.14135 [Accessed: 2026-02-22]  
   — Informs IO-aware tiled kernel design for batch vector search and Tensor Core optimizations.

4. Y. Gao, K. Xiong, X. Gao, J. Ding, and C. D. Carothers, "NVIDIA Tensor Core for machine learning and deep learning," *IEEE Micro*, vol. 40, no. 6, pp. 33–45, Nov.–Dec. 2020, doi: 10.1109/MM.2020.3037720. [Online]. Available: https://ieeexplore.ieee.org/document/9269176 [Accessed: 2026-02-22]  
   — Informs FP16/BF16/INT8 mixed-precision kernels in `cuda_backend.cpp` and `tensor_core_matmul.cpp`.

5. C. Ding, A. Sharma, S. C. Suh, M. R. Amer, A. Bhattacharya, and S. Kumar, "ScaNN: Efficient vector similarity search at scale," in *Proc. 37th Int. Conf. Machine Learning (ICML)*, 2020, pp. 2589–2599. [Online]. Available: https://arxiv.org/abs/1908.10396 [Accessed: 2026-02-22]  
   — Informs quantization-aware ANN search and hybrid CPU/GPU search strategies.

6. Khronos Group, "Vulkan API Specification v1.3," Khronos Registries. [Online]. Available: https://www.khronos.org/registry/vulkan/ [Accessed: 2026-02-22]  
   — Informs Vulkan compute shader pipeline and cross-platform GPU support (`vulkan_backend_full.cpp`); including specialization constants for workgroup-size tuning.

7. AMD, "ROCm documentation: Software platform for GPU computing," AMD. [Online]. Available: https://rocmdocs.amd.com/ [Accessed: 2026-02-22]  
   — Informs HIP API usage, rocBLAS, and RCCL multi-GPU collectives (`hip_backend.cpp`, `rccl_vector_backend.cpp`).

## See Also

- [`src/gpu/`](../gpu/README.md) — Low-level GPU device discovery and driver wrappers used by the acceleration backends.
- [`src/geo/`](../geo/README.md) — Geospatial operators whose GPU path calls through `geo_acceleration_bridge.cpp`.
- [`src/graph/`](../graph/README.md) — Graph analytics engine; GPU-accelerated traversal delegates to backends registered here.
- [`src/index/`](../index/README.md) — Vector index layer; calls `IVectorBackend::batchKnnSearch()` for GPU ANN search.
- [`src/performance/`](../performance/README.md) — Benchmarking infrastructure validating the ≥ 10× GPU speedup targets.
- [`include/acceleration/FUTURE_ENHANCEMENTS.md`](../../include/acceleration/FUTURE_ENHANCEMENTS.md) — Complementary enhancements to the public header interfaces.
