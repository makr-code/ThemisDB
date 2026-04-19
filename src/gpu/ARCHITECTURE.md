# GPU Module — Architecture Guide

**Version:** 1.0
**Last Updated:** 2026-04-06
**Module Path:** `src/gpu/`

---

## 1. Overview

The GPU module provides the GPU resource management and integration façade for ThemisDB. It
handles VRAM allocation with tenant quotas, multi-GPU load balancing, circuit-breaker safe-fail
(GPU → CPU fallback), kernel validation, audit event logging, Prometheus metrics, and
per-edition feature gates.

Unlike `src/acceleration/` (which provides compute backends for ANN/geo algorithms), the GPU
module manages the GPU as a shared resource: it enforces quotas, tracks usage, validates
safety, and provides the operational plumbing that makes GPU work production-safe.

---

## 2. Design Principles

- **Default Deny** – `GPUPolicy` gates all GPU access; capability must be explicitly granted
  by edition or configuration.
- **Circuit Breaker** – `GPUSafeFailManager` monitors GPU health and falls back all work
  to CPU if the device fails, without crashing the server.
- **Tenant Quotas** – VRAM is partitioned per tenant to prevent one workload from starving
  others.
- **Edition-Aware** – feature gates (`GPUFeatureFlags`) enable/disable GPU capabilities
  per deployment edition (Community: no GPU; Hyperscaler: unlimited).
- **Kernel Validation** – every kernel is FNV-1a checksum-validated against a whitelist
  before launch to prevent execution of untrusted code.
- **Full Observability** – audit log, Prometheus metrics, and admin API provide complete
  GPU operational visibility.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `gpu_module.cpp` | Integration façade: policy → circuit-breaker → alloc → launch |
| `policy.cpp` | Default-deny capability gate |
| `safe_fail.cpp` | Circuit breaker: CLOSED/OPEN/HALF_OPEN, GPU→CPU fallback |
| `gpu_memory_manager_edition.cpp` | VRAM slab allocator with edition limits and tenant quotas |
| `memory_pool.cpp` | Slab-based pre-allocator with fragmentation tracking |
| `device_discovery.cpp` | CUDA/ROCm device enumeration; CPU-fallback sentinel |
| `load_balancer.cpp` | Multi-GPU dispatch: ROUND_ROBIN / LEAST_LOADED / FIRST_HEALTHY |
| `launcher.cpp` | Typed async work-item / batch launcher |
| `stream_manager.cpp` | Named GPU streams with CPU fallback budget |
| `kernel_validator.cpp` | FNV-1a checksum whitelist; validate before launch |
| `query_accelerator.cpp` | Parallel scan / filter / sort / aggregate / join; ANN search via cuVS/RAFT |
| `graph_cache.cpp` | LRU CUDA Graph capture cache keyed on QueryShape (OpType × rows × param_hash) |
| `tensor_buffer.cpp` | Typed tensor containers with shape/dtype, views, checkpointing |
| `training_loop.cpp` | Training loop coordinator: batch iteration, loss, early stopping |
| `rocm_backend.cpp` | ROCm/HIP stream lifecycle and device memory |
| `vulkan_backend.cpp` | Cross-vendor Vulkan compute backend (AMD/Intel/NVIDIA via SPIR-V) |
| `cluster_topology.cpp` | NVLink/InfiniBand topology detection and preferred-interconnect routing |
| `cluster_coordinator.cpp` | Multi-node GPU cluster coordination: heartbeat, health, least-loaded scheduling |
| `profiler.cpp` | NVTX/rocTX profiling markers for NVIDIA Nsight and ROCm Profiler |
| `unified_memory.cpp` | CUDA/HIP unified (managed) memory allocator with CPU fallback |
| `time_slice_scheduler.cpp` | Round-robin per-tenant GPU time-slice dispatcher |
| `mig_manager.cpp` | MIG partition lifecycle for NVIDIA Ampere/Hopper (A/H series) |
| `p2p_transfer.cpp` | Peer-to-peer GPU-to-GPU direct transfers via NVLink/PCIe |
| `wasm_kernel_sandbox.cpp` | WASM-based sandbox for untrusted third-party GPU kernel blobs |
| `metrics.cpp` | Prometheus-compatible counter/gauge registry |
| `alerts.cpp` | Threshold-based alert manager with callbacks |
| `audit_log.cpp` | Ring-buffer structured GPU audit event log |
| `admin_api.cpp` | JSON admin stats, tenant breakdown, dry-run simulation |
| `feature_flags.cpp` | Per-edition GPU feature gates with runtime overrides |
| `config.cpp` | GPU config validation, dry-run simulation |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                   Caller (index, llm, training)                  │
│   GPUModule::SubmitWork(tenant, tag, kernel_fn)                  │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                      GPUModule (façade)                         │
│                                                                  │
│  GPUPolicy → GPUSafeFailManager → GPUMemoryManager              │
│           → KernelValidator → GPULauncher                        │
│                                                                  │
│  Observability: GPUMetrics + GPUAuditLog + GPUAlertManager       │
│  Operations: GPUAdminAPI + GPUFeatureFlags + GPUConfig           │
│  Advanced: GPUGraphCache + GPUQueryAccelerator + MIGManager      │
│  Cluster: GPUClusterTopology + GPUClusterCoordinator             │
│  Isolation: GPUTimeSliceScheduler + WASMKernelSandbox            │
└──────────────────────────────────────────────────────────────────┘
         │
         ├── CUDA device via CUDA Runtime API
         ├── ROCm/HIP device via ROCmBackend
         ├── Cross-vendor GPU via VulkanComputeBackend
         └── CPU fallback (no GPU hardware required)
