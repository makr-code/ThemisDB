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

# ThemisDB Acceleration Module

<!-- Status: PRODUCTION_CANDIDATE | Phase 1-4 complete | validated: 2026-10-29 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module purpose

The acceleration module manages capability-driven backend selection, runtime discovery, and fail-closed fallback behavior for performance-critical operations in ThemisDB. It provides the execution contracts for CPU, CUDA, HIP, Vulkan, OpenCL, and AI-specific dispatch surfaces while keeping the public API in `include/acceleration/` distinct from the implementation in `src/acceleration/`.

## Relevant interfaces

| Interface / file | Role |
|---|---|
| `backend_registry.cpp` | runtime backend discovery and backend registration |
| `compute_backend.cpp` | core backend contract and registry implementation |
| `device_manager.cpp` | capability probing, cache behavior, CPU fallback synthesis |
| `ai_hardware_dispatcher.cpp` | AI workload dispatch across available hardware |
| `plugin_loader.cpp` | dynamic plugin loading path |
| `plugin_security.cpp` | plugin trust validation and deny-path diagnostics |
| `shader_integrity.cpp` | shader and kernel validation before runtime execution |
| `multi_gpu_backend.cpp` | multi-device fan-out and merge behavior |
| `nccl_vector_backend.cpp` | NCCL collective/vector backend implementation |
| `rccl_vector_backend.cpp` | RCCL collective/backend implementation |
| `vllm_resource_manager.cpp` | shared resource lease/state management for vLLM |

## Scope

In scope:
- backend registration and capability-driven dispatch
- CPU/GPU fallback selection logic and deterministic fail-closed behavior
- plugin and shader validation for runtime execution
- multi-GPU and shared resource coordination
- acceleration observability and focused test injection paths

Out of scope:
- higher-level business logic in consumer modules
- direct vendor SDK internals beyond the public integration boundary
- transport/bootstrap logic outside the acceleration layer

## Known limitations

- Runtime behavior depends on hardware, driver state, and build-feature configuration.
- If discovery produces no healthy accelerator snapshot, the module emits a synthetic CPU fallback device and keeps backend selection fail-closed.
- Some backend combinations remain deployment-specific and require hardware-in-the-loop validation rather than universal local correctness claims.
- Advanced plugin and multi-device scenarios continue to require explicit benchmark and regression evidence before release sign-off.

## Source verification

- Verified files:
  - `src/acceleration/backend_registry.cpp`
  - `src/acceleration/compute_backend.cpp`
  - `src/acceleration/device_manager.cpp`
  - `src/acceleration/ai_hardware_dispatcher.cpp`
  - `src/acceleration/plugin_loader.cpp`
  - `src/acceleration/plugin_security.cpp`
  - `src/acceleration/shader_integrity.cpp`
  - `src/acceleration/multi_gpu_backend.cpp`
  - `src/acceleration/nccl_vector_backend.cpp`
  - `src/acceleration/rccl_vector_backend.cpp`
  - `src/acceleration/vllm_resource_manager.cpp`
- Verified behavior surfaces:
  - backend registration and selection
  - deterministic fallback and CPU synthesis behavior
  - plugin, shader, and resource validation paths
  - multi-device coordination and shared resource management
- Note:
  - detailed roadmap and gate status remain in `ROADMAP.md` and `FUTURE_ENHANCEMENTS.md`
  - implementation history remains in `CHANGELOG.md`
  - hardware-only performance claims are not treated as universal module guarantees without explicit benchmark evidence
