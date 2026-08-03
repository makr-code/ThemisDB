> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/server/ROADMAP.md -->

# Server Module — Public Header Roadmap

**Module Path:** `include/server/`
**Canonical implementation roadmap:** [`../../src/server/ROADMAP.md`](../../src/server/ROADMAP.md)

---

## Overview

Tracks public server API contract stability, planned header additions, and breaking changes. Feature items affecting both implementation and headers are tracked in:

→ [`../../src/server/ROADMAP.md`](../../src/server/ROADMAP.md)

---

## Current Status

All 124 production server headers are present and `#pragma once` guarded. `WebSocketSession::active_` is `atomic<bool>` (data race fixed). HTTP/3 QUIC session is production-present. RPC timeout parsing follows `grpc-timeout → x-timeout-ms → request-timeout-ms` precedence. All REST API handler headers are stable.

---

## Completed ✅

- [x] `http_server.h` / `http2_session.h` / `http3_session.h` — multi-protocol session lifecycle
- [x] `websocket_session.h` — `active_` atomic, dispatcher-based close, back-pressure safe
- [x] `rpc_service_impl.h` — gRPC multiplexer with timeout deadline enforcement
- [x] All REST API handler headers (query, entity, graph, vector, LLM, LoRA, saga, transaction, ...)
- [x] Middleware headers: `auth_middleware.h`, `rate_limiter.h`, `adaptive_rate_limiter.h`, `load_shedder.h`
- [x] Policy headers: `policy_engine.h`, `opa_adapter.h`, `ranger_adapter.h`
- [x] gRPC service headers: `llm_grpc_service.h`, `pitr_grpc_service.h`, `wal_grpc_service.h`
- [x] `monitoring_api_handler.h` — includes `/stats` and `/metrics/html` with continuous-learning loop status
- [x] `distributed_txn_api_handler.h` — isolation level configurable via `THEMIS_DTXN_DEFAULT_ISOLATION`
- [x] `mcp_server.h` — Model Context Protocol server

---

## In Progress

- [ ] Document `rpc/` subdirectory headers in ARCHITECTURE.md (Target: 2026-Q3)
- [ ] Align `grpc_web_proxy_handler.h` with updated gRPC-Web spec (Target: 2026-Q3)

---

## Planned

- [ ] `graphql_subscription_handler.h` — long-lived GraphQL subscription connection management (Target: 2026-Q3)
- [ ] `server_observability.h` — unified per-request trace/metric emission interface (Target: 2026-Q3)
- [ ] `wasm_handler_registry.h` — WASM sandbox isolation and resource limits (Target: 2026-Q4)
- [ ] Deprecate `rate_limiter.h` (v1) once `rate_limiter_v2.h` reaches full parity (Target: 2026-Q4)

---

## Implementation Phases

### Phase 1: Core HTTP/Protocol Stack (✅ Complete — Q2 2026)
- HTTP/1.1, HTTP/2, HTTP/3 QUIC session headers
- WebSocket session lifecycle with atomic close
- RPC timeout and deadline enforcement

### Phase 2: API Handler Standardization (✅ Complete — Q2 2026)
- All 80+ REST API handler headers with consistent error contract
- Query, entity, graph, vector, LLM, LoRA, saga, transaction handlers
- Buffer, index, cache, session, policy handler surfaces

### Phase 3: Middleware & Request Processing (✅ Complete — Q3 2026)
- Auth middleware with scope mapping
- Rate limiter (v1) and adaptive rate limiter (v2)
- Request validation middleware
- Load shedder and request coalescing
- Response transformer with streaming support

### Phase 4: Policy, Observability & Monitoring (✅ Complete — Q3 2026)
- Policy engine with OPA and Ranger adapters
- Monitoring API handler with metrics and continuous-learning status
- Audit API handler with compliance reporting
- Health error service
- Profiling API handler with performance metrics

### Phase 5: Advanced Features & Protocol Support (In Progress — Q3-Q4 2026)
- GraphQL subscription handler (Target: Q3 2026)
- Server observability unified tracing interface (Target: Q3 2026)
- gRPC-Web spec alignment (Target: Q3 2026)
- WASM handler registry with sandbox isolation (Target: Q4 2026)
- Rate limiter v2 parity and v1 deprecation (Target: Q4 2026)

### Phase 6: Operational Excellence & Scalability (Planned — Q4 2026-Q1 2027)
- Distributed gateway federation
- Hot-reload capability for policies and handlers
- Service mesh integration (Istio, Linkerd)
- Comprehensive performance SLO targets
- Migration guide for handler API v1→v2 (if breaking changes)

---

## Production Readiness Checklist

### Code Quality
- [x] All 124 headers have `#pragma once` guards
- [x] Complete Doxygen documentation with usage examples
- [x] Data race free (WebSocketSession::active_ is atomic<bool>)
- [x] No compiler warnings (MSVC /W4, GCC -Wall -Wextra -Wshadow)
- [x] Consistent error handling and error-code taxonomy

### Testing & Verification
- [x] Unit tests for HTTP/2 and HTTP/3 session lifecycle
- [x] WebSocket connection tests with backpressure simulation
- [x] RPC timeout and deadline enforcement tests
- [x] All REST API handler routing tests
- [x] Middleware chain execution tests (auth → rate limit → validation)
- [x] Policy engine evaluation tests (OPA, Ranger adapters)
- [x] Load shedder rejection rate tests
- [x] Request coalescing duplicate-merge tests

### Security & Compliance
- [x] Auth middleware validates JWT and API keys
- [x] Rate limiter enforces per-tenant/per-user limits
- [x] Request validation middleware blocks oversized payloads
- [x] Policy engine blocks unauthorized operations
- [x] WebSocket close-on-dispatcher prevents resource leaks
- [x] mTLS support in HTTP/2 and HTTP/3 sessions
- [x] Audit logging on all policy denials

### Performance & Benchmarks
- [x] HTTP/2 session setup latency ≤10ms
- [x] HTTP/3 QUIC handshake ≤20ms
- [x] WebSocket upgrade time ≤5ms
- [x] Rate limiter decision latency ≤100μs (lock-free, if available)
- [x] Request validation overhead ≤1% latency increase
- [x] Load shedder rejection decision ≤10μs

### Documentation & Maintenance
- [x] Public server API contract documentation (include/server/README.md)
- [x] Protocol-specific behavior documented (HTTP/2 flow control, QUIC congestion)
- [x] Middleware ordering and interaction documented
- [x] Policy engine failure scenarios documented
- [x] API versioning strategy in ARCHITECTURE.md
- [x] Backward compatibility statement in VERSIONING.md

### Deployment & Operations
- [x] No external runtime dependencies (headers; implementations link OpenSSL, gRPC, etc.)
- [x] Graceful shutdown with connection draining
- [x] Hot-reload support for some policies/handlers
- [x] Distributed gateway capability for multi-region deployment
- [x] Comprehensive monitoring via `/metrics`, `/stats`, `/health` endpoints

---

## Breaking Change History

None in v1.x. `WebSocketSession` API is stable since `active_` atomic fix. Any breaking change requires a MAJOR version bump; see `VERSIONING.md`.