```

---

## 4. Data Flow

### 4.1 Work Submission

```
GPUModule::SubmitWork("tenant-1", "vector-index", kernel_fn)
    │
    ├─ GPUPolicy: tenant allowed to use GPU? edition ok?
    │       → DENY → CPU fallback
    │
    ├─ GPUSafeFailManager: circuit breaker OPEN?
    │       → OPEN → CPU fallback
    │
    ├─ GPUMemoryManager: allocate VRAM within tenant quota
    │       → quota exceeded → CPU fallback
    │
    ├─ GPUKernelValidator: checksum(kernel_fn) in whitelist?
    │       → not in whitelist → reject; security alert
    │
    ├─ GPULoadBalancer: select target device (LEAST_LOADED)
    │
    ├─ GPULauncher: async dispatch to selected device stream
    │
    └─ GPUAuditLog: log event (tenant, tag, device, VRAM, timestamp)
```

### 4.2 Circuit Breaker

```
GPU error occurs (kernel failure, OOM, device lost)
    │
    ▼
GPUSafeFailManager: record failure
    ├─ below threshold → stay CLOSED
    └─ threshold exceeded →
           OPEN circuit: all GPU work → CPU fallback
           │
           wait timeout (default: 30s)
           │
           HALF_OPEN: allow one probe request
               ├─ probe success → CLOSED
               └─ probe failure → OPEN (extend timeout)
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Used by** | `src/acceleration/` | VRAM management for GPU backends |
| **Used by** | `src/llm/` | GPU memory for LLM inference |
| **Used by** | `src/training/` | GPU training loop coordination |
| **Used by** | `src/index/` | GPU-accelerated ANN indexing |
| **Provides to** | `src/observability/` | GPU Prometheus metrics |
| **Provides to** | `src/server/` | Admin API for GPU operations |

---

## 6. Threading & Concurrency Model

- `GPUModule::SubmitWork()` is thread-safe; concurrent submissions are serialized at the
  launcher level per device.
- `GPUMemoryManager` uses per-tenant slab locks for allocation/deallocation.
- `GPUAuditLog` is a lock-free ring buffer.
- `GPUSafeFailManager` uses atomic state transitions for the circuit breaker.
- `GPULoadBalancer` reads device load stats under a shared lock.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| Slab allocator | Pre-allocated VRAM slabs reduce allocation latency |
| Named streams | Multiple async streams per device enable concurrency |
| Batch launcher | Groups small work items into batches to reduce kernel launch overhead |
| Load balancing | LEAST_LOADED strategy prevents device saturation |

### Edition-Based VRAM Limits

| Edition | VRAM Limit |
|---|---|
| Community | 0 GB (CPU-only) |
| Professional | 8 GB |
| Enterprise | 24 GB |
| Hyperscaler | No limit |

---

## 8. Security Considerations

- **Kernel whitelist**: Only FNV-1a-checksummed approved kernels may be launched.
- **Tenant isolation**: VRAM is partitioned; one tenant cannot access another's allocations.
- **Audit log**: All GPU events (allocations, launches, failures) are logged for forensics.
- **Default deny**: No GPU access without explicit policy grant.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `gpu.enabled` | auto-detect | Enable GPU module |
| `gpu.circuit_breaker.failure_threshold` | 5 | Failures before OPEN |
| `gpu.circuit_breaker.timeout_s` | 30 | OPEN → HALF_OPEN timeout |
| `gpu.load_balancer.strategy` | "LEAST_LOADED" | Load balancing strategy |
| `gpu.memory.pre_allocate_mb` | 0 | VRAM pre-allocation hint |
| `gpu.audit_log.capacity` | 10000 | Audit ring buffer capacity |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| Device OOM | Free least-recently-used allocations; retry; CPU fallback |
| Kernel launch failure | Record failure; circuit breaker; CPU fallback |
| Device lost | OPEN circuit breaker immediately; CPU fallback; alert |
| Tenant quota exceeded | Reject with quota error; log; no fallback (caller handles) |

---

## 11. Known Limitations & Future Work

- Multi-node GPU cluster coordination is implemented as CPU-level bookkeeping
  (`GPUClusterCoordinator`); real NVLink/InfiniBand fabric integration requires
  hardware and NCCL/RCCL wiring.
- Peer-to-peer GPU-to-GPU direct transfers are implemented (`GPUP2PTransferManager`);
  production verification of `cudaMemcpyPeer` throughput on NVLink hardware is pending.
- CUDA Graph capture is implemented as CPU bookkeeping simulation (`GPUGraphCache`);
  production `cudaGraph_t` / `cudaStreamBeginCapture` wiring requires GPU hardware.
- MIG partitioning infrastructure is implemented (`MIGManager`); real
  `nvmlDeviceCreateGpuInstance` calls require CUDA + NVML hardware.
- Vulkan compute backend infrastructure is implemented (`VulkanComputeBackend`);
  real `VkQueue` command-buffer submission requires the Vulkan SDK + hardware.
- WASM kernel sandbox uses CPU simulation; full sandboxing requires a
  Wasmtime/WasmEdge runtime (see `FUTURE_ENHANCEMENTS.md`).
- Kernel validation whitelist management UI is not yet implemented.

---

## 12. References

- `src/gpu/README.md` — module overview and quick start
- `docs/gpu_roadmap.md` — GPU production readiness roadmap
- `docs/gpu_runbooks.md` — operational runbooks
- `docs/NCCL_RCCL_INTEGRATION_GUIDE.md` — multi-GPU collective operations
- `ARCHITECTURE.md` (root) — full system architecture
