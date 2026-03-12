### Context

This issue implements the roadmap item 'Multi-GPU Sharding for Large Embedding Datasets' for the acceleration domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Mid-term (v1.9.0 – v2.0.0) and targets milestone v1.9.0.

Primary detail section: Multi-GPU Sharding for Large Embedding Datasets

### Goal

Deliver the scoped changes for Multi-GPU Sharding for Large Embedding Datasets in src/acceleration/ and complete the linked detail section in a release-ready state for v1.9.0.

### Detailed Scope

### Multi-GPU Sharding for Large Embedding Datasets
**Priority:** Medium
**Target Version:** v1.9.0

`nccl_vector_backend.cpp` and `rccl_vector_backend.cpp` stub NCCL/RCCL collective operations. Implement a sharding strategy in `BackendRegistry` that partitions an embedding index across N GPUs and scatters queries using NCCL `ncclBcast` + `ncclAllGather`. The tensor-parallel all-reduce communication pattern follows the Megatron-LM approach \[7\].

**Implementation Notes:**
- `[x]` Introduce `MultiGPUVectorBackend` in `multi_gpu_backend.cpp`; register it in `BackendRegistry` when `cudaGetDeviceCount() > 1`.
- `[x]` Shard by contiguous vector-ID ranges; store shard metadata in a `std::vector<ShardDescriptor>` on the host.
- `[~]` Use `ncclGroupStart` / `ncclGroupEnd` to batch cross-GPU transfers. (NCCL/RCCL backends initialized; actual group-call wiring pending `mergeTopK` implementation above.)
- `[x]` RCCL mirror: `rccl_vector_backend.cpp` exposes the same `IVectorBackend` interface; `BackendRegistry` selects NCCL vs RCCL at runtime via `cudaGetDeviceProperties`.
- `[x]` Graceful degradation: if NCCL init fails, fall back to single-GPU or CPU backend.

**Performance Targets:**
- 100M × 128-dim index distributed across 4× A100 80GB; query latency < 15 ms @ 99th percentile for k=100.
- Linear scaling efficiency ≥ 75% from 1→4 GPUs.

---

### Acceptance Criteria

- [ ] Introduce `MultiGPUVectorBackend` in `multi_gpu_backend.cpp`; register it in `BackendRegistry` when `cudaGetDeviceCount() > 1`.
- [ ] Shard by contiguous vector-ID ranges; store shard metadata in a `std::vector<ShardDescriptor>` on the host.
- [ ] Use `ncclGroupStart` / `ncclGroupEnd` to batch cross-GPU transfers. (NCCL/RCCL backends initialized; actual group-call wiring pending `mergeTopK` implementation above.)
- [ ] RCCL mirror: `rccl_vector_backend.cpp` exposes the same `IVectorBackend` interface; `BackendRegistry` selects NCCL vs RCCL at runtime via `cudaGetDeviceProperties`.
- [ ] Graceful degradation: if NCCL init fails, fall back to single-GPU or CPU backend.
- [ ] 100M × 128-dim index distributed across 4× A100 80GB; query latency < 15 ms @ 99th percentile for k=100.
- [ ] Linear scaling efficiency ≥ 75% from 1→4 GPUs.

### Relationships

- Roadmap row: #222 (🟡 Medium Priority — Mid-term (v1.9.0 – v2.0.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/acceleration/FUTURE_ENHANCEMENTS.md#multi-gpu-sharding-for-large-embedding-datasets
- Source key: roadmap:222:acceleration:v1.9.0:multi-gpu-sharding-for-large-embedding-datasets

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:222:acceleration:v1.9.0:multi-gpu-sharding-for-large-embedding-datasets -->
<!-- roadmap-ref: row=222;module=acceleration;target=v1.9.0 -->
<!-- roadmap-detail: src/acceleration/FUTURE_ENHANCEMENTS.md#multi-gpu-sharding-for-large-embedding-datasets -->
