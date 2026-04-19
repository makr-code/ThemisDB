# Cross-Shard Inference Debugging — RUNBOOK

**Version:** 1.0.0  
**Module:** LLM+RAID  
**Phase:** 6 (Phase 6 Documentation)  
**Date:** 2026-04-19  

---

## 1. Scope

This runbook covers debugging and operational procedures for cross-shard LLM
inference in ThemisDB.  It applies when requests are routed by
`AdaptiveShardRouter` to a remote shard based on domain specialisation or
load, including speculative decoding from a remote draft shard.

---

## 2. Glossary

| Term | Meaning |
|------|---------|
| Source shard | Shard that receives the original inference request |
| Target shard | Shard selected by `routeByDomain()` for actual inference |
| Draft shard | Shard running the lightweight draft model for speculative decoding |
| KV prefix | Serialised KV cache of a shared system-prompt prefix |
| TTFT | Time-to-first-token |
| CB | Circuit Breaker |

---

## 3. Architecture Overview

```
Client
  │
  ▼
LLMAQLHandler.executeInfer()
  ├── validate prompt
  ├── check CB("infer")               ← throws if OPEN
  ├── parseDomainHint(options)
  │     └── domain_route_resolver_()  ← AdaptiveShardRouter.routeByDomain()
  │           └── selects target_shard_id (highest accuracy_delta, LEAST_LOADED tie-break)
  ├── KVPrefixTransferManager (Phase 5)
  │     └── postBinary(target_shard, "/api/v1/kv-prefix/ingest", kv_state)
  └── LLMPluginManager.generate(request)
        ├── local path: request runs on this node
        └── request.metadata["target_shard_id"] set for remote handoff
```

---

## 4. Common Failure Scenarios

### 4.1 Circuit Breaker Open — inference requests blocked

**Symptom:** `executeInfer()` throws `LLMException(INFERENCE_FAILED, "Circuit breaker is open")`.

**Cause:** The configured `failure_threshold` consecutive failures were recorded
within `failure_window`.

**Diagnosis:**

```bash
# Check circuit breaker state via Prometheus metrics (scraped by Grafana)
curl -s http://<node>:9090/metrics | grep 'llm_circuit_breaker_state'

# Or check the handler state in a running process via admin API:
curl -s http://<node>:8080/api/v1/admin/llm/circuit-breaker-states
```

**Recovery:**

```bash
# Wait for CB timeout (default: 60 s) → transitions to HALF_OPEN automatically.
# Alternatively, if the root cause is fixed, reset via admin API:
curl -X POST http://<node>:8080/api/v1/admin/llm/circuit-breaker/reset?op=infer
```

**Prevention:** Tune `failure_threshold` and `timeout` in `LLMAQLHandler::Config`:

```cpp
cfg.infer_circuit_breaker.failure_threshold = 10;         // default: 5
cfg.infer_circuit_breaker.timeout           = std::chrono::seconds(30); // default: 60 s
```

---

### 4.2 Domain Routing Selects Wrong Shard

**Symptom:** Inference runs on `shard-general` even though `shard-legal` has the
legal LoRA adapter.

**Cause options:**
1. `AdapterCapabilityAnnouncement` for `shard-legal` was not propagated (gossip lag).
2. `accuracy_delta` for `shard-legal` is below `kMinRoutingAccuracyDelta = 0.4`.
3. `domain_hint` option not set in the call.

**Diagnosis:**

```bash
# Check which capabilities are known to the router:
# Emit a test routing request and inspect the routing_decision metadata field.
curl -X POST http://<node>:8080/api/v1/llm/infer \
  -H "Content-Type: application/json" \
  -d '{"prompt":"test","options":{"domain_hint":"legal"}}'
# Response includes: "routing_decision":"ADAPTER_DOMAIN" or "LOCAL_FALLBACK_*"
```

**Check gossip propagation:**

```bash
# Distributed Knowledge service exposes adapter capabilities:
curl -s http://<node>:8080/api/v1/knowledge/adapter-capabilities
```

**Resolution:** Force a re-announcement from the affected shard:

```bash
curl -X POST http://<shard-legal>:8080/api/v1/knowledge/announce-capabilities
```

---

### 4.3 Speculative Decoder — Low Accept Rate

**Symptom:** Grafana shows `llm_speculative_decoder_accept_rate_avg < 0.5` for
extended periods.

**Cause options:**
1. Draft model vocabulary distribution diverges from target model (model drift).
2. `gamma` (draft steps K) is too large — long sequences are more likely to diverge.
3. Mismatch between local and `remote_draft_shard_id` tokeniser versions.

**Diagnosis:**

```bash
# SpeculativeDecoder exposes per-step stats:
curl -s http://<node>:9090/metrics | grep 'llm_speculative_decoder_accept_rate'

# Check if remote draft shard is healthy:
curl -s http://<draft-shard>:8080/api/v1/health
```

