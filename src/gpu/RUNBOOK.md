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

# GPU Incident Runbook

## Scope and safety

Use this page for GPU allocation, dispatch, and degradation incidents. It
assumes the service owner can inspect the GPU module's existing diagnostics.
Do not disable caller-policy checks, tenant limits, audit, or validation as a
recovery shortcut. Escalate host/driver incidents to the platform owner and
workload correctness incidents to the consuming module owner.

## First-response evidence

Capture before restarting or changing runtime flags:

1. Timestamp, request/workload, build edition, enabled GPU/backend flags, and
   device/driver/runtime versions.
2. The caller-visible result (`InitResult`, `SubmitResult`, or allocation
   result), especially `SubmitResult::via_gpu` and `error`.
3. `GPUModule::getSafeFailStatus()`, `getMemoryStats()`, and recent
   `getAuditLog()` events where available.
4. `GPUAdminAPI` stats, tenant, and device JSON if that surface is enabled.
5. The corresponding metrics snapshot, including allocation-denial,
   fallback, and circuit-open counters.

The facade can only emit audit/metrics events when their feature flags are
enabled. Preserve these values with the incident record; do not assume a
missing event means no failure.

## Symptom-to-action

| Symptom | Diagnosis | Safe response |
|---|---|---|
| Initialization returns an error | Read `GPUConfig::validate()` errors; check pool slab/count, thresholds, timeout, and explicit VRAM reserve/limit consistency. | Correct the invalid deployment config, validate it before rollout, and reinitialize. Do not loosen a limit without the resource owner. |
| Allocation is denied | Check caller capability, edition/global usage, registered tenant quota, request size, and concurrent allocations. | Free confirmed-unused allocations or adjust an approved quota/edition configuration; retry after verifying accounting. |
| Device is absent or unhealthy | Compare build backend flags with runtime discovery and the host's driver/device state. | Keep the service in its supported degraded mode if correctness is preserved; restore the device/runtime or roll back the backend rollout. |
| Fallback/circuit-open counters rise | Correlate safe-fail status, backend errors, timeout, and device health; identify whether failures are isolated or sustained. | Reduce or stop new GPU work through the deployment's approved feature controls. Retry only after the circuit reset interval and a successful health check. |
| Admin stats are unavailable | Check the admin API error JSON and server-side logs; the API catches allocation/runtime/other exceptions and reports an unavailable result. | Use local logs and safe-fail/memory snapshots; escalate if diagnostic access remains unavailable. |

## Recovery and rollback

1. Stop or drain the affected workload if correctness, memory safety, or tenant
   isolation is uncertain.
2. Preserve diagnostics, then use an existing feature-flag/deployment
   mechanism to disable the affected backend or optional path. Do not invent a
   runtime override or change authorization controls during an incident.
3. Roll back to the last known-good backend/configuration if errors continue
   after device recovery. Confirm resource accounting and a representative
   correctness test before restoring traffic.
4. Escalate when CPU fallback also fails, no fallback path is available,
   accounting remains inconsistent, or cross-tenant exposure is suspected.
   Treat suspected data exposure as a security incident under project
   [SECURITY.md](../../SECURITY.md).

**Important:** `GPUModule::submitWork` currently reports the result of its
fallback callback, which records fallback and returns success without itself
executing the submitted work item on CPU. Do not use `submitted == true` with
`via_gpu == false` as evidence that the request's computation completed
correctly. Confirm the consuming workload's fallback implementation before
replaying or resuming traffic.

Detailed field/config contracts are in [API.md](API.md),
[PRODUCTION_REQUIREMENTS.md](PRODUCTION_REQUIREMENTS.md), and
[`GPU_CONTRACT.md`](../../include/gpu/GPU_CONTRACT.md).
