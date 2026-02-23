# Acceleration Module Roadmap
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
Pre-production hardening — backend surface is broad, but production-grade kernel coverage, runtime negotiation and cross-backend validation are still in progress.

## Completed ✅
- [x] Directory structure for CUDA and Vulkan backends
- [x] Vector similarity search acceleration stubs
- [x] Geospatial query acceleration stubs
- [x] Parallel graph algorithm stubs
- [x] Matrix operations for embeddings (scaffolding)
- [x] Documentation cross-references (CUDA_BACKEND.md, VULKAN_BACKEND.md)

## In Progress 🚧
- [P] CUDA kernel implementations for vector similarity (Target: Q2 2026) (Issue: #1366)
- [I] Vulkan compute shader pipeline for cross-platform GPU (Target: Q2 2026) (Issue: #1367)
- [P] Integration with geo module GPU backend (Target: Q3 2026) (Issue: #1368)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [P] CUDA-accelerated ANN (Approximate Nearest Neighbor) search (Issue: #1369)
- [I] CUDA geospatial distance and containment kernels (Target: Q3 2026) (Issue: #1372)
- [x] Vulkan fallback for non-NVIDIA hardware (Target: Q3 2026) (Issue: #1373)
- [I] Runtime device detection and capability negotiation (Target: Q3 2026) (Issue: #1374)
- [P] Benchmark harness for CUDA vs CPU performance comparison (Target: Q3 2026) (Issue: #1375)
### Long-term (6-12 months)
- [P] ROCm/HIP support for AMD GPU acceleration (Issue: #1370) — `src/acceleration/hip/ann_kernels.hip` and `src/acceleration/hip/geo_kernels.hip` implemented
- [I] Multi-GPU sharding for large embedding datasets (Target: Q4 2026) (Issue: #1376)
- [I] Tensor Core utilization for matrix operations (FP16/BF16) (Target: Q4 2026) (Issue: #1377)
- [P] CUDA graph capture for recurring query workloads (Target: Q4 2026) (Issue: #1378)
- [I] OpenCL backend for broad hardware compatibility (Target: Q1 2027) (Issue: #1379)
## Implementation Phases

### Phase 1: Design / API-Vertrag
- [P] Define backend capability contract (feature matrix, precision modes, fallback order) (Target: Q2 2026) (Issue: #1380)
- [P] Freeze kernel invocation interfaces for ANN + geospatial operations (Target: Q2 2026) (Issue: #1381)
- [I] Define error taxonomy for device selection, kernel launch and validation failures (Target: Q2 2026) (Issue: #1382)

### Phase 2: Core-Implementierung
- [P] Implement CUDA ANN + geospatial kernels with production execution paths (Target: Q3 2026) (Issue: #1383) — ANN vector kernels (L2, cosine, inner-product, top-K) complete; geospatial pending
- [I] Implement Vulkan compute equivalents for baseline feature parity (Target: Q3 2026) (Issue: #1384)
- [x] Integrate capability-driven backend registry selection into runtime startup (Target: Q3 2026) (Issue: #1385) — `initializeRuntime()` added to `BackendRegistry`; `defaultVectorRequirements()` / `defaultGraphRequirements()` / `defaultGeoRequirements()` factory helpers; `getSelectedVectorBackend()` / `getSelectedGraphBackend()` / `getSelectedGeoBackend()` accessors; tests in `tests/test_backend_registry_startup.cpp`

### Phase 3: Fehlerbehandlung & Edge Cases
- [P] Add strict input validation for shape/dtype/range and reject unsafe batches (Target: Q3 2026) (Issue: #1386) — null pointer, zero-dim/count guards and k-clamp added to all active backends: `CUDAVectorBackend`, `CPUVectorBackend`, `CPUVectorBackendMT`, `CPUVectorBackendTBB`, `HIPVectorBackend`; geo and graph CPU backends also guarded via `BatchValidator` utility (`include/acceleration/batch_validator.h`)
- [I] Implement fallback/retry semantics for unsupported kernels and transient device states (Target: Q3 2026) (Issue: #1387)
- [I] Add deterministic behavior constraints for tie-breaking and partial-failure handling (Target: Q3 2026) (Issue: #1388)

### Phase 4: Tests
- [P] Add unit tests for backend selection and capability negotiation matrix (Target: Q3 2026) (Issue: #1389) — `tests/test_cuda_ann_search.cpp` added
- [P] Add GPU/CPU parity integration tests for ANN and geospatial queries (Target: Q3 2026) (Issue: #1390) — CPU parity tests and GPU end-to-end tests (skipped gracefully when no hardware) in `test_cuda_ann_search.cpp`
- [P] Add regression tests for invalid input and runtime fallback correctness (Target: Q3 2026) (Issue: #1391) — null-pointer, zero-dim, k-clamp regression tests in `test_cuda_ann_search.cpp`

### Phase 5: Performance/Hardening
- [P] Add benchmark suite with latency/throughput baselines per backend (Target: Q3 2026) (Issue: #1392) — `bench_cuda_vs_cpu` harness (CPU ANN + Geo) with JSON output; baselines in `benchmarks/baselines/acceleration/`
- [P] Establish performance gates for key workloads and batch sizes (Target: Q3 2026) (Issue: #1393) — regression thresholds (minor 5 %, major 10 %, critical 20 %) enforced in `.github/workflows/acceleration-benchmark-ci.yml`
- [I] Run security hardening pass for plugin/driver interaction surfaces (Target: Q4 2026) (Issue: #1394)

### Phase 6: Dokumentation & Abnahme
- [x] Publish backend capability matrix and configuration guide (Target: Q4 2026) (Issue: #1395) — covered in `docs/acceleration/capability_negotiation.md`
- [x] Publish operational troubleshooting guide for fallback and driver issues (Target: Q4 2026) (Issue: #1396) — covered in `docs/acceleration/capability_negotiation.md`
- [I] Final production-readiness review and API stability sign-off (Target: Q4 2026) (Issue: #1397)

## Implementation Phases

### Phase 1: Infrastructure Scaffolding (Status: Completed)
- [x] Created directory structure for `cuda/` and `vulkan/` backends
- [x] Implemented vector similarity search stubs (`acceleration/vector_similarity.cpp`)
- [x] Implemented geospatial query acceleration stubs (`acceleration/geo_stubs.cpp`)
- [x] Implemented parallel graph algorithm stubs (`acceleration/graph_stubs.cpp`)
- [x] Implemented matrix operation stubs for embeddings (`acceleration/matrix_ops.cpp`)
- [x] Added documentation cross-references (`CUDA_BACKEND.md`, `VULKAN_BACKEND.md`)

### Phase 2: CUDA and Vulkan Kernel Implementation (Status: In Progress)
- [I] Implement CUDA kernels for HNSW ANN search (`cuda/ann_kernels.cu`) (Issue: #1461)
- [I] Implement Vulkan compute shaders for cross-platform GPU pipeline (Issue: #1462)
- [!] Implement runtime device capability detection (`acceleration/device_manager.cpp`) (Issue: #2164)
- [P] Implement geo CUDA kernels for distance and containment (`cuda/geo_kernels.cu`)
- [P] Integrate with geo module GPU backend via `GeoAccelerationBridge` (Issue: #2134)

### Phase 3: Extended Hardware and Advanced Features (Status: Planned)
- [P] Add ROCm/HIP backend for AMD GPU acceleration (`hip/ann_kernels.hip`) (Issue: #1456) — `src/acceleration/hip/ann_kernels.hip` and `src/acceleration/hip/geo_kernels.hip` implemented; ANN kernels (L2, cosine, inner-product, top-K) and geospatial kernels (Haversine, point-in-polygon) complete; dispatch tables wired via `populateHIPANNDispatch`/`populateHIPGeoDispatch`
- [I] Implement multi-GPU sharding for large embedding datasets (Issue: #1457)
- [I] Enable Tensor Core FP16/BF16 matrix operations via `cublasHgemm` (Issue: #1458)
- [P] Implement CUDA graph capture for recurring query workloads (Issue: #1459) — `CUDAGraphCache` + `batchKnnSearchWithGraph()` implemented in `cuda_backend.h`/`cuda_backend.cpp`
- [x] Add benchmark harness comparing CUDA vs CPU throughput per operation type (Issue: #1460) — `bench_cuda_vs_cpu` harness with CPU ANN (L2/Cosine/InnerProduct/TopK/BatchKNN) and Geo (Haversine/PointInPolygon) benchmarks; CUDA benchmarks skipped gracefully without GPU; JSON output with regression detection via CI workflow `acceleration-benchmark-ci.yml`

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1398)
- [I] Integration tests for CPU/GPU parity across supported backends (Issue: #1399)
- [P] Performance benchmarks with regression thresholds in CI (Issue: #1400)
- [I] Security audit for backend plugin loading and runtime probes (Issue: #1401)
- [x] Documentation complete for capability negotiation and fallback behavior (Issue: #1402)
- [x] API stability guaranteed for acceleration backend contracts (Issue: #1403) — `BACKEND_CONTRACT_VERSION = 100` added to `compute_backend.h`; tests in `tests/test_backend_api_stability.cpp` verify all frozen enum values, struct field existence, version constants, and dispatcher behaviour

## Known Issues & Limitations
- CUDA and Vulkan backends are currently stub/scaffolding implementations
- Actual GPU kernels have not yet been written; all operations fall through to CPU
- No runtime device capability detection yet
- Multi-GPU sharding backend (`MultiGPUVectorBackend`) implemented in acceleration layer; uses CPU sub-backends pending real CUDA kernels
- Some backend source files are staged but not feature-complete for production traffic

## Breaking Changes
- GPU kernel APIs are not yet stable; function signatures may change before v1.0
