# ThemisDB GPU Module

<!-- Status: current | validated: 2026-08-17 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · PRODUCTION_REQUIREMENTS.md · PERFORMANCE_EXPECTATIONS.md · SECURITY.md -->

## Module Purpose

The GPU module provides hardware-accelerated execution and governance-safe resource control for ThemisDB, including device discovery, memory/quota management, launch/stream orchestration, backend abstraction, fallback handling, and GPU-assisted query/training paths.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| gpu_module.cpp | integration facade for policy, allocation, execution, and fallback |
| gpu_memory_manager_edition.cpp | edition-aware VRAM quota and allocation behavior |
| memory_pool.cpp | slab/pool allocation and fragmentation control |
| device_discovery.cpp | CUDA/ROCm device enumeration and capability probing |
| stream_manager.cpp | stream lifecycle and synchronization surfaces |
| launcher.cpp | execution dispatch and backend launch flow |
| safe_fail.cpp | circuit-breaker and GPU-to-CPU fallback behavior |
| query_accelerator.cpp | accelerated scan/filter/sort/aggregate/join paths |
| training_loop.cpp | accelerated training loop orchestration |
| tensor_buffer.cpp | tensor memory layout and transfer surfaces |
| rocm_backend.cpp | ROCm/HIP backend adapter |
| vulkan_backend.cpp | Vulkan backend adapter |
| p2p_transfer.cpp | peer-to-peer transfer management |
| cluster_coordinator.cpp | multi-GPU/multi-node coordination |
| cluster_topology.cpp | topology-aware scheduling metadata |
| mig_manager.cpp | MIG partition management behavior |
| metrics.cpp | GPU metrics and telemetry registry |
| profiler.cpp | profiling markers and runtime measurement surfaces |
| admin_api.cpp | operational/admin JSON surfaces |
| feature_flags.cpp | runtime feature gating |

## Scope

In scope:
- GPU device/runtime orchestration and resource governance
- backend abstraction (CUDA/ROCm/Vulkan where enabled)
- accelerated query/training execution and bounded fallback behavior

Out of scope:
- domain-specific algorithm ownership outside GPU runtime boundaries
- non-GPU authorization or identity ownership outside GPU policy checks
- external orchestration platform ownership beyond module interfaces

## Runtime Behavior and Limits

- behavior is bounded by enabled backend support, feature flags, and edition policy limits.
- GPU-disabled and degraded-hardware paths must remain deterministic and fallback-safe.
- cluster/topology capabilities can be limited by host/runtime environment features.

## Sourcecode Verification (Module: gpu/readme)

- Verified files:
  - src/gpu/gpu_module.cpp
  - src/gpu/gpu_memory_manager_edition.cpp
  - src/gpu/memory_pool.cpp
  - src/gpu/device_discovery.cpp
  - src/gpu/stream_manager.cpp
  - src/gpu/launcher.cpp
  - src/gpu/safe_fail.cpp
  - src/gpu/query_accelerator.cpp
  - src/gpu/training_loop.cpp
  - src/gpu/tensor_buffer.cpp
  - src/gpu/rocm_backend.cpp
  - src/gpu/vulkan_backend.cpp
  - src/gpu/p2p_transfer.cpp
  - src/gpu/cluster_coordinator.cpp
  - src/gpu/cluster_topology.cpp
  - src/gpu/mig_manager.cpp
  - src/gpu/metrics.cpp
  - src/gpu/profiler.cpp
  - src/gpu/admin_api.cpp
  - src/gpu/feature_flags.cpp
- Verified behavior surfaces:
  - resource governance, launch/fallback, backend abstraction, accelerated execution
  - operational telemetry and coordination boundaries
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md