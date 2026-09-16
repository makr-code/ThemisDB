# Runbook: ONNX CLIP Module

<!-- Status: current | validated: 2026-09-16 | Wave D operability deliverable -->

## Purpose

Operator remediation guide for the ThemisDB `onnx_clip` module. Covers the
five most critical incident classes, their log patterns, triage steps, and
recommended remediation actions.

---

## Scenario 1 — Model Load Failure

**Log pattern:** `[ONNX_CLIP:ModelLoadFailed]`

**Symptoms**
- ONNX CLIP model fails to initialize on startup or hot-swap.
- Log lines contain `[ONNX_CLIP:ModelLoadFailed]` with model path and error code.
- All image and text embedding requests fail immediately.

**Triage**
1. Verify the model file exists and is readable: `ls -lh <model_path>`.
2. Check the SHA-256 integrity hash matches the expected value in the config.
3. Verify that the configured backend (CUDA/TensorRT/CPU) is available.
4. Check available disk space and memory for the model load operation.

**Remediation**
1. Re-download or restore the model file from the model registry.
2. Validate the model file integrity: `themis_admin onnx_clip model verify`.
3. Switch to a lower-resource backend if CUDA/TensorRT load fails:
   set `onnx_clip.backend: cpu` in the runtime config.
4. Restart the ONNX CLIP plugin: `themis_admin plugin restart onnx_clip`.

**Escalation**
If model load fails after restoring the file and switching backends, escalate
to the ML platform team with the full model load error and system memory/GPU
status.

---

## Scenario 2 — Inference Timeout

**Log pattern:** `[ONNX_CLIP:InferenceTimeout]`

**Symptoms**
- Image or text embedding inference requests time out.
- Log lines contain `[ONNX_CLIP:InferenceTimeout]` with request ID and elapsed time.
- Embedding service latency p99 exceeds SLO.

**Triage**
1. Check GPU/CPU utilisation at the time of the timeout.
2. Inspect the current batch queue depth: `themis_admin onnx_clip batch status`.
3. Verify that the inference timeout threshold is appropriate for the workload.
4. Look for concurrent workloads competing for GPU resources.

**Remediation**
1. Increase the inference timeout: set `onnx_clip.inference_timeout_ms`.
2. Reduce the maximum batch size to decrease per-batch latency: set `onnx_clip.max_batch_size`.
3. Scale to additional inference workers if GPU capacity is saturated.
4. Restart the plugin if the inference queue is deadlocked: `themis_admin plugin restart onnx_clip`.

**Escalation**
Persistent inference timeouts after tuning indicate a hardware or driver
issue — escalate to the infrastructure team with GPU utilisation traces and
the p99 latency histogram.

---

## Scenario 3 — Embedding Corruption

**Log pattern:** `[ONNX_CLIP:EmbeddingCorruption]`

**Symptoms**
- Generated embeddings fail shape or value validation.
- Log lines contain `[ONNX_CLIP:EmbeddingCorruption]` with embedding ID and validation error.
- Downstream similarity search returns anomalous results.

**Triage**
1. Identify the failing embedding ID and request type (image vs. text).
2. Check the ONNX model version against the expected deployment config.
3. Verify output tensor shapes match the configured expected dimensions.
4. Review recent model hot-swap events for partial update failures.

**Remediation**
1. Force a model reload to clear any partially updated state:
   `themis_admin onnx_clip model reload --force`.
2. Run the built-in health check: `themis_admin onnx_clip health`.
3. If the health check fails, roll back to the previous model version.
4. Invalidate any cached embeddings generated during the corruption window.

**Escalation**
Embedding corruption that persists after model reload indicates a model
integrity issue — escalate to the ML platform team with the model hash,
output tensor dumps, and the health check output.

---

## Scenario 4 — Runtime Error

**Log pattern:** `[ONNX_CLIP:RuntimeError]`

**Symptoms**
- The ONNX Runtime reports an unhandled exception during inference.
- Log lines contain `[ONNX_CLIP:RuntimeError]` with runtime error message.
- The plugin enters a degraded state; new requests may be rejected.

**Triage**
1. Read the full runtime error message from the log.
2. Check the ONNX Runtime version compatibility with the installed model.
3. Verify CUDA driver and ONNX Runtime library versions are compatible.
4. Check for out-of-memory conditions on the GPU.

**Remediation**
1. Restart the ONNX CLIP plugin to reset the runtime state:
   `themis_admin plugin restart onnx_clip`.
2. If the error is memory-related, reduce `onnx_clip.max_batch_size`.
3. Update or pin the ONNX Runtime library version if compatibility is the root cause.
4. Switch to CPU backend as a temporary measure: set `onnx_clip.backend: cpu`.

**Escalation**
Recurring runtime errors after restart require investigation by the ML
infrastructure team — provide the ONNX Runtime version, model version, GPU
driver version, and the runtime error stack trace.

---

## Scenario 5 — Model Hot-Swap Failure

**Log pattern:** `[ONNX_CLIP:ModelLoadFailed]` during hot-swap

**Symptoms**
- A model hot-swap operation fails mid-flight.
- The plugin is left in an indeterminate state — either old or new model is active.
- Log lines contain `[ONNX_CLIP:ModelLoadFailed]` with `hot_swap` context tag.

**Triage**
1. Determine which model version is currently active:
   `themis_admin onnx_clip model status`.
2. Check whether the new model file was fully transferred before the swap attempt.
3. Review the hot-swap log entries for the specific failure stage (load, verify, swap).
4. Inspect memory and GPU state for any leftover resource locks.

**Remediation**
1. Force a clean reload of the last verified model:
   `themis_admin onnx_clip model reload --version <stable_version>`.
2. Verify the active model is the expected version: `themis_admin onnx_clip health`.
3. Fix the broken model artifact before retrying the hot-swap.
4. Use a maintenance window for the next hot-swap if the model is large.

**Escalation**
If the plugin cannot recover to a stable model after a failed hot-swap,
escalate immediately — the embedding service is in a degraded state and
requires manual intervention.

---

## Reference

| Log Pattern                      | Severity | SLO Impact | Owner     |
|----------------------------------|----------|------------|-----------|
| `[ONNX_CLIP:ModelLoadFailed]`    | Critical | Yes        | onnx_clip |
| `[ONNX_CLIP:InferenceTimeout]`   | High     | Yes        | onnx_clip |
| `[ONNX_CLIP:EmbeddingCorruption]`| High     | Yes        | onnx_clip |
| `[ONNX_CLIP:RuntimeError]`       | High     | Yes        | onnx_clip |

---

*Wave D operability deliverable — see `src/onnx_clip/ROADMAP.md`.*
