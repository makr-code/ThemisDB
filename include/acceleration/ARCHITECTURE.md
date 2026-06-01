> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/acceleration/ARCHITECTURE.md -->

# Acceleration Module — Public Header Architecture

**Module Path:** `include/acceleration/`  
**Implementation:** `../../src/acceleration/`  
**Canonical architecture doc:** [`../../src/acceleration/ARCHITECTURE.md`](../../src/acceleration/ARCHITECTURE.md)

---

## 1. Overview

`include/acceleration/` defines the **public compute-backend dispatch, GPU/CPU/Vulkan/OpenCL acceleration, FAISS GPU KNN, tensor-core matmul, multi-GPU coordination, and kernel fallback API contract** for ThemisDB.

For runtime composition and implementation internals see:
→ [`../../src/acceleration/ARCHITECTURE.md`](../../src/acceleration/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Compute Backend Interfaces

| Header | Public Type | Purpose |
|--------|------------|---------|
| `compute_backend.h` | `IComputeBackend` | Pluggable compute-backend contract |
| `cpu_backend.h` | `CPUBackend` | CPU SIMD execution backend |
| `cuda_backend.h` | `CUDABackend` | CUDA execution and memory backend |
| `hip_backend.h` | `HIPBackend` | HIP/ROCm execution backend |
| `opencl_backend.h` | `OpenCLBackend` | OpenCL cross-vendor backend |
| `vulkan_backend.h` | `VulkanBackend` | Vulkan compute backend |
| `quantized_backend.h` | `QuantizedBackend` | INT8/FP16 quantised execution backend |
### 2.2 Dispatch and Fallback

| Header | Public Type | Purpose |
|--------|------------|---------|
| `ai_hardware_dispatcher.h` | `AIHardwareDispatcher` | Hardware-capability-based kernel dispatcher |
| `kernel_fallback_dispatcher.h` | `KernelFallbackDispatcher` | Graceful fallback across GPU/CPU tiers |
| `kernel_invocation.h` | `KernelInvocation` | Unified kernel launch wrapper |
| `compute_graph.h` | `ComputeGraph` | DAG-based heterogeneous compute graph |
| `compute_future.h` | `ComputeFuture` | Async compute result future |
### 2.3 Device and Multi-GPU Management

| Header | Public Type | Purpose |
|--------|------------|---------|
| `device_manager.h` | `DeviceManager` | GPU/CPU device lifecycle and selection |
| `multi_gpu_backend.h` | `MultiGPUBackend` | Multi-device execution coordination |
| `nccl_vector_backend.h` | `NCCLVectorBackend` | NCCL collective communication backend |
| `rccl_vector_backend.h` | `RCCLVectorBackend` | RCCL collective communication backend (AMD) |
| `vllm_resource_manager.h` | `VLLMResourceManager` | vLLM-compatible resource manager |
### 2.4 Specialised Accelerators

| Header | Public Type | Purpose |
|--------|------------|---------|
| `faiss_gpu_backend.h` | `FAISSGPUBackend` | FAISS GPU vector-search backend |
| `tensor_core_matmul.h` | `TensorCoreMatmul` | Tensor-core batched GEMM |
| `geo_acceleration_bridge.h` | `GeoAccelerationBridge` | GPU-accelerated geospatial operations |
| `graphics_backends.h` | `GraphicsBackend` | Graphics-API compute integration |
| `vec_knn.h` | `VecKNN` | Batched KNN search entry point |
### 2.5 Security and Validation

| Header | Public Type | Purpose |
|--------|------------|---------|
| `batch_validator.h` | `BatchValidator` | Input batch shape and safety validation |
| `plugin_loader.h` | `PluginLoader` | Dynamic backend plugin loading |
| `plugin_security.h` | `PluginSecurity` | Backend plugin signature and sandbox checks |
| `shader_integrity.h` | `ShaderIntegrity` | Shader/kernel binary integrity verification |
| `error_codes.h` | `AccelerationError` | Typed error codes for backend failures |
| `error_context.h` | `ErrorContext` | Error propagation context carrier |

---

## 3. Namespace Layout

All public types reside in the `themis::acceleration` namespace (or a sub-namespace).

---

## 4. Contract Notes

- Headers in `include/acceleration/` expose the **stable public API**; internal types live in `src/acceleration/`.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **ANN/Tensor**.
