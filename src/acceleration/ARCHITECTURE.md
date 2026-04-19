> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# Acceleration Module — Architecture Guide
<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · docs/de/acceleration/README.md -->

**Version:** 1.0
**Last Updated:** 2026-04-06
**Status:** current
**Module Path:** `src/acceleration/`

---

## 1. Overview

The Acceleration module provides hardware-accelerated compute backends for ThemisDB. It decouples
the compute-heavy primitives (ANN vector search, graph analytics, geospatial operators) from
the higher-level subsystems that consume them by exposing a stable `ComputeBackend` interface
that works with CUDA, HIP, Vulkan, OpenCL, Metal, and plain CPU.

The module's central design contract is **graceful degradation**: if no GPU accelerator is
available, the system transparently falls back to CPU backends without any external intervention.

---

## 2. Design Principles

- **Hardware-Agnostic Interface** – consumers call `ComputeBackend`; they do not know or care
  whether CUDA, Vulkan, or CPU is doing the work.
- **Capability-Driven Selection** – backends are ranked and chosen at runtime based on
  measurable device capabilities (VRAM, compute capability, driver version).
- **Graceful Degradation** – every GPU path has a CPU fallback; kernel-level transient
  errors trigger exponential-back-off retry before falling back.
- **Optional Build** – all GPU code is guarded by feature flags (`THEMIS_ENABLE_CUDA`,
  `THEMIS_ENABLE_VULKAN`). The build and runtime must succeed without any GPU SDK.
- **Plugin Security** – dynamically loaded GPU backend plugins are signature-verified
  before use.

---

## 3. Component Architecture

### 3.1 Key Components

| File / Component | Role |
|---|---|
| `backend_registry.cpp` | Singleton registry: discovery, capability scoring, runtime selection |
| `compute_backend.cpp` | Abstract `ComputeBackend` base and helper utilities |
| `cpu_backend.cpp` / `cpu_backend_mt.cpp` / `cpu_backend_tbb.cpp` | Reference CPU implementations (single-thread, pthreads, TBB) |
| `cuda_backend.cpp` + `cuda/` | CUDA kernels and stream management (guarded by `THEMIS_ENABLE_CUDA`) |
| `hip_backend.cpp` + `hip/` | AMD HIP backend (guarded by `THEMIS_ENABLE_HIP`) |
| `vulkan_backend_full.cpp` + `vulkan/` | Vulkan compute pipeline (guarded by `THEMIS_ENABLE_VULKAN`) |
| `opencl_backend.cpp` | OpenCL backend |
| `metal_backend.mm` | Apple Metal backend (macOS/iOS) |
| `directx_backend_full.cpp` | DirectX Compute backend (Windows) |
| `faiss_gpu_backend.cpp` | FAISS GPU wrapper for ANN |
| `nccl_vector_backend.cpp` / `rccl_vector_backend.cpp` | Multi-GPU collective operations |
| `geo_acceleration_bridge.cpp` | Bridges geospatial operators to the acceleration layer |
| `multi_gpu_backend.cpp` | Multi-GPU load balancing and work distribution |
| `tensor_core_matmul.cpp` | Tensor Core matrix multiplication (CUDA/HIP) |
| `device_manager.cpp` | Device enumeration and lifecycle |
| `plugin_loader.cpp` | Dynamic loading of external backend plugins |
| `plugin_security.cpp` | Signature verification for loaded plugins |
| `shader_integrity.cpp` | SPIR-V shader integrity verification before pipeline creation |
| `graphics_backends.cpp` | Shared graphics/GPU utility helpers |
| `zluda_backend.cpp` | ZLUDA backend (AMD GPUs via CUDA API compatibility layer) |
| `oneapi_backend.cpp` | Intel oneAPI/SYCL backend |
| `vllm_resource_manager.cpp` | vLLM GPU resource management integration |
| `kernel_fallback_dispatcher.h` *(include/)* | `ANNKernelFallbackDispatcher` and `GeoKernelFallbackDispatcher` |

### 3.2 Component Diagram

```
┌──────────────────────────────────────────────────────────────────┐
│                    Callers (index, geo, graph)                   │
└───────────────────────────┬──────────────────────────────────────┘
                            │ ComputeBackend interface
┌───────────────────────────▼──────────────────────────────────────┐
│                    BackendRegistry (singleton)                   │
│  initializeRuntime() → autoDetect() → scoreCapabilities()        │
│  getSelectedVectorBackend() / getSelectedGraphBackend()           │
│  getSelectedGeoBackend()                                         │
└───┬──────────────┬───────────────────┬──────────────────┬────────┘
    │              │                   │                  │
┌───▼────┐  ┌──────▼──────┐  ┌────────▼────────┐  ┌─────▼──────┐
│  CUDA  │  │  HIP/ROCm   │  │ Vulkan/DX/Metal  │  │    CPU     │
│Backend │  │  Backend    │  │    Backends      │  │  Backends  │
└───┬────┘  └──────┬──────┘  └────────┬────────┘  └─────┬──────┘
    │              │                   │                  │
    └──────────────┴───────────────────┴──────────────────┘
                            │
              ┌─────────────▼──────────────┐
              │  ANNKernelFallbackDispatcher│
              │  GeoKernelFallbackDispatcher│
              │  (retry + CPU fallback)     │
              └────────────────────────────┘
```

---

## 4. Data Flow

### 4.1 Startup (Backend Selection)

