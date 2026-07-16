### Context

This issue implements the roadmap item 'NCCL/RCCL Distributed `mergeTopK` Implementation' for the acceleration domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.9.0.

Primary detail section: NCCL/RCCL Distributed `mergeTopK` Implementation

### Goal

Deliver the scoped changes for NCCL/RCCL Distributed `mergeTopK` Implementation in src/acceleration/ and complete the linked detail section in a release-ready state for v1.9.0.

### Detailed Scope

### NCCL/RCCL Distributed `mergeTopK` Implementation
**Priority:** High
**Target Version:** v1.9.0

`nccl_vector_backend.cpp:403–437` and the identical block in `rccl_vector_backend.cpp:403–437` both contain a stub that prints to `std::cerr` and returns `false` for any `worldSize > 1` call to `mergeTopK()`. The single-rank fast-path (device-to-device `cudaMemcpy`) is the only working code path. Without `mergeTopK`, the multi-GPU sharding strategy in `multi_gpu_backend.cpp` cannot aggregate partial top-K results from individual GPU shards.

**Root Cause:** The function signature, per-rank local buffers, and NCCL communicator handle are all in place; only the collective gather-and-sort logic at the root rank is missing.

**Implementation Notes:**
- `[x]` NCCL/RCCL communicator initialized; single-rank copy path implemented in `NCCLVectorBackend::mergeTopK()` / `RCCLVectorBackend::mergeTopK()`.
- `[ ]` Implement multi-rank gather in `NCCLVectorBackend::mergeTopK()` (`nccl_vector_backend.cpp:435`): call `ncclGather` (or `ncclAllGather` + root-side selection) to collect per-GPU top-K distances and indices; perform a CPU-side merge sort at the root rank using `std::nth_element` over `worldSize × k` candidates, then broadcast the global top-K via `ncclBcast`.
- `[ ]` Mirror identical fix in `RCCLVectorBackend::mergeTopK()` (`rccl_vector_backend.cpp:435`); the two files share the same logical structure.
- `[ ]` Add a `ncclGroupStart()` / `ncclGroupEnd()` bracket around the gather+bcast to pipeline the two collectives and reduce latency by ~30% on NVLink-connected nodes.
- `[ ]` Remove `(void)root; (void)stream;` suppression lines once the body is implemented.
- `[ ]` Add integration test `tests/acceleration/test_nccl_merge_topk.cpp` validating merge correctness for `worldSize` ∈ {2, 4, 8} with k ∈ {10, 100, 256}.

**Performance Targets:**
- 100M × 128-dim index distributed across 4× A100 80 GB; p99 query latency < 15 ms for k=100.
- `mergeTopK` overhead < 500 µs for worldSize=4, k=100 on NVLink-3 interconnect.
- Linear scaling efficiency ≥ 75% from 1→4 GPUs measured by `benchmarks/multi_gpu_bench.cpp`.

---

### Acceptance Criteria

- [ ] NCCL/RCCL communicator initialized; single-rank copy path implemented in `NCCLVectorBackend::mergeTopK()` / `RCCLVectorBackend::mergeTopK()`.
- [ ] Implement multi-rank gather in `NCCLVectorBackend::mergeTopK()` (`nccl_vector_backend.cpp:435`): call `ncclGather` (or `ncclAllGather` + root-side selection) to collect per-GPU top-K distances and indices; perform a CPU-side merge sort at the root rank using `std::nth_element` over `worldSize × k` candidates, then broadcast the global top-K via `ncclBcast`.
- [ ] Mirror identical fix in `RCCLVectorBackend::mergeTopK()` (`rccl_vector_backend.cpp:435`); the two files share the same logical structure.
- [ ] Add a `ncclGroupStart()` / `ncclGroupEnd()` bracket around the gather+bcast to pipeline the two collectives and reduce latency by ~30% on NVLink-connected nodes.
- [ ] Remove `(void)root; (void)stream;` suppression lines once the body is implemented.
- [ ] Add integration test `tests/acceleration/test_nccl_merge_topk.cpp` validating merge correctness for `worldSize` ∈ {2, 4, 8} with k ∈ {10, 100, 256}.
- [ ] 100M × 128-dim index distributed across 4× A100 80 GB; p99 query latency < 15 ms for k=100.
- [ ] `mergeTopK` overhead < 500 µs for worldSize=4, k=100 on NVLink-3 interconnect.
- [ ] Linear scaling efficiency ≥ 75% from 1→4 GPUs measured by `benchmarks/multi_gpu_bench.cpp`.

### Relationships

- Roadmap row: #39 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/acceleration/FUTURE_ENHANCEMENTS.md#ncclrccl-distributed-mergetopk-implementation
- Source key: roadmap:39:acceleration:v1.9.0:ncclrccl-distributed-mergetopk-implementation

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:39:acceleration:v1.9.0:ncclrccl-distributed-mergetopk-implementation -->
<!-- roadmap-ref: row=39;module=acceleration;target=v1.9.0 -->
<!-- roadmap-detail: src/acceleration/FUTURE_ENHANCEMENTS.md#ncclrccl-distributed-mergetopk-implementation -->
