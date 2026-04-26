> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# GPU Module

GPU utility functions and memory management for ThemisDB.

## Module Purpose

Provides GPU compute integration for ThemisDB, implementing VRAM management with tenant quotas, multi-GPU load balancing, circuit breaker safe-fail, kernel validation, and parallel query acceleration.

## Subsystem Scope

**In scope:** VRAM allocation and tenant quotas, CUDA/ROCm device enumeration, circuit breaker with GPU→CPU fallback, audit event log, capability gate, kernel whitelist, Prometheus metrics, multi-GPU load balancer, parallel scan/filter/sort/aggregate/join, ROCm/HIP stream and device-memory backend.

**Out of scope:** GPU kernel implementations for specific algorithms (handled by acceleration module), model training orchestration (handled by training module).

## Relevant Interfaces

- `gpu_memory_manager_edition.cpp` — VRAM slab allocator with tenant quotas
- `safe_fail.cpp` — GPU→CPU safe-fail (circuit breaker)
- `device_discovery.cpp` — CUDA/ROCm device discovery
- `query_accelerator.cpp` — parallel query operations
- `metrics.cpp` — Prometheus metrics

## Current Delivery Status

**Maturity:** 🟡 Beta — VRAM management, circuit breaker, parallel query acceleration, and ROCm/HIP backend parity are operational; multi-node coordination remains a planned hardening item.

## Components

| Header | Source | Description |
|---|---|---|
| `memory_manager.h` | `gpu_memory_manager_edition.cpp` | Edition-aware VRAM allocation, tenant quotas, pre-allocation hints |
| `device_discovery.h` | `device_discovery.cpp` | Enumerate CUDA/ROCm devices; CPU-fallback sentinel |
| `safe_fail.h` | `safe_fail.cpp` | Circuit-breaker safe-fail with GPU→CPU fallback |
| `audit_log.h` | `audit_log.cpp` | Ring-buffer structured audit event log |
| `policy.h` | `policy.cpp` | Default-deny capability gate for GPU usage |
| `memory_pool.h` | `memory_pool.cpp` | Slab-based pre-allocator with fragmentation tracking |
| `metrics.h` | `metrics.cpp` | Prometheus-compatible counter/gauge registry |
| `config.h` | `config.cpp` | GPU config validation, dry-run simulation |
| `kernel_validator.h` | `kernel_validator.cpp` | FNV-1a checksum whitelist; validate-before-launch |
| `alerts.h` | `alerts.cpp` | Threshold-based alert manager with callbacks |
| `launcher.h` | `launcher.cpp` | Typed async work-item / batch launcher |
| `load_balancer.h` | `load_balancer.cpp` | Multi-GPU load balancer (ROUND_ROBIN/LEAST_LOADED/FIRST_HEALTHY) |
| `feature_flags.h` | `feature_flags.cpp` | Per-edition GPU feature gates with runtime overrides |
| `admin_api.h` | `admin_api.cpp` | JSON admin stats, tenant breakdown, dry-run simulation |
| `gpu_module.h` | `gpu_module.cpp` | Integration facade: policy→circuit-breaker→alloc→launch |
| `stream_manager.h` | `stream_manager.cpp` | Named async GPU streams with CPU fallback budget |
| `query_accelerator.h` | `query_accelerator.cpp` | Parallel scan/filter/sort/aggregate/join with GPU threshold dispatch |
| `tensor_buffer.h` | `tensor_buffer.cpp` | Typed tensor containers with shape/dtype, views, checkpointing |
| `training_loop.h` | `training_loop.cpp` | Training loop coordinator: batch iteration, loss tracking, early stopping |
| `rocm_backend.h` | `rocm_backend.cpp` | ROCm/HIP backend: stream lifecycle, device memory, launcher BackendFn |

## Architecture

```
GPUModule (gpu_module.h)
 ├── GPUPolicy           – default-deny capability gate
 ├── GPUSafeFailManager  – circuit-breaker, GPU→CPU fallback
 ├── GPUMemoryManager    – edition-aware VRAM, tenant quotas, hints
 ├── GPUMemoryPool       – slab pre-allocator, zero-on-free
 ├── GPUDeviceDiscovery  – enumerate devices, CPU-fallback sentinel
 ├── GPULoadBalancer     – multi-device dispatch strategies
 ├── GPULauncher         – typed async work-item / batch launcher
 ├── GPUStreamManager    – named streams, CPU budget enforcement
 ├── GPUKernelValidator  – checksum whitelist, validate-before-launch
 ├── GPUMetricsRegistry  – Prometheus-compatible counters/gauges
 ├── GPUAlertManager     – threshold alerts with callbacks
 ├── GPUAuditLog         – ring-buffer structured event log
 ├── GPUAdminAPI         – JSON stats, tenants, simulate endpoints
 ├── GPUFeatureFlags     – per-edition feature gates
 ├── GPUConfig           – startup validation, dry-run simulation
 ├── GPUQueryAccelerator – scan/filter/sort/aggregate/join
 ├── GPUTensorBuffer     – typed tensors, views, checkpointing
 ├── GPUTrainingLoop     – batch training coordinator
 └── ROCmBackend         – HIP stream lifecycle, device memory, launcher backend
```

