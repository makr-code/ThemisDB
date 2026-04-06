# Runbook: GPU Out-of-Memory (OOM) Recovery

**Component:** ThemisDB LLM module
**Severity:** Critical
**Last Updated:** April 2026
**Related Alert:** `LLMGPUMemoryCritical`, `LLMGPUMemoryHigh`

---

## Symptoms

- Inference requests fail with errors containing `CUDA out of memory`, `cudaMalloc failed`, or `cudaErrorMemoryAllocation`.
- `llm_gpu_memory_used_mb / llm_gpu_memory_total_mb` metric is ≥ 0.97.
- New inference requests are rejected or stall indefinitely.
- Log lines from `gpu_memory_manager.cpp` or `adaptive_vram_allocator.cpp` showing allocation failures.

---

## Impact

All in-flight and new inference requests fail until VRAM pressure is reduced. The server process itself does **not** crash, but the inference engine becomes non-functional until a model is unloaded or the process is restarted.

---

## Immediate Response (< 5 min)

### Step 1 — Confirm the symptom

```bash
# Check current VRAM usage via Prometheus (once MetricsServer is live)
curl -s http://localhost:9091/metrics | grep llm_gpu_memory
# Example output:
# llm_gpu_memory_used_mb 38500
# llm_gpu_memory_total_mb 40960
```

Or check directly with `nvidia-smi`:

```bash
nvidia-smi --query-gpu=memory.used,memory.total --format=csv,noheader
# Example output: 38500 MiB, 40960 MiB
```

### Step 2 — Identify memory consumers

```bash
# List loaded models and their VRAM usage (once GET /models API is implemented)
curl -s http://localhost:8080/models | jq '.[] | {id, memory_mb, device}'

# Until the API is available, query the Prometheus gauge:
curl -s http://localhost:9091/metrics | grep llm_model_memory_mb
```

### Step 3 — Unload the least-recently-used model

```bash
# Unload a specific model (once hot-reload API is implemented)
curl -X DELETE http://localhost:8080/admin/models/<model_id>

# As a temporary workaround, reduce max_loaded_models in config and hot-reload:
curl -X POST http://localhost:8080/admin/models/reload
```

### Step 4 — Reduce the active batch size

If unloading a model is not possible, reduce the continuous batch size to free KV-cache memory:

1. Edit `config/llm.yaml`: decrease `n_batch` by 50 %.
2. Trigger a hot-reload (once implemented): `POST /admin/models/reload`.
3. Verify VRAM drops below 90 %: `nvidia-smi` or `llm_gpu_memory_used_mb` metric.

---

## Medium-term Remediation (< 1 h)

### Option A — Increase GPU VRAM

- Upgrade to a GPU with more VRAM or add a second GPU (requires multi-GPU config changes in `AdaptiveVRAMAllocator`).

### Option B — Switch to a smaller quantisation

- Replace the loaded model file with a more aggressively quantised variant (e.g., Q4_K_M instead of Q8_0).
- For a 7B model: Q8_0 ≈ 8 GB, Q4_K_M ≈ 4 GB.

```bash
# Convert using llama.cpp quantize tool
./llama-quantize input.gguf output_q4km.gguf Q4_K_M
```

### Option C — Enable CPU fallback

- Set `Config::gpu_fallback_on_oom = true` (planned for Q2).
- The inference engine will offload layers to CPU RAM when VRAM is exhausted, at the cost of higher latency.

### Option D — Enable KV-cache paging

- Verify `Config::use_paged_kv_cache = true` is set; paged KV avoids pre-allocating the full context window.

---

## Prevention

| Measure | Owner | Status |
|---------|-------|--------|
| Alert on > 90 % VRAM: `LLMGPUMemoryHigh` | SRE | ✅ Defined in `prometheus/rules/llm_alerts.yml` |
| Per-request VRAM budget enforcement | LLM team | ❌ Not implemented (Q2) |
| Automatic GPU→CPU fallback on OOM | LLM team | ❌ Not implemented (Q2) |
| `GET /models` admin API for live VRAM reporting | LLM team | ❌ Not implemented (Q1) |

---

## Related Documents

- `docs/llm_roadmap.md` — Production-readiness roadmap
- `prometheus/rules/llm_alerts.yml` — Alerting rules
- `docs/llm/GPU_VRAM_ALLOCATION_GUIDE.md` — VRAM configuration guide
- `docs/llm/GPU_MEMORY_BEST_PRACTICES.md` — GPU memory best practices
- `src/llm/adaptive_vram_allocator.cpp` — Dynamic VRAM allocation source
- `src/llm/gpu_safe_fail.cpp` — GPU safe-fail mechanisms
