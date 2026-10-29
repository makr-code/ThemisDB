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

> Build: `cmake --preset linux-release && cmake --build --preset linux-release`

# Acceleration tests

Path: `tests/acceleration`

## Purpose

This directory holds the focused regression, fallback, and hardware-dispatch tests for the acceleration module. The suite validates runtime selection logic, fail-closed behavior, multi-device coordination, plugin/security guardrails, and performance-gate regressions against the code in `src/acceleration/`.

## Test categories

- **Backend selection and dispatch**: `test_acceleration_dispatch.cpp`, `test_acceleration_regression.cpp`, `test_acceleration_failure_handling.cpp`
- **AI and device routing**: `test_ai_hardware_dispatcher.cpp`, `test_backend_registry_thread_safety.cpp`
- **Vector, graph, and distributed paths**: `test_nccl_merge_topk.cpp`, `test_vec_knn_insert.cpp`
- **Performance and hardening**: `test_acceleration_metrics.cpp`, `test_acceleration_performance_gates.cpp`, `test_acceleration_highcardinality_stress.cpp`, `test_acceleration_plugin_security_hardening.cpp`, `test_acceleration_vllm_resource_manager_focused.cpp`
- **Specialized hardware**: `test_zluda_backend.cpp`

## Source-aligned validation notes

- The code paths under test correspond to the runtime registry and dispatch surfaces in `src/acceleration/backend_registry.cpp`, `device_manager.cpp`, `ai_hardware_dispatcher.cpp`, and the backend implementations.
- CPU fallback, capability injection, plugin validation, and degraded runtime behavior are explicitly exercised by the test set rather than assumed.
- Benchmark/performance gates remain hardware-dependent and are treated as benchmark evidence, not universal host-independent guarantees.

## Open assumptions

- Hardware-specific performance gates still depend on the execution environment; local validation must be treated as environment-specific evidence.
- If a backend is unavailable in a given build, tests that require it must be gated accordingly rather than reported as module-wide pass criteria.

## Related docs

- `../../src/acceleration/README.md`
- `../../src/acceleration/ARCHITECTURE.md`
- `../../src/acceleration/ROADMAP.md`
- `../../src/acceleration/FUTURE_ENHANCEMENTS.md`
