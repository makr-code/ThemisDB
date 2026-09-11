# Acceleration Module - Architecture Guide

<!-- Status: current | validated: 2026-09-09 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

Version: 1.0
Last Updated: 2026-09-09
Module Path: src/acceleration/

## 1. Overview

The Acceleration module implements backend discovery, capability-driven dispatch, hardware-accelerated kernels, and deterministic fallback paths.

## 2. Architecture Surfaces

| Surface | Source files |
|---|---|
| Registry and contracts | src/acceleration/backend_registry.cpp, src/acceleration/compute_backend.cpp |
| Device and capability management | src/acceleration/device_manager.cpp, src/acceleration/ai_hardware_dispatcher.cpp |
| GPU/CPU backend implementations | src/acceleration/cuda_backend.cpp, src/acceleration/hip_backend.cpp, src/acceleration/vulkan_backend_full.cpp, src/acceleration/cpu_backend.cpp |
| Multi-device coordination | src/acceleration/multi_gpu_backend.cpp, src/acceleration/nccl_vector_backend.cpp, src/acceleration/rccl_vector_backend.cpp |
| Plugin and integrity controls | src/acceleration/plugin_loader.cpp, src/acceleration/plugin_security.cpp, src/acceleration/shader_integrity.cpp |
| Resource control and scheduling | src/acceleration/vllm_resource_manager.cpp, src/acceleration/tensor_core_matmul.cpp |

## 3. Runtime Control Flow

1. Registry and device manager probe available capabilities, using runtime discovery by default or an injected deterministic snapshot in focused validation.
2. Runtime selection chooses matching backend surfaces by requirements.
3. Workloads route through selected backend or deterministic fallback path.
4. Plugin/shader security checks gate dynamic execution surfaces.
5. Resource and multi-device managers coordinate sustained execution.

## 4. Integration Boundaries

| Direction | Integration |
|---|---|
| Used by | index, geo, graph, llm, and related runtime consumers |
| Uses | hardware drivers, optional SDK/plugin surfaces, system capabilities |
| Exposes | backend selection APIs, dispatch contracts, and diagnostics |

## 5. Concurrency Model

- runtime selection is initialized once and then consumed concurrently
- device probing is cache-backed and can be deterministically overridden in focused tests without changing production selection code
- backend operations support concurrent workload execution paths
- multi-device and resource-control paths coordinate shared state explicitly

## 6. Known Limits

- performance and feature coverage vary by hardware and build profile
- some plugin/runtime combinations are deployment dependent
- fallback behavior is essential where accelerator capabilities are unavailable

## 7. Sourcecode Verification (Module: acceleration/architecture)

- Verified files:
  - src/acceleration/backend_registry.cpp
  - src/acceleration/compute_backend.cpp
  - src/acceleration/device_manager.cpp
  - src/acceleration/cuda_backend.cpp
  - src/acceleration/hip_backend.cpp
  - src/acceleration/vulkan_backend_full.cpp
  - src/acceleration/multi_gpu_backend.cpp
  - src/acceleration/plugin_loader.cpp
  - src/acceleration/plugin_security.cpp
  - src/acceleration/vllm_resource_manager.cpp
- Verified interfaces and behavior:
  - capability and backend-selection flow
  - dispatch/fallback and multi-device behavior
  - plugin/security/resource-control integration

---

## Module Dependencies

### Direct Upstream Dependencies (this module uses)

| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| *(none — leaf module)* | Hardware drivers, CUDA/HIP/Vulkan/OpenCL SDKs, system capabilities | Direct hardware abstraction; no ThemisDB module dependencies |

> `acceleration` is intentionally a **leaf module** in the ThemisDB dependency graph. It depends only on hardware SDKs and OS capabilities, never on other ThemisDB modules. This keeps the acceleration layer portable and independently testable.

### Direct Downstream Consumers (modules that use this module)

