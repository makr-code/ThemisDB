<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# Server Module Roadmap

## Current Status
v1.x – Production-ready API surface built on Boost.Beast/Asio. HTTP/1.1, HTTP/2, HTTP/3, WebSocket, MQTT, PostgreSQL wire protocol, gRPC, and MCP server are implemented with 40+ specialized REST endpoints.

## Completed ✅
- [x] HTTPServer – multi-protocol async I/O server (HTTP/1.1, HTTP/2, HTTP/3)
- [x] TLS 1.3 with modern cipher suites
- [x] 40+ specialized REST API handlers
- [x] WebSocket support for real-time notifications and changefeeds
- [x] MQTT broker integration for IoT use cases
- [x] PostgreSQL wire protocol for SQL client compatibility
- [x] gRPC services for high-performance RPC
- [x] API Gateway (routing, versioning, load balancing)
- [x] JWT, Kerberos, API token, and USB admin authentication
- [x] Rate limiting (token bucket, sliding window, distributed)
- [x] Load shedding and circuit breaking
- [x] Server-Sent Events (SSE) for changefeeds
- [x] Multi-tenancy with tenant isolation
- [x] Apache Ranger policy enforcement integration
- [x] Response compression (Gzip, Brotli, Zstd)
- [x] Model Context Protocol (MCP) server for AI integrations
- [x] Graceful shutdown and connection draining
- [x] Throughput: 50K–200K req/sec; p50 < 5 ms, p99 < 50 ms

## In Progress 🚧
- [I] HTTP/3 QUIC performance tuning and production hardening (Target: Q2 2026) (Issue: #1436)
- [I] GraphQL endpoint for schema-driven API access (Target: Q2 2026) (Issue: #1437)
- [I] API versioning strategy (deprecation headers, sunset dates) (Target: Q3 2026) (Issue: #2308)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [I] OpenAPI 3.1 spec auto-generation from handler annotations (Issue: #1448)
- [x] Request validation middleware (JSON Schema per endpoint)
- [x] Response streaming for large result sets (chunked transfer) (Issue: #2466)
- [I] Per-tenant custom domain routing (Issue: #2301)
- [I] WebSocket binary frame support for wire protocol upgrade (Issue: #2299)

### Long-term (6-12 months)
- [I] gRPC-web proxy for browser clients (Issue: #2303)
- [~] Serverless function hosting (run user code in-process) (Issue: #2467)
- [I] Edge caching integration (CDN cache-control header management) (Issue: #2305)
- [I] Service mesh sidecar proxy mode (Envoy xDS compatibility) (Issue: #2306)
- [I] HTTP/3 datagram support for real-time low-latency streams (Issue: #2307)

## Implementation Phases

### Phase 1: Multi-Protocol Server & Core API (Status: Completed ✅)
- [x] `HTTPServer` – multi-protocol async I/O server (HTTP/1.1, HTTP/2, HTTP/3) on Boost.Beast/Asio
- [x] TLS 1.3 with modern cipher suites
- [x] 40+ specialized REST API handlers
- [x] WebSocket support for real-time notifications and changefeeds
- [x] MQTT broker integration for IoT use cases
- [x] PostgreSQL wire protocol for SQL client compatibility
- [x] gRPC services for high-performance RPC
- [x] API Gateway (routing, versioning, load balancing)
- [x] JWT, Kerberos, API token, and USB admin authentication
- [x] Rate limiting (token bucket, sliding window, distributed)
- [x] Load shedding and circuit breaking
- [x] Server-Sent Events (SSE) for changefeeds
- [x] Multi-tenancy with tenant isolation
- [x] Apache Ranger policy enforcement integration
- [x] Response compression (Gzip, Brotli, Zstd)
- [x] Model Context Protocol (MCP) server for AI integrations
- [x] Graceful shutdown and connection draining

### Phase 2: HTTP/3 Hardening & GraphQL (Status: In Progress 🚧)
- [~] HTTP/3 QUIC performance tuning and production hardening
- [~] GraphQL endpoint for schema-driven API access
- [~] API versioning strategy (deprecation headers, sunset dates)

### Phase 3: OpenAPI & Request Validation (Status: In Progress 🚧)
- [ ] OpenAPI 3.1 spec auto-generation from handler annotations
- [x] Request validation middleware (JSON Schema per endpoint)
- [x] Response streaming for large result sets (chunked transfer)
- [ ] Per-tenant custom domain routing
- [ ] WebSocket binary frame support for wire protocol upgrade

### Phase 4: gRPC-Web, Serverless & Service Mesh (Status: Planned 📋)
- [ ] gRPC-web proxy for browser clients
- [~] Serverless function hosting (run user code in-process) (Issue: #2467)
- [ ] Edge caching integration (CDN cache-control header management)
- [ ] Service mesh sidecar proxy mode (Envoy xDS compatibility)
- [ ] HTTP/3 datagram support for real-time low-latency streams

## Production Readiness Checklist
- [?] Unit tests coverage > 80%
- [?] Integration tests (all 40+ endpoints, TLS, auth, rate limiting)
- [?] Performance benchmarks (req/sec, p99 latency, concurrent connections)
- [?] Security audit (header injection, CORS misconfiguration, DoS vectors)
- [?] Documentation complete
- [?] API stability guaranteed

## Known Issues & Limitations
- HTTP/3 is implemented but not yet hardened for high-throughput production workloads.
- GraphQL support is planned; only REST and gRPC are available currently.
- PostgreSQL wire protocol compatibility is partial; advanced PG features may not be supported.

## Breaking Changes
- REST API path versioning (`/api/v1/`) guarantees stability for v1.x endpoints.
- gRPC service `.proto` definitions are stable; no breaking field removals planned.
- MCP server protocol follows the MCP spec; updates track upstream spec changes.
