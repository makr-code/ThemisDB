---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# GPU Implementation Guide

## Ownership and source layout

The `src/gpu` implementation owns GPU module orchestration and control
surfaces. Public contracts live under `include/themis/gpu/` and `include/gpu/`.
Backend kernels and higher-level vector/LLM algorithms may be owned by
`src/acceleration/`, `src/index/`, or `src/llm/`; keep changes in the owning
module and use the GPU interfaces for shared resource/dispatch behavior.

Useful implementation entry points:

| Concern | Source |
|---|---|
| Configuration and validation | `config.cpp`, `include/themis/gpu/config.h` |
| Facade and launch orchestration | `gpu_module.cpp`, `launcher.cpp`, `include/themis/gpu/gpu_module.h` |
| Caller capabilities | `policy.cpp`, `include/themis/gpu/policy.h` |
| Fallback/circuit state | `safe_fail.cpp`, `include/themis/gpu/safe_fail.h` |
| Device discovery and backend adapters | `device_discovery.cpp`, `rocm_backend.cpp`, `vulkan_backend.cpp` |
| VRAM accounting and pooling | `gpu_memory_manager_edition.cpp`, `memory_pool.cpp`, `include/themis/gpu/ivram_policy.h` |
| Metrics and audit events | `metrics.cpp`, `audit_log.cpp`, `admin_api.cpp` |

## Required invariants

- Validate `GPUConfig` before the facade becomes initialized. Invalid
  configuration returns an unsuccessful `InitResult`; do not partially start
  a caller-visible execution path.
- Perform policy and quota checks through their canonical interfaces. Do not
  duplicate edition or tenant accounting in a backend adapter.
- Make backend availability explicit. Compile-time flags, edition feature
  flags, and runtime device state are distinct conditions.
- Keep allocations paired with deallocation/accounting updates; rollback
  reservation state if the physical allocation fails.
- Treat timeouts and backend errors as failures, not successful GPU execution.
  Preserve diagnostics, metrics, and audit behavior when adding a path.
- Keep CPU fallback semantics explicit. The `GPUModule::submitWork` fallback
  closure currently records a fallback and returns success but does not invoke
  the submitted work item on a CPU backend. Do not use this facade result as
  proof of CPU computation without an execution path that actually performs
  the work.

The dispatch error taxonomy and frozen behavior are documented in
[`GPU_CONTRACT.md`](../../include/gpu/GPU_CONTRACT.md).

## Change validation

Add focused coverage alongside the affected behavior in `tests/gpu/`. Prefer
existing tests for config validation, policy denial, allocation limits,
backend discovery, fallback state, admin output, or dispatch timeout instead
of broad new fixtures. When behavior is backend-dependent, state whether the
test uses physical hardware, a CPU-safe path, or a controlled test backend.

GPU module sources are conditionally added by
[`EditionFeatures.cmake`](../../cmake/EditionFeatures.cmake). Review both the
enabled and CPU-only source lists when changing shared symbols; do not assume
every file in `src/gpu` is compiled in every configuration.

See [MODULE_GUIDE.md](MODULE_GUIDE.md) for developer workflow and
[RUNBOOK.md](RUNBOOK.md) for incident response.
