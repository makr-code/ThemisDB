# Runbook: Model Swap Procedure

**Component:** ThemisDB LLM module
**Severity:** Operational
**Last Updated:** April 2026
**Related Alert:** `LLMNoModelsLoaded`

---

## Overview

This runbook covers the procedure for **swapping a loaded LLM model** with a different model version or quantisation variant. The procedure applies to:

- Upgrading to a newer model checkpoint.
- Switching quantisation format (e.g., Q8_0 → Q4_K_M) to reduce VRAM usage.
- Replacing a misbehaving model with a known-good fallback.
- Decommissioning a model that is no longer needed.

> **Current state (v1.5):** Hot-reload is not yet implemented (`POST /admin/models/reload` is a Q1 deliverable). The current procedure requires a configuration change and a graceful process restart.

---

## Pre-swap Checklist

- [ ] Confirm the new model file exists on disk and is a valid GGUF v3 file.
- [ ] Check the new model's quantisation type is supported (F32, F16, Q4_K_M, Q8_0). See `docs/GGUF_SUPPORT.md` for the support matrix.
- [ ] Verify available VRAM: the new model must fit within `llm_gpu_memory_total_mb - safety_margin_mb`. A 20 % safety margin is recommended.
- [ ] Note the current active session count and queue depth before swapping.
- [ ] If possible, schedule the swap during a low-traffic window.

---

## Procedure (Current — v1.5 without hot-reload)

### Step 1 — Drain in-flight requests

Wait for `llm_concurrent_requests` to drop to 0, or forcibly shed the queue:

```bash
# Watch queue depth and concurrent requests
watch -n 2 'curl -s http://localhost:9091/metrics | grep -E "llm_(concurrent|scheduler_queue)"'
```

If the queue does not drain within the maintenance window, stop accepting new requests at the load balancer before proceeding.

### Step 2 — Update the model configuration

Edit `config/llm.yaml` (or the relevant config file) to point to the new model:

```yaml
llm:
  model_path: /models/llama-3.2-7b-q4_k_m.gguf   # was: llama-3.2-7b-q8_0.gguf
  n_ctx: 4096
  n_batch: 512
```

### Step 3 — Restart the ThemisDB server

```bash
# Systemd
sudo systemctl restart themisdb

# Docker
docker compose restart themisdb

# Kubernetes
kubectl rollout restart deployment/themisdb -n <namespace>
```

### Step 4 — Verify the new model is loaded

```bash
# Once GET /models is implemented (Q1):
curl -s http://localhost:8080/models | jq '.'

# Until then, check the startup log:
journalctl -u themisdb -n 50 | grep -E "(model loaded|LLM init|GGUF)"
```

### Step 5 — Smoke test

Send a single test inference request and verify a sensible response:

```bash
curl -X POST http://localhost:8080/llm/infer \
  -H 'Content-Type: application/json' \
  -d '{"prompt": "Hello, world!", "max_tokens": 20}' \
  | jq '.response'
```

---

## Procedure (Future — v1.6+ with hot-reload)

Once `POST /admin/models/reload` is implemented (Q1 roadmap item), the swap can be performed without a process restart:

```bash
# 1. Update config/llm.yaml with the new model path.
# 2. Trigger hot-reload:
curl -X POST http://localhost:8080/admin/models/reload \
  -H 'Content-Type: application/json' \
  -d '{"drain_timeout_ms": 30000}'

# 3. Monitor reload progress:
curl -s http://localhost:9091/metrics | grep llm_model_load_duration_seconds
```

---

## Rollback

If the new model fails to load or produces degraded output:

1. Revert `config/llm.yaml` to the previous model path.
2. Restart the server (or hot-reload once available).
3. Verify the old model is serving requests correctly.
4. File a bug report with the error logs from `journalctl -u themisdb`.

---

## Supported GGUF Formats

| Format | Status | Notes |
|--------|--------|-------|
| F32 | ✅ Supported | Full precision; high VRAM usage |
| F16 | ✅ Supported | Half precision |
| Q4_K_M | ✅ Supported | Recommended for production |
| Q8_0 | ✅ Supported | High accuracy, higher VRAM |
| Q4_0, Q4_1, Q5_0, Q5_1, Q8_1, Q5_K, Q6_K | ❌ Not supported | Will produce an error (Q1 fix) |

---

## Related Documents

- `docs/GGUF_SUPPORT.md` — Full GGUF format support matrix
- `docs/llm_roadmap.md` — Production-readiness roadmap (Q1: hot-reload API)
- `prometheus/rules/llm_alerts.yml` — `LLMNoModelsLoaded` alert definition
- `src/llm/llama_wrapper.cpp` — Model load/unload implementation
- `src/llm/model_loader.cpp` — Low-level model loading
