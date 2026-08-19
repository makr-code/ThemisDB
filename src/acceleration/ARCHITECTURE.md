# Acceleration Module - Architecture Guide

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

Version: 1.0
Last Updated: 2026-05-31
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
