### Context

This issue implements the roadmap item 'OpenGL Compute Shader Backend: Complete 5 Remaining Stubs' for the acceleration domain. It is sourced from the consolidated roadmap under 🟢 Low Priority — Future (v1.9.0+) and targets milestone v2.0.0.

Primary detail section: OpenGL Compute Shader Backend: Complete 5 Remaining Stubs

### Goal

Deliver the scoped changes for OpenGL Compute Shader Backend: Complete 5 Remaining Stubs in src/acceleration/ and complete the linked detail section in a release-ready state for v2.0.0.

### Detailed Scope

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

### Acceptance Criteria

- [ ] Implement `OpenGLVectorBackend::batchKnnSearch()` using the existing EGL + compute-shader infrastructure from `computeDistances()` (line 1766): dispatch the L2/cosine GLSL shader, read back distances, perform top-K on CPU with `std::nth_element`, return `std::vector<std::vector<std::pair<uint32_t,float>>>`.
- [ ] Implement `batchBFS()` and `batchShortestPaths()` using GLSL compute shaders equivalent to the Vulkan `batch_search.comp` and `topk_selection.comp`; store adjacency list in an SSBO.
- [ ] Implement `batchPointInPolygon()` and `batchHaversineDistance()` porting the Vulkan `point_in_polygon.comp` and `haversine_distance.comp` shaders to GLSL 4.30 (minimal changes — both shaders are already GLSL-compatible).
- [ ] Update the status banner comment to *"Stubs: 0"* and maturity to `🟢 PRODUCTION-READY` once all five are complete.
- [ ] Mark `OpenGLVectorBackend::getCapabilities().supportsAsync = false` remains accurate; document it in the header.

### Relationships

- Roadmap row: #235 (🟢 Low Priority — Future (v1.9.0+))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/acceleration/FUTURE_ENHANCEMENTS.md#opengl-compute-shader-backend-complete-5-remaining-stubs
- Source key: roadmap:235:acceleration:v2.0.0:opengl-compute-shader-backend-complete-5-remaining-stubs

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:235:acceleration:v2.0.0:opengl-compute-shader-backend-complete-5-remaining-stubs -->
<!-- roadmap-ref: row=235;module=acceleration;target=v2.0.0 -->
<!-- roadmap-detail: src/acceleration/FUTURE_ENHANCEMENTS.md#opengl-compute-shader-backend-complete-5-remaining-stubs -->