```
Server start
    │
    ▼
BackendRegistry::initializeRuntime()
    │
    ├─ autoDetect() → probe all compiled/plugin backends
    ├─ scoreCapabilities(requirements) for each category
    │      (vector, graph, geo)
    ├─ select highest-scoring backend per category
    └─ cache selections (getSelectedVectorBackend(), ...)
```

### 4.2 Request Path (Vector Similarity)

```
VectorIndex::search(query, k)
    │
    ▼
ANNKernelFallbackDispatcher::dispatch(kernel, args)
    │
    ├─ primary slot is GPU kernel?
    │       ├─ yes → run GPU kernel
    │       │         ├─ success → return result
    │       │         └─ transient error (DeviceLost, Timeout)
    │       │                 → exponential back-off retry (RetryPolicy)
    │       │                 → after maxAttempts → CPU fallback
    │       └─ no (null slot) → CPU fallback immediately
    │
    └─ CPU fallback: cpu_backend.cpp
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Consumed by** | `src/index/` | `ComputeBackend` for ANN search |
| **Consumed by** | `src/geo/` | `GeoKernelFallbackDispatcher` for spatial ops |
| **Consumed by** | `src/graph/` | graph analytics acceleration |
| **Consumed by** | `src/llm/` | `VllmResourceManager` for GPU VRAM |
| **Provides to** | All GPU consumers | `BackendRegistry::getSelected*Backend()` |
| **Uses** | `src/plugins/` | `plugin_loader.cpp` for external backends |

---

## 6. Threading & Concurrency Model

- `BackendRegistry` is a **singleton**; `initializeRuntime()` must be called **once** during
  single-threaded server startup.
- After `initializeRuntime()` completes, **all read operations are safe for concurrent access**
  from worker threads.
- Individual backends handle their own internal concurrency (CUDA streams, thread pools).
- `cpu_backend_mt.cpp` uses pthreads; `cpu_backend_tbb.cpp` uses Intel TBB.
- Plugin loading (`plugin_loader.cpp`) acquires a mutex during the load/unload phase.

---

## 7. Performance Architecture

| Technique | Where Applied |
|---|---|
| Capability scoring | Selects fastest backend per workload category |
| Tensor Core matmul | `tensor_core_matmul.cpp` for FP16/BF16 matmul |
| Multi-GPU work splitting | `multi_gpu_backend.cpp` |
| NCCL/RCCL collectives | `nccl_vector_backend.cpp` / `rccl_vector_backend.cpp` |
| FAISS GPU index | `faiss_gpu_backend.cpp` for billion-scale ANN |
| Exponential back-off | Avoids thundering-herd on transient device errors |

Performance targets:
- GPU ANN search: ≥8× speedup over CPU baseline on RTX-class GPU (goal for v2.0)
- CPU fallback: within 2× of hand-optimized SIMD baseline

---

## 8. Security Considerations

- **Plugin signing**: `plugin_security.cpp` verifies Ed25519/SHA-256 signatures on
  externally loaded `.so`/`.dll` backend plugins before execution.
- **Sandbox allow-list**: only backends explicitly approved in the manifest can be loaded.
- **No user data in logs**: device capability probes log hardware metadata only, never query
  data or embeddings.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `THEMIS_ENABLE_CUDA` | OFF | Compile CUDA backend |
| `THEMIS_ENABLE_VULKAN` | OFF | Compile Vulkan backend |
| `THEMIS_ENABLE_HIP` | OFF | Compile HIP/ROCm backend |
| `acceleration.retry.max_attempts` | 3 | Max retry attempts on transient GPU error |
| `acceleration.retry.initial_delay_ms` | 10 | Initial back-off delay (ms) |
| `acceleration.retry.backoff_multiplier` | 2.0 | Exponential multiplier |

---

## 10. Error Handling

| Error Class | Handling Strategy |
|---|---|
| Transient device error (`DeviceLost`, `OperationTimeout`, `SynchronizationFailed`) | Retry with exponential back-off; fall back to CPU after exhaustion |
| Permanent device error / OOM | Immediate CPU fallback; log structured error |
| Plugin signature failure | Reject load; log security alert; proceed without plugin |
| Backend initialization failure | Skip backend in selection; warn; continue with remaining backends |
| No backend available | `getSelected*Backend()` returns `nullptr`; caller must handle |

---

## 11. Known Limitations & Future Work

- CUDA ANN backends are still in progress; ANN vector operations fall through to CPU pending full HNSW integration.
- Tensor Core matrix ops (`CUDAMatrixBackend`) require a CUDA-capable device (SM 7.0+ for FP16, SM 8.0+ for BF16).
- Multi-GPU sharding backend (`MultiGPUVectorBackend`) is implemented; uses CPU sub-backends pending real CUDA kernels; `ncclGroupStart`/`ncclGroupEnd` wiring is deferred to v2.5+.
- DirectX and Metal backends are experimental.
- Plugin ABI stability guarantee starts at v2.0; breaking changes possible before then.

---

## 12. References

- `src/acceleration/README.md` — module overview and development guide
- `src/acceleration/ROADMAP.md` — development roadmap and production-readiness checklist
- `src/acceleration/FUTURE_ENHANCEMENTS.md` — detailed roadmap
- `docs/acceleration/capability_negotiation.md` — capability negotiation deep dive
- `docs/acceleration/troubleshooting.md` — operational runbook
- `include/acceleration/kernel_fallback_dispatcher.h` — dispatcher API
- `ARCHITECTURE.md` (root) — full system architecture overview
