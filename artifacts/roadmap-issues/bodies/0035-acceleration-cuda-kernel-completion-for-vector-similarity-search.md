### Context

This issue implements the roadmap item 'CUDA Kernel Completion for Vector Similarity Search' for the acceleration domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: CUDA Kernel Completion for Vector Similarity Search

### Goal

Deliver the scoped changes for CUDA Kernel Completion for Vector Similarity Search in src/acceleration/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### CUDA Kernel Completion for Vector Similarity Search
**Priority:** High
**Target Version:** v1.7.0

`cuda_backend.cpp` declares stub kernel launch functions (`launchL2DistanceKernel`, `launchCosineDistanceKernel`, `launchTopKKernel`, …). The core kernels have been implemented in `cuda/ann_kernels.cu` and `cuda/vector_kernels.cu`; the remaining work is wiring the HNSW index layer to call these CUDA kernels instead of the CPU fallback. cuBLAS batched GEMM is the target for L2/cosine distance; CUB `DeviceSegmentedSort` is the target for top-k selection \[6\].

**Implementation Notes:**
- `[~]` `.cu` kernel files (`cuda/ann_kernels.cu`, `cuda/vector_kernels.cu`) are implemented; HNSW graph traversal wiring into `CUDAVectorBackend` is still pending.
- `[ ]` Cosine distance: fuse L2-norm and dot-product into a single tiled kernel to avoid a second pass over device memory (IO-aware pattern per FlashAttention \[3\]).
- `[ ]` Top-k (k ≤ 1024): use CUB `DeviceSegmentedSort` \[6\]; for k > 1024 fall back to `thrust::partial_sort`.
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

### Acceptance Criteria

- [ ] `.cu` kernel files (`cuda/ann_kernels.cu`, `cuda/vector_kernels.cu`) are implemented; HNSW graph traversal wiring into `CUDAVectorBackend` is still pending.
- [ ] Cosine distance: fuse L2-norm and dot-product into a single tiled kernel to avoid a second pass over device memory (IO-aware pattern per FlashAttention \[3\]).
- [ ] Top-k (k ≤ 1024): use CUB `DeviceSegmentedSort` \[6\]; for k > 1024 fall back to `thrust::partial_sort`.
- [ ] Add `CUDA_ARCH` compile-time guard: require sm_70+ (Tensor Core availability); emit warning for sm_60.
- [ ] 1M × 128-dim float32 L2 search in < 8 ms on RTX 3090 (single GPU).
- [ ] Throughput ≥ 10× CPU AVX2 baseline measured by `benchmarks/vector_bench.cpp`.
- [ ] GPU memory footprint < 2 GB for 10M 128-dim vectors.

### Relationships

- Roadmap row: #35 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/acceleration/FUTURE_ENHANCEMENTS.md#cuda-kernel-completion-for-vector-similarity-search
- Source key: roadmap:35:acceleration:v1.7.0:cuda-kernel-completion-for-vector-similarity-search

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:35:acceleration:v1.7.0:cuda-kernel-completion-for-vector-similarity-search -->
<!-- roadmap-ref: row=35;module=acceleration;target=v1.7.0 -->
<!-- roadmap-detail: src/acceleration/FUTURE_ENHANCEMENTS.md#cuda-kernel-completion-for-vector-similarity-search -->
