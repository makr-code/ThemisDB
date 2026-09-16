# RUNBOOK: Stable Diffusion Module — Operator Remediation Guide

**Module:** `stable_diffusion`
**Wave:** D (Q1 2027)
**Maintainers:** ThemisDB Platform Team
**Last Updated:** 2026-09-16

---

## Overview

This runbook covers operator-critical incident scenarios for the ThemisDB
`stable_diffusion` module. Each scenario includes diagnostic log patterns,
triage steps, and remediation actions.

Log patterns use structured prefixes of the form `[STABLEDIFF:<EVENT>]` and
appear in the application log stream (spdlog, JSON lines format).

---

## Scenario 1 — Model Load Failure (`[STABLEDIFF:ModelLoadFailed]`)

### Symptoms
- Log line: `[STABLEDIFF:ModelLoadFailed] model_id=<id> reason=<reason>`
- Image generation requests return `503 Service Unavailable`
- `SDPlugin::initialize()` returns a non-OK status

### Triage
1. Check available disk space: `df -h <model_dir>`.
2. Verify the GGUF model file exists and is not truncated:
   `ls -lh <model_path> && file <model_path>`
3. Confirm `THEMIS_ENABLE_STABLE_DIFFUSION=ON` is set in the build / runtime config.
4. Check system memory: `free -h`. Model load requires ≥ model-size × 1.2 RAM.
5. Review `SDCppGenerator` logs for ABI mismatch or library load errors.

### Remediation
- If disk is full: free space and restart the service.
- If model file is corrupted: re-download and verify checksum, then restart.
- If memory is insufficient: scale up the node or reduce concurrency.
- If ABI mismatch: rebuild with the correct `stable-diffusion.cpp` revision.

---

## Scenario 2 — Inference Timeout (`[STABLEDIFF:InferenceTimeout]`)

### Symptoms
- Log line: `[STABLEDIFF:InferenceTimeout] request_id=<id> elapsed_ms=<ms>`
- Client receives HTTP 504 or a timed-out inference response
- p99 latency spike visible in Prometheus (`themis_sd_inference_duration_p99`)

### Triage
1. Check GPU utilization: `nvidia-smi` or equivalent.
2. Inspect queue depth metric: `themis_sd_queue_depth`.
3. Verify no competing workloads are monopolizing the GPU.
4. Check for thread starvation in the inference thread pool.

### Remediation
- If GPU is overloaded: reduce concurrency (`sd_max_parallel_inferences` config key).
- If queue is backed up: scale out horizontally or increase timeout threshold.
- If thread pool is starved: increase `sd_thread_pool_size`.
- Restart the module if the state appears stuck (circuit breaker tripped).

---

## Scenario 3 — Acceleration Unavailable (`[STABLEDIFF:AccelerationUnavailable]`)

### Symptoms
- Log line: `[STABLEDIFF:AccelerationUnavailable] fallback=cpu device=<dev>`
- Throughput drops significantly (CPU fallback is 10–100× slower)
- GPU health check fails in monitoring

### Triage
1. Check GPU driver status: `nvidia-smi` / `rocm-smi`.
2. Verify CUDA/ROCm libraries are correctly linked.
3. Inspect kernel logs for GPU errors: `dmesg | grep -i gpu`.
4. Confirm `THEMIS_SD_ACCELERATION_ENABLED=true` in runtime config.

### Remediation
- If driver crashed: reload driver (`sudo rmmod nvidia && sudo modprobe nvidia`) or reboot.
- If library missing: reinstall CUDA toolkit and restart.
- If fallback is acceptable short-term: the module fails closed to CPU — no data loss.
- File a hardware ticket if the GPU hardware is faulty.

---

## Scenario 4 — Memory Pressure (`[STABLEDIFF:MemoryPressure]`)

### Symptoms
- Log line: `[STABLEDIFF:MemoryPressure] used_bytes=<n> limit_bytes=<n>`
- OOM kill events in kernel log: `dmesg | grep oom`
- Model eviction events logged: `SDPlugin model evicted due to memory pressure`

### Triage
1. Check current memory usage: `free -h` and `cat /proc/meminfo`.
2. Identify which models are loaded: query `SDPlugin::getLoadedModels()`.
3. Check for memory leaks with `valgrind` or `heaptrack` in a dev environment.
4. Verify `sd_model_cache_size_limit_bytes` is configured appropriately.

### Remediation
- Reduce `sd_model_cache_size_limit_bytes` to fit available RAM.
- Evict unused models via the admin API: `POST /admin/sd/evict?model_id=<id>`.
- Scale up node memory if baseline footprint exceeds available RAM.
- If OOM kill occurred: restart service and investigate root cause before re-enabling.

---

## Scenario 5 — General Module Degradation / Circuit Breaker Open

### Symptoms
- Multiple log patterns firing simultaneously
- Circuit breaker open: `SDPlugin state=OPEN`
- All requests rejected with `503`

### Triage
1. Check logs for the root cause event (ModelLoadFailed, InferenceTimeout, etc.).
2. Verify system resources (CPU, GPU, RAM, disk).
3. Check upstream dependencies (model storage, config service).

### Remediation
1. Resolve the root cause (see relevant scenario above).
2. Reset circuit breaker via admin API: `POST /admin/sd/reset`.
3. Gradually restore traffic (shadow mode → canary → full).
4. Validate p95/p99 latency returns to baseline before full traffic restore.
5. File a post-incident report and update this runbook if new patterns emerge.

---

## Alert Reference

| Alert Name                         | Log Pattern                          | Severity |
|------------------------------------|--------------------------------------|----------|
| `SDModelLoadFailed`                | `[STABLEDIFF:ModelLoadFailed]`       | Critical |
| `SDInferenceTimeout`               | `[STABLEDIFF:InferenceTimeout]`      | High     |
| `SDAccelerationUnavailable`        | `[STABLEDIFF:AccelerationUnavailable]` | High   |
| `SDMemoryPressure`                 | `[STABLEDIFF:MemoryPressure]`        | Medium   |

---

## Related Documents

- `src/stable_diffusion/ROADMAP.md` — Wave D contribution
- `tests/integration/test_stable_diffusion_soak.cpp` — soak test coverage
- `tests/stable_diffusion/test_stable_diffusion_highcardinality_stress.cpp` — stress coverage
- `docs/operability/WAVE_D_ACCEPTANCE_CHECKLIST.md` — sign-off checklist
