# Acceleration Module Roadmap
<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · docs/de/acceleration/README.md -->
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
Production hardening complete — all GPU kernel surfaces and design contracts are in place; the CUDA ANN end-to-end path (HNSW integration) is fully wired: `CUDAVectorBackend::buildHnswAnnIndex()` loads the graph onto the device and `batchKnnSearch()` delegates to GPU-accelerated HNSW traversal when an index is pre-built (`cuda/cuda_hnsw_kernels.cu`).

## Completed ✅
- [x] Directory structure for CUDA and Vulkan backends
- [x] Vector similarity search acceleration stubs
- [x] Geospatial query acceleration stubs
- [x] Parallel graph algorithm stubs
- [x] Matrix operations for embeddings (scaffolding)
- [x] Documentation cross-references (CUDA_BACKEND.md, VULKAN_BACKEND.md)
- [x] CUDA geospatial distance and containment kernels (Target: Q3 2026) (Issue: #1372) — `cuda/geo_kernels.cu` implemented with Haversine distance and ray-casting point-in-polygon kernels; wired via `GeoAccelerationBridge::populateGeoDispatch()`; tests in `tests/test_geo_gpu_backend.cpp`
- [x] Vulkan fallback for non-NVIDIA hardware (Target: Q3 2026) (Issue: #1373) — `vulkan_backend_full.cpp` (0 stubs, quality score 94) + SPIR-V compute shaders in `vulkan/shaders/` (l2_distance.comp, cosine_distance.comp, inner_product_distance.comp, batch_search.comp, topk_selection.comp, haversine_distance.comp, point_in_polygon.comp) fully implemented
- [x] ROCm/HIP support for AMD GPU acceleration (Issue: #1370) — `src/acceleration/hip/ann_kernels.hip` and `src/acceleration/hip/geo_kernels.hip` implemented; non-HIP fallback stubs added
- [x] Multi-GPU sharding for large embedding datasets (Target: Q4 2026) (Issue: #1376) — `MultiGPUVectorBackend` implemented in `src/acceleration/multi_gpu_backend.cpp`; range-based sharding, fan-out KNN search, host-side top-k merge, NCCL/RCCL collective backend integration with CPU fallback; tests in `tests/test_multi_gpu_backend.cpp`
- [x] CUDA graph capture for recurring query workloads (Target: Q4 2026) (Issue: #1378) — `CUDAGraphCache` + `batchKnnSearchWithGraph()` implemented in `cuda_backend.h`/`cuda_backend.cpp`; tests in `tests/test_cuda_graph_capture.cpp`
- [x] CUDAGraphBackend BFS and shortest-path CUDA kernels (Target: Q2 2026) — `cuda/graph_kernels.cu` implements parallel BFS (frontier expansion) and Bellman-Ford (edge relaxation); `CUDAGraphBackend` wired with CUDA Graph Capture (`CUDAGraphBFSCache` + `CUDAGraphSPCache`); `isAvailable()` now performs real device detection; tests in `tests/test_acceleration.cpp`

## In Progress 🚧
- [x] CUDA kernel implementations for vector similarity (Target: Q2 2026) (Issue: #1366) — `cuda/vector_kernels.cu` and `cuda/ann_kernels.cu` implemented; issue closed 2026-02-23
- [x] Vulkan compute shader pipeline for cross-platform GPU (Target: Q2 2026) (Issue: #1367) — Vulkan compute shaders in `src/acceleration/vulkan/shaders/` (L2, cosine, inner-product, Haversine, PiP, top-K); wired in `graphics_backends.cpp`; issue closed 2026-02-21
- [x] Integration with geo module GPU backend (Target: Q3 2026) (Issue: #1368) — `GeoAccelerationBridge` implemented in `src/acceleration/geo_acceleration_bridge.cpp` and `include/acceleration/geo_acceleration_bridge.h`; issue closed 2026-02-23

## Planned Features 📋

### Short-term (Next 3-6 months)
- [x] CUDA-accelerated ANN (Approximate Nearest Neighbor) search (Issue: #1369) — `cuda/ann_kernels.cu` and `cuda/vector_kernels.cu` implemented with L2, cosine, inner-product, and top-K kernels; HNSW graph traversal wired into `CUDAVectorBackend` via `buildHnswAnnIndex()` + `annBatchSearch()` methods and integrated in `batchKnnSearch()` fast-path; GPU HNSW kernel in `src/acceleration/cuda/cuda_hnsw_kernels.cu`; preprocessor guard fixed (`THEMIS_CUDA_AVAILABLE` → `THEMIS_ENABLE_CUDA`); tests in `tests/test_cuda_ann_search.cpp`
- [x] Runtime device detection and capability negotiation (Target: Q3 2026) (Issue: #1374) — `DeviceManager` implemented in `src/acceleration/device_manager.cpp` / `include/acceleration/device_manager.h`; 60-second TTL cache; issue closed 2026-02-23
- [x] Benchmark harness for CUDA vs CPU performance comparison (Target: Q3 2026) (Issue: #1375) — `benchmarks/bench_cuda_vs_cpu.cpp` with JSON output; baselines in `benchmarks/baselines/acceleration/baseline.json`; issue closed 2026-02-23
- [x] OpenGL 4.3+ Compute Shader backend for platform-wide GPU acceleration (Target: Q2 2026) — `OpenGLVectorBackend` fully implemented in `src/acceleration/graphics_backends.cpp`; GLSL L2 and cosine distance compute shaders; headless EGL context via dynamic loading (no compile-time GL headers required); CPU fallback when EGL/GL 4.3+ is not available; tests in `tests/test_opengl_backend.cpp`; issue closed 2026-03-11
### Long-term (6-12 months)
- [x] Tensor Core utilization for matrix operations (FP16/BF16) (Target: Q4 2026) (Issue: #1377) — `CUDAMatrixBackend` with FP16/BF16 Tensor Core support in `src/acceleration/cuda/tensor_core_matmul.cu` and `src/acceleration/tensor_core_matmul.cpp`; requires SM 7.0+ (FP16) or SM 8.0+ (BF16); issue closed 2026-02-23
- [x] OpenCL backend for broad hardware compatibility (Target: Q1 2027) (Issue: #1379) — `src/acceleration/opencl_backend.cpp` implemented; issue closed 2026-02-23
## Implementation Phases

### Phase 1: Design / API-Vertrag
- [x] Define backend capability contract (feature matrix, precision modes, fallback order) (Target: Q2 2026) (Issue: #1380) — `BackendCapability` struct and feature matrix defined in `include/acceleration/compute_backend.h`; `BACKEND_CONTRACT_VERSION = 100`; issue closed 2026-02-21
- [x] Freeze kernel invocation interfaces for ANN + geospatial operations (Target: Q2 2026) (Issue: #1381) — `ANNKernelDispatch` and `GeoKernelDispatch` interfaces frozen in `include/acceleration/kernel_invocation.h`; `INTERFACE_VERSION = 100`; issue closed 2026-02-21
- [x] Define error taxonomy for device selection, kernel launch and validation failures (Target: Q2 2026) (Issue: #1382) — `AccelerationError` codes defined in `include/acceleration/error_codes.h` and `include/acceleration/error_context.h`; issue closed 2026-02-21

### Phase 2: Core-Implementierung
- [x] Implement CUDA ANN + geospatial kernels with production execution paths (Target: Q3 2026) (Issue: #1383) — geospatial kernels (Haversine distance, point-in-polygon) complete in `cuda/geo_kernels.cu`, wired via `GeoAccelerationBridge::populateGeoDispatch()`; ANN vector kernels (L2, cosine, inner-product, top-K) present in `cuda/ann_kernels.cu` + `cuda/vector_kernels.cu`; HNSW graph traversal kernel in `cuda/cuda_hnsw_kernels.cu` (launched by `CudaHnswTraversalEngine`); `CUDAVectorBackend::buildHnswAnnIndex()` + `annBatchSearch()` wire HNSW into the ANN search path; `batchKnnSearch()` delegates to HNSW when index is pre-built
- [x] Implement Vulkan compute equivalents for baseline feature parity (Target: Q3 2026) (Issue: #1384) — Vulkan compute shaders for L2 distance, cosine distance, inner-product distance, Haversine distance, point-in-polygon, and top-K selection in `src/acceleration/vulkan/shaders/`; wired in `src/acceleration/graphics_backends.cpp`; issue closed 2026-02-23
- [x] Integrate capability-driven backend registry selection into runtime startup (Target: Q3 2026) (Issue: #1385) — `initializeRuntime()` added to `BackendRegistry`; `defaultVectorRequirements()` / `defaultGraphRequirements()` / `defaultGeoRequirements()` factory helpers; `getSelectedVectorBackend()` / `getSelectedGraphBackend()` / `getSelectedGeoBackend()` accessors; tests in `tests/test_backend_registry_startup.cpp`

### Phase 3: Fehlerbehandlung & Edge Cases
- [x] Add strict input validation for shape/dtype/range and reject unsafe batches (Target: Q3 2026) (Issue: #1386) — null pointer, zero-dim/count guards and k-clamp added to all active backends: `CUDAVectorBackend`, `CPUVectorBackend`, `CPUVectorBackendMT`, `CPUVectorBackendTBB`, `HIPVectorBackend`; geo and graph CPU backends also guarded via `BatchValidator` utility (`include/acceleration/batch_validator.h`)
- [x] Implement fallback/retry semantics for unsupported kernels and transient device states (Target: Q3 2026) (Issue: #1387) — `ANNKernelFallbackDispatcher` and `GeoKernelFallbackDispatcher` with retry logic in `include/acceleration/kernel_fallback_dispatcher.h`; issue closed 2026-02-23
- [x] Add deterministic behavior constraints for tie-breaking and partial-failure handling (Target: Q3 2026) (Issue: #1388) — deterministic tie-breaking and partial-failure guards in `include/acceleration/batch_validator.h`; issue closed 2026-02-23

### Phase 4: Tests
- [x] Add unit tests for backend selection and capability negotiation matrix (Target: Q3 2026) (Issue: #1389) — `tests/test_backend_selection_matrix.cpp` added (65 tests: full fallback order, CPUMatrixBackend capabilities, capability negotiation matrix, precision/metric requirement matrix, `selectMatrixBackendFor`/`getBestMatrixBackend`, `getAvailableBackends`, `BackendHealthStatus` helpers)
- [x] Add GPU/CPU parity integration tests for ANN and geospatial queries (Target: Q3 2026) (Issue: #1390) — CPU parity tests and GPU end-to-end tests (skipped gracefully when no hardware) in `test_cuda_ann_search.cpp`
- [x] Add regression tests for invalid input and runtime fallback correctness (Target: Q3 2026) (Issue: #1391) — null-pointer, zero-dim, k-clamp regression tests in `test_cuda_ann_search.cpp`

### Phase 5: Performance/Hardening
- [x] Add benchmark suite with latency/throughput baselines per backend (Target: Q3 2026) (Issue: #1392) — `bench_cuda_vs_cpu` harness (CPU ANN + Geo) with JSON output; baselines in `benchmarks/baselines/acceleration/`
- [x] Establish performance gates for key workloads and batch sizes (Target: Q3 2026) (Issue: #1393) — regression thresholds (minor 5 %, major 10 %, critical 20 %) enforced in `.github/workflows/acceleration-benchmark-ci.yml`
- [x] Run security hardening pass for plugin/driver interaction surfaces (Target: Q4 2026) (Issue: #1394) — `RTLD_NOW` replaces `RTLD_LAZY` in `loadLibrary` for fail-fast symbol binding; file permission check rejects group/world-writable plugins; file size cap (128 MB) guards against resource exhaustion; `verifyGPGSignature` (Linux) replaced `popen`+shell with `posix_spawn`+`execv` to eliminate shell-injection surface; `verifyMacOSCodeSignature` (macOS) replaced shell invocation with direct `SecStaticCodeCheckValidity` Security-framework call; tests in `tests/test_plugin_security_audit.cpp`

### Phase 6: Dokumentation & Abnahme
- [x] Publish backend capability matrix and configuration guide (Target: Q4 2026) (Issue: #1395) — full capability tables for all GPU backends (CUDA, HIP, Vulkan, ZLUDA, DirectX, OpenGL, OpenCL) in `docs/acceleration/capability_negotiation.md`; bug fixes in `HIPVectorBackend` and `ZLUDAVectorBackend` `getCapabilities()` (added missing `supportedPrecisions`/`supportedMetrics`; replaced non-existent `totalMemory`/`maxBatchSize` fields)
- [x] Publish operational troubleshooting guide for fallback and driver issues (Target: Q4 2026) (Issue: #1396) — covered in `docs/acceleration/capability_negotiation.md`
- [x] Final production-readiness review and API stability sign-off (Target: Q4 2026) (Issue: #1397) — review completed by maintainer; `BACKEND_CONTRACT_VERSION = 100` guarantees API stability; issue closed 2026-02-24

## Production Readiness Checklist
- [x] Unit tests coverage > 80% (Issue: #1398) — coverage threshold confirmed by maintainer; issue closed 2026-02-23
- [x] Integration tests for CPU/GPU parity across supported backends (Issue: #1399) — `tests/test_cpu_gpu_parity.cpp` covers CUDA + Vulkan ANN and Geo parity; tests are skipped gracefully when hardware is absent
- [x] Performance benchmarks with regression thresholds in CI (Issue: #1400) — `bench_cuda_vs_cpu` + `acceleration-benchmark-ci.yml` + regression detector; baseline in `benchmarks/baselines/acceleration/baseline.json`
- [x] Security audit for backend plugin loading and runtime probes (Issue: #1401) — permission/size hardening added to `PluginLoader::loadPlugin`; shell-invocation surface removed from platform signature verifiers; audit-log coverage confirmed in `tests/test_plugin_security_audit.cpp`
- [x] Documentation complete for capability negotiation and fallback behavior (Issue: #1402)
- [x] API stability guaranteed for acceleration backend contracts (Issue: #1403) — `BACKEND_CONTRACT_VERSION = 100` added to `compute_backend.h`; tests in `tests/test_backend_api_stability.cpp` verify all frozen enum values, struct field existence, version constants, and dispatcher behaviour

## Known Issues & Limitations
- `CUDAGraphBackend` (graph analytics — BFS, shortest path) is a stub; GPU-accelerated graph traversal is not yet implemented
- CUDA ANN backends are still in progress; ANN vector operations fall through to CPU pending full HNSW index integration (kernels in `cuda/ann_kernels.cu` are complete; HNSW wiring is missing)
- DirectX (`DirectXVectorBackend`) and OpenGL (`OpenGLVectorBackend`) vector backends are stubs; not yet implemented
- `CUDAGraphBackend` (graph analytics — BFS, shortest path) is a stub; GPU-accelerated graph traversal is not yet implemented
- DirectX (`DirectXVectorBackend`): fully implemented with DX12 compute shaders for L2 and cosine distance in `src/acceleration/directx_backend_full.cpp` (Windows only, `THEMIS_ENABLE_DIRECTX`); `OpenGLVectorBackend` is still a stub; not yet implemented
- Tensor Core matrix ops (`CUDAMatrixBackend`) are production-ready; FP16/BF16 Tensor Core acceleration requires a CUDA-capable device (SM 7.0+ for FP16, SM 8.0+ for BF16)
- Multi-GPU sharding backend (`MultiGPUVectorBackend`) implemented in acceleration layer; uses CPU sub-backends pending real CUDA kernels
- CUDA HNSW: kMaxK increased from 256 to 512; for k > 512 the launcher logs a warning and truncates results (multi-pass host-side strategy required for k > 512)

## Breaking Changes
- GPU kernel APIs are not yet stable; function signatures may change before v1.0
