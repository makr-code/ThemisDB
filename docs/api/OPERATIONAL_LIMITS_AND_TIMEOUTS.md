# ThemisDB API Operational Limits and Timeouts

**Version:** v2.4.0-rc1  
**Last Updated:** 2026-08-03  
**Status:** 🟢 PRODUCTION-READY (GA Phase 6)  
**Document Owner:** Platform Engineering  
**Review Frequency:** Quarterly  

---

## Overview

This document specifies operational limits, timeout thresholds, resource constraints, and failure behavior for ThemisDB's public APIs across critical modules: **server**, **llm**, and **sharding**. These limits are enforced at GA release and govern production behavior.

All values are measured under standard operational conditions (Linux/x86_64, 64 GB RAM, 16 vCPU, local SSD).

---

## 1. Server Module Operational Limits

### 1.1 HTTP/REST API Limits

| Limit | Value | Enforced By | Failure Mode |
|-------|-------|------------|--------------|
| **Max concurrent connections** | 10,000 | TCP listen backlog + SO_REUSEADDR | HTTP 503 Service Unavailable; new clients rejected at accept() |
| **Max request body size** | 256 MB | `HttpServer::Config::max_body_bytes` | HTTP 413 Payload Too Large; connection closed after response |
| **Max URL path length** | 8 KB | HTTP spec RFC 3986 | HTTP 414 URI Too Long; connection kept alive |
| **Max header size per request** | 32 KB | `HttpServer::Config::max_header_bytes` | HTTP 431 Request Header Fields Too Large; connection closed |
| **Total headers per request** | 100 headers | Header parsing loop limit | HTTP 400 Bad Request; connection kept alive |
| **Request timeout (idle)** | 60 seconds | `HttpServer::Config::request_timeout_sec` | HTTP 408 Request Timeout; connection closed after sending response |
| **Response timeout (write)** | 30 seconds | Socket write with select/epoll | HTTP 500 Internal Server Error + async error log + connection closed |
| **Keep-Alive max requests per connection** | 1,000 | `HttpServer::Config::keepalive_max_requests` | Connection closed gracefully after 1000 requests |
| **Keep-Alive timeout (idle between requests)** | 10 seconds | Epoll timeout on idle read | Connection closed silently (no error sent) |

### 1.2 Graceful Shutdown Limits

| Limit | Value | Enforced By | Failure Mode |
|-------|-------|------------|--------------|
| **Graceful shutdown timeout** | 30 seconds | `HttpServer::shutdown()` max_wait | All in-flight requests completed or forcibly terminated; connections closed |
| **Max requests allowed during shutdown** | Unlimited (drain) | `HttpServer::entering_shutdown_state()` | New connections rejected; existing requests drained |
| **Drain time for active requests** | 25 seconds | 30s total - 5s buffer | In-flight requests have up to 25s to complete |
| **Force termination after drain** | 5 seconds | 30s - 25s | Remaining requests terminated with error; connection forcibly closed |

### 1.3 Query API Handler Limits

| Limit | Value | Enforced By | Failure Mode |
|-------|-------|------------|--------------|
| **Max query execution time** | 5 minutes | Query engine timeout gate | Query aborted; Status=TIMEOUT returned to client |
| **Max query result set size** | 100 MB | Result buffer limit | Query aborted; Status=RESULT_TOO_LARGE returned to client |
| **Max concurrent queries per client** | 100 | Per-principal connection state | New query rejected with Status=QUOTA_EXCEEDED |
| **Query plan cache size** | 10,000 plans | LRU eviction | Oldest plan evicted when 10,001st is added |

### 1.4 Rate Limiting Defaults

