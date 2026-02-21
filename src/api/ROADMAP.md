# API Module Roadmap

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
