# GPU Module — Frozen Resource, Backend, Acceleration & Operations Contracts

**Version:** 1.0.0 (Frozen)  
**Status:** FROZEN — Q3 2026 gate delivery  
**Last Validated:** 2026-08-09  
**Frozen By:** Copilot agent (roadmap gate: Phase 1 Design / API Contract)

---

## 1. Scope

This document freezes the **resource, backend, acceleration, and operations
contracts** for the GPU module's active major line, and documents the
**explicit error taxonomy** for quota, degradation, and fallback failures.

The machine-readable error taxonomy is in
`include/gpu/gpu_backend_dispatch_contract.h` (`GPUDispatchErrorCode` enum).

---

## 2. Resource Contract

### 2.1 Allocation Bounds

| Limit | Value | Source |
|---|---|---|
| Max allocation size | Per `GpuMemoryAllocator::Config::max_allocation_bytes` | `gpu_memory_allocator.h` |
| Max quota check latency | 10 µs | `gpu_backend_dispatch_contract.h:MAX_QUOTA_CHECK_LATENCY_US` |
| Max allocate latency | 1 ms | Contract documented in Phase 2 delivery |

**Allocation invariants:**
- Allocations exceeding `max_allocation_bytes` → `ALLOC_SIZE_EXCEEDS_LIMIT` (fail-closed, not thrown — returned via result)
- Invalid parameters (null ptr, 0 size) → `ALLOC_INVALID_PARAMS` (fail-closed)
- Quota exceeded → `ALLOC_QUOTA_EXCEEDED` (non-retryable without freeing memory)
- All allocations MUST be released via RAII scope (no raw pointer ownership transfer)

### 2.2 RAII Lifecycle Requirement

- All GPU memory allocations MUST be managed via RAII wrappers
- No raw `cudaMalloc` / `cudaFree` calls outside the allocator
- `GpuMemoryManager::allocate()` returns a scoped handle that frees memory on destruction

---

## 3. Backend / Device Selection Contract

### 3.1 `selectDevice()` Contract

- `selectDevice()` executes within `MAX_SELECT_DEVICE_LATENCY_US` (100 µs)
- When no suitable device is available: emit `BACKEND_NO_DEVICE_AVAILABLE`
  diagnostic and return CPU fallback strategy
- Device health checks MUST complete within 100 µs
- Lock ordering: `allocation_mutex → device_state_mutex → dispatch_mutex`

### 3.2 Topology-Aware Selection

- `setTopology()` / `selectTopologyAware()` honor load balancer bounds
- When topology is unavailable: fall back to `LEAST_LOADED` strategy with
  diagnostic emission (no silent failure)

---

## 4. Acceleration Contract

- CUDA dispatch paths are conditionally compiled under `THEMIS_ENABLE_CUDA`
- When `THEMIS_ENABLE_CUDA` is OFF: CPU fallback path MUST be completely
  unmodified — no additional code paths, no stubs, no UB
- CPU fallback MUST produce identical outputs to the CUDA path (parity)
- GPU acceleration is advisory-only for Category C kernels (no GPU path permitted)

---

## 5. Operations Contract

### 5.1 Kernel SLA

- All GPU kernels MUST complete within the configured `kernel_timeout_ms` budget
- Timeout → emit `BACKEND_DEGRADED` diagnostic + CPU fallback (non-retryable
  until device state clears)
- `cudaGetLastError()` MUST be called after every kernel launch

### 5.2 Diagnostics

All GPU dispatch events are routed through `GPUBackendDispatchDiagnostics`
(see `include/gpu/gpu_backend_dispatch_diagnostics.h`). Callers MUST register
a diagnostic listener to receive quota, degradation, and fallback events.

---

## 6. Error Taxonomy Reference

The frozen `GPUDispatchErrorCode` enum is in
`include/gpu/gpu_backend_dispatch_contract.h`.

### 6.1 Quota Errors

| Code | Name | Retryable |
|---|---|---|
| `ALLOC_QUOTA_EXCEEDED` (14) | Allocation exceeds per-process or per-device quota | Yes, after freeing memory |
| `ALLOC_SIZE_EXCEEDS_LIMIT` | Single allocation exceeds `max_allocation_bytes` | No |

### 6.2 Degradation Errors

| Code | Name | Retryable |
|---|---|---|
| `BACKEND_DEGRADED` (24) | Device health check failed or device is overloaded | Yes, with backoff |
| `BACKEND_NO_DEVICE_AVAILABLE` | No GPU device meets the dispatch requirements | CPU fallback |

### 6.3 Fallback Errors

| Code | Name | Retryable |
|---|---|---|
| `FALLBACK_CPU_DEGRADED` (40) | CPU fallback path also degraded | No — escalate to operator |
| `FALLBACK_UNAVAILABLE` (41) | No fallback path available | No — escalate to operator |

---

## 7. Backward Compatibility

- `GPUDispatchErrorCode` values 0–72 (as of v1.0.0) are immutably frozen.
- New codes may be added in vacant ranges (additive).
- The bounded latency constants (`MAX_QUOTA_CHECK_LATENCY_US`, etc.) are
  frozen; tightening requires a major version bump.
- CPU fallback MUST remain semantically equivalent to GPU path (parity contract).
