# Architecture - GPU Module

<!-- Status: current | validated: 2026-08-17 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · PRODUCTION_REQUIREMENTS.md · PERFORMANCE_EXPECTATIONS.md · SECURITY.md -->

## Overview

The GPU module composes resource governance, backend abstraction, execution orchestration, and fallback safety into a bounded acceleration subsystem for ThemisDB.

## Main Execution Planes

1. Resource and policy plane
- VRAM quota enforcement and pool lifecycle behavior
- feature/policy gates and edition-aware constraints

2. Device and backend plane
- device discovery and capability probing
- backend adapters for CUDA/ROCm/Vulkan (as configured)

3. Execution and acceleration plane
- stream/launcher orchestration and accelerated query/training operations
- topology, P2P, and partition-aware dispatch decisions

4. Safety and operations plane
- circuit-breaker fallback and degraded-mode behavior
- metrics/profiling/admin operational observability surfaces

## Unified GPU Memory Manager Hierarchy

All GPU memory managers in ThemisDB share a consolidated `IVRAMPolicy` interface
(`include/themis/gpu/ivram_policy.h`) so that edition limits, tenant quotas, and OOM
handling are enforced at a single control point.

```
IVRAMPolicy  (themis::gpu)
└── GPUMemoryManager   — canonical singleton, edition-aware
    ├── themis::llm::GPUMemoryManager   — multi-model/multi-GPU LLM serving manager
    │   delegates allocateGPU() / freeGPU() / freeModel() through canonical policy
    └── themis::llm::lora::VRAMAllocator — LoRA training VRAM allocator
        delegates allocate() / deallocate() through canonical policy (GPU backends only)
```

### Delegation Contract

- Every GPU allocation in `themis::llm::GPUMemoryManager` is pre-checked via
  `themis::gpu::GPUMemoryManager::TryAllocateGPU()`.  On physical allocation failure
  the canonical reservation is undone via `DeallocateGPU()`.
- Every GPU deallocation in `themis::llm::GPUMemoryManager` calls
  `themis::gpu::GPUMemoryManager::DeallocateGPU()` to keep global accounting consistent.
- `themis::llm::lora::VRAMAllocator` applies the same gate for GPU backends (CUDA, HIP,
  Vulkan, DirectX, ROCm, ZLUDA).  CPU-backed allocators skip the policy check.
- The canonical manager is a no-op gate when `isGPUEnabled()` returns false (e.g.
  Community edition with an 8 GB VRAM limit), preserving backward compatibility.

### Tenant Isolation

Tenant VRAM quotas are registered on the canonical `GPUMemoryManager` singleton via
`SetTenantQuota(tenant_id, quota_bytes)`.  All subsystem managers that delegate through
the canonical gate automatically inherit this quota enforcement.

### OOM Policy

The canonical `GPUMemoryManager::canAllocate()` / `TryAllocateGPU()` methods fail fast
(return `false`) instead of throwing, providing deterministic OOM signaling.  Subsystem
managers map this to `nullptr` returns or error-log paths as appropriate.

## Core Contracts

| Contract | Behavior |
|---|---|
| resource contract | bounded allocation and deterministic quota behavior |
| backend contract | explicit capability-aware backend selection and execution |
| acceleration contract | deterministic accelerated path semantics with fallback guards |
| operations contract | explicit telemetry/profiling/admin/coordination behavior |

## Failure Semantics

- unsupported or unavailable GPU capability paths degrade deterministically.
- safety policy or quota violations fail before unsafe execution proceeds.
- backend/stream failures trigger explicit bounded fallback behavior.

## Sourcecode Verification (Module: gpu/architecture)

- Verified files:
  - src/gpu/gpu_module.cpp
  - src/gpu/gpu_memory_manager_edition.cpp
  - include/themis/gpu/ivram_policy.h
  - include/themis/gpu/memory_manager.h
  - src/gpu/device_discovery.cpp
  - src/gpu/stream_manager.cpp
  - src/gpu/launcher.cpp
  - src/gpu/safe_fail.cpp
  - src/gpu/query_accelerator.cpp
  - src/gpu/rocm_backend.cpp
  - src/gpu/vulkan_backend.cpp
  - src/gpu/p2p_transfer.cpp
  - src/llm/gpu_memory_manager.cpp
  - src/llm/lora_framework/vram_allocator.cpp
- Verified architecture claims:
  - explicit resource/backend/execution/operations planes
  - deterministic fallback/degraded behavior boundaries
  - module-local ownership of GPU orchestration surfaces
  - unified IVRAMPolicy hierarchy across namespaces