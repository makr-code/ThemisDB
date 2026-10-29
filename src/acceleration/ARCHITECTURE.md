---
Author: "ThemisDB Team"
Created: "2026-10-29"
Last Updated: "2026-10-29"
Status: "active"
AI_Used: true
AI_Models: "GitHub Copilot"
AI_Contributor: "GitHub Copilot"
Human_Review: true
Human_Author: "ThemisDB Team"
---

# Acceleration module architecture

## Purpose

The acceleration module provides the runtime selection, capability probing, and fail-closed fallback behavior used by compute-heavy consumers in ThemisDB. The source implementation lives in `src/acceleration/`; the public contract is exposed through `include/acceleration/`.

## Primary runtime surfaces

| Surface | Key files | Role |
|---|---|---|
| Registry and contract | `backend_registry.cpp`, `compute_backend.cpp` | selects and exposes the active backend for vector/graph/geo operations |
| Device capability probing | `device_manager.cpp` | enumerates devices, caches the result, synthesizes a CPU fallback when necessary |
| AI routing | `ai_hardware_dispatcher.cpp` | routes AI inference requests to the best available backend or CPU fallback |
| Backend implementations | `cuda_backend.cpp`, `hip_backend.cpp`, `opencl_backend.cpp`, `cpu_backend.cpp`, `vulkan_backend_full.cpp` | backend-specific execution paths and runtime gating |
| Multi-device coordination | `multi_gpu_backend.cpp`, `nccl_vector_backend.cpp`, `rccl_vector_backend.cpp` | fan-out / reduce / distributed coordination |
| Security and integrity | `plugin_loader.cpp`, `plugin_security.cpp`, `shader_integrity.cpp` | trust checks and runtime guardrails |
| Resource management | `vllm_resource_manager.cpp`, `tensor_core_matmul.cpp` | quota and resource-state management |

## Runtime control flow

1. `DeviceManager::probeDevices()` enumerates the current device set and caches the result for 60 seconds.
2. The result is normalized into `DeviceCapabilityInfo` values and can be overridden by `DeviceManager::setEnumerateFn()` in deterministic tests.
3. `BackendRegistry` selects a runtime backend based on discovered capability data and the workload category.
4. AI and compute dispatch paths prefer the highest-capability healthy device and degrade to CPU fallback when necessary.
5. Security and integrity checks guard plugin and shader paths before execution proceeds.

## Safety and fallback model

- The baseline fallback is CPU; empty or unhealthy discovery snapshots produce a synthetic `CPU Fallback` device.
- `getBestDevice()` filters out unhealthy backends first and only falls back to CPU when no healthy alternative exists.
- Fail-closed behavior is the preferred design for invalid or partial backend states; runtime errors are not silently ignored.
- Performance and feature availability remain hardware- and build-dependent. Any throughput claim should be treated as benchmark evidence, not a guarantee for all hosts.

## Boundary expectations

| Direction | Contract |
|---|---|
| Consumed by | higher-level index, graph, geo, and AI runtime paths |
| Depends on | hardware drivers, optional SDKs, host capabilities, and runtime feature gates |
| Exposes | backend registry, device capability info, dispatch API, and result/error diagnostics |

## Source validation

The architecture described here is consistent with the current runtime implementation in:
- `src/acceleration/backend_registry.cpp`
- `src/acceleration/device_manager.cpp`
- `src/acceleration/ai_hardware_dispatcher.cpp`
- `src/acceleration/plugin_loader.cpp`
- `src/acceleration/plugin_security.cpp`
- `src/acceleration/shader_integrity.cpp`
- `include/acceleration/compute_backend.h`
- `include/acceleration/device_manager.h`

## Open assumptions

- Advanced GPU performance or parity gates remain benchmark- and hardware-specific and must not be treated as blanket module guarantees.
- Optional backend paths still require host-side validation before they are described as fully production-ready for a given deployment.