| Limit | Value | Enforced By | Failure Mode |
|-------|-------|------------|--------------|
| **Global request rate limit** | 100,000 req/sec | `RateLimiterV2` token bucket | HTTP 429 Too Many Requests; clients receive Retry-After header |
| **Per-principal rate limit** | 10,000 req/sec | Per-auth-principal token bucket | HTTP 429 Too Many Requests; Retry-After header specifies delay |
| **Per-IP rate limit** | 5,000 req/sec | Per-source-IP token bucket | HTTP 429 Too Many Requests; Retry-After header specifies delay |
| **Burst allowance** | 1.5× rate limit | Token bucket refill | Exceeding burst → rate limiting kicks in after burst exhausted |

### 1.5 Circuit Breaker (Server → Backend Services)

| Limit | Value | Enforced By | Failure Mode |
|-------|-------|------------|--------------|
| **Failure threshold** | 5 consecutive failures | CircuitBreaker state machine | Circuit opens; new requests fail immediately with Status=SERVICE_UNAVAILABLE (fail-fast) |
| **Recovery timeout** | 30 seconds | State transition timer | After 30s in OPEN state, transitions to HALF_OPEN for probe |
| **Probe request limit** | 3 requests | Half-open state counter | After 3 successful probes, circuit closes and normal traffic resumes |
| **Success threshold for close** | 3 consecutive success | Probe counter | Circuit transitions from HALF_OPEN → CLOSED |

---

## 2. LLM Module Operational Limits

### 2.1 Model Loading and Initialization

| Limit | Value | Enforced By | Failure Mode |
|-------|-------|------------|--------------|
| **Max models loaded concurrently** | 4 | Model manager slot allocator | New model load rejected with Status=RESOURCE_LIMIT; existing models remain loaded |
| **Model load timeout** | 2 minutes | Model initialization timer | Load aborted; Status=INITIALIZATION_TIMEOUT; model remains unloaded |
| **Max model VRAM per model** | 16 GB (Community — 1× T4), 320 GB (Enterprise — 4× A100 80 GB) | GPU memory allocator | Load aborted; Status=OUT_OF_MEMORY; fallback to CPU (if configured) |
| **Model cache size (memory, not VRAM)** | 8 GB | ModelCache LRU eviction | Oldest model evicted from RAM cache when new one exceeds 8 GB |

### 2.2 Inference Request Limits

| Limit | Value | Enforced By | Failure Mode |
|-------|-------|------------|--------------|
| **Max input tokens per request** | 32,000 tokens | Tokenizer + input validator | Request rejected with Status=CONTEXT_LENGTH_EXCEEDED |
| **Max output tokens per request** | 4,000 tokens | Generation early stopping | Generation stopped after 4,000 tokens; partial output returned |
| **Max concurrent inference requests** | 1,000 | Request queue semaphore | Request queued (non-blocking) with priority scheduling |
| **Inference request timeout** | 5 minutes | Per-request timeout gate | Request terminated; Status=INFERENCE_TIMEOUT; partial result (if any) discarded |
| **Max batch size for embeddings** | 512 texts | Batch validator | Request rejected with Status=BATCH_SIZE_EXCEEDED |

### 2.3 Memory Safety & Lifecycle

| Limit | Value | Enforced By | Failure Mode |
|-------|-------|------------|--------------|
| **Model cleanup timeout after unload** | 10 seconds | Resource destructor timeout | Cleanup forced; dangling refs logged as warnings; memory leak avoided |
| **Exception propagation timeout** | 500 ms | Exception handler deadline | Exception handling forced to complete; request failed with Status=INTERNAL_ERROR |
| **Memory allocation failure fallback** | Graceful degrade | RAII + smart pointers | Operation fails with Status=OUT_OF_MEMORY; no undefined behavior |
| **Dangling reference detection (debug)** | Enabled in DEBUG builds | ASAN/UBSan instrumentation | Detected references logged; behavior caught in testing, not production |

### 2.4 Embedding Cache Limits (Phase 2+)

