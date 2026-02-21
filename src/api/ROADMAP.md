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
- [ ] GraphQL API layer (Target: Q2 2026)
- [ ] OpenAPI 3.x specification completeness (Target: Q2 2026)
- [ ] Streaming query result endpoints (SSE/WebSocket) (Target: Q3 2026)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [ ] GraphQL schema for multi-model queries
- [ ] WebSocket support for real-time change subscriptions
- [ ] Rate limiting middleware
- [ ] Request tracing and correlation IDs
- [ ] Versioned API endpoints (v1, v2 prefix routing)
- [ ] Bulk operation endpoints (batch insert, batch delete)

### Long-term (6-12 months)
- [ ] gRPC API surface alongside REST
- [ ] API gateway integration (Kong, Nginx)
- [ ] SDK generation from OpenAPI spec (Python, JavaScript, Go)
- [ ] API key management endpoint
- [ ] Multi-tenant namespace routing
- [ ] Async job API for long-running queries

## Implementation Phases

### Phase 1: Core HTTP API (Status: Completed)
- [x] Integrated Crow/Beast HTTP server with request routing
- [x] Implemented RESTful CRUD endpoints for documents, graphs, and collections
- [x] Implemented AQL query execution endpoint (`api/aql_handler.cpp`)
- [x] Implemented authentication and authorization middleware (`api/auth_middleware.cpp`)
- [x] Added TLS/SSL support with certificate configuration
- [x] Built request/response handling pipeline with error serialization

### Phase 2: GraphQL, WebSocket, and API Hardening (Status: In Progress)
- [~] Implement GraphQL schema and resolver for multi-model queries (`api/graphql_handler.cpp`)
- [~] Implement WebSocket upgrade handler for real-time change subscriptions (`api/ws_handler.cpp`)
- [~] Complete OpenAPI 3.x spec for all existing endpoints
- [~] Add rate limiting middleware with configurable per-client token bucket
- [~] Add request correlation IDs propagated through all log lines

### Phase 3: gRPC, Versioning, and SDK Generation (Status: Planned)
- [ ] Implement gRPC surface with proto definitions mirroring REST API (`api/grpc_server.cpp`)
- [ ] Add versioned endpoint routing (`/v1/`, `/v2/` prefixes) with deprecation headers
- [ ] Generate client SDKs from OpenAPI spec for Python, JavaScript, and Go
- [ ] Implement async job API for long-running AQL queries with polling endpoint

## Production Readiness Checklist
- [ ] Unit tests coverage > 80%
- [ ] Integration tests
- [ ] Performance benchmarks
- [ ] Security audit
- [ ] Documentation complete
- [ ] API stability guaranteed

## Known Issues & Limitations
- GraphQL endpoint is not yet implemented
- WebSocket/streaming endpoints are not yet available
- OpenAPI specification may be incomplete for newer endpoints

## Breaking Changes
- GraphQL schema will be introduced as a new endpoint (non-breaking to REST)
- gRPC surface planned for a future major version
