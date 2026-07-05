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
### 2.5 Kernel Classification and Safety

| Header | Public Type | Purpose |
|--------|------------|---------|
| `graph_kernel_classification.h` | `KernelCategory`, `KernelType`, `KernelClassificationTraits` | Bounded graph kernel classification framework (Issue #5469) |

### 2.6 Security and Validation

| Header | Public Type | Purpose |
|--------|------------|---------|
| `batch_validator.h` | `BatchValidator` | Input batch shape and safety validation |
| `plugin_loader.h` | `PluginLoader` | Dynamic backend plugin loading |
| `plugin_security.h` | `PluginSecurity` | Backend plugin signature and sandbox checks |
| `shader_integrity.h` | `ShaderIntegrity` | Shader/kernel binary integrity verification |
| `error_codes.h` | `AccelerationError` | Typed error codes for backend failures |
| `error_context.h` | `ErrorContext` | Error propagation context carrier |

---

## 5. Bounded Graph Kernels Classification Framework (Issue #5469)

The Acceleration module implements a three-category classification framework for graph operations eligible for GPU acceleration:

### Categories

| Category | Eligibility | Semantics | Timeline |
|----------|-------------|-----------|----------|
| **A: Acceleration-Eligible** | ✅ GPU safe (no constraints) | Advisory candidates | Phase A (Q3 2026) |
| **B: Conditional Acceleration** | ⚠️ GPU with validation gates | Advisory with validation | Phase B (Q3 2026+) |
| **C: CPU-First Only** | ❌ Never GPU | Truth-bearing, exact | Never GPU |

### Category A Examples

- ANN distance kernels (L2, Cosine, Inner Product)
- TopK selection
- Vector KNN insertion
- Tensor Core matrix multiply

**Constraint**: Must have CPU fallback; output validated before use.

### Category B Examples

- Geospatial distance (with coordinate validation gates)
- Geospatial containment (with polygon validation gates)
- Graph BFS (bounded k ≤ 3, frontier ≤ 10K nodes)
- Graph Dijkstra (bounded pairs, edge weight validation)

**Constraint**: Input/output validation gates required; CPU fallback mandatory; exact parity test required.

### Category C Examples (CPU-Only)

- ACL enforcement (security-critical)
- Provenance chains (determinism-critical)
- Policy validation (compliance-critical)
- Exact multi-hop validation (correctness-critical)
- Irregular truth-bearing traversals (schema-dependent)

**Constraint**: No GPU path; CPU execution always required.

### Kernel Safety Guarantees

1. ✅ **Graph Truth remains canonical** — CPU-computed results are source of truth
2. ✅ **GPU outputs remain advisory-only** — GPU kernels produce candidates for downstream validation
3. ✅ **Summary-first never replaces exact-on-demand** — GPU summaries don't bypass CPU checks
4. ✅ **Policy decisions on CPU first** — All security/compliance decisions are CPU-side
5. ✅ **Provenance chains unbroken** — Evidence chains constructed on CPU with exact ordering
6. ✅ **ACL enforcement non-negotiable** — Access control gates pre-process all GPU operations
7. ✅ **Fallback-to-CPU always available** — Every GPU operation has CPU fallback

### Integration Point

```cpp
// Compile-time kernel classification query
using Traits = KernelClassificationTraits<KernelType::ANN_L2_DISTANCE>;
static_assert(Traits.category == KernelCategory::ACCELERATION_ELIGIBLE);
static_assert(Traits.has_cpu_fallback);
static_assert(Traits.is_advisory_only);
```

### References

- [`../../docs/acceleration/BOUNDED_GRAPH_KERNELS.md`](../../docs/acceleration/BOUNDED_GRAPH_KERNELS.md) — Full classification framework
- [`../../docs/acceleration/KERNEL_ACCELERATION_EXAMPLES.md`](../../docs/acceleration/KERNEL_ACCELERATION_EXAMPLES.md) — Code patterns (allowed/disallowed)
- [`graph_kernel_classification.h`](graph_kernel_classification.h) — Classification enums and traits
- [`../../benchmarks/bounded_kernel_validation.cpp`](../../benchmarks/bounded_kernel_validation.cpp) — Validation tests and benchmarks
- Issue #5469 — Kernel classification definition

---

## 4. Namespace Layout

All public types reside in the `themis::acceleration` namespace (or a sub-namespace).

---

## 6. Contract Notes

- Headers in `include/acceleration/` expose the **stable public API**; internal types live in `src/acceleration/`.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **ANN/Tensor**.