| Limit | Value | Enforced By | Failure Mode |
|-------|-------|------------|--------------|
| **Embedding cache size** | 1 GB | RocksDB column family limit | Oldest embeddings evicted by LRU; cache hit rate may degrade |
| **Cache hit check timeout** | 100 ms | Concurrent RocksDB read | Cache miss (timeout treated as miss); re-embedding triggered |
| **Embedding re-use key** | SHA256(content) | Key hashing | Cache keyed on content hash; identical content hits same cache entry |

---

## 3. Sharding Module Operational Limits

### 3.1 Distributed Transaction Limits

| Limit | Value | Enforced By | Failure Mode |
|-------|-------|------------|--------------|
| **Max shards per transaction** | 100 | Participant list validator | Transaction rejected with Status=TOO_MANY_SHARDS |
| **2PC prepare timeout** | 10 seconds | Phase 1 timer gate | Participants timeout → vote NO; transaction aborted; all participants rollback |
| **2PC commit timeout** | 10 seconds | Phase 2 timer gate | Coordinator timeout → send ABORT to all; participants rollback; transaction undone |
| **Max transaction WAL entries** | 100,000 | WAL segment size limit | WAL rotated to new segment; old segment archived for long-running txns |
| **In-doubt transaction resolution timeout** | 5 minutes | Recovery coordinator timer | Unresolved txns on crashed coordinator auto-committed (Presumed Commit) |
| **Replication lag tolerance** | 30 seconds | Quorum reads gate | Read from replica with lag > 30s rejected; redirected to primary |

### 3.2 Shard Failover & Rebalancing

| Limit | Value | Enforced By | Failure Mode |
|-------|-------|------------|--------------|
| **Failover detection time (MTTD)** | 5 seconds | Gossip/heartbeat failure detector | Failed primary marked down; replica promoted after detection |
| **Failover promotion time (MTTR)** | 10 seconds | Replica promotion + WAL catch-up | New primary takes over; clients redirected (transparent retry) |
| **Cluster rejoin time** | 2 seconds | Gossip protocol + state sync | Rejoined node catches up on log + metadata |
| **Max concurrent rebalancing operations** | 5 | Rebalance scheduler | New rebalance operations queued until < 5 in-flight |
| **Rebalance per-shard timeout** | 60 seconds | Shard move timer | Move aborted; shard remains on source; error logged for operator |
| **Parallel shard move limit** | 10 shards | MoveScheduler::max_parallel_moves | More than 10 moves queued; executed FIFO with 60s timeout each |

### 3.3 WAL & Durability Limits

| Limit | Value | Enforced By | Failure Mode |
|-------|-------|------------|--------------|
| **WAL segment size** | 256 MB | Segment allocator | Segment rotated when size exceeded; new segment created |
| **WAL fsync interval** | 100 ms | Batch commit timer | Commits buffered for up to 100ms before fsync (tunable) |
| **WAL checkpoint interval** | 5 minutes | Periodic checkpoint timer | Snapshot taken; older WAL segments deleted; PITR window shrinks |
| **Max uncommitted transactions per shard** | 10,000 | Transaction queue limit | New transaction rejected with Status=QUEUE_FULL |
| **Orphaned lock cleanup interval** | 2 minutes | Percolator stale-lock detector | Orphaned locks from crashed coordinators cleaned up; transaction auto-aborted |

### 3.4 Network & RPC Limits

| Limit | Value | Enforced By | Failure Mode |
|-------|-------|------------|--------------|
| **Shard RPC timeout (peer-to-peer)** | 5 seconds | gRPC deadline | RPC aborted; peer marked suspect; retry triggered with backoff |
| **Gossip protocol timeout** | 3 seconds | Gossip round timer | Node marked DOWN after 3 missed gossip rounds (9 seconds total) |
| **Connection pool size per shard peer** | 10 | Connection allocator | New connections queued; existing requests use available connections |
| **Max pending RPC requests per peer** | 1,000 | Request queue limit | New RPC rejected with Status=QUEUE_FULL; retry from client expected |

