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

# Acceleration implementation guide

## Purpose and scope

Use this page when implementing or changing backend selection, dispatch, capability discovery, or fallback behavior. Public interfaces live under [`include/acceleration/`](../../include/acceleration/); implementation files under this directory are internal. Architectural boundaries and runtime dependencies are summarized in [ARCHITECTURE.md](./ARCHITECTURE.md).

## Runtime construction and selection

1. `BackendRegistry::instance()` constructs the process-wide registry and always registers CPU vector, graph, geo, and matrix implementations.
2. Optional backends are compiled behind feature definitions and registered only when their `isAvailable()` check succeeds.
3. `initializeRuntime()` discovers devices, loads plugins, selects vector/graph/geo backends from capability requirements, and logs those selections.
4. Selection follows the registry's fixed fallback order and verifies the requested capability set. If runtime discovery finds no healthy accelerator, registry selectors restrict their choice to CPU.
5. `DeviceManager` separately caches discovery for 60 seconds. `refresh()` bypasses that cache; `setEnumerateFn()` provides deterministic discovery injection for tests and invalidates the cache.

The AI inference dispatcher has its own probe cache (120 seconds), capability ordering, precision check, and retry chain. Its behavior is not interchangeable with registry selection. Details of the decision criteria are in [MODELING.md](./MODELING.md).

## Implementation invariants

- Keep CPU implementations registered as the baseline and make absence of optional drivers a supported state.
- Capability-driven APIs must reject a candidate that lacks any requested operation, precision, or metric; handle a null selection explicitly.
- Report backend and kernel failures through the existing result/error types. A fallback is not success unless it produces a valid result.
- Preserve request validation before dispatch. In vector similarity, query/corpus pointers, counts, dimensions, element count, and `top_k` are checked before launching.
- Run plugin path and trust validation before dynamic loading; keep deny outcomes observable.
- For new performance behavior, add deterministic correctness/failure coverage and a benchmark mapping rather than relying on device-name assumptions.

## Failure and edge cases

| Condition | Existing behavior to preserve |
|---|---|
| No discovered devices or no healthy accelerator | Device discovery provides or synthesizes a CPU fallback; runtime registry selection is CPU-only |
| Requested capability unavailable | Capability selector returns `nullptr` |
| AI precision unsupported | Try FP32 only if the candidate advertises it; otherwise continue to the next backend |
| AI backend execution fails | Log the failure and continue through the ordered chain; return a failed result if exhausted |
| CUDA vector-similarity initialization or kernel fails | Fall back to CPU similarity execution |
| Plugin path, signature, or policy validation fails | Reject the plugin before loading and record a security event |

## Test and review path

Build and run the focused module tests using the repository's existing [acceleration test instructions](../../tests/acceleration/README.md). For new selection behavior, include deterministic tests for both eligible and rejected capability combinations, empty/unhealthy device snapshots, and no-hardware builds. For numerical operations, compare CPU and accelerator results under the same precision and metric. Use [PERFORMANCE_EXPECTATIONS.md](./PERFORMANCE_EXPECTATIONS.md) for benchmark naming and release-gate mapping.

Before changing a public contract, review [API.md](./API.md), the corresponding Doxygen declarations, and the backend contract version in [compute_backend.h](../../include/acceleration/compute_backend.h). A breaking API change requires the contract version and migration/release documentation to be handled together.

## Source references

- [Backend registration and dispatch selection](./backend_registry.cpp)
- [Device discovery/cache](./device_manager.cpp)
- [AI dispatcher](./ai_hardware_dispatcher.cpp)
- [Plugin loader](./plugin_loader.cpp)
- [Backend interfaces](../../include/acceleration/compute_backend.h)

**Exact model backend not verifiable from final document text alone.**
