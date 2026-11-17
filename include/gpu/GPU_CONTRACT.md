---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-11-16"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

AI-assisted update; exact model backend not verifiable from final document text alone.

# GPU Resource and Dispatch Contract Reference

This page summarizes declared GPU allocation and dispatch interfaces for
maintainers. Public declarations are canonical; this document does not create
runtime guarantees that are absent from those declarations or their
implementations.

## Resource ownership

- `themis::gpu::GPUMemoryManager` is an edition-aware VRAM accounting and quota
  manager. Its `TryAllocateGPU` methods return `bool`; it does not allocate
  physical device memory or return a scoped allocation handle.
- `themis::gpu::GPUMemoryAllocator::allocate(size)` returns a `MemoryAllocation`
  record. The caller releases it through `deallocate`; it is not itself a
  scoped handle. `DeviceMemoryRegion` is a separate move-only RAII wrapper.
- Raw device-memory and stream operations exist in several GPU sources and
  helpers. Do not infer from this accounting API that all device allocations
  are routed through one allocator.

Declarations: [`themis/gpu/memory_manager.h`](../themis/gpu/memory_manager.h),
[`themis/gpu/ivram_policy.h`](../themis/gpu/ivram_policy.h), and
[`gpu_memory_allocator.h`](gpu_memory_allocator.h).

## Dispatch errors and declared bounds

`GPUDispatchErrorCode` currently defines these values:

| Value | Code |
|---:|---|
| 0 | `SUCCESS` |
| 10–14 | `ALLOC_SIZE_EXCEEDS_LIMIT`, `ALLOC_INSUFFICIENT_VRAM`, `ALLOC_DEVICE_FAILURE`, `ALLOC_INVALID_PARAMS`, `ALLOC_QUOTA_EXCEEDED` |
| 20–24 | `BACKEND_NO_DEVICE_AVAILABLE`, `BACKEND_CAPABILITY_MISMATCH`, `BACKEND_TOPOLOGY_UNAVAILABLE`, `BACKEND_NOT_ENABLED`, `BACKEND_DEGRADED` |
| 30–34 | `DISPATCH_TIMEOUT`, `DISPATCH_KERNEL_LAUNCH_FAILED`, `DISPATCH_STREAM_FULL`, `DISPATCH_CONCURRENT_EXECUTION_REJECTED`, `DISPATCH_QUERY_TYPE_UNSUPPORTED` |
| 40–41 | `FALLBACK_CPU_DEGRADED`, `FALLBACK_UNAVAILABLE` |
| 99 | `INTERNAL_ERROR` |

These are the values declared in
[`gpu_backend_dispatch_contract.h`](gpu_backend_dispatch_contract.h). The
unused numeric slots are not enum values; this reference makes no claim that
every number from 0 through 72 is frozen. Treat existing values as
compatibility-sensitive and review changes to them explicitly.

That header also declares latency constants (for example, 1 ms allocation,
100 µs device selection, and 10 µs quota checks), a lock-order string, and
fail-closed flags. These are documented contract targets, not measurements
proving every implementation path meets the bounds.

## Fallback boundary

`GPUSafeFail::executeWithFallback` invokes the supplied fallback callback only
when fallback is enabled and a callback is present. `GPUModule::initialize`
accepts an optional CPU backend callback and passes it the submitted work item.
The module reports `OkDegraded` with mode `CpuFallback` only when that callback
returns true; missing or unsuccessful CPU fallback is reported as `Failed`.
GPU completion is `Ok` with mode `Gpu`. A caller-provided callback must only
return true after completing the workload; the module cannot establish
GPU/CPU output equivalence. The default resolver selects only an enabled
ROCm/Vulkan backend that reports an available device; it does not install a
CPU-success callback in the GPU launcher. The current ROCm and Vulkan launcher
wrappers fail closed until they submit a real kernel; device availability or
device synchronization alone is not GPU success.

`SubmitResult::serialize()` exposes the stable status and mode labels together
with the legacy booleans and error. When enabled, `themis_gpu_dispatch_total`
metrics and `DISPATCH_RESULT` audit events expose the same status/mode pair.
An active observability span receives `gpu.execution.status` and
`gpu.execution.mode` attributes. Fallback metrics and audit events are recorded
only after the CPU fallback callback completes successfully.

Parity must be established by the workload-specific implementation and tests.
There is no module-wide guarantee that every failure automatically degrades to
CPU or that all GPU paths have a CPU-equivalent implementation.

## Diagnostics

The dispatch diagnostics interface supports event callbacks, but registration
is not a universal prerequisite for every GPU operation. Emission depends on
the code path calling the diagnostics API; the facade also has separate,
feature-gated audit and metrics surfaces. See
[`themis/gpu/gpu_backend_dispatch_diagnostics.h`](../themis/gpu/gpu_backend_dispatch_diagnostics.h)
and [`themis/gpu/gpu_module.h`](../themis/gpu/gpu_module.h).
