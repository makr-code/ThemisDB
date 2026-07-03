# Architecture - GPU Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

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
  - src/gpu/device_discovery.cpp
  - src/gpu/stream_manager.cpp
  - src/gpu/launcher.cpp
  - src/gpu/safe_fail.cpp
  - src/gpu/query_accelerator.cpp
  - src/gpu/rocm_backend.cpp
  - src/gpu/vulkan_backend.cpp
  - src/gpu/p2p_transfer.cpp
- Verified architecture claims:
  - explicit resource/backend/execution/operations planes
  - deterministic fallback/degraded behavior boundaries
  - module-local ownership of GPU orchestration surfaces