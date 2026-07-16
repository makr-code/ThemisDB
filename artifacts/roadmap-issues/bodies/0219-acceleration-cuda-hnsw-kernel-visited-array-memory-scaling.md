### Context

This issue implements the roadmap item 'CUDA HNSW Kernel: Visited Array Memory Scaling' for the acceleration domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Mid-term (v1.9.0 – v2.0.0) and targets milestone v1.9.0.

Primary detail section: CUDA HNSW Kernel: Visited Array Memory Scaling

### Goal

Deliver the scoped changes for CUDA HNSW Kernel: Visited Array Memory Scaling in src/acceleration/ and complete the linked detail section in a release-ready state for v1.9.0.

### Detailed Scope

### CUDA HNSW Kernel: Visited Array Memory Scaling
**Priority:** Medium
**Target Version:** v1.9.0

`cuda/cuda_hnsw_kernels.cu:328–336` allocates a flat device buffer of `num_queries × num_nodes × sizeof(uint8_t)` bytes for the per-query visited bitset before each kernel launch (via `cudaMalloc` at line 330). For a production-scale graph of 10M nodes and a batch of 512 queries this is `512 × 10M = 5 GB` of device memory — far exceeding the `VLLMResourceManager::Config::max_gpu_vram_mb = 2048` limit — causing the `cudaMalloc` to fail silently (the kernel returns without writing output at line 332–334).

**Implementation Notes:**
- `[ ]` Replace per-invocation `cudaMalloc` / `cudaFree` with a persistent, pre-allocated pool owned by `CUDAVectorBackend`; size the pool at `maxBatchSize × numNodes × 1 byte` and allocate it once during `initialize()`.
- `[ ]` Switch from `uint8_t` visited array to a 1-bit-per-node bitset: allocate `ceil(numNodes / 8)` bytes per query (10M nodes → 1.25 MB per query, 512 queries → 640 MB — still large but feasible on 80 GB A100).
- `[ ]` For graphs where even the bitset exceeds budget: implement chunked batch processing — split `numQueries` into sub-batches small enough for the available pool, process serially, and concatenate results on the host.
- `[ ]` Expose `CUDAVectorBackend::setMaxBatchSize(size_t n)` so callers can tune the pool allocation at construction time.
- `[ ]` Add a `BackendHealthStatus::makeDegraded()` response when `cudaMalloc` fails during the HNSW kernel launch (currently the function returns silently, leaving output buffers zeroed).

**Performance Targets:**
- Eliminate per-query `cudaMalloc`/`cudaFree` round trips; visited-pool reuse should reduce HNSW launch overhead by ≥ 15% for repeated fixed-batch queries.
- Pool allocation must not exceed `BackendCapabilities::maxMemoryBytes` at construction time.

---

### Acceptance Criteria

- [ ] Replace per-invocation `cudaMalloc` / `cudaFree` with a persistent, pre-allocated pool owned by `CUDAVectorBackend`; size the pool at `maxBatchSize × numNodes × 1 byte` and allocate it once during `initialize()`.
- [ ] Switch from `uint8_t` visited array to a 1-bit-per-node bitset: allocate `ceil(numNodes / 8)` bytes per query (10M nodes → 1.25 MB per query, 512 queries → 640 MB — still large but feasible on 80 GB A100).
- [ ] For graphs where even the bitset exceeds budget: implement chunked batch processing — split `numQueries` into sub-batches small enough for the available pool, process serially, and concatenate results on the host.
- [ ] Expose `CUDAVectorBackend::setMaxBatchSize(size_t n)` so callers can tune the pool allocation at construction time.
- [ ] Add a `BackendHealthStatus::makeDegraded()` response when `cudaMalloc` fails during the HNSW kernel launch (currently the function returns silently, leaving output buffers zeroed).
- [ ] Eliminate per-query `cudaMalloc`/`cudaFree` round trips; visited-pool reuse should reduce HNSW launch overhead by ≥ 15% for repeated fixed-batch queries.
- [ ] Pool allocation must not exceed `BackendCapabilities::maxMemoryBytes` at construction time.

### Relationships

- Roadmap row: #219 (🟡 Medium Priority — Mid-term (v1.9.0 – v2.0.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/acceleration/FUTURE_ENHANCEMENTS.md#cuda-hnsw-kernel-visited-array-memory-scaling
- Source key: roadmap:219:acceleration:v1.9.0:cuda-hnsw-kernel-visited-array-memory-scaling

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:219:acceleration:v1.9.0:cuda-hnsw-kernel-visited-array-memory-scaling -->
<!-- roadmap-ref: row=219;module=acceleration;target=v1.9.0 -->
<!-- roadmap-detail: src/acceleration/FUTURE_ENHANCEMENTS.md#cuda-hnsw-kernel-visited-array-memory-scaling -->
