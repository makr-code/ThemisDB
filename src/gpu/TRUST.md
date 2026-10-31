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

# GPU Trust Boundaries

## Purpose and scope

This page describes the trust assumptions at the GPU module boundary for
maintainers and operators. The existing [SECURITY.md](SECURITY.md) remains the
module threat summary; this page makes caller, tenant, backend, and fallback
responsibilities explicit. It is not a substitute for application
authentication, authorization, or device/driver isolation.

## Trust boundaries

| Boundary | What the module checks | What the caller/operator must establish |
|---|---|---|
| Caller to `GPUModule` | When `POLICY_GATE` is enabled, `GPUPolicy` checks the supplied caller string against explicit capability grants; grants are default-deny. | Authenticate the caller and ensure its ID cannot be chosen or spoofed by an untrusted request. Protect `grantCaller` and `revokeCaller` behind the owning administrative authorization boundary. |
| Tenant to memory accounting | The canonical manager applies a registered non-zero tenant quota when a tenant ID is supplied. | Bind tenant ID to the authenticated principal. Empty tenant IDs intentionally omit tenant-specific accounting; do not use them for tenant-scoped requests. |
| GPU memory/backend | Edition limits, feature gates, and allocation checks constrain module-managed use. | The host driver, device firmware, backend runtime, and physical GPU remain trusted platform components. Module quotas do not isolate hostile kernels or replace process/VM/device isolation. |
| Kernel or backend input | Backend availability and selected validation surfaces are controlled by build/runtime flags. | Do not accept untrusted kernel code or binary artifacts without the applicable validation/sandbox path and deployment review. Never assume a feature flag alone sanitizes code. |
| Diagnostics/admin | Metrics, audit, and admin JSON surfaces expose resource and failure state. | Restrict admin endpoints and diagnostic data to authorized operators; tenant IDs and backend errors may be sensitive operational metadata. |
| Fallback | Safe-fail tracks failures and invokes the configured fallback callback. | Verify that the workload's fallback actually computes equivalent output. The current `GPUModule::submitWork` callback records fallback and returns success without executing the submitted item on CPU. |

## Configuration and fail-closed posture

Do not disable `POLICY_GATE`, quota enforcement, validation, audit, or safe-fail
to recover availability. The facade's policy check is conditional on the
`POLICY_GATE` feature flag; disabling that flag bypasses the caller capability
check at this boundary. Treat flag changes as security-sensitive and require
the same review and rollout controls as code changes.

The module's checks do not prove that the configured caller or tenant identity
is genuine. They enforce controls over the values supplied by the caller.
Authentication and tenant binding must happen upstream, before those values
reach GPU APIs.

## Incident handling

For suspected unauthorized allocation, tenant quota bypass, cross-device data
exposure, or untrusted kernel execution:

1. Stop affected work and preserve audit records, metrics, configuration, and
   relevant backend/runtime logs.
2. Restrict or disable the affected optional GPU path using approved
   deployment controls; do not weaken policy to retain throughput.
3. Notify the service security owner and platform/device owner. Follow the
   project-level [SECURITY.md](../../SECURITY.md) incident process.
4. Resume only after identity binding, quotas, backend integrity, and the
   consuming workload's fallback/correctness have been verified.

Implementation sources: [`gpu_module.cpp`](gpu_module.cpp),
[`policy.cpp`](policy.cpp), [`gpu_memory_manager_edition.cpp`](gpu_memory_manager_edition.cpp),
[`feature_flags.cpp`](feature_flags.cpp), and
[`safe_fail.cpp`](safe_fail.cpp).
