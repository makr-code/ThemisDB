<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · AUDIT.md · SECURITY.md -->

# Roadmap — API Module Public Headers

**Module Path:** `include/api/`
**Implementation Roadmap:** `../../src/api/ROADMAP.md`

---

## Current Status

Public headers at v1.8.0. GraphQL (query, mutation, subscription, WS, persisted queries),
gRPC, HTTP, WebSocket, rate limiting, audit logging, and OTLP tracing headers are all stable.
15 open findings at implementation level tracked in `../../src/api/AUDIT.md`.

---

## Completed Features

- [x] `IGraphQLEngine` with subscription and persisted query support
- [x] `GraphQLSchemaBuilder` for programmatic schema construction
- [x] `IGraphQLCache` for response-level caching
- [x] `IGRPCServer`, `IGRPCBridge`, `ThemisDBGRPCService`
- [x] `IHTTPHandler`, `IWebSocketHandler`, `IWSHandler`
- [x] `IRateLimiter` with policy enum (token_bucket, sliding_window, fixed_window)
- [x] `IAuditLogger` and `AuditEvent`
- [x] `ITracingMiddleware` and `IOTLPExporter`
- [x] `CorrelationID` and `CorrelationContext`
- [x] `IPersistedQueryStore` for APQ
- [x] `APIVersionRouter` with content-type negotiation

---

## Planned Features

- [x] `ISubscriptionMultiplexer` header for multi-subscription fan-out (Target: Q3 2026)
- [x] `IAPIGatewayHook` for plugin-based API gateway extensions (Target: Q3 2026)
- [ ] Resolve `ws_handler.h` / `websocket_handler.h` overlap (Target: v1.9.0)
- [ ] `ICircuitBreaker` at API boundary for downstream protection (Target: Q4 2026)

---

## Implementation Phases

### Phase 1: Protocol Headers
- [x] GraphQL, gRPC, HTTP, WebSocket interfaces defined

### Phase 2: Cross-Cutting Headers
- [x] Rate limiting, audit logging, correlation ID, tracing, OTLP

### Phase 3: Advanced GraphQL
- [x] Schema builder, cache, metrics, WS handler, persisted queries

### Phase 4: Observability
- [x] OTLP export, tracing middleware, OTLP config

### Phase 5: Hardening
- [ ] Resolve WS handler overlap (v1.9.0)
- [x] `ISubscriptionMultiplexer` (Q3 2026)

### Phase 6: Documentation & Acceptance
- [x] Architecture and audit docs present
- [ ] 15 open implementation findings resolved (tracked in `../../src/api/AUDIT.md`)

---

## Production Readiness Checklist

- [x] All major protocol headers stable (GraphQL, gRPC, HTTP, WS)
- [x] Rate limiting and audit logging headers present
- [x] Tracing and OTLP headers present
- [ ] 15 open findings in `../../src/api/AUDIT.md` resolved
- [ ] `ws_handler.h` / `websocket_handler.h` overlap resolved
