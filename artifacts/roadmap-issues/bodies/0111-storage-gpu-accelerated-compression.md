### Context

This issue implements the roadmap item 'GPU-Accelerated Compression' for the storage domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.6.0.

Primary detail section: GPU-Accelerated Compression

### Goal

Deliver the scoped changes for GPU-Accelerated Compression in src/storage/ and complete the linked detail section in a release-ready state for v1.6.0.

### Detailed Scope

### GPU-Accelerated Compression
**Priority:** High  
**Target Version:** v1.6.0

Use CUDA/ROCm for parallel compression/decompression.

**Target Algorithms:**
- Zstd (NVIDIA nvCOMP library)
- Snappy (GPU-accelerated variant)
- LZ4 (parallel decompress)

**Expected Improvement:** 5-10x compression throughput

---

### Acceptance Criteria

- [ ] Zstd (NVIDIA nvCOMP library)
- [ ] Snappy (GPU-accelerated variant)
- [ ] LZ4 (parallel decompress)

### Relationships

- Roadmap row: #111 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/storage/FUTURE_ENHANCEMENTS.md#gpu-accelerated-compression
- Source key: roadmap:111:storage:v1.6.0:gpu-accelerated-compression

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:111:storage:v1.6.0:gpu-accelerated-compression -->
<!-- roadmap-ref: row=111;module=storage;target=v1.6.0 -->
<!-- roadmap-detail: src/storage/FUTURE_ENHANCEMENTS.md#gpu-accelerated-compression -->
