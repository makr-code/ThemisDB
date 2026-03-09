# Acceleration Module Roadmap
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
Pre-production hardening — backend surface is broad, but production-grade kernel coverage, runtime negotiation and cross-backend validation are still in progress.

## In Progress 🚧
- [~] CUDA kernel implementations for vector similarity (Issue: #1366) — `cuda/vector_kernels.cu` implemented; `cuda_backend.cpp` still has 6 stubs pending production wiring
- [~] CUDA-accelerated ANN (Approximate Nearest Neighbor) search (Issue: #1369) — `cuda/ann_kernels.cu` implemented; full production dispatch wiring pending

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
- [x] Runtime device detection and capability negotiation (Issue: #1374) — `device_manager.cpp` / `device_manager.h` with `DeviceCapabilityInfo`; `BackendRegistry::initializeRuntime()` / `deviceInfo()` / `refresh()`; tests in `tests/test_backend_registry_startup.cpp`
- [x] Benchmark harness for CUDA vs CPU performance comparison (Issue: #1375) — `bench_cuda_vs_cpu` harness + JSON output; baselines in `benchmarks/baselines/acceleration/`; CI gates in `acceleration-benchmark-ci.yml`
- [x] Integration with geo module GPU backend (Issue: #1368) — `geo_acceleration_bridge.cpp` wired via `GeoAccelerationBridge::populateGeoDispatch()`; CUDA and HIP geo kernels both implemented
- [x] Tensor Core utilization for matrix operations (FP16/BF16) (Issue: #1377) — `tensor_core_matmul.cpp` + `cuda/tensor_core_matmul.cu` implemented; SM 7.0+ for FP16, SM 8.0+ for BF16

## Planned Features 📋

### Short-term (Next 3-6 months)
- [ ] Complete CUDA vector similarity kernel production wiring (replace 6 remaining stubs in `cuda_backend.cpp`) (Issue: #1366)
- [ ] Complete CUDA ANN production dispatch wiring (`cuda/ann_kernels.cu` → `cuda_backend.cpp` integration) (Issue: #1369)
- [ ] Complete OpenCL backend (1 remaining stub in `opencl_backend.cpp`) (Issue: #1379)
### Long-term (6-12 months)
- [ ] Cross-backend deterministic tie-breaking and partial-failure handling (Issue: #1388)
- [I] Final production-readiness review and API stability sign-off (Issue: #1397)
## Implementation Phases

### Phase 1: Design / API-Vertrag
- [x] Define backend capability contract (feature matrix, precision modes, fallback order) (Issue: #1380) — `BACKEND_CONTRACT_VERSION = 100` in `compute_backend.h`; `KERNEL_INVOCATION_INTERFACE_VERSION = 100` in `kernel_invocation.h`
- [x] Freeze kernel invocation interfaces for ANN + geospatial operations (Issue: #1381) — dispatch tables in `kernel_invocation.h` (`ANNKernelDispatch`, `GeoKernelDispatch`) marked stable at `INTERFACE_VERSION = 100`
- [x] Define error taxonomy for device selection, kernel launch and validation failures (Issue: #1382) — `AccelerationErrorCode` enum + `errorCodeToString()` in `include/acceleration/error_codes.h`; `ErrorContext` in `error_context.h`

### Phase 2: Core-Implementierung
- [~] Implement CUDA ANN + geospatial kernels with production execution paths (Issue: #1383) — Geospatial kernels complete (`cuda/geo_kernels.cu` wired via `GeoAccelerationBridge::populateGeoDispatch()`); ANN vector kernels (`cuda/ann_kernels.cu`, `cuda/vector_kernels.cu`) implemented but 6 stubs remain in `cuda_backend.cpp` pending full production dispatch wiring
- [x] Implement Vulkan compute equivalents for baseline feature parity (Issue: #1384) — `vulkan_backend_full.cpp` (0 stubs, quality score 94) + complete SPIR-V shader set in `vulkan/shaders/` (l2_distance.comp, cosine_distance.comp, inner_product_distance.comp, batch_search.comp, topk_selection.comp, haversine_distance.comp, point_in_polygon.comp)
- [x] Integrate capability-driven backend registry selection into runtime startup (Issue: #1385) — `initializeRuntime()` added to `BackendRegistry`; `defaultVectorRequirements()` / `defaultGraphRequirements()` / `defaultGeoRequirements()` factory helpers; `getSelectedVectorBackend()` / `getSelectedGraphBackend()` / `getSelectedGeoBackend()` accessors; tests in `tests/test_backend_registry_startup.cpp`

### Phase 3: Fehlerbehandlung & Edge Cases
- [x] Add strict input validation for shape/dtype/range and reject unsafe batches (Issue: #1386) — null pointer, zero-dim/count guards and k-clamp added to all active backends: `CUDAVectorBackend`, `CPUVectorBackend`, `CPUVectorBackendMT`, `CPUVectorBackendTBB`, `HIPVectorBackend`; geo and graph CPU backends also guarded via `BatchValidator` utility (`include/acceleration/batch_validator.h`)
- [x] Implement fallback/retry semantics for unsupported kernels and transient device states (Issue: #1387) — `ANNKernelFallbackDispatcher` and `GeoKernelFallbackDispatcher` in `include/acceleration/kernel_fallback_dispatcher.h` implement exponential back-off retry for `DeviceLost`/`OperationTimeout`/`SynchronizationFailed` with CPU fallback after exhaustion
- [I] Add deterministic behavior constraints for tie-breaking and partial-failure handling (Issue: #1388)

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
- [I] Final production-readiness review and API stability sign-off (Target: Q4 2026) (Issue: #1397)

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1398)
- [x] Integration tests for CPU/GPU parity across supported backends (Issue: #1399) — `tests/test_cpu_gpu_parity.cpp` covers CUDA + Vulkan ANN and Geo parity; tests are skipped gracefully when hardware is absent
- [x] Performance benchmarks with regression thresholds in CI (Issue: #1400) — `bench_cuda_vs_cpu` + `acceleration-benchmark-ci.yml` + regression detector; baseline in `benchmarks/baselines/acceleration/baseline.json`
- [x] Security audit for backend plugin loading and runtime probes (Issue: #1401) — permission/size hardening added to `PluginLoader::loadPlugin`; shell-invocation surface removed from platform signature verifiers; audit-log coverage confirmed in `tests/test_plugin_security_audit.cpp`
- [x] Documentation complete for capability negotiation and fallback behavior (Issue: #1402)
- [x] API stability guaranteed for acceleration backend contracts (Issue: #1403) — `BACKEND_CONTRACT_VERSION = 100` added to `compute_backend.h`; tests in `tests/test_backend_api_stability.cpp` verify all frozen enum values, struct field existence, version constants, and dispatcher behaviour

## Known Issues & Limitations
- `cuda_backend.cpp` has 6 remaining stubs (`launchL2DistanceKernel` etc.); CUDA ANN/vector ops fall through to CPU pending full production wiring to `cuda/vector_kernels.cu` and `cuda/ann_kernels.cu`
- Tensor Core matrix ops (`CUDAMatrixBackend`) are implemented; FP16/BF16 Tensor Core acceleration requires a CUDA-capable device (SM 7.0+ for FP16, SM 8.0+ for BF16)
- `MultiGPUVectorBackend` uses CPU sub-backends internally pending NCCL group-call wiring with real CUDA kernels
- `opencl_backend.cpp` has 1 remaining stub; not yet production-ready

## Breaking Changes
- GPU kernel APIs are not yet stable; function signatures may change before v1.0
