# Runbook: LLM Streaming — Wave D Operability

<!-- Runbook: llm_streaming | Wave D | validated: 2026-09-16 -->
<!-- Links: src/llm_streaming/ROADMAP.md · docs/operability/README.md -->

## Overview

This runbook covers operator-critical incident scenarios for the `llm_streaming`
module. Use it to diagnose and remediate stream buffer overflow, token generation
stall, backpressure cascade, mid-stream disconnection, and token ordering failure.

---

## Scenario 1 — Stream Buffer Overflow

**Log pattern:** `[LLM_STREAM:BufferOverflow]`

### Symptoms
- Token buffer fills up faster than the consumer can drain it.
- `[LLM_STREAM:BufferOverflow]` emitted with `stream_id`, `buffer_size`, and `dropped_tokens` fields.
- Callers may receive incomplete responses or backpressure signals.

### Diagnosis
1. Confirm buffer overflow:
   ```
   grep '\[LLM_STREAM:BufferOverflow\]' /var/log/themisdb/llm_streaming.log
   ```
2. Review `dropped_tokens`, `buffer_size`, and `stream_id`.
3. Check token generation throughput vs. client consumption rate.
4. Inspect active stream count — high concurrency may saturate buffer pool.

### Remediation
1. Increase token buffer capacity in configuration (`llm_streaming.token_buffer_capacity`).
2. Apply client-side flow control: ensure clients signal readiness before the server sends.
3. Throttle the LLM inference engine to limit token production rate if clients are slow.
4. Scale out stream processing workers to increase buffer drain throughput.

### Escalation
Escalate to the platform team if buffer overflow is sustained across multiple
streams simultaneously or if token drop rate exceeds 0.1 % of total tokens.

---

## Scenario 2 — Token Generation Stall

**Log pattern:** `[LLM_STREAM:GenerationStall]`

### Symptoms
- Token stream pauses for longer than the configured stall timeout.
- `[LLM_STREAM:GenerationStall]` emitted with `stream_id`, `stall_duration_ms`, and `last_token_seq`.
- Callers experience increased latency or partial response delivery.

### Diagnosis
1. Confirm stall events:
   ```
   grep '\[LLM_STREAM:GenerationStall\]' /var/log/themisdb/llm_streaming.log
   ```
2. Review `stall_duration_ms` and `stream_id`.
3. Check LLM inference engine health and GPU utilisation:
   ```
   nvidia-smi  # or relevant GPU monitoring command
   ```
4. Check for inference queue saturation or OOM on the inference host.

### Remediation
1. If inference engine is stalled due to GPU OOM, reduce concurrent inference requests.
2. Cancel stalled streams and return an appropriate error to callers to free resources.
3. If stall is due to inference engine hang, restart the inference process.
4. Verify stall timeout is configured appropriately (`llm_streaming.generation_stall_timeout_ms`).

### Escalation
Escalate to the ML-Ops team if generation stalls recur on healthy hardware
or if they are correlated with a specific model or prompt pattern.

---

## Scenario 3 — Backpressure Cascade

**Log pattern:** `[LLM_STREAM:BackpressureCascade]`

### Symptoms
- A backpressure event on one stream causes degradation across multiple streams.
- `[LLM_STREAM:BackpressureCascade]` emitted with `triggered_stream_id`, `affected_streams`, and `duration_ms`.
- Throughput drops system-wide; queue depths increase across all active streams.

### Diagnosis
1. Confirm cascade:
   ```
   grep '\[LLM_STREAM:BackpressureCascade\]' /var/log/themisdb/llm_streaming.log
   ```
2. Identify `triggered_stream_id` — this is the source of the cascade.
3. Inspect the triggering stream for slow consumers or disconnected clients.
4. Review backpressure controller configuration (`llm_streaming.backpressure_buffer_capacity`).

### Remediation
1. Isolate and cancel the triggering stream if the consumer is unresponsive.
2. Ensure stream-level backpressure does not share a global lock with other streams.
3. Increase buffer pool size and per-stream isolation to prevent cascade propagation.
4. Monitor recovery: confirm `affected_streams` count returns to 0.

### Escalation
Escalate to the platform team if backpressure cascades affect more than 10 %
of active streams in any 5-minute window.

---

## Scenario 4 — Mid-Stream Disconnection

**Log pattern:** `[LLM_STREAM:Disconnected]`

### Symptoms
- Streaming sessions terminate unexpectedly before the response is complete.
- `[LLM_STREAM:Disconnected]` emitted with `stream_id`, `tokens_delivered`, and `disconnect_reason`.
- Callers receive partial LLM output without a finish reason.

### Diagnosis
1. Confirm disconnection:
   ```
   grep '\[LLM_STREAM:Disconnected\]' /var/log/themisdb/llm_streaming.log
   ```
2. Check `disconnect_reason`: client-side close, server timeout, or network error.
3. Review client reconnection behavior — do clients retry?
4. Check server-side stream lifetime limits (`llm_streaming.max_stream_duration_ms`).

### Remediation
1. For client-side disconnects: ensure clients implement resumable streaming or retry logic.
2. For server timeouts: increase `llm_streaming.max_stream_duration_ms` if long generations are expected.
3. For network errors: work with infrastructure to stabilise the path.
4. Ensure the server cleans up all resources on disconnect to prevent stream leaks.

### Escalation
Escalate to the networking and platform teams if disconnect rate exceeds 0.5 %
of total streams over any 10-minute window.

---

## Scenario 5 — Token Ordering Failure

**Log pattern:** `[LLM_STREAM:TokenOrderViolation]`

### Symptoms
- Tokens are received out of order, producing garbled or semantically incorrect output.
- `[LLM_STREAM:TokenOrderViolation]` emitted with `stream_id`, `expected_seq`, and `received_seq`.
- Callers experience incoherent LLM responses.

### Diagnosis
1. Confirm ordering failure:
   ```
   grep '\[LLM_STREAM:TokenOrderViolation\]' /var/log/themisdb/llm_streaming.log
   ```
2. Record `expected_seq` and `received_seq` — identify the gap.
3. Confirm the streaming protocol assumes single-producer, single-consumer ordering (by design).
4. Check for any load-balancing or multiplexing layer that may be re-ordering packets.

### Remediation
1. Token ordering must be enforced at the stream dispatcher layer.
   Verify `stream_dispatcher.cpp` is correctly serialising token emissions per stream.
2. If a multiplexing proxy is in the path, ensure it preserves per-stream ordering.
3. Reset affected streams and replay from the last confirmed sequence number if the protocol supports it.
4. After fix, confirm `[LLM_STREAM:TokenOrderViolation]` no longer appears for the affected stream.

### Escalation
Escalate to the streaming infrastructure team if ordering violations are
reproducible or affect more than one stream concurrently.

---

## References

- `src/llm_streaming/ROADMAP.md` — Wave D operability contribution
- `include/llm_streaming/` — streaming API contracts (streaming_server.h, token_buffer.h, backpressure_controller.h)
- `tests/integration/test_llm_streaming_soak.cpp` — Wave D soak tests
- `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp` — stress tests
- `benchmarks/llm_streaming/bench_llm_streaming_dedicated_gates.cpp` — benchmark gates
