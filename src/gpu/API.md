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

# GPU Module API

## Audience and source of truth

This page summarizes the module-facing C++ facade for integrators. Header
declarations are canonical: [`gpu_module.h`](../../include/themis/gpu/gpu_module.h),
[`config.h`](../../include/themis/gpu/config.h),
[`launcher.h`](../../include/themis/gpu/launcher.h), and
[`policy.h`](../../include/themis/gpu/policy.h). The separate
allocation/dispatch contract reference is in
[`GPU_CONTRACT.md`](../../include/gpu/GPU_CONTRACT.md).

## `themis::gpu::GPUModule`

`GPUModule` integrates configuration, caller policy, the launcher, safe-fail
state, memory accounting, metrics, and audit events. It may be used as an
instance (useful for tests) or through `GetInstance()` in production code.
The header documents its public methods as thread-safe.

| Method | Contract and edge cases |
|---|---|
| `initialize(config, backend, cpu_fallback_backend)` | Validates `config`; returns `{ok=false, error}` on validation failure. The launcher is created only when `ASYNC_LAUNCHER` is enabled and an enabled hardware backend is available. A supplied GPU backend overrides default selection. Without an available default backend, the explicit CPU callback is used by `submitWork`; it must execute the same item and return true only on completion. |
| `submitWork(caller_id, tenant_id, item)` | Returns failure if uninitialized. If `POLICY_GATE` is enabled, caller must hold `GPU_ALLOCATE`. Result includes `status` (`Ok`, `OkDegraded`, or `Failed`), `mode` (`Gpu`, `CpuFallback`, or `None`), `submitted`, `via_gpu`, and `error`. CPU fallback success is reported as `OkDegraded`; no callback or a failing callback is `Failed`. |
| `allocate(caller_id, tenant_id, bytes, tag)` | Returns false if uninitialized, denied by an enabled policy gate, or rejected by the canonical memory manager. An empty tenant ID omits tenant-specific accounting. |
| `deallocate(tenant_id, bytes)` | No-op before initialization; otherwise releases the accounted bytes and updates enabled telemetry/audit surfaces. The method has no result value. |
| `grantCaller(caller_id, capability)` / `revokeCaller(caller_id)` | Mutate in-process caller grants. `grantCaller` defaults to `GPU_ANY`; revocation removes all grants for that caller. Protect these operations at the owning administrative boundary. |
| `getMemoryStats()` / `getSafeFailStatus()` / `getAuditLog(last_n)` | Return current shared memory statistics, safe-fail health, and up to the requested number of recent local audit events (default 64). |

`GPUConfig::validate()` returns all detected errors and warnings. Validation
includes pool, OOM-threshold, circuit-breaker, and VRAM-reserve consistency;
see [`config.h`](../../include/themis/gpu/config.h) for exact conditions and
defaults. `simulateAllocation(bytes, current_allocated_bytes)` is a
configuration-level simulation and does not access physical GPU state.

`SubmitResult::serialize()` emits JSON containing the stable status/mode
labels, legacy booleans, and error text. When the metrics and audit-log
features are enabled, dispatches also increment
`themis_gpu_dispatch_total{status,mode}` and record a `DISPATCH_RESULT` event.
When an observability span is active, dispatches set
`gpu.execution.status` and `gpu.execution.mode` attributes on that span.

## Backend and diagnostics contracts

`GPULauncher::BackendFn` and `WorkItem` are defined in
[`launcher.h`](../../include/themis/gpu/launcher.h). The current module default
resolver checks ROCm and then Vulkan availability; callers can inject a
backend function during initialization. Feature flags can gate subsystem
creation independently of CMake backend compilation.

The default resolver selects ROCm, then enabled Vulkan, only when that backend
reports an available device; it does not install a CPU-success callback as a
GPU backend.

For allocation and dispatch error codes and diagnostic expectations, use the
current
[`GPU_CONTRACT.md`](../../include/gpu/GPU_CONTRACT.md) and
[`gpu_backend_dispatch_contract.h`](../../include/gpu/gpu_backend_dispatch_contract.h).
Treat existing enum values as compatibility-sensitive; do not silently convert
a denial or timeout into success.
