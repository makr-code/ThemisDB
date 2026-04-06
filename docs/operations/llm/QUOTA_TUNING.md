# Runbook: Quota Tuning Guide

**Component:** ThemisDB LLM module — Scheduler and resource governance
**Severity:** Operational
**Last Updated:** April 2026
**Related Alert:** `LLMQueueDepthHigh`, `LLMQueueDepthCritical`

---

## Overview

This guide explains how to tune the LLM module's throughput limits and queue parameters to balance latency, throughput, and fairness. It also covers the planned per-user and per-model quota system (Q1–Q2 roadmap items) and the steps to configure it once available.

> **Current state (v1.5):** Per-user quotas and backpressure-based rejection are not yet implemented. The parameters listed here relate to the continuous batch scheduler configuration and the maximum queue depth that will be added in Q1.

---

## Understanding the Queue and Batch Pipeline

```text
Incoming requests
       │
       ▼
┌─────────────────────┐
│ ContinuousBatchScheduler │  ← queue_max_depth (Q1)
│  queue: pending reqs │
└────────┬────────────┘
         │ (batch step)
         ▼
┌─────────────────────┐
│  Active Batch       │  ← n_batch (max sequences per step)
│  (llama_decode)     │
└────────┬────────────┘
         │ (tokens)
         ▼
   Response stream
```

Key parameters:

| Parameter | Config key | Current default | Effect |
|-----------|-----------|----------------|--------|
| Context window | `n_ctx` | 4096 | Max tokens per session (KV cache size) |
| Batch size | `n_batch` | 512 | Max tokens processed per decode step |
| Thread count | `n_threads` | 4 | CPU threads for non-GPU layers |
| Max queue depth | `queue_max_depth` | unlimited (Q1) | Requests rejected above this limit |
| Request timeout | `request_timeout_ms` | none (Q1) | Max wall-clock time per request |

---

## Symptoms and Tuning Actions

### Symptom: Queue depth growing (alert `LLMQueueDepthHigh`)

**Cause:** Inference throughput is lower than the arrival rate.

**Actions:**

1. **Increase `n_batch`** — allows more tokens per decode step, increasing throughput at the cost of slightly higher latency per step.

   ```yaml
   llm:
     n_batch: 1024   # was 512
   ```

2. **Increase `n_threads`** — helps on CPU-heavy layers (e.g., embedding, logit processing).

   ```yaml
   llm:
     n_threads: 8   # was 4
   ```

3. **Enable GPU offload** — if not already enabled, move more layers to GPU with `n_gpu_layers`:

   ```yaml
   llm:
     n_gpu_layers: 40   # increase to use more GPU layers
   ```

4. **Reduce context window** — smaller `n_ctx` reduces KV-cache memory, allowing more concurrent sessions.

5. **Set a maximum queue depth** — once implemented (Q1), configure `queue_max_depth` to prevent unbounded memory growth and enable backpressure signalling to callers.

### Symptom: High per-request latency despite low queue depth

**Cause:** Individual requests are using large context windows or generating many tokens.

**Actions:**

1. Set `max_new_tokens` at the API layer to cap output length.
2. Reduce `n_ctx` if large context is not needed.
3. Switch to a smaller/faster model quantisation (Q4_K_M over Q8_0).

### Symptom: All requests hitting timeout (once implemented in Q1)

**Cause:** `request_timeout_ms` is too aggressive for the workload.

**Actions:**

1. Increase `request_timeout_ms` proportionally to the expected `max_new_tokens`.
2. Profile typical request durations: `histogram_quantile(0.99, rate(llm_inference_duration_ms_bucket[5m]))`.
3. Set the timeout to ≥ 2× the p99 duration.

---

## Per-User Quota Configuration (Planned — Q2)

Once per-user quota enforcement is implemented, the configuration will resemble:

```yaml
llm:
  quotas:
    default_user:
      tokens_per_minute: 10000
      requests_per_minute: 60
    power_user:
      tokens_per_minute: 100000
      requests_per_minute: 600
    model_overrides:
      llama-3.2-70b-q4_k_m:
        tokens_per_minute: 5000   # large model has stricter quota
```

Quota violations will be recorded as `llm_scheduler_rejected_total{reason="quota_exceeded"}` and emitted to the audit log.

---

## Capacity Planning

### Estimating throughput

Use the token generation rate metric to estimate sustainable throughput:

```promql
# Steady-state tokens/sec
sum(rate(llm_tokens_generated_total[15m]))
```

For a 7B Q4_K_M model on an A100 (80 GB), expect roughly:

- ~2 000 tokens/sec with a batch of 512 (continuous batching).
- ~150 tokens/sec on CPU (32-core, AVX-512).

### Estimating queue size limits

Set `queue_max_depth` to approximately 2× the number of requests that can be served within the target SLO latency:

```text
queue_max_depth = (token_throughput / avg_tokens_per_request) × target_latency_seconds × 2
```

Example: 2 000 tokens/sec, 200 avg tokens/request, 30 s SLO target:

```text
queue_max_depth = (2000 / 200) × 30 × 2 = 600 requests
```

---

## Related Documents

- `docs/llm_roadmap.md` — Q1: timeouts/quota/backpressure; Q2: per-user quotas
- `prometheus/rules/llm_alerts.yml` — `LLMQueueDepthHigh`, `LLMQueueDepthCritical` alert definitions
- `docs/observability/llm_metrics_schema.md` — `llm_scheduler_queue_length`, `llm_scheduler_rejected_total` metrics
- `src/llm/continuous_batch_scheduler.cpp` — Scheduler implementation
- `src/llm/llama_wrapper.cpp` — Configuration validation
