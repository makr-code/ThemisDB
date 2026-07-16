### Context

This issue implements the roadmap item 'Reed-Solomon Repair Engine Parallelisation' for the sharding domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.6.0.

Primary detail section: [ ] Reed-Solomon Repair Engine Parallelisation

### Goal

Deliver the scoped changes for Reed-Solomon Repair Engine Parallelisation in src/sharding/ and complete the linked detail section in a release-ready state for v1.6.0.

### Detailed Scope

### [ ] Reed-Solomon Repair Engine Parallelisation
**Priority:** Medium
**Target Version:** v0.9.0

Parallelise the anti-entropy scan and Reed-Solomon reconstruction in `shard_repair_engine.cpp` to exploit multi-core hardware. The current implementation is single-threaded; large shards (~100 GB) take hours to fully scan. GPU-accelerated erasure coding via `gpu_erasure_coder.cpp` should be optionally engaged for bulk repair.

**Implementation Notes:**
- Refactor `shard_repair_engine.cpp` to use a work-stealing thread pool (`utils/thread_pool_manager.cpp`) for parallel segment scanning; partition the shard key space into scan bands, one per worker thread.
- Gate GPU erasure coding behind a runtime feature flag in `shard_resource_manager.cpp`; fall back to CPU path (`gpu_erasure_coder_opencl.cpp`) when no CUDA device is present.
- Throttle repair I/O using token-bucket rate limiter in `shard_resource_manager.cpp` to enforce the 10% IOPS budget constraint.
- Report repair progress via `slo_monitor.cpp` so operators can track time-to-full-repair.

**Performance Targets:**
- Anti-entropy scan throughput: >1 GB/s per node on NVMe with 8 parallel workers.
- GPU Reed-Solomon reconstruction: >4 GB/s on NVIDIA A10 (`gpu_erasure_coder.cu`).
- IOPS consumption during repair: <10% of node peak IOPS.

---

### Acceptance Criteria

- [ ] Refactor `shard_repair_engine.cpp` to use a work-stealing thread pool (`utils/thread_pool_manager.cpp`) for parallel segment scanning; partition the shard key space into scan bands, one per worker thread.
- [ ] Gate GPU erasure coding behind a runtime feature flag in `shard_resource_manager.cpp`; fall back to CPU path (`gpu_erasure_coder_opencl.cpp`) when no CUDA device is present.
- [ ] Throttle repair I/O using token-bucket rate limiter in `shard_resource_manager.cpp` to enforce the 10% IOPS budget constraint.
- [ ] Report repair progress via `slo_monitor.cpp` so operators can track time-to-full-repair.
- [ ] Anti-entropy scan throughput: >1 GB/s per node on NVMe with 8 parallel workers.
- [ ] GPU Reed-Solomon reconstruction: >4 GB/s on NVIDIA A10 (`gpu_erasure_coder.cu`).
- [ ] IOPS consumption during repair: <10% of node peak IOPS.

### Relationships

- Roadmap row: #204 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/sharding/FUTURE_ENHANCEMENTS.md#-reed-solomon-repair-engine-parallelisation
- Source key: roadmap:204:sharding:v1.6.0:reed-solomon-repair-engine-parallelisation

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:204:sharding:v1.6.0:reed-solomon-repair-engine-parallelisation -->
<!-- roadmap-ref: row=204;module=sharding;target=v1.6.0 -->
<!-- roadmap-detail: src/sharding/FUTURE_ENHANCEMENTS.md#-reed-solomon-repair-engine-parallelisation -->
