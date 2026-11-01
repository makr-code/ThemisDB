---
Author: "ThemisDB Contributors"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/server/FUTURE_ENHANCEMENTS.md -->

# Server Module — Public Header Future Enhancements

**Module Path:** `include/server/`
**Canonical implementation enhancements:** [`../../src/server/FUTURE_ENHANCEMENTS.md`](../../src/server/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/server/`. Implementation-level enhancements are in:

→ [`../../src/server/FUTURE_ENHANCEMENTS.md`](../../src/server/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` All API handler headers must be individually includable without pulling in the full server.
- `[x]` `WebSocketSession::active_` must remain `atomic<bool>`; no regression to plain `bool`.
- `[!]` Header guards are inconsistent: 125 of 129 top-level headers use `#pragma once`; `distributed_gateway.h`, `request_coalescing.h`, `smart_routing.h`, and `wasm_handler_registry.h` currently have neither `#pragma once` nor conventional include guards. Verify intended single-inclusion constraints before changing these public headers.
- `[x]` `[[nodiscard]]` on all factory functions and error-returning methods.
- `[x]` gRPC-conditional headers (`THEMIS_ENABLE_GRPC`) must not be unconditionally included.
- `[x]` Middleware headers must expose only the `IMiddleware` interface, not internal pipeline state.

---

## Required Interfaces (Header Contract)

| Interface | Declared In | Consumer | Status |
|-----------|------------|----------|--------|
| `HttpServer::start() / stop()` | `http_server.h` | Server entrypoint | ✅ Stable |
| `WebSocketSession::send() / close()` | `websocket_session.h` | WebSocket upgrade path | ✅ Stable |
| `RPCServiceImpl::dispatch()` | `rpc_service_impl.h` | gRPC dispatch loop | ✅ Stable |
| `MonitoringApiHandler::handleRequest()` | `monitoring_api_handler.h` | /stats / /metrics | ✅ Stable |
| `PolicyEngine::evaluate()` | `policy_engine.h` | All API handlers | ✅ Stable |
| `TokenBucketRateLimiter::tryAcquire()` | `rate_limiter_v2.h` | Middleware pipeline | ✅ Stable |
| `TenantManager::resolveTenant()` | `tenant_manager.h` | Auth middleware | ✅ Stable |

---

## Planned Enhancements

### Short-Term (Q3 2026)

- `graphql_subscription_handler.h` — long-lived WebSocket-based GraphQL subscription sessions; uses `SSEConnectionManager` for fan-out.
- `server_observability.h` — `IServerMetricsSink` unified interface for per-request trace/span/metric emission; decouples handlers from Prometheus specifics.
- Document and expose `rpc/` subdirectory headers (`rpc_service_impl.h` sub-types) in ARCHITECTURE.

### Medium-Term (Q4 2026)

- `connection_draining_coordinator.h` — graceful connection drain during rolling restarts; signals in-flight requests via `shutdown_requested` flag.
- `api_quota_manager.h` — per-tenant API quota enforcement with Redis-backed distributed counters.
- Deprecate `rate_limiter.h` (v1) with `[[deprecated]]` once `rate_limiter_v2.h` reaches full parity.
- `wasm_sandbox_config.h` — resource limits (memory cap, CPU time) for WASM-based handler plugins.

### Long-Term

- Unified handler registration API: `HandlerRegistry::registerHandler<THandler>(route, method)` replacing per-handler `HttpServer::addRoute()` calls.
- Server-side streaming API extension: `IStreamingApiHandler` interface for handlers that return `ResultStream` instead of a single response body.