---

## 4. Cross-Cutting Operational Limits

### 4.1 Resource Consumption (Per-Process)

| Limit | Value | Enforced By | Failure Mode |
|-------|-------|------------|--------------|
| **Max process virtual memory** | 1 TB | ulimit -v (OS-level) | Process killed by OS; service restarts via systemd/k8s |
| **Max open file descriptors** | 100,000 | ulimit -n (OS-level) | New connections rejected with "Too many open files"; service degraded |
| **Max thread pool threads** | 256 | ThreadPool size limit | New work items queued; execution delayed until thread available |
| **CPU time quota (cgroup, if enabled)** | Unlimited | CGroup v2 cpu.max | Process throttled when exceeding quota (if configured) |

### 4.2 Authentication & Authorization

| Limit | Value | Enforced By | Failure Mode |
|-------|-------|------------|--------------|
| **JWT token lifetime** | 24 hours | Token expiry claim | Request rejected with Status=UNAUTHORIZED; client must re-authenticate |
| **Auth cache entry lifetime** | 5 minutes | AuthCache TTL | Cache hit expires; re-auth against identity provider |
| **Auth principal max group memberships** | 1,000 | RBAC validator | Groups list truncated; oldest groups evicted; audit logged |
| **Max API permissions per principal** | 10,000 | Permission bitmap size | New permission grants rejected with Status=POLICY_SIZE_EXCEEDED |

### 4.3 Logging & Observability

| Limit | Value | Enforced By | Failure Mode |
|-------|-------|------------|--------------|
| **Log buffer size** | 10 MB | Ring buffer in libspdlog | Oldest log entries overwritten when buffer full |
| **Max structured log fields per entry** | 100 fields | Log entry validator | Extra fields truncated; audit logged |
| **Metrics cardinality limit** | 1,000,000 | Prometheus metrics registry | Excessive cardinality → dropped; alert raised for operator |
| **Trace sampling default** | 1% (configurable) | TracingContext sampler | 99% of requests not traced (reduces overhead); high-error requests traced at higher rate |

---

## 5. Failure Behavior & Error Semantics

### 5.1 Transient vs. Permanent Failures

**Transient Failures (Client Retries Recommended):**
- Status=TIMEOUT (query, inference, RPC)
- Status=SERVICE_UNAVAILABLE (circuit open, all backends down)
- Status=QUEUE_FULL (request queue exceeded)
- HTTP 503 Service Unavailable
- HTTP 429 Too Many Requests (rate limited)
- gRPC UNAVAILABLE, RESOURCE_EXHAUSTED, DEADLINE_EXCEEDED

