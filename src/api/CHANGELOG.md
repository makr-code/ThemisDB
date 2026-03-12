<!-- Status: current | validated: 2026-03-12 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — API Module

All notable changes to the API module are documented here.  
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]
- Versioned API endpoint routing (`/v1/`, `/v2/` prefixes with deprecation headers) — Issue #1497
- API key management endpoint — Issue #1502

## [1.7.0] — 2026-03-09
### Added
- GraphQL over WebSocket subscription transport (`graphql-transport-ws` protocol) — `api/graphql_ws_handler.cpp`
- `QueryLimits::max_subscriptions` per-connection cap to prevent GraphQL subscription fan-out DoS
- gRPC surface with proto definitions mirroring REST API (`api/grpc_server.cpp`, `proto/themisdb.proto`)
- Async job API for long-running AQL queries with polling endpoint (Issue #1504)
- Multi-tenant namespace routing for all endpoints (Issue #1503)
- Client SDK generation from OpenAPI spec (Python, JavaScript, Go) (Issue #1501)
- API gateway integration (Kong, Nginx) configuration support (Issue #1500)

### Changed
- OpenTelemetry tracing middleware (`tracing_middleware.cpp`) now propagates `X-Correlation-ID` through all log lines

## [1.6.0] — 2026-02-01
### Added
- GraphQL API layer with full schema for multi-model queries (`api/graphql.cpp`) (Issue #1447)
- Streaming query result endpoints via SSE and WebSocket (`api/ws_handler.cpp`) (Issue #1492)
- WebSocket support for real-time change subscriptions (Issue #1494)
- Rate limiting middleware with configurable per-client token bucket (Issue #1495)
- Request tracing middleware with `X-Correlation-ID` propagation via `TracingMiddleware` (Issue #1496)
- Bulk operation endpoints: batch insert, batch delete (Issue #1498)
- OTLP exporter integration for distributed tracing (`api/otlp_exporter.cpp`)
- gRPC service handler (`api/themisdb_grpc_service.cpp`)

### Fixed
- GraphQL schema validation for multi-model queries now correctly handles graph traversal responses

## [1.0.0] — 2024-01-01
### Added
- HTTP server integration (Crow/Beast) with RESTful document CRUD endpoints
- AQL query execution endpoint
- Graph operation endpoints
- Authentication and authorization middleware
- TLS/SSL support with certificate configuration
- Request/response handling pipeline with error serialization
- Geo index hooks for geospatial query routing (`api/geo_index_hooks.cpp`)
