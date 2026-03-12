### Context

This issue implements the roadmap item 'FAISS GPU Backend: HNSW and ScalarQuantizer Index Types' for the acceleration domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Mid-term (v1.9.0 – v2.0.0) and targets milestone v1.9.0.

Primary detail section: FAISS GPU Backend: HNSW and ScalarQuantizer Index Types

### Goal

Deliver the scoped changes for FAISS GPU Backend: HNSW and ScalarQuantizer Index Types in src/acceleration/ and complete the linked detail section in a release-ready state for v1.9.0.

### Detailed Scope

### FAISS GPU Backend: HNSW and ScalarQuantizer Index Types
**Priority:** Medium
**Target Version:** v1.9.0

`faiss_gpu_backend.cpp` implements only `IVF_FLAT` (line 164) and `IVF_PQ` (line 187) index types. The FAISS library provides `GpuIndexIVFScalarQuantizer` (IVF_SQ8) for memory-efficient 8-bit quantized search with better recall than PQ at equivalent memory, and the CPU-only `IndexHNSWFlat` which can be combined with a GPU flat index for hybrid search. These omissions mean callers that need higher recall or lower-latency search at medium scale have no GPU-accelerated option.

**Implementation Notes:**
- `[ ]` Add `IndexType::IVF_SQ8` case in `FAISSGPUBackend::buildIndex()`: create a `faiss::gpu::GpuIndexIVFScalarQuantizer` with `faiss::ScalarQuantizer::QT_8bit`; register the destructor in the cleanup `switch` at line 226.
- `[ ]` Add `IndexType::FLAT` case: create a `faiss::gpu::GpuIndexFlatL2` or `GpuIndexFlatIP` depending on the distance metric; this is the baseline for exact search and already required for the IVF quantizer — expose it as a first-class index type.
- `[ ]` Add a `IndexType::HNSW_FLAT` case using CPU-side `faiss::IndexHNSWFlat` backed by a `faiss::gpu::StandardGpuResources` distance oracle for the inner-product step (FAISS `IndexHNSW` supports custom `DistanceComputer` that delegates to GPU flat index).
- `[ ]` Update `FAISSGPUBackend::getCapabilities()` to advertise the additional index types in the capabilities struct.
- `[ ]` Add a `default:` branch in the index creation switch (line 164) that returns `false` and sets `lastError_` to `AccelerationErrorCode::InvalidInputShape` — currently the switch falls through silently for unknown types.

---

### Acceptance Criteria

- [ ] Add `IndexType::IVF_SQ8` case in `FAISSGPUBackend::buildIndex()`: create a `faiss::gpu::GpuIndexIVFScalarQuantizer` with `faiss::ScalarQuantizer::QT_8bit`; register the destructor in the cleanup `switch` at line 226.
- [ ] Add `IndexType::FLAT` case: create a `faiss::gpu::GpuIndexFlatL2` or `GpuIndexFlatIP` depending on the distance metric; this is the baseline for exact search and already required for the IVF quantizer — expose it as a first-class index type.
- [ ] Add a `IndexType::HNSW_FLAT` case using CPU-side `faiss::IndexHNSWFlat` backed by a `faiss::gpu::StandardGpuResources` distance oracle for the inner-product step (FAISS `IndexHNSW` supports custom `DistanceComputer` that delegates to GPU flat index).
- [ ] Update `FAISSGPUBackend::getCapabilities()` to advertise the additional index types in the capabilities struct.
- [ ] Add a `default:` branch in the index creation switch (line 164) that returns `false` and sets `lastError_` to `AccelerationErrorCode::InvalidInputShape` — currently the switch falls through silently for unknown types.

### Relationships

- Roadmap row: #221 (🟡 Medium Priority — Mid-term (v1.9.0 – v2.0.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/acceleration/FUTURE_ENHANCEMENTS.md#faiss-gpu-backend-hnsw-and-scalarquantizer-index-types
- Source key: roadmap:221:acceleration:v1.9.0:faiss-gpu-backend-hnsw-and-scalarquantizer-index-types

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:221:acceleration:v1.9.0:faiss-gpu-backend-hnsw-and-scalarquantizer-index-types -->
<!-- roadmap-ref: row=221;module=acceleration;target=v1.9.0 -->
<!-- roadmap-detail: src/acceleration/FUTURE_ENHANCEMENTS.md#faiss-gpu-backend-hnsw-and-scalarquantizer-index-types -->