**Resolution:**
- Reduce `gamma` from 4 → 2 in `SpeculativeDecoder::Config`.
- Ensure draft shard and target shard run the same base model version.
- If accept rate consistently < 0.3, disable speculative decoding temporarily:
  ```cpp
  cfg.remote_draft_shard_id = ""; // fallback to local draft
  ```

---

### 4.4 KV Prefix Transfer Failing

**Symptom:** `[KVPrefix] Transfer to shard=<id> failed: ...` in spdlog WARN logs.
TTFT savings not observed.

**Cause options:**
1. mTLS cert mismatch between source and target shard.
2. Target shard `/api/v1/kv-prefix/ingest` endpoint not registered.
3. Payload too large for HTTP body limit (default: 64 MB).

**Diagnosis:**

```bash
# Check KV prefix transfer counters:
curl -s http://<node>:9090/metrics | grep 'llm_kv_prefix_transfer'

# Test endpoint reachability:
curl -k --cert /etc/themis/certs/shard.crt \
        --key  /etc/themis/certs/shard.key \
        -X POST https://<target-shard>:8080/api/v1/kv-prefix/ingest \
        -H "Content-Type: application/json" \
        -d '{"kv_state_b64":"","size":0}'
```

**Resolution:**
- Verify TLS certificates are current and mutually trusted.
- Check HTTP body size limit in target shard config: `http.max_body_bytes`.
- KV prefix transfer is best-effort — a failure only means no TTFT savings,
  inference always continues.

---

### 4.5 Batch Fan-Out — Out-of-Order Results

**Symptom:** `executeBatchInfer()` returns results in wrong order or with empty entries.

**Cause:** The fan-out implementation uses `std::async` tasks indexed by `size_t`
position; a task returning an empty string (e.g., due to CB open) leaves a gap.

**Diagnosis:**

```bash
# Check if CB was open for any domain during the batch:
# (See 4.1 above for CB state check)
```

**Code-level investigation:**

```cpp
auto results = handler.executeBatchInfer(requests);
for (size_t i = 0; i < results.size(); ++i) {
    if (results[i].empty()) {
        spdlog::error("BatchInfer: empty result at index {}", i);
    }
}
```

**Resolution:** The ordering guarantee is structural (futures indexed by slot);
empty results are caused by CB or timeout, not ordering bugs.  Fix the
underlying CB issue (§ 4.1) or increase retry count in `RetryPolicy`.

---

## 5. Key Metrics (Grafana Dashboard: `LLM/RAID Distributed Inference`)

| Metric name | Description | Alert threshold |
|-------------|-------------|----------------|
| `llm_circuit_breaker_state{op="infer"}` | CB state (0=CLOSED,1=HALF_OPEN,2=OPEN) | > 0 for > 60 s |
| `llm_routing_decision_total{decision="LOCAL_FALLBACK_*"}` | Fallback routing rate | > 20 % |
| `llm_speculative_decoder_accept_rate_avg` | Average draft token accept rate | < 0.40 |
| `llm_kv_prefix_transfer_success_total` | Successful KV prefix transfers | — |
| `llm_kv_prefix_transfer_attempt_total` | KV prefix transfer attempts | — |
| `llm_batch_infer_latency_p99_ms` | Batch inference p99 latency | > 2000 ms |
| `llm_ttft_ms_p99` | Time-to-first-token p99 | > 500 ms |

---

## 6. Log Reference

| Log level | Pattern | Meaning |
|-----------|---------|---------|
| DEBUG | `[KVPrefix] Skipping transfer ... tokens < threshold` | Prefix too short |
| DEBUG | `[KVPrefix] KV state transferred to shard=...` | Transfer succeeded |
| WARN | `[KVPrefix] Transfer to shard=... failed:` | Transfer failed; inference continues cold |
| DEBUG | `LLM EMBED locality routing selected shard=...` | Embedding routed to data shard |
| ERROR | `Circuit breaker is open` | CB tripped; requests blocked |

---

## 7. Related Documentation

- `docs/de/llm/AI_ECOSYSTEM_SHARDING_ARCHITECTURE.md` — Architecture overview
- `include/llm/kv_prefix_transfer_manager.h` — Phase 5 KV prefix API
- `include/sharding/adaptive_shard_router.h` — Domain routing algorithm
- `include/llm/speculative_decoder.h` — Speculative decoding internals
- `src/llm/ROADMAP.md` — Feature roadmap and completion status

---

## 8. Emergency Procedures

### Disable cross-shard routing entirely

```cpp
// In LLMAQLHandler setup:
handler.setDomainRouteResolver(nullptr);
// All requests will use LOCAL routing regardless of domain_hint.
```

### Force-open a circuit breaker for testing

```cpp
sharding::CircuitBreaker cb;
cb.forceOpen();
// Test fallback behaviour without waiting for natural failure accumulation.
```

### Reset all circuit breakers (maintenance window)

```bash
for op in infer rag embed finetune; do
  curl -X POST http://<node>:8080/api/v1/admin/llm/circuit-breaker/reset?op=$op
done
```
