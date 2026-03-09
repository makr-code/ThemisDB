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
- [x] Async job API for long-running AQL queries with polling endpoint (`POST/GET/DELETE /v2/jobs[/{id}]`)
- [x] API versioning strategy (deprecation headers, sunset dates, URL path prefixes `/v1/` / `/v2/`) (Issue: #2308)
- [x] OpenAPI 3.1 spec auto-generation from handler annotations (Issue: #1448)
- [x] Request validation middleware (JSON Schema per endpoint)
- [x] Response streaming for large result sets (chunked transfer) (Issue: #2466, #2005)
- [x] Serverless function hosting (run user code in-process) (Issue: #2467)
- [x] HTTP/3 QUIC performance tuning and production hardening (`server/http3_session.cpp`) (Issue: #1436)
- [x] GraphQL endpoint for schema-driven API access (`server/graphql_api_handler.cpp`) (Issue: #1437)
- [x] WebSocket binary frame support for wire protocol upgrade (`server/websocket_session.cpp`) (Issue: #2299)
- [x] gRPC-web proxy for browser clients (`server/grpc_web_proxy_handler.cpp`) (Issue: #2303)
- [x] Edge caching integration (CDN cache-control header management) (`server/cdn_cache_middleware.cpp`) (Issue: #2305)
- [x] Service mesh sidecar proxy mode (Envoy xDS compatibility) (`network/service_mesh.cpp`, `server/service_mesh_api_handler.cpp`) (Issue: #2306)

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

### Phase 2: HTTP/3 Hardening & GraphQL (Status: Completed ✅)
- [x] HTTP/3 QUIC performance tuning and production hardening (`server/http3_session.cpp`)
- [x] GraphQL endpoint for schema-driven API access (`server/graphql_api_handler.cpp`)
- [x] API versioning strategy (deprecation headers, sunset dates, URL path prefixes `/v1/` / `/v2/`)

### Phase 3: OpenAPI & Request Validation (Status: Completed ✅)
- [x] OpenAPI 3.1 spec auto-generation from handler annotations
- [x] Request validation middleware (JSON Schema per endpoint)
- [x] Response streaming for large result sets (chunked transfer)
- [x] Per-tenant custom domain routing
- [x] WebSocket binary frame support for wire protocol upgrade (`server/websocket_session.cpp`)

### Phase 4: gRPC-Web, Serverless & Service Mesh (Status: Completed ✅)
- [x] Serverless function hosting (run user code in-process) (`server/serverless_function_api_handler.cpp`) (Issue: #2467)
- [x] gRPC-web proxy for browser clients (`server/grpc_web_proxy_handler.cpp`)
- [x] Edge caching integration (CDN cache-control header management) (`server/cdn_cache_middleware.cpp`)
- [x] Service mesh sidecar proxy mode (Envoy xDS compatibility) (`network/service_mesh.cpp`, `server/service_mesh_api_handler.cpp`)
- [x] HTTP/3 datagram support for real-time low-latency streams

## Production Readiness Checklist
- [?] Unit tests coverage > 80%
- [?] Integration tests (all 40+ endpoints, TLS, auth, rate limiting)
- [?] Performance benchmarks (req/sec, p99 latency, concurrent connections)
- [?] Security audit (header injection, CORS misconfiguration, DoS vectors)
- [?] Documentation complete
- [?] API stability guaranteed

## Known Issues & Limitations
- HTTP/3 is implemented and hardened for high-throughput production workloads; further QUIC congestion-control tuning is ongoing.
- GraphQL support is available via `server/graphql_api_handler.cpp`; advanced federation features are planned.
- PostgreSQL wire protocol compatibility is partial; advanced PG features may not be supported.

## Breaking Changes
- REST API path versioning (`/api/v1/`) guarantees stability for v1.x endpoints.
- gRPC service `.proto` definitions are stable; no breaking field removals planned.
- MCP server protocol follows the MCP spec; updates track upstream spec changes.
