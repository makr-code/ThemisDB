---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# ONNX CLIP Runbook

Exact model backend not verifiable from final document text alone.

## Scope and safety

This runbook covers diagnosis of the `ONNXClipPlugin` reference implementation.
It does not operate or recover an ONNX inference session because the current
source does not construct one. Follow the owning service's incident process for
request routing, collection access, and persistence; this plugin has no
caller-level authentication or rate limiter.

## Initial checks

1. Call `isReady()` and `healthCheck()`. Health means initialized with a
   positive dimension; it does not validate a model artifact or provider.
2. Read `getStatistics()` and record `ready`, `backend`, `model_name`,
   `embedding_dim`, `max_batch_size`, totals, errors, and `avg_latency_ms`.
3. Preserve the exact request/configuration and confirm the caller checked
   `EmbeddingResult.success`.
4. Determine whether the report concerns a failed call, a configuration/hash
   failure, or an unexpected semantic result. The latter is not a runtime
   defect in the current reference algorithm: vectors are hash/PRNG outputs.

The plugin does not emit its own logs, traces, Prometheus registration, or
provider health metrics. Collect service and host diagnostics through the
caller that owns the plugin.

## Symptoms and recovery

| Symptom | Diagnosis | Safe action |
|---|---|---|
| `ONNXClipPlugin not initialized` | A request arrived before successful initialization or after shutdown. | Stop/reject embedding traffic; inspect `initialize()` result/config; initialize before reopening traffic. |
| `Image data is empty` / `Text input is empty` | Empty payload rejected by source. | Correct the caller payload; do not retry an unchanged request. |
| `initialize()` returns false with a digest configured | The file could not be hashed or digest did not match; on non-OpenSSL builds, check whether a hash callback is installed and what it returned. | Keep the plugin out of service; verify the artifact through the trusted deployment pipeline, path permissions, expected digest, and OpenSSL/callback configuration. Do not disable digest checking just to restore availability. |
| `enable_mmap_loading` has no visible effect | Mapping is best-effort, and the mapped bytes are not used by inference. Mapping failure is silent. | Treat it as a resource-mapping option only; do not use it as evidence that a model loaded or as a performance lever. |
| `reloadModel()` returns false | Could be a pre-publication validation/hash failure, not-ready state, or drain timeout. | Read current `model_name`, `ready`, and configuration state. A timeout occurs after publication, so the new config may already be active. |
| Embeddings are stable but search quality is poor | Current vectors are deterministic synthetic references, not learned CLIP vectors. | Do not tune retrieval thresholds against them. Route to a model-backed implementation/evaluation decision. |
| Batch request uses unexpectedly high memory | Chunk size is not a total-request bound; output storage is proportional to input count × dimension. | Enforce request count/byte limits in the caller and reduce configured dimension/batch request size as appropriate. |

## Reload and rollback procedure

1. Save the active configuration before requesting reload.
2. Record `model_name`, dimension, backend, and statistics before the call.
3. Call `reloadModel()` and record its bool result.
4. If it returns `true`, verify readiness and the expected new label/dimension.
5. If it returns `false`, do not infer that the old state is active. The new
   implementation is published before the 30-second old-request drain. Inspect
   `getStatistics()`; if the new state is active and rollback is needed, issue
   another reload using the saved configuration, then verify the resulting
   label/dimension and service health.

If calls remain blocked or readiness does not match expected state, stop new
traffic at the caller boundary and escalate with the configuration, return
value, statistics snapshot, and host/service logs. There is no plugin-level
drain-timeout metric or logging signal.

## Escalation evidence

Provide maintainers:

- exact plugin config with secrets removed;
- backend enum or reload `backend` string;
- result error text and whether the call was single, batch, or text;
- before/after `getStatistics()` output and timestamps;
- model path/digest status (never send model contents or credentials);
- focused test command and platform/build configuration.

Use the owning service's on-call and security escalation paths if a model file
was modified unexpectedly or derived embeddings may have been exposed.
