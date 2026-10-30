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

# Acceleration API guide

## Purpose and ownership

This page describes the main C++ integration contracts for acceleration consumers. The authoritative declarations and Doxygen comments are in [`include/acceleration/`](../../include/acceleration/); this guide is explanatory and must not supersede them. A pointer returned by the process-wide registry is registry-owned; callers must not delete it.

## Backend contract

`IComputeBackend` defines backend identity (`name()`, `type()`), availability, capabilities, lifecycle (`initialize()`, `shutdown()`), and diagnostic accessors. Specialized interfaces add operation contracts:

| Interface | Operation family | Main inputs / outputs |
|---|---|---|
| `IVectorBackend` | Distance and batched KNN | Query/corpus pointers, counts, dimension, `k`, metric selection; returns distances or per-query neighbors |
| `IGraphBackend` | Batched BFS and shortest paths | Adjacency data, starts/ends, optional weights, vertex/pair counts |
| `IGeoBackend` | Batched distance and point-in-polygon | Coordinate arrays and polygon coordinates; returns per-input results |
| `IMatrixBackend` | Matrix multiplication | `MatrixKernelParams` and optional opaque stream; returns an integer status |

Most operation methods return results directly. Callers must validate the inputs required by their selected operation and inspect structured error/status results where available; an empty container alone is not a universal success indicator.

## Registry and device APIs

- `BackendRegistry::instance()` provides the singleton registry. `registerBackend()` takes ownership of a `std::unique_ptr<IComputeBackend>`.
- `getBest*Backend()` returns a non-owning pointer or `nullptr` if no interface is available. `select*BackendFor()` additionally applies `CapabilityRequirements`.
- `getAvailableBackends()` and `getFallbackOrder()` expose registered types and the fixed preference sequence. `initializeRuntime()` initializes discovery and selection; `shutdownAll()` shuts down registered backends.
- `DeviceManager::probeDevices()` returns a cached vector of `DeviceCapabilityInfo`; `refresh()` performs immediate re-enumeration. `getBestDevice()` chooses the healthy accelerator with the most reported free VRAM, otherwise CPU fallback. Its cache TTL is 60 seconds.

## AI dispatcher API

`AiHardwareDispatcher::run(AiInferenceRequest&)` returns `AiInferenceResult`, including `success`, `error`, `backend_used`, `ep_used`, output tensor/shape, and latency. The request's host FP32 input pointer and `input_elements` must describe valid caller-owned storage for the duration of the call. `runOn()` requests one backend directly; it does not provide the retry chain of `run()`.

For a vector-similarity request, additionally provide a non-null corpus pointer, positive query/vector/dimension counts, `input_elements == similarity_num_queries * similarity_dim`, and positive `similarity_top_k`. The dispatcher caps `k` at the corpus count and returns flattened top-K indices and distances. Validation failures produce `success == false` and an explanatory `error`.

## Compatibility and failure behavior

`BACKEND_CONTRACT_VERSION` encodes major and minor API compatibility. The declaration requires consumers to check it together with `KERNEL_INVOCATION_INTERFACE_VERSION`; both must match before a shared library is treated as compatible. The current backend contract value is `100` (v1.0).

Capability selectors may return `nullptr`, inference may return a failed result after exhausting providers, and a backend may report an unhealthy or degraded `BackendHealthStatus`. Callers should surface these conditions and must not assume a GPU exists. See [TRUST.md](./TRUST.md) for plugin-related API boundaries and [EMBEDDING.md](./EMBEDDING.md) for embedding/similarity integration.

## Source references

- [Backend and registry declarations](../../include/acceleration/compute_backend.h)
- [Device manager declaration](../../include/acceleration/device_manager.h)
- [AI dispatcher request/result](../../include/acceleration/ai_hardware_dispatcher.h)
- [Plugin interface](../../include/acceleration/plugin_loader.h)

**Exact model backend not verifiable from final document text alone.**
