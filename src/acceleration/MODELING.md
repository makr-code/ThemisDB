---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Acceleration capability and selection model

## Purpose and scope

This document describes the runtime decision model for selecting an execution backend. “Modeling” here means capability and dispatch modeling, not machine-learning model training. The implementation and public contracts are in [backend_registry.cpp](./backend_registry.cpp), [device_manager.cpp](./device_manager.cpp), and [compute_backend.h](../../include/acceleration/compute_backend.h).

## Candidate eligibility

An operation supplies a `BackendRegistry::CapabilityRequirements` value. A backend is eligible only when its `BackendCapabilities` satisfy every requested operation flag (`vector`, `graph`, `geo`, `matrix`, `batch`, and `async`), every requested precision bit, and every requested distance-metric bit. `BackendRegistry::satisfies()` implements this as a conjunction; it does not estimate runtime cost or select by measured throughput.

The registry considers registered backends in its fixed fallback order, from `MULTI_GPU` through available accelerator types to `CPU`. The first registered backend with the requested interface and capabilities is selected. Registration itself checks `isAvailable()`. After runtime initialization, a device snapshot with no healthy non-CPU device restricts registry selection to CPU.

`DeviceManager` exposes a separate device summary policy: it filters unhealthy and CPU entries when choosing a device, then chooses the healthy accelerator with the greatest reported free VRAM. If no such device exists, it returns a CPU entry; if discovery did not provide one, it synthesizes `"CPU Fallback"` with index `-1`. This device ranking is not the same operation as registry fallback ordering.

## AI dispatcher ordering

`AiHardwareDispatcher` builds a capability list, stable-sorts available entries before unavailable ones and then sorts by reported TOPS. `run()` skips unavailable entries and checks the requested precision. If the requested non-FP32 precision is unsupported but FP32 is supported, it logs the fallback and tries that backend; otherwise it skips the backend. A failed dispatch advances to the next candidate. If the chain is exhausted, the result reports failure.

This ordering is a preference chain, not a performance guarantee: TOPS can be unknown (zero), and a backend's advertised availability does not prove that a particular model or operation will execute successfully.

## Inputs, outputs, and boundary cases

| Model input | Source representation | Selection consequence |
|---|---|---|
| Operation requirements | `CapabilityRequirements` flags, precision mask, metric mask | Unsupported operations are excluded |
| Backend support | `BackendCapabilities` | Candidates must satisfy all requested fields |
| Device state | `DeviceCapabilityInfo` health and backend type | Runtime CPU-only mode is used when no healthy accelerator is present |
| AI request | preferred precision and dispatcher capability probes | Unsupported precision is skipped or reduced to FP32 when available |

If requirements cannot be met, registry selection returns `nullptr`; callers must handle that result. A capability snapshot can become stale between probes: `DeviceManager` caches for 60 seconds and `AiHardwareDispatcher` for 120 seconds. Use their refresh/force mechanisms when an operator or test needs an immediate re-probe.

## Evaluation and limits

Test selection determinism and capability mismatch in the regression and selection suites documented in [tests/acceleration/README.md](../../tests/acceleration/README.md). Evaluate performance independently using the mapped cases in [PERFORMANCE_EXPECTATIONS.md](./PERFORMANCE_EXPECTATIONS.md). The selector has no learned cost model, topology-aware score, or automatic benchmark feedback loop; multi-device topology and partial-failure policy remain tracked work in [FUTURE_ENHANCEMENTS.md](./FUTURE_ENHANCEMENTS.md).

## Source references

- [Capability types and registry API](../../include/acceleration/compute_backend.h)
- [Registry selection and fallback order](./backend_registry.cpp)
- [Device ranking and cache](./device_manager.cpp)
- [AI capability sorting and precision checks](./ai_hardware_dispatcher.cpp)

**Exact model backend not verifiable from final document text alone.**
