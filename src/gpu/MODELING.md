---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "ThemisDB documentation orchestration specialist"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# GPU Resource and Dispatch Model

## Purpose

This page describes the module's resource/admission and dispatch model for
developers changing allocation or backend selection. It is not a machine
learning model specification and does not define workload-specific scheduling
objectives.

## Resource state

The canonical policy surface is `themis::gpu::IVRAMPolicy`, implemented by the
edition-aware `themis::gpu::GPUMemoryManager`. It accounts for process/edition
usage and, when a non-empty tenant is supplied and a quota is registered,
tenant usage. Selected LLM and LoRA allocation paths call the canonical
manager, but the inspected calls use model/tag labels and do not pass tenant
IDs; registered tenant-specific quotas therefore do not automatically apply
through those calls. See [ARCHITECTURE.md](ARCHITECTURE.md) and
[`ivram_policy.h`](../../include/themis/gpu/ivram_policy.h).

`GPUConfig` provides these inputs to startup validation:

- `max_vram_bytes`: zero selects the edition default;
- `min_free_vram_bytes`: default reserve is 512 MiB;
- `oom_warning_threshold`: warning threshold in `(0, 1]`;
- pool slab size/count and pool enablement;
- circuit-breaker failure/success thresholds and reset timeout;
- CPU-fallback enablement and optional fallback time budget.

The config validator rejects invalid thresholds, non-positive enabled pool
slabs, invalid circuit thresholds/timeouts, and a minimum-free reserve that is
not below an explicitly configured maximum. If slab count is derived from the
maximum, an unset maximum produces a warning. Refer to
[`config.h`](../../include/themis/gpu/config.h) before adding a new field or
assuming that a default is a deployment limit.

Conceptually, an allocation is admissible only when the canonical manager
accepts it under the edition cap and any applicable tenant quota. Callers must
use the manager's result rather than calculate availability from a device
query alone: free device memory can change between observation and allocation,
and accounting is shared across delegating subsystems.

## Dispatch state

The integration facade validates configuration, initializes safe-fail state,
and creates its asynchronous launcher only when the `ASYNC_LAUNCHER` feature is
enabled. Unless a backend function is supplied, the current resolver tries
ROCm first and Vulkan second. Backend-specific availability and compilation
are separate: a compiled backend is not evidence that a compatible device is
usable at runtime.

The circuit breaker tracks operation outcomes and may stop attempting GPU work
while open or failed. Once the configured reset interval elapses, its reset
path allows a probe in the degraded state. The configured CPU callback is
separate from GPU dispatch. In `GPUModule::submitWork`, that callback records
fallback and returns success; it does not execute the submitted work item.
Therefore a successful result with `via_gpu == false` is not, on its own,
evidence that a CPU implementation produced equivalent output. Callers that
require computational fallback must provide/verify that behavior at their
own execution boundary.

## Trade-offs and validation

- Larger batches can amortize dispatch/transfer costs but increase temporary
  memory demand and the cost of a failed operation.
- Pooling can reduce repeated allocation work but retains reserved memory;
  validate quota/accounting and fragmentation behavior under concurrent use.
- Multi-device and peer-transfer paths depend on runtime topology and feature
  gates; do not infer availability from a device count alone.
- CPU-only tests validate only the paths they execute. Compare actual outputs
  and resource counters for backend parity; measure latency using the mapped
  cases in [PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md).

Supporting contracts: [`GPU_CONTRACT.md`](../../include/gpu/GPU_CONTRACT.md),
[`GPUConfig`](../../include/themis/gpu/config.h), and
[`GPUModule`](../../include/themis/gpu/gpu_module.h).
