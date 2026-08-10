# ThemisDB Acceleration Module

<!-- Status: PRODUCTION_CANDIDATE | Phase 1-4 complete | validated: 2026-08-10 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The Acceleration module provides hardware-aware compute backends and fallback dispatch paths for performance-critical operations in ThemisDB.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| backend_registry.cpp | runtime backend discovery and selection |
| compute_backend.cpp | core backend contract and shared interfaces |
| device_manager.cpp | device probing and capability metadata |
| ai_hardware_dispatcher.cpp | AI workload dispatch across available hardware |
| plugin_loader.cpp | dynamic backend/plugin loading path |
| plugin_security.cpp | plugin validation and security checks |
| shader_integrity.cpp | shader integrity validation before runtime use |
| multi_gpu_backend.cpp | multi-device fan-out and merge behavior |
| nccl_vector_backend.cpp | NCCL-based vector backend operations |
| rccl_vector_backend.cpp | RCCL-based vector backend operations |
| vllm_resource_manager.cpp | shared resource management for vLLM-related paths |

## Scope

In scope:
- backend registration, selection, and capability-driven dispatch
- GPU and CPU acceleration backends with deterministic fallback behavior
- plugin and shader validation for acceleration runtime surfaces
- multi-GPU and resource-management coordination paths
- acceleration-specific observability and runtime diagnostics

Out of scope:
- higher-level business logic in consuming modules
- external driver/SDK internals beyond exposed integration boundaries
- non-acceleration transport and API bootstrap logic

## Known Limitations

- runtime behavior depends on hardware, driver, and build-flag configuration
- some backend combinations remain deployment-specific and environment-dependent
- advanced distributed and plugin scenarios require continuous hardening evidence

## Sourcecode Verification (Module: acceleration/readme)

- Verified files:
  - src/acceleration/backend_registry.cpp
  - src/acceleration/compute_backend.cpp
  - src/acceleration/device_manager.cpp
  - src/acceleration/ai_hardware_dispatcher.cpp
  - src/acceleration/plugin_loader.cpp
  - src/acceleration/plugin_security.cpp
  - src/acceleration/shader_integrity.cpp
  - src/acceleration/multi_gpu_backend.cpp
  - src/acceleration/nccl_vector_backend.cpp
  - src/acceleration/rccl_vector_backend.cpp
  - src/acceleration/vllm_resource_manager.cpp
- Verified behavior surfaces:
  - backend selection and fallback dispatch
  - plugin and shader trust/validation controls
  - multi-device and resource-management integration
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - implementation history remains in CHANGELOG.md