| Module | Via | Notes |
|--------|-----|-------|
| index | `include/acceleration/compute_backend.h`, `include/acceleration/device_manager.h` | GPU-accelerated vector index paths, HNSW GPU traversal |
| llm | `include/acceleration/compute_backend.h`, `include/acceleration/device_manager.h` | GPU kernel execution, VRAM allocation routing |
| geo | `include/acceleration/geo_acceleration_bridge.h` | Geospatial computation acceleration |
| graph | `include/acceleration/graph_kernel_classification.h` | GNN and graph kernel GPU dispatch |

---

## Integration Points

### Critical Integration: ComputeBackend — Hardware Dispatch Contract
**Files:** `include/acceleration/compute_backend.h` ↔ `src/acceleration/backend_registry.cpp`, `src/acceleration/compute_backend.cpp`
**Contract:** `ComputeBackend` defines the stable dispatch interface for all hardware execution paths. `BackendRegistry` selects the best available backend (CUDA → HIP → Vulkan → OpenCL → CPU) at runtime based on `DeviceManager` capability probing. Selection is initialized once; subsequent dispatches are lock-free.
**Thread Safety:** Backend selection is initialized once at startup; concurrent dispatch calls are safe after initialization.
**Failure Mode:** Preferred backend unavailable → deterministic fallback to next-best backend; final fallback is always CPU. Never silently drops work.

### Critical Integration: DeviceManager — Capability Probing and Override
**Files:** `include/acceleration/device_manager.h` ↔ `src/acceleration/device_manager.cpp`, `src/acceleration/ai_hardware_dispatcher.cpp`
**Contract:** `DeviceManager` probes hardware capabilities at startup and caches results. In focused tests, capability snapshot can be deterministically injected without modifying production selection code. Downstream modules call `DeviceManager::getBestBackend()` or use `AiHardwareDispatcher` for AI-specific routing.
**Thread Safety:** Capability cache is read-only after initialization; thread-safe for concurrent queries.
**Failure Mode:** Hardware probe failure → fallback capability set assumed; warning logged; CPU-only mode activated.

### Critical Integration: CUDA RAII Guards — GPU Resource Safety
**Files:** `include/gpu/cuda_raii.h` ↔ `src/acceleration/cuda_backend.cpp`, `src/index/gpu_vector_index.cpp`
**Contract:** CUDA RAII guard wrappers (`cuda_raii.h`) ensure deterministic GPU resource cleanup on scope exit. Delivered in current wave for index and LLM GPU paths. Phase C CUDA reduction gate remains OPEN.
**Thread Safety:** Each RAII guard owns exactly one GPU resource; no shared state across guards.
**Failure Mode:** CUDA API failure at guard construction → exception thrown, resource never acquired; no leak.

### Critical Integration: Multi-GPU / NCCL-RCCL Collective Backend
**Files:** `include/acceleration/` ↔ `src/acceleration/multi_gpu_backend.cpp`, `src/acceleration/nccl_vector_backend.cpp`, `src/acceleration/rccl_vector_backend.cpp`
**Contract:** Multi-GPU coordination for distributed vector operations and collective reduction. Requires NCCL (NVIDIA) or RCCL (AMD) SDK present at build time. Advanced collective ops for LLM distributed inference are in Wave C.
**Thread Safety:** Multi-device coordination paths explicitly manage shared state; see `multi_gpu_backend.cpp`.
**Failure Mode:** NCCL/RCCL unavailable → operations fall back to single-GPU or CPU path; collective ops disabled.

### Critical Integration: Plugin Loader + Shader Integrity
**Files:** `include/acceleration/` ↔ `src/acceleration/plugin_loader.cpp`, `src/acceleration/plugin_security.cpp`, `src/acceleration/shader_integrity.cpp`
**Contract:** Dynamic acceleration plugins are loaded with security validation (hash and signature checks) before execution. Shader/kernel code integrity verified before JIT compilation.
**Thread Safety:** Plugin load/unload is single-threaded at startup; shader integrity checks are stateless.
**Failure Mode:** Plugin integrity check failure → plugin rejected; no fallback to unverified code; error logged.

