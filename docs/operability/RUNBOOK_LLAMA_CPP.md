# RUNBOOK: llama_cpp Module — Operator Remediation Guide

**Module:** `llama_cpp`
**Wave:** D (Q1 2027)
**Maintainers:** ThemisDB Platform Team
**Last Updated:** 2026-09-16

---

## Overview

This runbook covers operator-critical incident scenarios for the ThemisDB
`llama_cpp` module. Each scenario includes diagnostic log patterns,
triage steps, and remediation actions.

Log patterns use structured prefixes of the form `[LLAMA:<EVENT>]` and
appear in the application log stream (spdlog, JSON lines format).

---

## Scenario 1 — Model Load Failure (`[LLAMA:ModelLoadFailed]`)

### Symptoms
- Log line: `[LLAMA:ModelLoadFailed] model_path=<path> reason=<reason>`
- LLM inference requests return `503 Service Unavailable`
- `LlamaCppPlugin::initialize()` returns a non-OK status

### Triage
1. Check that the model file exists and is not truncated:
   `ls -lh <model_path> && file <model_path>`
2. Verify GGUF magic bytes: first 4 bytes must be `GGUF` (hex `47 47 55 46`).
3. Check available memory: `free -h`. GGUF models require ≥ model-size RAM.
4. Confirm `THEMIS_LLM_ENABLED=1` is set in the build/runtime config.
5. Check for LoRA adapter integrity issues if a LoRA is configured
   (`importLoRA` validates GGUF magic + 2 GB size bound).

### Remediation
- If model file missing/corrupted: re-download, verify checksum, restart.
- If memory insufficient: scale up node or reduce `llm_context_size`.
- If LoRA adapter invalid: remove the LoRA config and restart in base model mode.
- If ABI mismatch: rebuild with the correct `llama.cpp` revision.

---

## Scenario 2 — Inference Timeout (`[LLAMA:InferenceTimeout]`)

### Symptoms
- Log line: `[LLAMA:InferenceTimeout] request_id=<id> elapsed_ms=<ms>`
- Client receives HTTP 504 or times out waiting for token generation
- TTFT (time-to-first-token) p99 spike in Prometheus: `themis_llm_ttft_p99`

### Triage
1. Check GPU utilization: `nvidia-smi` or `rocm-smi`.
2. Inspect inference queue depth: `themis_llm_queue_depth`.
3. Verify context size is not set too large (large contexts slow TTFT).
4. Check if a batch-generate request is blocking the queue.
5. Confirm `inference_count_` and `error_count_` atomics via debug endpoint.

### Remediation
- If GPU overloaded: reduce `llm_max_parallel_requests`.
- If context too large: reduce `llm_context_size` in config and reload.
- If queue backed up: scale horizontally or increase per-request timeout.
- If cancellation tokens are not propagating: verify `InferenceRequest::cancellation_token` is set.

---

## Scenario 3 — Acceleration Unavailable (`[LLAMA:AccelerationUnavailable]`)

### Symptoms
- Log line: `[LLAMA:AccelerationUnavailable] fallback=cpu device=<dev>`
- Throughput drops to CPU-only levels
- GPU health metrics absent from Prometheus

### Triage
1. Check GPU driver: `nvidia-smi` / `rocm-smi`.
2. Verify CUDA/Metal/Vulkan libraries are linked correctly.
3. Check kernel logs: `dmesg | grep -i gpu`.
4. Confirm `llm_acceleration_backend` config key is set correctly.

### Remediation
- If driver crashed: reload or reboot.
- If library missing: reinstall and restart.
- Module fails closed to CPU path — inference continues, latency degrades.
- Open hardware ticket if GPU is faulty.

---

## Scenario 4 — Context Overflow (`[LLAMA:ContextOverflow]`)

### Symptoms
- Log line: `[LLAMA:ContextOverflow] request_id=<id> tokens=<n> limit=<n>`
- Inference request rejected with `context_overflow` error code
- Client application receives truncated or error response

### Triage
1. Identify the request with the oversized context from the log `request_id`.
2. Check configured `llm_context_size` limit.
3. Determine if RAG assembly is prepending too many retrieved chunks
   (`generateRAG` with large `context` parameter).
4. Check if prompt templates are unexpectedly large.

### Remediation
- Increase `llm_context_size` if hardware allows (requires model reload).
- Implement context truncation in the application layer before submitting.
- For RAG workflows: reduce `rag_max_retrieved_chunks` configuration.
- Return a clear `context_overflow` error to the caller so they can retry with a shorter prompt.

---

## Scenario 5 — General Module Degradation / Policy Gate Rejection

### Symptoms
- Multiple log patterns firing simultaneously
- Policy gate active: `LlamaCppPlugin inference rejected by policy`
- All or most requests rejected with `policy_denied` error code

### Triage
1. Identify the active policy function via `getPolicyFn()`.
2. Check recent policy configuration changes.
3. Verify model digest if `verify_model_digest=true` is set.
4. Check `stream_retry_count_` for excessive streaming retries.

### Remediation
1. Resolve root cause (see relevant scenario above).
2. If policy is misconfigured: update policy via `setPolicyFn()` and hot-reload.
3. If model digest mismatch: re-verify model file and update `expected_model_digest`.
4. Reset error counters via admin API after root cause is resolved.
5. File post-incident report and update this runbook.

---

## Alert Reference

| Alert Name                    | Log Pattern                        | Severity |
|-------------------------------|------------------------------------|----------|
| `LlamaModelLoadFailed`        | `[LLAMA:ModelLoadFailed]`          | Critical |
| `LlamaInferenceTimeout`       | `[LLAMA:InferenceTimeout]`         | High     |
| `LlamaAccelerationUnavailable`| `[LLAMA:AccelerationUnavailable]`  | High     |
| `LlamaContextOverflow`        | `[LLAMA:ContextOverflow]`          | Medium   |

---

## Related Documents

- `src/llama_cpp/ROADMAP.md` — Wave D contribution
- `tests/integration/test_llama_cpp_soak.cpp` — soak test coverage
- `tests/llama_cpp/test_llama_cpp_highcardinality_stress.cpp` — stress coverage
- `docs/operability/WAVE_D_ACCEPTANCE_CHECKLIST.md` — sign-off checklist