## Edition-Based GPU Limits

| Edition | VRAM Limit | Notes |
|---|---|---|
| Community | 0 GB | CPU-only; all GPU paths fall back gracefully |
| Professional | 8 GB | Small models, limited acceleration |
| Enterprise | 24 GB | Medium models, production use |
| Hyperscaler/Unlimited | No limit | Large models, research |

## Quick Start

```cpp
#include "themis/gpu/gpu_module.h"
using namespace themis::gpu;

// Submit GPU work through the integration facade.
// Policy, circuit-breaker, VRAM allocation, metrics and audit are handled automatically.
GPUModule module;
module.SubmitWork("my-tenant", "index-build", [](float* buf, size_t n) {
    // CPU-side stub — replace with real CUDA/ROCm kernel call
    for (size_t i = 0; i < n; ++i) buf[i] *= 2.0f;
});
```

```cpp
#include "themis/gpu/memory_manager.h"
using namespace themis::gpu;

auto& mgr = GPUMemoryManager::GetInstance();
if (mgr.TryAllocateGPU(1ULL << 30, "vector-index", "tenant-a")) {
    // use 1 GB VRAM ...
    mgr.DeallocateGPU(1ULL << 30, "tenant-a");
}
auto stats = mgr.GetStats();
// stats.allocated_bytes, peak_bytes, allocation_count, deallocation_count
```

## Thread Safety

All components are thread-safe (mutex-protected). Concurrent alloc/dealloc,
metric writes, and audit-log appends are safe.

## Dependencies

- **Edition Module**: Edition-specific VRAM limits (`gpu_memory_manager_edition.cpp`)
- **C++17 standard library**: `<mutex>`, `<thread>`, `<atomic>` — no external deps
- **CUDA/ROCm** (optional): Hardware integration in `FUTURE_ENHANCEMENTS.md`

## Documentation

- [Production Readiness Roadmap](../../docs/gpu_roadmap.md)
- [On-call Runbooks](../../docs/gpu_runbooks.md)
- [Future Enhancements](FUTURE_ENHANCEMENTS.md)

## Version History

- **v1.0.0**: Edition-aware GPU memory manager
- **v1.1.0**: Device discovery, safe-fail circuit breaker, audit log, policy gate
- **v1.2.0**: Memory pool, metrics, config validation, kernel validator, alerts,
  launcher, load balancer, feature flags, admin API, integration facade,
  stream manager, query accelerator, tensor buffer, training loop
- **v1.3.0**: ROCm/HIP backend parity (`rocm_backend.cpp`): HIP stream lifecycle,
  device memory (`hipMalloc`/`hipFree`/`hipMemset`), launcher BackendFn with CPU
  fallback; `GPUStreamManager` default backend now wires through `ROCmBackend`
- **v1.8.0**: Public header documentation: added `include/themis/gpu/README.md`
  documenting all 28 GPU public headers with key types, APIs, and usage examples

## See Also

- [LLM Module](../llm/README.md) — GPU model inference
- [Vector Index](../index/README.md) — GPU-accelerated indexing

## Scientific References

1. Nickolls, J., Buck, I., Garland, M., & Skadron, K. (2008). **Scalable Parallel Programming with CUDA**. *Queue*, 6(2), 40–53. https://doi.org/10.1145/1365490.1365500

2. NVIDIA Corporation. (2023). **CUDA C++ Programming Guide (v12.x)**. NVIDIA Developer Documentation. https://docs.nvidia.com/cuda/cuda-c-programming-guide/

3. Ryoo, S., Rodrigues, C. I., Baghsorkhi, S. S., Stone, S. S., Kirk, D. B., & Hwu, W. M. W. (2008). **Optimization Principles and Application Performance Evaluation of a Multithreaded GPU Using CUDA**. *Proceedings of the 13th ACM SIGPLAN Symposium on Principles and Practice of Parallel Programming (PPoPP)*, 73–82. https://doi.org/10.1145/1345206.1345220

4. Johnson, J., Douze, M., & Jégou, H. (2019). **Billion-Scale Similarity Search with GPUs**. *IEEE Transactions on Big Data*, 7(3), 535–547. https://doi.org/10.1109/TBDATA.2019.2921572

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.
