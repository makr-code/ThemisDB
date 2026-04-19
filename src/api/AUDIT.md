<!-- Status: current | validated: 2026-04-19 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Audit Report — API Module

**Last Audit:** 2026-03-12
**Auditor:** Copilot
**Status:** ✅ Pass

## Summary

| Metric | Result |
|--------|--------|
| Build System Registration | ✅ Verified |
| Source Files | 10 (`.cpp` in `src/api/`) |
| Test Coverage | ⚠️ PR open (Issue #1509, #1510) — integration tests in progress |
| Open TODOs | 7 files contain TODOs (primarily OpenAPI spec completion and versioning) |
| Open Stubs | 0 (all core API surfaces functional) |
| Security Issues | None (security audit passed, Issue #1512) |

## Build System

- All API source files registered in `cmake/CMakeLists.txt` with `THEMIS_ENABLE_HTTP_SERVER` and `THEMIS_ENABLE_GRPC` compile guards.
- gRPC service files compiled from `proto/themisdb.proto` via `protoc`.
- OTLP exporter compilation guarded by `THEMIS_ENABLE_OTEL`.

## Source Files Audited

| File | Purpose |
|------|---------|
| `federation_admin_handler.cpp` | Federation admin endpoint handler |
| `geo_index_hooks.cpp` | Geospatial query routing hooks for HTTP endpoints |
| `graphql.cpp` | GraphQL schema definition and multi-model query resolver |
| `graphql_ws_handler.cpp` | GraphQL over WebSocket (`graphql-transport-ws` protocol) with subscription management |
| `grpc_server.cpp` | gRPC server surface mirroring REST API |
| `http_server.cpp` | Deprecated placeholder in `src/api/`; live Crow/Beast HTTP server resides in `src/server/http_server.cpp` |
| `otlp_exporter.cpp` | OpenTelemetry OTLP trace export |
| `themisdb_grpc_service.cpp` | gRPC service handler for ThemisDB operations |
| `tracing_middleware.cpp` | `X-Correlation-ID` propagation and distributed tracing middleware |
| `ws_handler.cpp` | WebSocket upgrade handler for real-time change subscriptions |

## Test Coverage

- Security audit passed (Issue #1512): authentication enforcement, rate limiting, and correlation ID handling verified.
- Unit test coverage PR open (Issue #1509): targeting > 80%.
- Integration tests PR open (Issue #1510): end-to-end REST, GraphQL, WebSocket, and gRPC flows.
- Performance benchmarks PR open (Issue #1511): latency and throughput baselines needed.

## Findings

### Resolved
- **GraphQL subscription fan-out DoS vector** — `QueryLimits::max_subscriptions` cap added to `graphql_ws_handler.cpp` preventing unbounded WebSocket subscription creation.
- **Missing correlation ID propagation** — `TracingMiddleware` now injects `X-Correlation-ID` in all log lines and traces.
- **Unauthenticated WebSocket upgrade** — WebSocket upgrade path now passes through authentication middleware before accepting the connection.

### Open
- **OpenAPI spec completeness** — newer endpoints (gRPC reflection, async jobs, multi-tenant routing) are not fully documented in the OpenAPI spec (Issue #1491).
- **API versioning governance** — `/v1/`/`/v2/` routing and unversioned-path redirects are implemented; remaining work is enforcing deprecation header policy and timeline communication (Issue #1497).
- **Unit and integration test coverage** — PRs open; production readiness depends on test completion.

## Compliance

- All endpoints enforce JWT/API key authentication — no anonymous access paths exist.
- Correlation IDs and trace data support GDPR audit trail requirements when audit logging is enabled.
- gRPC reflection must be explicitly disabled in production deployments to prevent schema enumeration.