**Permanent Failures (No Retry, Operator Action Required):**
- Status=INVALID_ARGUMENT (malformed input)
- Status=PERMISSION_DENIED (auth failure)
- Status=NOT_FOUND (resource doesn't exist)
- Status=UNSUPPORTED (feature not enabled)
- HTTP 400 Bad Request
- HTTP 401 Unauthorized
- HTTP 404 Not Found
- gRPC INVALID_ARGUMENT, PERMISSION_DENIED, NOT_FOUND, UNIMPLEMENTED

### 5.2 Retry Behavior Guidelines

| Failure Type | Retry Strategy | Max Attempts | Backoff |
|--------------|---|---|---|
| TIMEOUT (server/llm/sharding) | Exponential backoff | 3 | 100ms × 2^attempt (capped at 10s) |
| SERVICE_UNAVAILABLE (circuit/shutdown) | Exponential backoff | 5 | 500ms × 2^attempt (capped at 30s) |
| QUEUE_FULL (rate limit, transaction queue) | Exponential backoff | 5 | 100ms × 2^attempt (capped at 5s) |
| INVALID_ARGUMENT (malformed input) | No retry | — | — |
| PERMISSION_DENIED (auth failure) | No retry | — | — |
| NOT_FOUND (resource absent) | No retry | — | — |

**Recommended Client Logic:**
```
for attempt in 1..max_attempts:
  try:
    result = send_request()
    return result
  catch TRANSIENT_ERROR:
    if attempt < max_attempts:
      sleep(backoff_delay(attempt))
      continue
    else:
      raise error (all retries exhausted)
  catch PERMANENT_ERROR:
    raise error immediately (no retry)
```

### 5.3 Failure Logging & Observability

| Failure Class | Logging Level | Metrics Emitted | Alert Trigger |
|---|---|---|---|
| Transient (TIMEOUT, QUEUE_FULL) | WARN | counters: {timeout_count, queue_full_count} | Threshold: > 100 errors/min × 5 min window |
| Transient (SERVICE_UNAVAILABLE) | ERROR | gauge: circuit_breaker_state, counter: unavailable_count | Threshold: > 50 errors/min × 5 min window OR circuit OPEN > 2 min |
| Permanent (BAD_ARGUMENT, AUTH) | WARN | counters: {bad_argument_count, auth_failure_count} | Threshold: > 1000 errors/min (attack pattern) |
| Critical (DATA_LOSS, CORRUPTION) | ERROR + ALERT | counters: {data_loss_incidents, corruption_detected} | Immediate escalation (P0 incident) |

---

## 6. Configuration & Tuning

### 6.1 Server Configuration Example

```yaml
server:
  port: 9200
  # HTTP limits
  max_concurrent_connections: 10000
  max_body_bytes: 268435456  # 256 MB
  max_header_bytes: 32768    # 32 KB
  request_timeout_sec: 60
  response_timeout_sec: 30
  keepalive_max_requests: 1000
  keepalive_timeout_sec: 10
  
  # Graceful shutdown
  shutdown_timeout_sec: 30
  
  # Query limits
  max_query_time_sec: 300
  max_result_size_bytes: 104857600  # 100 MB
  
  # Rate limiting
  global_rate_limit_rps: 100000
  per_principal_rate_limit_rps: 10000
  per_ip_rate_limit_rps: 5000
```

### 6.2 LLM Configuration Example

```yaml
llm:
  # Model loading
  max_concurrent_models: 4
  model_load_timeout_sec: 120
  max_model_vram_gb: 24  # Community edition
  model_cache_size_gb: 8
  
  # Inference
  max_input_tokens: 32000
  max_output_tokens: 4000
  max_concurrent_requests: 1000
  inference_timeout_sec: 300
  max_embedding_batch_size: 512
  
  # Embedding cache (Phase 2+)
  embedding_cache_size_gb: 1
  embedding_cache_hit_timeout_ms: 100
```

### 6.3 Sharding Configuration Example

```yaml
sharding:
  # 2PC/3PC
  max_shards_per_transaction: 100
  prepare_timeout_sec: 10
  commit_timeout_sec: 10
  in_doubt_resolution_timeout_sec: 300
  
  # Failover
  heartbeat_interval_ms: 500
  failure_detection_timeout_ms: 5000
  failover_promotion_timeout_sec: 10
  
  # WAL & durability
  wal_segment_size_bytes: 268435456  # 256 MB
  wal_fsync_interval_ms: 100
  wal_checkpoint_interval_sec: 300
  
  # Network
  peer_rpc_timeout_sec: 5
  max_pending_rpcs_per_peer: 1000
  connection_pool_size: 10
```

---

## 7. Monitoring & Alerting

### 7.1 Key Metrics to Monitor

**Server Module:**
- `http_requests_total` — total requests by status code
- `http_request_duration_seconds` — histogram of request latencies (p50, p95, p99)
- `http_active_connections` — current open connections
- `http_connection_errors_total` — connection rejection rate
- `http_rate_limit_exceeded_total` — rate-limited requests

**LLM Module:**
- `llm_model_load_time_seconds` — model load latency
- `llm_inference_duration_seconds` — inference latency (p50, p95, p99)
- `llm_active_models` — currently loaded models count
- `llm_queue_size` — pending inference requests
- `llm_token_usage_total` — total tokens processed

**Sharding Module:**
- `txn_2pc_duration_seconds` — 2PC transaction latency (p50, p95, p99)
- `txn_prepare_failures_total` — prepare phase failures
- `txn_abort_total` — aborted transactions
- `shard_failover_time_seconds` — failover latency
- `wal_write_latency_seconds` — WAL write latency
- `replication_lag_seconds` — replication lag (per replica)

### 7.2 Alert Examples

```yaml
- alert: HighRequestLatency
  expr: histogram_quantile(0.95, http_request_duration_seconds) > 0.2
  for: 5m
  annotations:
    summary: "API P95 latency > 200ms (SLA breach)"
    
- alert: HighErrorRate
  expr: rate(http_requests_total{status=~"5.."}[5m]) > 0.001
  for: 5m
  annotations:
    summary: "Error rate > 0.1% (SLA breach)"
    
- alert: CircuitBreakerOpen
  expr: circuit_breaker_state{circuit="backend_service"} == 1
  for: 2m
  annotations:
    summary: "Circuit breaker open; service degraded"
    
- alert: ReplicationLagHigh
  expr: replication_lag_seconds > 30
  for: 1m
  annotations:
    summary: "Replication lag > 30s; read consistency at risk"
    
- alert: LLMModelLoadFailed
  expr: llm_model_load_failures_total > 0
  for: 1m
  annotations:
    summary: "Model load failure detected; investigate"
```

---

## 8. Known Limitations & Deferrals

| Item | Limitation | Target Version | Workaround |
|------|-----------|---|---|
| System-package RocksDB path (linux-release) | Requires RocksDB development package | v2.4.1 | Use community-release preset or install librocksdb-dev |
| HTTP/3 QUIC support | Not yet enabled | v2.5.0 | Use HTTP/2 (default) or HTTP/1.1 with keep-alive |
| GPU memory overflow handling | Fallback to CPU not yet implemented | v2.4.1 | Monitor GPU VRAM; reduce batch size or model count if approaching limit |
| Automatic query optimization based on workload | Not yet implemented | v2.5.0 | Manually tune index hints and query plans |

---

## 9. Compliance & SLA

**Availability SLA:** 99.99% (4.38 minutes downtime/month)  
**Latency SLA:** P95 ≤ 200 ms, P99 ≤ 500 ms  
**Error Rate SLA:** ≤ 0.1%  
**RTO (Recovery Time Objective):** < 5 minutes (failover + recovery)  
**RPO (Recovery Point Objective):** < 15 minutes (WAL durability + replication lag tolerance)  

These limits are validated by Wave 7/8/9 test suites and pentest evidence (v2.4.0-rc1).

---

## 10. References

- **Server Module Runbook:** `docs/runbooks/server_operations.md`
- **LLM Module Runbook:** `docs/runbooks/llm_operations.md`
- **Sharding Module Runbook:** `docs/runbooks/sharding_operations.md`
- **Operational Compliance Checklist:** `docs/production/CHECKLISTS/operational_compliance.md`
- **Edition Limits Matrix:** `docs/de/deployment/EDITION_LIMITS_MATRIX.md`
- **Authentication & Rate Limiting:** `docs/api/AUTHENTICATION_AND_RATE_LIMITING.md`
- **GA Readiness Checklist:** `FINAL_GA_READINESS_CHECKLIST.md`
- **GA Promotion Sign-Off:** `docs/governance/GA_PROMOTION_SIGN_OFF.md`

---

**Document Status:** ✅ GA PRODUCTION-READY (v2.4.0-rc1)  
**Last Review:** 2026-08-03  
**Next Review:** 2026-11-03 (quarterly)
