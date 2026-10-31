# Architecture - GPU Module

<!-- Source-reviewed: 2026-10-31; no runtime validation performed -->
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

## VRAM accounting and subsystem boundaries

`themis::gpu::GPUMemoryManager` implements `IVRAMPolicy` and provides
edition-wide accounting plus tenant-aware overloads. Its `TryAllocateGPU` /
`DeallocateGPU` methods account for reservations; they do not allocate physical
device memory.

The current LLM manager and LoRA `VRAMAllocator` contain call sites that use
this canonical manager for GPU-backed accounting. In the inspected paths, the
LLM manager supplies a model ID as the allocation tag, and the LoRA allocator
supplies `"lora-vram"`; neither passes a tenant ID to the tenant-aware overload.
Those call sites therefore do not automatically enforce registered
tenant-specific quotas. Do not generalize these integrations to every GPU
allocator or workload.

Both integrations conditionally call the canonical accounting gate when
`isGPUEnabled()` is true. That method currently reflects whether the compiled
edition VRAM cap is greater than zero; a zero-cap build skips the accounting
gate. The Community cap is configured as 16 GiB by edition configuration, but
the code does not require or identify a particular physical GPU model.

### OOM and ownership

The canonical manager's admission methods return `false` on rejection. Physical
allocation and lifetime remain the responsibility of each subsystem; some
integrations undo accounting after a later allocation failure. Review the
owning allocator before relying on rollback, tenant propagation, or RAII
behavior.

### OOM Policy

The canonical manager's admission methods return `false` on rejection.
Subsystem handling is implementation-specific; inspect the owning allocator
for its return, rollback, and error-reporting behavior.

## Core Contracts

| Contract | Behavior |
|---|---|
| resource contract | bounded allocation and deterministic quota behavior |
| backend contract | explicit capability-aware backend selection and execution |
| acceleration contract | backend-specific execution; fallback behavior depends on the caller-supplied path |
| operations contract | explicit telemetry/profiling/admin/coordination behavior |

## Failure Semantics

- unsupported or unavailable GPU capability paths may fail or invoke a caller-supplied fallback; CPU computation is not universal.
- safety policy or quota violations fail before unsafe execution proceeds.
- backend/stream failures are surfaced through their owning path; fallback execution and its bounds are not universal.

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
  - fallback/degraded behavior boundaries in selected paths
  - module-local ownership of GPU orchestration surfaces
  - canonical IVRAMPolicy accounting implementation and selected LLM/LoRA call sites
---

### Direct Downstream Consumers (modules that use this module)

| Module | Via | Notes |
|--------|-----|-------|
| `themis` | `include/gpu/cuda_raii.h`, `include/gpu/gpu_backend_dispatch_diagnostics.h` | `themis/gpu/stream_manager.h` wraps CUDA RAII lifetime management; `themis/gpu/gpu_backend_dispatch_diagnostics.h` re-exports the GPU backend diagnostics surface (`include/themis/gpu/stream_manager.h`, `include/themis/gpu/gpu_backend_dispatch_diagnostics.h`) |
