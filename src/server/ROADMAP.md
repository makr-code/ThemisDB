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
- [x] Rate limiting (token bucket, sliding window) — node-local only; cluster-wide Redis backend planned for v1.6.0 (see Planned Features)
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

## In Progress 🚧
*(none currently in progress – all Phase 1–4 items completed)*

## Planned Features 📋

### Short-term (Next 3-6 months)
- [x] OAuth2/OIDC native support (authorization code flow, PKCE, refresh token rotation) (Target: v1.6.0)
  - Files: `server/oauth2_provider.cpp` + `include/server/oauth2_provider.h` ✅ implemented
  - Behavior: full RFC 6749 authorization-code + PKCE flow; discovery via `/.well-known/openid-configuration`; refresh token rotation on each use; JWT introspection at `POST /api/v1/auth/token/introspect`
  - Errors: expired access token → 401 with `WWW-Authenticate: Bearer error="invalid_token"`; invalid refresh token → 400; PKCE verifier mismatch → 400
  - Tests: 30 unit tests in `tests/test_oauth2_provider.cpp` (construction, authorize, callback, token exchange, refresh, introspect, logout, state TTL, custom token factory, deterministic PKCE)
- [ ] Distributed rate limiting via Redis backend (cluster-wide token bucket) (Target: v1.6.0)
  - Files: `server/rate_limiter_v2.cpp` + `include/server/rate_limiter_v2.h` (add `Backend::REDIS` strategy)
  - Behavior: all gateway nodes share a single token bucket per client key in Redis using atomic `EVALSHA`; propagation delay ≤ 10 ms; graceful fallback to local bucket on Redis unavailability
  - Errors: Redis timeout → fall back to node-local limit + emit `WARN`; Redis connection failure → same fallback; over-limit → 429 with `Retry-After` header
  - Tests: unit (local fallback), integration (multi-node Redis mock, concurrent burst), property-based (bucket never exceeds capacity under concurrent writers)
  - Perf: Redis round-trip ≤ 5 ms p99 on same LAN; throughput ≥ 50 000 check/s per node

### Long-term (6-12 months)
- [x] Distributed API Gateway with Raft-based config sync and automatic failover (Target: v2.1.0)
  - Files: `src/server/distributed_gateway.cpp` + `include/server/distributed_gateway.h` implemented
  - Behavior: multi-node gateway cluster (3 or 5 nodes); routing rules and rate-limit config replicated via Raft log; leader failover ≤ 500 ms; session affinity for WebSocket/SSE via consistent-hash ring
  - Errors: quorum loss → gateway continues with last-known config + emits `CRITICAL` alert; split-brain → reject writes to config until quorum restored
  - Tests: 41 unit tests in `tests/test_distributed_gateway.cpp` covering config replication, consistent-hash routing, session affinity, quorum handling
  - Perf: config propagation ≤ 100 ms across 5 nodes on LAN; no additional per-request latency vs single-node gateway
- [ ] gRPC-Web TypeScript client auto-generation (Target: v1.7.0)
  - Files: new `scripts/gen_grpc_web_ts.py`; reads existing `.proto` files under `protos/`
  - Behavior: generates typed TypeScript stubs for all public gRPC services; emits `@themisdb/client-grpc-web` npm package
  - Errors: proto syntax error → generator exits with non-zero code and line-level error message; missing import → clear diagnostic
  - Tests: unit (generator parses protos, emits valid TS), integration (generated client calls live `GrpcWebProxyHandler`)
  - Perf: generation completes in ≤ 5 s for current proto set (< 50 files)
- [ ] WebAssembly API handlers (user-defined handlers in WASI sandbox) (Target: v1.8.0)
  - Files: new `server/wasm_handler_registry.cpp` + `include/server/wasm_handler_registry.h`; depends on `themis/base/wasm_runtime_injector.h`
  - Behavior: tenant uploads `.wasm` binary; handler registered at `POST /api/v1/functions/{id}/wasm`; invoked per request in isolated WASI sandbox with CPU-time limit (default 500 ms) and memory cap (default 64 MB)
  - Errors: CPU limit exceeded → 504 + `grpc-status: DEADLINE_EXCEEDED`; memory overflow → 500 + sandbox kill; invalid wasm binary → 400 at upload time
  - Tests: unit (sandbox isolation, memory cap enforcement), integration (Rust→wasm handler round-trip), security (escape-attempt wasm modules must not access host memory)
  - Perf: wasm invocation overhead ≤ 5 ms per call; throughput ≥ 5 000 simple handler calls/s per node
- [x] SAML 2.0 Service Provider support for enterprise SSO (Target: v1.7.0)
  - Files: `server/saml_auth_provider.cpp` + `include/server/saml_auth_provider.h` ✅ implemented
  - Behavior: SP-initiated SSO redirect; validates SAML assertions (signature, audience, NotBefore/NotOnOrAfter); maps attributes to ThemisDB user model; Single Logout (SLO) via `POST /api/v1/auth/saml/slo`; SP metadata via `GET /api/v1/auth/saml/metadata`
  - Errors: invalid assertion signature → 401; expired assertion → 401 with clock-skew hint; missing required attribute → 403
  - Tests: 27 unit tests in `tests/test_saml_auth_provider.cpp` (login redirect, ACS success/failure, SLO, metadata, replay detection, custom token factory)

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
- [x] Rate limiting (token bucket, sliding window) — node-local; Redis distributed backend planned v1.6.0
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

