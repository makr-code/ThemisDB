<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · AUDIT.md · SECURITY.md -->

# Roadmap — Acceleration Module Public Headers

**Module Path:** `include/acceleration/`  
**Implementation Roadmap:** `../../src/acceleration/ROADMAP.md`

---

## Current Status

Public headers at v1.7.0. Five backends (CUDA, HIP, Vulkan, OpenCL, CPU) have stable
abstract interfaces. Security headers (`plugin_security.h`, `shader_integrity.h`) are
present. Multi-GPU and collective operation headers are available.

---

## Completed Features

- [x] `IComputeBackend` abstract interface with capabilities query
- [x] `DeviceManager` for device enumeration and fallback selection
- [x] Portable `KernelInvocation` descriptor
- [x] `KernelFallbackDispatcher` priority-ordered fallback
- [x] CUDA, HIP, Vulkan, OpenCL, CPU backend headers
- [x] `FAISSGPUBackend` for ANN search
- [x] `MultiGPUBackend` and NCCL/RCCL collective headers
- [x] `TensorCoreMatmul` for embedding GEMM
- [x] `PluginSecurity` and `ShaderIntegrity` for dynamic code safety
- [x] `VLLMResourceManager` for LLM GPU budget management
- [x] `GeoAccelerationBridge` connecting geo module to GPU

---

## Planned Features

- [~] `IComputeGraph` header for DAG-based kernel scheduling (Target: Q3 2026)
- [~] `QuantizedBackend` header for INT4/INT8 quantised inference (Target: Q3 2026)
- [ ] `AccelProfiler` interface for per-kernel profiling hooks (Target: Q4 2026)
- [ ] Unified `BackendHealthProbe` interface for liveness/readiness checks (Target: Q4 2026)

---

## Implementation Phases

### Phase 1: Interface Design
- [x] Define `IComputeBackend` contract and `BackendCapabilities`
- [x] Define `DeviceManager` enumeration API
- [x] Error code taxonomy established

### Phase 2: Core Backend Headers
- [x] CUDA, HIP, Vulkan, OpenCL, CPU headers published
- [x] `KernelInvocation` and `KernelFallbackDispatcher`

### Phase 3: Advanced Compute Headers
- [x] FAISS GPU, multi-GPU, NCCL/RCCL collective headers
- [x] TensorCore, vLLM resource, geo bridge headers

### Phase 4: Security Headers
- [x] `plugin_security.h` and `shader_integrity.h`

### Phase 5: Observability & Profiling Headers
- [ ] `AccelProfiler` interface (Target: Q4 2026)
- [ ] `BackendHealthProbe` (Target: Q4 2026)

### Phase 6: Documentation & Acceptance
- [x] Architecture and audit documentation
- [ ] Doxygen annotations complete on all public headers

---

## Production Readiness Checklist

- [x] All backend headers guarded by feature flags
- [x] Error codes unified across backends
- [x] Security headers present for plugin and shader paths
- [ ] Doxygen fully annotated on all exported types
- [~] `IComputeGraph` and `QuantizedBackend` headers published
