<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

# Roadmap — Server Module (Public Headers)

## Current Status

Public headers at v1.5.0. All core server interfaces are stable and production-ready.  
Implementation: `../../src/server/` at v1.5.0.  
gRPC-Web TypeScript generator: `scripts/gen_grpc_web_ts.py` → `@themisdb/client-grpc-web`
v1.7.0.

---

## Completed

- [x] HTTP/2 and HTTP/3 (QUIC) sessions: `http2_session.h`, `http3_session.h`
- [x] WebSocket, MQTT, PostgreSQL wire-protocol sessions
- [x] API gateway with tenant-aware dispatch: `api_gateway.h`, `distributed_gateway.h`
- [x] Auth middleware: JWT, mTLS, OAuth2, SAML, API key
- [x] Rate limiting stack: `rate_limiter_v2.h`, `adaptive_rate_limiter.h`, `cost_based_rate_limiter.h`
- [x] gRPC and gRPC-Web proxy: `rpc_service_impl.h`, `grpc_web_proxy_handler.h`
- [x] LLM / LoRA / Voice / Prompt engineering API handlers
- [x] PITR, WAL streaming, replication topology handlers
- [x] Policy engine with OPA and Ranger adapters
- [x] WASM handler registry and serverless function API
- [x] Model Context Protocol server: `mcp_server.h`

---

## In Progress

- [~] HTTP/3 datagram API hardening (`http3_datagram.h`) — production stability work (Target: Q2 2026)
- [~] Smart routing ML model integration (`smart_routing.h`) — feedback loop API (Target: Q2 2026)

---

## Planned Features

- [ ] `websocket_multiplexer.h` — logical channel multiplexing over a single WebSocket
  connection (Target: Q3 2026)
  - Inputs: channel ID + framed message
  - Outputs: per-channel delivery guarantee
  - Tests: unit + load test at 10k concurrent channels
- [ ] `graphql_subscription_handler.h` — WebSocket-backed GraphQL subscription interface
  (Target: Q3 2026)
  - Requires `websocket_session.h` upgrade
  - Perf: < 5 ms fanout latency for 1k subscribers
- [ ] `grpc_web_ts_v2.h` — typed TypeScript v2 client generation contract for
  `@themisdb/client-grpc-web` v2.x (Target: Q4 2026)
  - Breaking change from v1.7.0 generator; parallel v1/v2 support required during transition
- [ ] `http3_push_handler.h` — HTTP/3 server push interface (Target: Q4 2026)

---

## Implementation Phases

### Phase 1: Design / API Contract ✅
- All core session, gateway, and handler interfaces defined

### Phase 2: Core Implementation ✅
- 116 public headers; full protocol stack covered

### Phase 3: Error Handling & Edge Cases ✅
- All middleware and handlers return typed error responses; no raw status codes in headers

### Phase 4: Tests ✅
- Full test coverage via `../../src/server/tests/`

### Phase 5: Performance / Hardening ✅
- HTTP/3 production config, adaptive rate limiting, load shedding

### Phase 6: Documentation & Sign-off ✅
- This document

### Phase 7: WorkloadFingerprintEngine — IMPL-B8 (Target: Q3 2026)

> *Paper 2 — Layer 8: Multi-Tenant Workload Fingerprinting*
> Issue: [docs/issues/optimization_layers/IMPL-B8-workload-fingerprint.md](../../docs/issues/optimization_layers/IMPL-B8-workload-fingerprint.md)

- [ ] New header: `include/server/workload_fingerprint_engine.h`
- [ ] `WorkloadFingerprintEngine::computeFingerprint(const TenantWorkloadWindow&)` → `WorkloadFingerprint`
- [ ] `WorkloadPattern` enum: `OLTP_WRITE_HEAVY`, `OLTP_READ_HEAVY`, `ANALYTICAL`, `VECTOR_SEARCH`, `MIXED`, `BURST_INGEST`, `UNKNOWN`
- [ ] `WorkloadFingerprint { tenant_id, dominant_pattern, confidence, qps_p50, qps_p99, read_write_ratio, vector_fraction, fingerprint_hash }`
- [ ] Pattern matching against registered baseline fingerprints — Jaccard distance on feature vector
- [ ] `fingerprintHash()` — 64-bit deterministic hash for cross-shard comparison (Layer 11D)
- [ ] Writes `DecisionRecord` to `AIDecisionAuditor` when dominant_pattern changes
- [ ] GDPR guard: `TenantWorkloadWindow` must not contain query content, only metrics
- [ ] Performance target: fingerprint computation ≤ 1 ms for 1 000-query window
- [ ] 8 unit tests `WFE-01` … `WFE-08` in `tests/test_workload_fingerprint_engine.cpp`:
  - `WFE-01` write-heavy window → `OLTP_WRITE_HEAVY` pattern
  - `WFE-02` vector search fraction ≥ 0.7 → `VECTOR_SEARCH` pattern
  - `WFE-03` mixed window → `MIXED` with confidence < 0.8
  - `WFE-04` `fingerprintHash()` is deterministic for same input
  - `WFE-05` `DecisionRecord` written on pattern change
  - `WFE-06` no query content in fingerprint (GDPR)
  - `WFE-07` computation ≤ 1 ms for 1 000-query window
  - `WFE-08` cross-shard Jaccard distance: identical workloads → distance 0.0

---

## Production Readiness Checklist

- [x] All headers compile with `-Wall -Wextra -Wpedantic`
- [x] No implementation code in public headers
- [x] All public interfaces documented with Doxygen comments
- [x] `rate_limiter.h` deprecation notice in place; removal tracked for v2.0
- [x] gRPC-Web TypeScript client generation verified at `@themisdb/client-grpc-web` v1.7.0
- [x] Tenant isolation enforced at gateway layer before handler dispatch
- [x] No circular include dependencies

---

## Known Issues & Limitations

- `rate_limiter.h` (`IRateLimiter`) is deprecated; retained for ABI compatibility until v2.0.
- `http3_datagram.h` is production-ready but QUIC path MTU discovery is driver-dependent.
- `smart_routing.h` ML routing policy requires a trained model artefact at runtime; no
  fallback to static routing if model is absent — planned fix in Q2 2026.

---

## Breaking Changes

### v2.0.0 (planned Q1 2027)
- Remove `rate_limiter.h` (`IRateLimiter`); all consumers must migrate to `rate_limiter_v2.h`.
- `grpc_web_proxy_handler.h` TypeScript generator contract will follow `@themisdb/client-grpc-web`
  v2.x schema.