### Phase 5: Distributed Tracing (Status: Completed ✅)
- [x] OpenTelemetry `Tracer::startSpan()` instrumentation for all 64 API handler files (`utils/tracing.h`) (March 2026)
  — Covers: llm, voice, lora, monitoring, cache_admin, distributed_txn, task_scheduler, pii, audit, session, branch, pitr, diff, merge, mvcc, snapshot, import, pki, profiling, geo_topology, policy_*, async_job, hot_reload, wal, serverless_function, service_mesh, update, bpmn, compliance_reporting, prompt, prompt_engineering, replication_topology, review_scheduling, udf, retention, keys, classification, error, saga, feedback, reports and all previously-instrumented handlers
  — Tests: `tests/test_otel_api_tracing.cpp` (162 tests; 120+ tests added March 2026)
  — Acceptance criteria met: `Tracer::startSpan("handleXxx")` present at the entry point of every request-handling method

## Production Readiness Checklist
- [x] Unit tests coverage > 80% — 208+ tests across 9 server test files (service_mesh_api_handler: 24, grpc_web_proxy_handler: 21, serverless_function_api_handler: 37, http_server_network: 28, service_mesh: 33, api_grpc_server: 13, http2_server_push: 10, themis_wire_protocol_server: 33, http2_protocol: 9); all Phase 1–5 components covered
- [x] Integration tests (all 40+ endpoints, TLS, auth, rate limiting) — unified suite added in `tests/test_server_integration_complete.cpp` (111 tests, 6 sub-suites): live server auth enforcement (401 without/with invalid Bearer, non-401 with valid token), `RateLimitingMiddleware` token-bucket exhaustion + whitelist + endpoint overrides + stats + concurrency, legacy `RateLimiter` blacklist + anomaly-detection + per-user limits, `HttpServer::Config` completeness (HTTP/2, HTTP/3, WebSocket, feature flags, timeouts, connection limits), live server rate-limit enforcement via `X-Forwarded-For` (429 after bucket exhausted, whitelist bypass, `Retry-After` header), and 25+ additional endpoint-breadth tests; existing per-feature suites (`test_api_integration.cpp`, `test_http_audit.cpp`, `test_http_timeseries.cpp`, `test_http_vector.cpp`, `test_http_changefeed*.cpp`, `bench_api_endpoints.cpp`, `stress_test_wire_vs_http.sh`) complement the coverage
- [x] Performance benchmarks (req/sec, p99 latency, concurrent connections) — `benchmarks/bench_api_endpoints.cpp` (634 lines, 14 micro-benchmarks: GraphQL parse/execute, JSON serialisation, correlation-ID overhead, REST roundtrip latency); `benchmarks/stress_test_wire_vs_http.sh` measures peak throughput under 1–500 concurrent clients; documented targets: 50K–200K req/sec, p50 < 5 ms, p99 < 50 ms
- [x] Security audit (header injection, CORS misconfiguration, DoS vectors) — CORS fully implemented: `cors_allowed_origins_` / `cors_allow_all_` / `cors_allow_credentials_` / `cors_allowed_methods_` in `http_server.h`; `cors_allow_origin` in `grpc_web_proxy_handler.h`; header injection mitigated via `sanitize_filename_part` in `export_api_handler.cpp`; DoS protection via `RateLimiter` (http_server.h:936, initialised in http_server.cpp:1280) and configurable `max_request_size_mb` body limit
- [x] Distributed tracing complete — all 64 API handler files instrumented with `Tracer::startSpan()` (March 2026); 162 tracing tests in `tests/test_otel_api_tracing.cpp`
- [x] Documentation complete — `include/server/README.md` (890 lines, includes `distributed_gateway.h` section), `src/server/README.md` (1 342 lines), `docs/de/server/README.md` (276 lines, German, validated 2026-03-10), `src/server/ARCHITECTURE.md`, `src/server/FUTURE_ENHANCEMENTS.md` (with IEEE references), `include/server/FUTURE_ENHANCEMENTS.md`, `src/server/rpc/README.md`, `docs/de/server/missing-implementations.md`, `docs/DISTRIBUTED_GATEWAY.md`; all public API handlers and configuration options documented
- [x] API stability guaranteed — REST `/api/v1/` path versioning enforced; `v2` prefix introduced for breaking changes; deprecation headers and `Sunset` dates emitted by versioning middleware; gRPC `.proto` definitions stable (no breaking field removals planned); MCP server protocol tracks upstream spec; see §Breaking Changes below

## Known Issues & Limitations
- HTTP/3 is implemented and hardened for high-throughput production workloads; further QUIC congestion-control tuning is ongoing.
- GraphQL support is available via `server/graphql_api_handler.cpp`; advanced federation features are planned.
- PostgreSQL wire protocol compatibility is partial; advanced PG features may not be supported.

## Breaking Changes
- REST API path versioning (`/api/v1/`) guarantees stability for v1.x endpoints.
- gRPC service `.proto` definitions are stable; no breaking field removals planned.
- MCP server protocol follows the MCP spec; updates track upstream spec changes.
