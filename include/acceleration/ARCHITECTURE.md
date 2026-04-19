<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · AUDIT.md · SECURITY.md -->

# Acceleration Module — Public Header Architecture

**Version:** 1.7.0
**Last Updated:** 2026-04-06
**Module Path:** `include/acceleration/`
**Implementation:** `../../src/acceleration/`

---

## 1. Overview

The `include/acceleration/` directory exposes public C++ headers for the GPU/CPU compute
acceleration backends of ThemisDB. These headers define the abstract backend interface, device
management API, kernel invocation contract, and backend-specific (CUDA, HIP, Vulkan, OpenCL,
CPU) extension points used by higher-level modules such as `search/`, `analytics/`, and `rag/`.

---

## 2. Design Principles

- **Backend Abstraction** – `compute_backend.h` defines the single unified `IComputeBackend`
  interface; all concrete backends implement this contract.
- **Device Agnosticism** – `device_manager.h` abstracts device enumeration and selection,
  allowing transparent fallback across CUDA → HIP → Vulkan → OpenCL → CPU.
- **Kernel Portability** – `kernel_invocation.h` and `kernel_fallback_dispatcher.h` decouple
  kernel launch semantics from device specifics.
- **Error Uniformity** – `error_codes.h` and `error_context.h` provide a device-independent
  error taxonomy; all backends map native errors to these codes.
- **Security Boundary** – `plugin_security.h` and `shader_integrity.h` enforce integrity
  checks for dynamically loaded compute plugins and shader binaries.

---

## 3. Interface Inventory

| Header | Classes / Interfaces | Purpose |
|--------|----------------------|---------|
| `compute_backend.h` | `IComputeBackend` | Primary backend abstraction; `submit()`, `synchronize()`, `capabilities()` |
| `compute_future.h` | `ComputeFuture<T>` | Async result handle for submitted compute tasks |
| `device_manager.h` | `DeviceManager`, `DeviceInfo` | Enumerate, select, and query compute devices |
| `kernel_invocation.h` | `KernelInvocation`, `KernelConfig` | Portable kernel launch descriptor |
| `kernel_fallback_dispatcher.h` | `KernelFallbackDispatcher` | Ordered fallback across backends for missing kernels |
| `batch_validator.h` | `BatchValidator` | Validates tensor/batch shapes before kernel submission |
| `cuda_backend.h` | `CUDABackend` | CUDA-specific backend implementation header |
| `hip_backend.h` | `HIPBackend` | AMD HIP backend header |
| `vulkan_backend.h` | `VulkanBackend` | Vulkan compute backend header |
| `opencl_backend.h` | `OpenCLBackend` | OpenCL backend header |
| `cpu_backend.h` | `CPUBackend` | Multithreaded CPU fallback backend |
| `faiss_gpu_backend.h` | `FAISSGPUBackend` | FAISS GPU ANN search backend |
| `graphics_backends.h` | `GraphicsBackendRegistry` | Unified registry for graphics-compute backends |
| `multi_gpu_backend.h` | `MultiGPUBackend` | Multi-GPU data-parallel dispatch |
| `nccl_vector_backend.h` | `NCCLVectorBackend` | NCCL collective operations for vector workloads |
| `rccl_vector_backend.h` | `RCCLVectorBackend` | RCCL (AMD) collective operations |
| `geo_acceleration_bridge.h` | `GeoAccelerationBridge` | Bridge to `include/geo/` for GPU-accelerated geospatial ops |
| `tensor_core_matmul.h` | `TensorCoreMatmul` | Tensor core GEMM for embedding similarity |
| `vllm_resource_manager.h` | `VLLMResourceManager` | GPU resource budget manager for vLLM inference |
| `plugin_loader.h` | `PluginLoader` | Dynamic compute plugin loading with integrity verification |
| `plugin_security.h` | `PluginSecurity` | Plugin signature and allowlist enforcement |
| `shader_integrity.h` | `ShaderIntegrity` | SPIR-V / PTX shader hash verification |
| `error_codes.h` | `AccelErrorCode` enum | Canonical backend-independent error codes |
| `error_context.h` | `AccelErrorContext` | Rich error context with device/kernel/batch info |

> **Implementation details:** `../../src/acceleration/`

---

## 4. Header Organisation

```
include/acceleration/
├── compute_backend.h          ← primary interface
├── compute_future.h
├── device_manager.h
├── kernel_invocation.h
├── kernel_fallback_dispatcher.h
├── batch_validator.h
├── cuda_backend.h
├── hip_backend.h
├── vulkan_backend.h
├── opencl_backend.h
├── cpu_backend.h
├── faiss_gpu_backend.h
├── graphics_backends.h
├── multi_gpu_backend.h
├── nccl_vector_backend.h
├── rccl_vector_backend.h
├── geo_acceleration_bridge.h
├── tensor_core_matmul.h
├── vllm_resource_manager.h
├── plugin_loader.h
├── plugin_security.h
├── shader_integrity.h
├── error_codes.h
├── error_context.h
└── metrics/                   ← per-backend metric descriptors
```
