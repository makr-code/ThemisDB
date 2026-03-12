### Context

This issue implements the roadmap item 'Vulkan Compute Shader Pipeline for Cross-Platform GPU' for the acceleration domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: Vulkan Compute Shader Pipeline for Cross-Platform GPU

### Goal

Deliver the scoped changes for Vulkan Compute Shader Pipeline for Cross-Platform GPU in src/acceleration/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### Vulkan Compute Shader Pipeline for Cross-Platform GPU
**Priority:** High
**Target Version:** v1.7.0
**Status:** ✅ IMPLEMENTED

`vulkan_backend_full.cpp` is PRODUCTION-READY (0 stubs). All SPIR-V compute shaders for vector distance and geospatial operations are implemented in `vulkan/shaders/`: `l2_distance.comp`, `cosine_distance.comp`, `inner_product_distance.comp`, `batch_search.comp`, `topk_selection.comp`, `haversine_distance.comp`, `point_in_polygon.comp`. The LoRA shaders (`matmul.comp`, `elementwise.comp`, `gradient.comp`, etc.) are also complete.

**Remaining Hardening:**
- `[ ]` MoltenVK path: verify `VK_KHR_buffer_device_address` capability probe on Apple M-series.
- `[ ]` Benchmark on Mali-G710 and RDNA2 to validate workgroup size (256 threads for `batch_search.comp`, 16×16 for `l2_distance.comp`) occupancy targets; expose as SPIR-V specialization constants (see Kernel Block-Dimension Occupancy Tuning above).
- `[ ]` Double-buffer staging buffers to overlap host→device DMA with shader dispatch.

**Performance Targets:**
- 500K × 128-dim cosine search in < 20 ms on Apple M2 Pro via MoltenVK.
- < 5% throughput regression versus CUDA path on AMD RX 7800 XT.

---

### Acceptance Criteria

- [ ] MoltenVK path: verify `VK_KHR_buffer_device_address` capability probe on Apple M-series.
- [ ] Benchmark on Mali-G710 and RDNA2 to validate workgroup size (256 threads for `batch_search.comp`, 16×16 for `l2_distance.comp`) occupancy targets; expose as SPIR-V specialization constants (see Kernel Block-Dimension Occupancy Tuning above).
- [ ] Double-buffer staging buffers to overlap host→device DMA with shader dispatch.
- [ ] 500K × 128-dim cosine search in < 20 ms on Apple M2 Pro via MoltenVK.
- [ ] < 5% throughput regression versus CUDA path on AMD RX 7800 XT.

### Relationships

- Roadmap row: #36 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/acceleration/FUTURE_ENHANCEMENTS.md#vulkan-compute-shader-pipeline-for-cross-platform-gpu
- Source key: roadmap:36:acceleration:v1.7.0:vulkan-compute-shader-pipeline-for-cross-platform-gpu

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:36:acceleration:v1.7.0:vulkan-compute-shader-pipeline-for-cross-platform-gpu -->
<!-- roadmap-ref: row=36;module=acceleration;target=v1.7.0 -->
<!-- roadmap-detail: src/acceleration/FUTURE_ENHANCEMENTS.md#vulkan-compute-shader-pipeline-for-cross-platform-gpu -->
