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
- [!] ROCm/HIP support for AMD GPU acceleration (Issue: #1370)
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
- [P] Implement Vulkan compute equivalents for baseline feature parity (Target: Q3 2026) (Issue: #1384) — inner product pipeline added; ANNKernelDispatch + GeoKernelDispatch populated; VulkanGeoBackend implemented; haversine and point-in-polygon shaders added
- [P] Integrate capability-driven backend registry selection into runtime startup (Target: Q3 2026) (Issue: #1385) — `supportedPrecisions`/`supportedMetrics` now populated in `CUDAVectorBackend::getCapabilities()`

### Phase 3: Fehlerbehandlung & Edge Cases
- [P] Add strict input validation for shape/dtype/range and reject unsafe batches (Target: Q3 2026) (Issue: #1386) — null pointer, zero-dim/count guards and k-clamp added to `computeDistances()`/`batchKnnSearch()`
- [P] Implement fallback/retry semantics for unsupported kernels and transient device states (Target: Q3 2026) (Issue: #1387)
- [I] Add deterministic behavior constraints for tie-breaking and partial-failure handling (Target: Q3 2026) (Issue: #1388)

### Phase 4: Tests
- [P] Add unit tests for backend selection and capability negotiation matrix (Target: Q3 2026) (Issue: #1389) — `tests/test_cuda_ann_search.cpp` added
- [P] Add GPU/CPU parity integration tests for ANN and geospatial queries (Target: Q3 2026) (Issue: #1390) — CPU parity tests and GPU end-to-end tests (skipped gracefully when no hardware) in `test_cuda_ann_search.cpp`
- [P] Add regression tests for invalid input and runtime fallback correctness (Target: Q3 2026) (Issue: #1391) — null-pointer, zero-dim, k-clamp regression tests in `test_cuda_ann_search.cpp`

### Phase 5: Performance/Hardening
- [I] Add benchmark suite with latency/throughput baselines per backend (Target: Q3 2026) (Issue: #1392)
- [I] Establish performance gates for key workloads and batch sizes (Target: Q3 2026) (Issue: #1393)
- [I] Run security hardening pass for plugin/driver interaction surfaces (Target: Q4 2026) (Issue: #1394)

### Phase 6: Dokumentation & Abnahme
- [I] Publish backend capability matrix and configuration guide (Target: Q4 2026) (Issue: #1395)
- [I] Publish operational troubleshooting guide for fallback and driver issues (Target: Q4 2026) (Issue: #1396)
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
- [!] Add ROCm/HIP backend for AMD GPU acceleration (`hip/ann_kernels.hip`) (Issue: #1456)
- [I] Implement multi-GPU sharding for large embedding datasets (Issue: #1457)
- [I] Enable Tensor Core FP16/BF16 matrix operations via `cublasHgemm` (Issue: #1458)
- [P] Implement CUDA graph capture for recurring query workloads (Issue: #1459) — `CUDAGraphCache` + `batchKnnSearchWithGraph()` implemented in `cuda_backend.h`/`cuda_backend.cpp`
- [P] Add benchmark harness comparing CUDA vs CPU throughput per operation type (Issue: #1460)

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1398)
- [P] Integration tests for CPU/GPU parity across supported backends (Issue: #1399) — `tests/test_cpu_gpu_parity.cpp` added: 14 GTest cases across 5 fixtures (CUDA ANN, CUDA Geo, Vulkan ANN, Vulkan Geo, Registry); tests skip gracefully when backend not compiled in or no device present
- [I] Performance benchmarks with regression thresholds in CI (Issue: #1400)
- [I] Security audit for backend plugin loading and runtime probes (Issue: #1401)
- [I] Documentation complete for capability negotiation and fallback behavior (Issue: #1402)
- [I] API stability guaranteed for acceleration backend contracts (Issue: #1403)

## Known Issues & Limitations
- CUDA and Vulkan backends are currently stub/scaffolding implementations
- Actual GPU kernels have not yet been written; all operations fall through to CPU
- No runtime device capability detection yet
- Multi-GPU support not implemented
- Some backend source files are staged but not feature-complete for production traffic

## Breaking Changes
- GPU kernel APIs are not yet stable; function signatures may change before v1.0
