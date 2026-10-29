---
Author: "ThemisDB Team"
Created: "2026-10-29"
Last Updated: "2026-10-29"
Status: "active"
AI_Used: true
AI_Models: "GitHub Copilot"
AI_Contributor: "GitHub Copilot"
Human_Review: true
Human_Author: "ThemisDB Team"
---

> Build: `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-10-29 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/acceleration/ARCHITECTURE.md -->

# Acceleration Module — Public Header Architecture

**Module path:** `include/acceleration/`
**Implementation:** `../../src/acceleration/`
**Canonical architecture doc:** [`../../src/acceleration/ARCHITECTURE.md`](../../src/acceleration/ARCHITECTURE.md)

## Overview

The public API in `include/acceleration/` defines the stable contract used by vector index, geo, graph, and AI inference callers. The current implementation is centered on `BackendRegistry`, `DeviceManager`, `AiHardwareDispatcher`, and the backend interfaces declared in `compute_backend.h` and `kernel_invocation.h`.

For runtime internals and data-flow details see:
- [`../../src/acceleration/ARCHITECTURE.md`](../../src/acceleration/ARCHITECTURE.md)
- [`../../src/acceleration/README.md`](../../src/acceleration/README.md)

## Header groups

### Compute-backend contract

| Header | Public types | Purpose |
|---|---|---|
| `compute_backend.h` | `BackendType`, `PrecisionMode`, `DeviceCapabilityInfo`, `BackendCapabilities`, `IComputeBackend`, `IVectorBackend`, `IGraphBackend`, `IGeoBackend`, `IMatrixBackend`, `BackendRegistry` | Stable backend contract and runtime registry |
| `cpu_backend.h` | `CPUVectorBackend`, `CPUGraphBackend`, `CPUGeoBackend`, `CPUMatrixBackend` | CPU fallback implementations |
| `cuda_backend.h` | `CUDAVectorBackend`, `CUDAGraphBackend`, `CUDAGeoBackend`, `CUDAMatrixBackend` | CUDA entry points and runtime wrappers |
| `hip_backend.h` | `HIPVectorBackend`, `HIPGeoBackend` | HIP/ROCm integration surface |
| `opencl_backend.h` | `OpenCLVectorBackend` | OpenCL backend surface |
| `vulkan_backend.h` | `VulkanVectorBackend`, `VulkanGeoBackend` | Vulkan compute integration surface |
| `quantized_backend.h` | quantized backend interfaces | INT8/INT4-style execution contracts |

### Dispatch, kernels, async execution

| Header | Public types | Purpose |
|---|---|---|
| `ai_hardware_dispatcher.h` | `AiInferenceRequest`, `AiInferenceResult`, `AiHardwareCapability`, `AiHardwareDispatcher` | AI inference routing across CPU/NPU/GPU backends |
| `kernel_invocation.h` | `ANNKernelDispatch`, `GeoKernelDispatch`, `KernelResult` types | Fixed kernel launch interfaces |
| `kernel_fallback_dispatcher.h` | `ANNKernelFallbackDispatcher`, `GeoKernelFallbackDispatcher` | Retry + fail-closed fallback logic |
| `compute_graph.h` | `ComputeGraph` | DAG-style execution graph |
| `compute_future.h` | `ComputeFuture<T>` | Async result handle |
| `graph_kernel_classification.h` | kernel category enums and traits | Bounded graph routing classification |

### Device and multi-GPU management

| Header | Public types | Purpose |
|---|---|---|
| `device_manager.h` | `DeviceManager`, `DeviceCapabilityInfo` | Device probing, caching, and deterministic override hooks |
| `multi_gpu_backend.h` | `MultiGPUBackend` | multi-device orchestration |
| `nccl_vector_backend.h` | `NCCLVectorBackend` | NCCL collective path |
| `rccl_vector_backend.h` | `RCCLVectorBackend` | RCCL collective path |
| `vllm_resource_manager.h` | `VLLMResourceManager` | GPU lease/resource management for vLLM paths |

### Specialized accelerators and validation

| Header | Public types | Purpose |
|---|---|---|
| `faiss_gpu_backend.h` | FAISS-wrapped GPU backend types | FAISS GPU KNN routing |
| `tensor_core_matmul.h` | `TensorCoreMatmul` | FP16/BF16 matrix multiply helpers |
| `geo_acceleration_bridge.h` | `GeoAccelerationBridge` | Geospatial acceleration bridge |
| `graphics_backends.h` | DirectX/Vulkan/OpenGL wrapper types | graphics API compute integration |
| `vec_knn.h` | `VecKNN` | vector nearest-neighbour search entry point |
| `batch_validator.h` | `BatchValidator` | payload validation and bounded execution checks |
| `plugin_loader.h` | `PluginLoader` | dynamic backend plugin loading |
| `plugin_security.h` | `PluginSecurity` | plugin trust and verification checks |
| `shader_integrity.h` | `ShaderIntegrity` | shader validation before execution |
| `error_codes.h` | `AccelerationErrorCode` and enumerations | structured error taxonomy |
| `error_context.h` | `ErrorContext` | error context propagation |

## Runtime flow

1. `DeviceManager::probeDevices()` / `refresh()` enumerates capabilities and caches the result for up to 60 seconds.
2. `DeviceManager::setEnumerateFn()` allows deterministic injection in focused tests while preserving CPU fallback synthesis for empty snapshots.
3. `BackendRegistry::initializeRuntime()` selects the best backend for vector, graph, and geo requirements based on capability probing.
4. Selected backends are consumed via `getSelectedVectorBackend()`, `getSelectedGraphBackend()`, and `getSelectedGeoBackend()`.
5. Optional runtime failures route through structured error objects and fail-closed fallback paths before unsafe execution proceeds.

## Runtime guarantees

- CPU fallback is always available, and empty or unhealthy capability snapshots yield a synthetic `CPU Fallback` device.
- Public API names must remain in `themis::acceleration`; no implementation-only types should be surfaced in `include/acceleration/`.
- `THEMIS_ENABLE_*` feature gates control availability of GPU-specific surfaces; builds without SDKs remain supported through CPU paths.
- Multi-device and plugin paths are constrained by explicit validation and bounded execution rules.

## Contract notes

- Headers in `include/acceleration/` are the public API surface; implementation details live in `src/acceleration/`.
- Breaking changes must follow the project versioning policy in [`../../VERSIONING.md`](../../VERSIONING.md).
- Current module status is validated against the source code in `src/acceleration/*` and the current tests under `tests/acceleration/`.
- Some advanced GPU and parity gates remain hardware-dependent and are explicitly tracked in the module roadmap rather than described as fully validated on all hosts.
