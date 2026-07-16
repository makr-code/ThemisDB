### Context

This issue implements the roadmap item 'CUDA HNSW Kernel: Remove Silent `k > kMaxK` Clamping' for the acceleration domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: CUDA HNSW Kernel: Remove Silent `k > kMaxK` Clamping

### Goal

Deliver the scoped changes for CUDA HNSW Kernel: Remove Silent `k > kMaxK` Clamping in src/acceleration/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### CUDA HNSW Kernel: Remove Silent `k > kMaxK` Clamping
**Priority:** Medium
**Target Version:** v1.8.0

`cuda/cuda_hnsw_kernels.cu:25` defines `static constexpr uint32_t kMaxK = 256u`. The launch wrapper at line 325 silently clamps `k` to `kMaxK` before launching: `if (k > kMaxK) k = kMaxK;`. The caller in `cuda_backend.cpp` receives truncated results with no indication that fewer than the requested `k` neighbours were returned. For use cases requiring k > 256 (e.g., re-ranking pipelines requesting k=512 candidates), this produces silently wrong output.

**Implementation Notes:**
- `[ ]` Replace the silent clamp in `cuda/cuda_hnsw_kernels.cu:325` with an explicit error return: set `cudaGetLastError()` to a sentinel, or add a `bool* d_overflow` output flag that the host can check; propagate the overflow condition up through `CUDAVectorBackend::buildHnswAnnIndex()` / `batchKnnSearchWithGraph()` as an `AccelerationErrorCode::InvalidInputShape` error.
- `[ ]` Increase `kMaxK` to 1024 by moving the per-query result buffers (`res_dist[kMaxK]`, `res_id[kMaxK]`) from fixed-size shared memory arrays to dynamically allocated shared memory via `extern __shared__`; compute required shared memory as `k * (sizeof(float) + sizeof(int32_t))` per thread and pass it as the third `<<<>>>` launch argument.
- `[ ]` For k > 1024 (extreme re-ranking): fall through to a multi-pass strategy — run `kMaxK`-at-a-time passes over graph layers and merge on host using `std::partial_sort`.
- `[ ]` Add static_assert or CUDA `__trap()` guard in debug builds when `k > kMaxK` is detected; surface as `BackendHealthStatus::makeDegraded()` in release builds.
- `[ ]` Test: add `tests/acceleration/test_cuda_hnsw_large_k.cpp` with k=257, k=512, k=1024 asserting result count equals requested k.

**Performance Targets:**
- k=256: no regression vs. current implementation.
- k=1024 with dynamic shared memory: < 20 ms for 10K queries × 1M vectors on RTX 3090.

---

### Acceptance Criteria

- [ ] Replace the silent clamp in `cuda/cuda_hnsw_kernels.cu:325` with an explicit error return: set `cudaGetLastError()` to a sentinel, or add a `bool* d_overflow` output flag that the host can check; propagate the overflow condition up through `CUDAVectorBackend::buildHnswAnnIndex()` / `batchKnnSearchWithGraph()` as an `AccelerationErrorCode::InvalidInputShape` error.
- [ ] Increase `kMaxK` to 1024 by moving the per-query result buffers (`res_dist[kMaxK]`, `res_id[kMaxK]`) from fixed-size shared memory arrays to dynamically allocated shared memory via `extern __shared__`; compute required shared memory as `k * (sizeof(float) + sizeof(int32_t))` per thread and pass it as the third `<<<>>>` launch argument.
- [ ] For k > 1024 (extreme re-ranking): fall through to a multi-pass strategy — run `kMaxK`-at-a-time passes over graph layers and merge on host using `std::partial_sort`.
- [ ] Add static_assert or CUDA `__trap()` guard in debug builds when `k > kMaxK` is detected; surface as `BackendHealthStatus::makeDegraded()` in release builds.
- [ ] Test: add `tests/acceleration/test_cuda_hnsw_large_k.cpp` with k=257, k=512, k=1024 asserting result count equals requested k.
- [ ] k=256: no regression vs. current implementation.
- [ ] k=1024 with dynamic shared memory: < 20 ms for 10K queries × 1M vectors on RTX 3090.

### Relationships

- Roadmap row: #132 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/acceleration/FUTURE_ENHANCEMENTS.md#cuda-hnsw-kernel-remove-silent-k--kmaxk-clamping
- Source key: roadmap:132:acceleration:v1.8.0:cuda-hnsw-kernel-remove-silent-k-kmaxk-clamping

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:132:acceleration:v1.8.0:cuda-hnsw-kernel-remove-silent-k-kmaxk-clamping -->
<!-- roadmap-ref: row=132;module=acceleration;target=v1.8.0 -->
<!-- roadmap-detail: src/acceleration/FUTURE_ENHANCEMENTS.md#cuda-hnsw-kernel-remove-silent-k--kmaxk-clamping -->
