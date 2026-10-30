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

# Acceleration incident runbook

## Scope and safety

Use this runbook when an acceleration backend is missing, unhealthy, failing requests, or causing resource pressure. It describes diagnostics available in the module; it does not replace host-driver procedures or the service's incident policy. Do not disable trust checks or force a backend past a failed health/capability check to restore throughput.

## 1. Backend missing or CPU fallback selected

1. Inspect startup logs from `BackendRegistry::initializeRuntime()` and `DeviceManager::logDeviceInfo()`. The device manager logs the count, health marker, backend type, free VRAM, compute version, FP16/BF16 support, and selected device.
2. Check that the backend was compiled into this build and that its runtime/driver can enumerate a healthy device. Compile-time support and runtime availability are separate conditions.
3. Inspect the cached discovery result. `DeviceManager` caches probes for 60 seconds; call `refresh()` when an immediate re-probe is required. AI capability probes have a 120-second cache; `initialize(true)` forces a new probe.
4. Confirm whether the active consumer uses `BackendRegistry` selection or `AiHardwareDispatcher`; their preference and fallback chains differ.
5. If no healthy accelerator is available, retain CPU operation and collect the build, host, driver, and probe diagnostics for the owning backend maintainer.

## 2. Request failure or unexpected output

1. Capture the returned backend identifier and structured `error`/status fields. For AI inference, inspect `AiInferenceResult.success`, `error`, `backend_used`, and `ep_used`.
2. Check request pointer validity, counts, dimensions, shape and precision. Similarity dispatch rejects null query/corpus pointers, zero dimensions/counts, a query element-count mismatch, or zero `top_k`.
3. For an accelerator failure, inspect the logged kernel or backend error. AI vector similarity falls back to CPU on CUDA initialization/kernel failure; the dispatcher logs the failed candidate before trying another.
4. Compare the same input and metric against CPU behavior. If CPU also fails, treat it as an input/contract defect rather than a device recovery issue.
5. Preserve the smallest failing request and host/build details for a regression test.

## 3. Plugin rejected or shader integrity failure

- For plugin rejection, inspect `PluginSecurityAuditor::getEventsForPlugin()` / `getAllEvents()` and the explicit validation error. Verify path canonicalization, file permissions, file size, signature metadata, trusted issuer, and blacklist state. Production builds require signatures; development builds permit unsigned plugins.
- For shader rejection, compare the verifier's `actualHash`, `expectedHash`, and message. In strict mode, a shader without a registered expected hash fails. Do not bypass the verification result; correct the signed/approved manifest or remove the invalid artifact.

## 4. Resource pressure and rollback

For vLLM-sharing incidents, inspect `VLLMResourceManager::getStats()` and `canUseGPU()` results. If the configured utilization policy blocks GPU work, the vector-similarity method uses CPU when that path is compiled. Reduce or disable the affected optional workload only through the owning deployment configuration; the acceleration API does not provide a universal runtime backend-disable switch.

If service stability is threatened, roll back to the last known-good deployment/build profile with its previously validated backend configuration. Verify CPU fallback operation, error rate, and consumer correctness before re-enabling the accelerator. Escalate persistent discovery, driver, signature, or shader-integrity failures to the acceleration/security maintainer with redacted logs and the failing host profile.

## Related operational references

- [Operations and metrics](./OPERATIONS.md)
- [Trust boundaries](./TRUST.md)
- [Architecture and failure domains](./ARCHITECTURE.md)
- [Security policy](./SECURITY.md)

**Exact model backend not verifiable from final document text alone.**
