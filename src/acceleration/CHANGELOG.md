> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Acceleration Module

All notable changes to the Acceleration module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

### Documentation
- `src/acceleration/README.md`: Added **Public API Entry Points** table, **Configuration Surface** section (build flags + runtime knobs: `CapabilityRequirements`, `RetryPolicy`, `VLLMResourceManager::Config`), **Error Cases & Limits** section, usage snippet, and troubleshooting quick links; fixed build preset (`linux-ninja-release` → `linux-release`).
- `include/acceleration/README.md`: Added **Primary Entry Points** table, **Runtime Configuration Options**, **Runtime Behavior / Error Cases / Limits**, **Usage Snippets**, and **Troubleshooting** cross-links; fixed build preset.
- `docs/acceleration/README.md`: Replaced auto-generated stub with a structured docs index: primary module docs, deep-dive pages, Installation and Usage guidance.
- `docs/de/acceleration/README.md`: Updated OpenCL status (`🔜 Geplant` → `✅ Implementiert`); replaced hardcoded file count with reference to `MODULE_FUNCTION_USAGE_MAP.md`; added Installation section; updated validated timestamp.
- `src/acceleration/PERFORMANCE_EXPECTATIONS.md`: Replaced incorrect LLM benchmark references (L-1…L-8) with acceleration-specific targets (ACC-1…ACC-10) sourced from `FUTURE_ENHANCEMENTS.md` and `ARCHITECTURE.md`.
- `src/acceleration/FUTURE_ENHANCEMENTS.md`: Design Constraints updated from `[ ]` to `[x]` (all constraints are enforced in production code); Required Interfaces self-reference corrected.
- Added cross-reference doc-comment blocks to `backend_registry.cpp`, `compute_backend.cpp`, `device_manager.cpp`, `plugin_loader.cpp`, `shader_integrity.cpp`, `graphics_backends.cpp`, `ai_hardware_dispatcher.cpp` to make backend/shader/hardware-dispatch relationships explicit.

## [1.7.0] — 2026-03-12
### Added
- `OpenGLVectorBackend` with GLSL L2 and cosine distance compute shaders; headless EGL context via dynamic loading (no compile-time GL headers required); CPU fallback when EGL/GL 4.3+ is unavailable
- Tensor Core utilization (`CUDAMatrixBackend`) for FP16/BF16 matrix operations; requires SM 7.0+ (FP16) or SM 8.0+ (BF16)
- OpenCL backend for broad hardware compatibility (`opencl_backend.cpp`)
- Capability matrix documentation for all GPU backends in `docs/acceleration/capability_negotiation.md`

### Changed
- `BACKEND_CONTRACT_VERSION = 100` guarantees API stability across backend contracts
- `verifyGPGSignature` (Linux) and `verifyMacOSCodeSignature` (macOS) replaced shell invocations with direct `posix_spawn`/`execv` and `SecStaticCodeCheckValidity` API calls

### Fixed
- `HIPVectorBackend` and `ZLUDAVectorBackend` `getCapabilities()` missing `supportedPrecisions` and `supportedMetrics` fields
- `RTLD_LAZY` replaced with `RTLD_NOW` in `loadLibrary` for fail-fast symbol binding

## [1.6.0] — 2026-02-24
### Added
- Multi-GPU sharding via `MultiGPUVectorBackend`: range-based sharding, fan-out KNN search, host-side top-K merge, NCCL/RCCL collective backend integration (`src/acceleration/multi_gpu_backend.cpp`)
- CUDA graph capture for recurring query workloads: `CUDAGraphCache` + `batchKnnSearchWithGraph()` (`cuda_backend.h/cpp`)
- CUDAGraphBackend BFS and Bellman-Ford (shortest-path) CUDA kernels with frontier expansion (`cuda/graph_kernels.cu`)
- `BackendRegistry::initializeRuntime()` with `defaultVectorRequirements()` / `defaultGraphRequirements()` / `defaultGeoRequirements()` factory helpers
- `ANNKernelFallbackDispatcher` and `GeoKernelFallbackDispatcher` with configurable retry semantics
- Deterministic tie-breaking and partial-failure guards via `BatchValidator` (`include/acceleration/batch_validator.h`)
- Benchmark harness `bench_cuda_vs_cpu.cpp` with JSON output and baseline in `benchmarks/baselines/acceleration/baseline.json`
- CI performance gates: minor 5%, major 10%, critical 20% regression thresholds via `acceleration-benchmark-ci.yml`

## [1.5.0] — 2026-02-23
### Added
- CUDA geospatial kernels: Haversine distance and ray-casting point-in-polygon in `cuda/geo_kernels.cu`; wired via `GeoAccelerationBridge::populateGeoDispatch()`
- Vulkan fallback for non-NVIDIA hardware: `vulkan_backend_full.cpp` plus SPIR-V compute shaders (L2, cosine, inner-product, Haversine, point-in-polygon, batch search, top-K selection)
- ROCm/HIP support: `hip/ann_kernels.hip` and `hip/geo_kernels.hip` with non-HIP fallback stubs
- `DeviceManager` with 60-second TTL cache for device capability detection
- `BackendCapability` struct and frozen `ANNKernelDispatch` / `GeoKernelDispatch` interfaces with `INTERFACE_VERSION = 100`
- `AccelerationError` typed error codes in `include/acceleration/error_codes.h`
- Null-pointer, zero-dim/count guards, and k-clamp in all active backends via `BatchValidator`

## [1.4.0] — 2026-02-21
### Added
- CUDA ANN kernels: L2, cosine, inner-product, top-K in `cuda/ann_kernels.cu` + `cuda/vector_kernels.cu`
- GPU HNSW graph traversal: `CUDAVectorBackend::buildHnswAnnIndex()` + `annBatchSearch()` + `cuda/cuda_hnsw_kernels.cu`
- Vulkan compute shaders for ANN distance operations wired in `graphics_backends.cpp`
- `GeoAccelerationBridge` (`src/acceleration/geo_acceleration_bridge.cpp`) wiring geo module to GPU kernels

## [1.0.0] — 2024-01-01
### Added
- Initial directory structure for CUDA and Vulkan backends
- Vector similarity search acceleration stubs
- Geospatial query acceleration stubs
- Parallel graph algorithm stubs
- CPU multi-threaded backends (`cpu_backend_mt.cpp`, `cpu_backend_tbb.cpp`)
- Backend registry infrastructure (`backend_registry.cpp`)
