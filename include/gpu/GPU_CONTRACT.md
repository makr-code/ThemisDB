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
when fallback is enabled and a callback is present. The callback's meaning and
whether it actually computes the workload belong to its caller. In particular,
the current `GPUModule::submitWork` callback records fallback in enabled
telemetry/audit surfaces and returns success; it does not execute the submitted
work item on CPU. A generic fail-closed classification or error code does not
prove that a CPU implementation ran or that GPU/CPU outputs are equivalent.

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
