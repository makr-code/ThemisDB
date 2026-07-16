### Context

This issue implements the roadmap item 'CUDA and OpenCL Implementation in `gpu_backend_production.cpp`' for the geo domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Immediate (≤ v1.4.0) and targets milestone v1.4.0.

Primary detail section: CUDA and OpenCL Implementation in `gpu_backend_production.cpp`

### Goal

Deliver the scoped changes for CUDA and OpenCL Implementation in `gpu_backend_production.cpp` in src/geo/ and complete the linked detail section in a release-ready state for v1.4.0.

### Detailed Scope

### CUDA and OpenCL Implementation in `gpu_backend_production.cpp`
**Priority:** High
**Target Version:** v1.4.0

`gpu_backend_production.cpp` has 2 explicit TODO comments:
- Line 262: `// TODO v1.4.0: Complete CUDA implementation with geometry data processing`
- Line 362: `// TODO v1.4.0: Complete OpenCL implementation`

Both GPU paths currently fall back to the CPU backend, making the "production" GPU backend no different from the CPU backend.

**Implementation Notes:**
- `[ ]` Implement CUDA geometry data processing at line 262: upload WGS-84 point arrays to device memory using `cudaMalloc`; dispatch the distance/containment kernels in `gpu/cuda_kernels.cu`; download results via `cudaMemcpy`.
- `[ ]` Implement OpenCL dispatch at line 362: compile geo kernels via `clCreateProgramWithSource` at startup; enqueue NDRange kernels for point-in-polygon and distance batch operations.
- `[ ]` Add CUDA/OpenCL parity tests: compare GPU and CPU results for 10 K point dataset; verify max absolute error ≤ 1 mm for WGS-84 distance.
- `[ ]` Register a `GpuBackendRegistry` entry (currently a "Simple internal registry stub" in `cpu_backend.cpp` line 914) so the production GPU backend is discoverable at runtime.

**Performance Targets:**
- GPU batch distance (1 M WGS-84 point pairs): ≥ 8× speedup vs. CPU `boost_cpu_exact_backend.cpp` on RTX-class hardware.
- GPU point-in-polygon (1 M points, 1 K polygon): ≥ 5× speedup vs. CPU.

---


**Priority:** High
**Target Version:** v1.6.0
**Status:** ✅ Implemented in `src/geo/cpu_backend.cpp` and `src/geo/boost_cpu_exact_backend.cpp`

All seven geometry types (`Point`, `MultiPoint`, `LineString`, `MultiLineString`, `Polygon`, `MultiPolygon`, `GeometryCollection`) are handled. `GeometryCollection` is parsed recursively. Strict coordinate-range validation rejects out-of-range WGS84 values; a `THEMIS_GEO_COMPAT_LAX` build flag provides a one-release migration window.

---

### Acceptance Criteria

- [ ] Line 262: `// TODO v1.4.0: Complete CUDA implementation with geometry data processing`
- [ ] Line 362: `// TODO v1.4.0: Complete OpenCL implementation`
- [ ] Implement CUDA geometry data processing at line 262: upload WGS-84 point arrays to device memory using `cudaMalloc`; dispatch the distance/containment kernels in `gpu/cuda_kernels.cu`; download results via `cudaMemcpy`.
- [ ] Implement OpenCL dispatch at line 362: compile geo kernels via `clCreateProgramWithSource` at startup; enqueue NDRange kernels for point-in-polygon and distance batch operations.
- [ ] Add CUDA/OpenCL parity tests: compare GPU and CPU results for 10 K point dataset; verify max absolute error ≤ 1 mm for WGS-84 distance.
- [ ] Register a `GpuBackendRegistry` entry (currently a "Simple internal registry stub" in `cpu_backend.cpp` line 914) so the production GPU backend is discoverable at runtime.
- [ ] GPU batch distance (1 M WGS-84 point pairs): ≥ 8× speedup vs. CPU `boost_cpu_exact_backend.cpp` on RTX-class hardware.
- [ ] GPU point-in-polygon (1 M points, 1 K polygon): ≥ 5× speedup vs. CPU.

### Relationships

- Roadmap row: #30 (🟠 High Priority — Immediate (≤ v1.4.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/geo/FUTURE_ENHANCEMENTS.md#cuda-and-opencl-implementation-in-gpu_backend_productioncpp
- Source key: roadmap:30:geo:v1.4.0:cuda-and-opencl-implementation-in-gpu-backend-productioncpp

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:30:geo:v1.4.0:cuda-and-opencl-implementation-in-gpu-backend-productioncpp -->
<!-- roadmap-ref: row=30;module=geo;target=v1.4.0 -->
<!-- roadmap-detail: src/geo/FUTURE_ENHANCEMENTS.md#cuda-and-opencl-implementation-in-gpu_backend_productioncpp -->
