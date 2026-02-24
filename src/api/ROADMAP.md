# API Module Roadmap
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
Core HTTP API server implemented with RESTful endpoints, AQL query execution, authentication, and TLS support.

## Completed ✅
- [x] HTTP server integration (Crow/Beast)
- [x] RESTful document CRUD endpoints
- [x] AQL query execution endpoint
- [x] Graph operation endpoints
- [x] Authentication and authorization middleware
- [x] TLS/SSL support
- [x] Request/response handling pipeline
- [x] API middleware infrastructure

## In Progress 🚧
- [x] GraphQL API layer (Target: Q2 2026) (Issue: #1447)
- [I] OpenAPI 3.x specification completeness (Target: Q2 2026) (Issue: #1491)
- [x] Streaming query result endpoints (SSE/WebSocket) (Target: Q3 2026) (Issue: #1492)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [x] GraphQL schema for multi-model queries (Issue: #1493)
- [I] WebSocket support for real-time change subscriptions (Issue: #1494)
- [x] Rate limiting middleware (Issue: #1495)
- [I] Request tracing and correlation IDs (Issue: #1496)
- [I] Versioned API endpoints (v1, v2 prefix routing) (Issue: #1497)
- [I] Bulk operation endpoints (batch insert, batch delete) (Issue: #1498)

### Long-term (6-12 months)
- [I] gRPC API surface alongside REST (Issue: #1499)
- [I] API gateway integration (Kong, Nginx) (Issue: #1500)
- [I] SDK generation from OpenAPI spec (Python, JavaScript, Go) (Issue: #1501)
- [x] API key management endpoint (Issue: #1502)
- [I] Multi-tenant namespace routing (Issue: #1503)
- [I] Async job API for long-running queries (Issue: #1504)

## Implementation Phases

### Phase 1: Core HTTP API (Status: Completed)
- [x] Integrated Crow/Beast HTTP server with request routing
- [x] Implemented RESTful CRUD endpoints for documents, graphs, and collections
- [x] Implemented AQL query execution endpoint (`api/aql_handler.cpp`)
- [x] Implemented authentication and authorization middleware (`api/auth_middleware.cpp`)
- [x] Added TLS/SSL support with certificate configuration
- [x] Built request/response handling pipeline with error serialization

### Phase 2: GraphQL, WebSocket, and API Hardening (Status: In Progress)
- [x] Implement GraphQL schema and resolver for multi-model queries (`api/graphql_handler.cpp`) (Issue: #1515)
- [x] Implement WebSocket upgrade handler for real-time change subscriptions (`api/ws_handler.cpp`) (Issue: #1516)
- [I] Complete OpenAPI 3.x spec for all existing endpoints (Issue: #1517)
- [x] Add rate limiting middleware with configurable per-client token bucket (Issue: #1518)
- [I] Add request correlation IDs propagated through all log lines (Issue: #1519)

### Phase 3: gRPC, Versioning, and SDK Generation (Status: Planned)
- [I] Implement gRPC surface with proto definitions mirroring REST API (`api/grpc_server.cpp`) (Issue: #1505)
- [I] Add versioned endpoint routing (`/v1/`, `/v2/` prefixes) with deprecation headers (Issue: #1506)
- [I] Generate client SDKs from OpenAPI spec for Python, JavaScript, and Go (Issue: #1507)
- [I] Implement async job API for long-running AQL queries with polling endpoint (Issue: #1508)

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1509)
- [I] Integration tests (Issue: #1510)
- [I] Performance benchmarks (Issue: #1511)
- [I] Security audit (Issue: #1512)
- [I] Documentation complete (Issue: #1513)
- [I] API stability guaranteed (Issue: #1514)

## Known Issues & Limitations
- OpenAPI specification may be incomplete for newer endpoints

## Breaking Changes
- GraphQL schema will be introduced as a new endpoint (non-breaking to REST)
- gRPC surface planned for a future major version
