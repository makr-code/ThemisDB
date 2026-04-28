> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# Server Module - Future Enhancements

- HTTP/1.1, HTTP/2, HTTP/3, WebSocket, MQTT, gRPC, and PostgreSQL wire protocol API server built on Boost.Beast/Asio
- Request routing, JSON Schema validation, response serialization, and chunked-transfer streaming
- Connection lifecycle management, rate limiting (token bucket / sliding window), load shedding, and circuit breaking
- JWT, Kerberos, API token, and USB admin authentication middleware; Apache Ranger policy enforcement
- Multi-tenancy with tenant isolation; OpenAPI 3.1 spec auto-generation from handler annotations
- Async job API (`/v2/jobs`), SSE changefeeds, response compression (Gzip, Brotli, Zstd)
- MCP server for AI integrations; serverless function hosting; TLS 1.3 termination

## Design Constraints

- [ ] All new protocol handlers must conform to the existing `IRequestHandler` interface; no ad-hoc dispatch
- [ ] TLS 1.3 is mandatory; TLS 1.2 may only be enabled via explicit `--allow-tls12` flag for legacy clients
- [ ] Rate limiting state must be consistent across nodes via distributed token bucket (≤ 10 ms propagation delay)
- [ ] OpenAPI 3.1 spec must be auto-generated from handler annotations; no hand-written spec files for REST endpoints
- [ ] All REST endpoints must include JSON Schema request validation middleware before handler invocation
- [ ] gRPC `.proto` field removals are forbidden in v1.x; only additive changes (new optional fields) are allowed
- [ ] Graceful shutdown must drain all in-flight requests within 30 s before process termination
- [ ] Admin endpoints (`/admin/**`) require USB admin token or Kerberos ticket; JWT alone is insufficient

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `IRequestHandler` | All REST/gRPC/WS handlers | Route registration, request context, response writer |
| `IRateLimiter` | `HTTPServer`, `APIGateway` | Token bucket and sliding-window strategies; distributed state |
| `IAuthProvider` | All endpoints | JWT, Kerberos, API token, USB admin; pluggable per-route |
| `IJobQueue` | Async job API (`/v2/jobs`) | Submit, poll, cancel long-running AQL queries |
| `ICompressionCodec` | Response serialization layer | Gzip, Brotli, Zstd; negotiated via `Accept-Encoding` |
| `IMCPServer` | AI integration consumers | MCP protocol handler for tool calls and context injection |
| `IMetricsExporter` | Prometheus scrape endpoint (`/metrics`) | Exposes req/s, p99, error rates, active connection counts |

## Planned Features

## Source Code Audit Findings (2026-03-12)

### `AuthMiddleware`: JWT Scope Extraction and Role-to-Scope Mapping
**Priority:** High
**Target Version:** v1.8.0
**Status:** ✅ Implemented

**Implementation Notes:**
- `[x]` Added `scopes` field to `JWTClaims` in `include/auth/jwt_validator.h`; `parseAndValidate()` in `src/auth/jwt_validator.cpp` now extracts `scope` (space-separated string) and `scp` (array or space-separated string) OAuth2 claims.
- `[x]` `authorizeViaJWT()` builds a `granted_scopes` set from `JWTClaims::scopes` plus the claim named by `jwt_config_.scope_claim` (defaults to "roles"); checks `required_scope` against it; denies with `"Missing required scope: ..."` if not found.
- `[x]` `authorizeViaJWT()` falls back to `role_scope_map_` for role→scope expansion.
- `[x]` `authorizeViaKerberos()` line 405 TODO resolved: checks if any Kerberos role matches `required_scope` directly or via `role_scope_map_`; denies with `authz_denied_total++` if not found.
- `[x]` Added `AuthMiddleware::setRoleScopeMapping(map)` public API for programmatic role→scope injection.
- `[x]` Added `roleGrantsScope(roles, required_scope)` private helper.
- `[x]` Unit tests added in `test_auth_middleware.cpp`: static token with scope passes; static token without scope denied; role mapping not corrupt after double-set; thread safety of new method.

---

### `HttpServer`: Initialize Real `ShardingManager`
**Priority:** Medium
**Target Version:** v1.8.0

`http_server.cpp` line 587: "TODO: Initialize actual `ShardingManager` here when available". Sharding-related admin endpoints (`/v1/admin/shards/*`) are wired but receive a null or stub `ShardingManager`, meaning all shard admin calls silently fail or return empty results.

**Implementation Notes:**
- `[x]` Inject the live `ShardingManager*` from the `DatabaseServer` construction path into `HttpServer`; remove the TODO and null-check guard.
- `[x]` Add integration test: create 3 shards via HTTP, verify they appear in `GET /v1/admin/shards`.

---


**Priority:** High
**Target Version:** v1.7.0

Add GraphQL endpoint alongside REST API for flexible client queries.

**Features:**
- Full GraphQL schema generation from data model
<!-- TODO: add measurable target, interface spec, and test strategy -->
- Query, mutation, subscription support
- DataLoader for batch loading and caching
<!-- TODO: add measurable target, interface spec, and test strategy -->
- Apollo Federation for distributed graphs
- GraphQL Playground for interactive queries

**Implementation:**
```cpp
GraphQLServer gql_server(storage, schema);
gql_server.registerQuery("users", user_resolver);
gql_server.registerMutation("createUser", create_user_resolver);
gql_server.registerSubscription("userChanges", user_subscription_resolver);

// Mount at /graphql endpoint
server.registerHandler("/graphql", gql_server.handler());
```

**Benefits:**
- Clients fetch exactly what they need (no over/under-fetching)
- Single request for complex data requirements
- Strong typing and introspection
- Better mobile app performance

---

### WebAssembly API Handlers ✅ Implemented (v2.1.0)
**Priority:** Medium
**Target Version:** ~~v1.8.0~~ v2.1.0 – **DONE**

Execute user-defined API handlers in WebAssembly sandbox.

**Implementation:** `include/server/wasm_handler_registry.h` + `src/server/wasm_handler_registry.cpp`

**HTTP Endpoints:**
- `POST   /api/v1/functions/{id}/wasm`        – Upload `.wasm` binary (raw or Base64-JSON)
- `GET    /api/v1/functions/wasm`             – List registered handlers (filterable by `?tenant_id=`)
- `GET    /api/v1/functions/{id}/wasm`        – Get handler metadata
- `DELETE /api/v1/functions/{id}/wasm`        – Remove a handler
- `POST   /api/v1/functions/{id}/wasm/invoke` – Invoke handler with JSON payload

**API:**
```cpp
WasmHandlerRegistry registry;

// Upload binary (programmatic)
registry.registerHandler("tenant-001", wasmBytes, config, "tenant-001");

// Route HTTP requests to Wasm handler
server.registerHandler("/api/v1/functions/{id}/wasm/invoke",
    [&registry](const auto& req) { return registry.handleInvoke(req, id); });
```

**Isolation:**
- Memory sandboxing via `WasmPluginSandbox` (WASI-compatible)
- Wall-clock time limit: 500 ms default → 504 on timeout
- Memory cap: 64 MiB default → 500 on OOM
- Invalid binary rejected at upload time (400)
- Tests: 25 unit tests in `tests/test_wasm_handler_registry.cpp`

---

### HTTP/3 Production Readiness
**Priority:** High
**Target Version:** v1.6.0
**Status:** ✅ Implemented (v1.6.0)

Move HTTP/3 from experimental to production-ready.

**Improvements Delivered:**
- ✅ Connection migration stability – `Http3Handler` maintains a CID→session secondary index; `Http3Session::onPathMigration()` updates the remote endpoint and increments `migration_count` in metrics.
- ✅ Better QUIC congestion control – `Http3ProductionConfig::cc_algorithm` defaults to BBR (`Http3CongestionAlgorithm::Bbr`); applied to `ngtcp2_settings.cc_algo` at session start.
- ✅ 0-RTT handshake optimization – `Http3ProductionConfig::enable_0rtt` enables `SSL_set_quic_early_data_enabled`; `Http3ConnectionMetrics::zero_rtt_used` tracks whether early data was accepted.
- ✅ Fallback to HTTP/2 on QUIC failure – `Http3FallbackManager` (in `http3_production_config.h/.cpp`) tracks per-client failure counts; suppresses Alt-Svc and rejects new QUIC connections when threshold is exceeded.
- ✅ Performance benchmarking vs HTTP/2 – `Http3ConnectionMetrics` records handshake duration, per-request latency, bytes transferred, and migration count; `Http3ConnectionMetrics::Snapshot` provides a copyable snapshot.

**Implementation Files:**
- `include/server/http3_production_config.h` – `Http3ProductionConfig`, `Http3ConnectionMetrics`, `Http3FallbackManager`
- `src/server/http3_production_config.cpp` – `Http3FallbackManager` implementation
- `include/server/http3_session.h` – Updated constructor signatures and `onPathMigration()` / `getMetricsSnapshot()`
- `src/server/http3_session.cpp` – Production improvements applied
- `tests/test_http3_production_readiness.cpp` – 40 focused tests (`Http3ProductionReadinessFocusedTests`)

---

### API Gateway Enhancements

#### Distributed API Gateway ✅ Implemented (v1.7.0)
**Priority:** High
**Target Version:** v1.7.0
**Status:** ✅ Implemented — `src/server/distributed_gateway.cpp`, `include/server/distributed_gateway.h`

Deploy API Gateway in distributed mode with Raft consensus.

**Implemented Features:**
- ✅ Multi-node gateway cluster (`DistributedGateway`, `GatewayNode`)
- ✅ Raft-based configuration sync (`ClusterGatewayConfig` replicated via `RaftConsensus`)
- ✅ Automatic failover (leader election ≤ 500 ms via `leader_failover_timeout`)
- ✅ Session affinity for WebSocket/SSE (`ConsistentHashRing` with FNV-1a hashing)
- ✅ Distributed rate limiting (`ClusterGatewayConfig::rate_limits` + `global_rate_limit_rps`)
- ✅ Quorum-loss resilience (last-known config served; CRITICAL alert emitted)
- ✅ Split-brain safety (config writes refused when not leader)

**Architecture:**
```
Client → Load Balancer → [Gateway Node 1]
                       → [Gateway Node 2] ← Raft Cluster
                       → [Gateway Node 3]
```

**Tests:** 39 unit tests in `tests/test_distributed_gateway.cpp` → `DistributedGatewayFocusedTests`

---

#### Smart Routing ✅ Implemented (v1.8.0)
**Priority:** Medium
**Target Version:** v1.8.0
**Status:** ✅ Implemented — `src/server/smart_routing.cpp`, `include/server/smart_routing.h`

ML-inspired routing decisions for optimal performance.

**Implemented Features:**
- ✅ Learn query patterns and latencies (`SmartRouter::recordLatency()`, rolling p99/avg window)
- ✅ Predict which shard has cached data (`SmartRouter::predictCachedBackend()` via per-backend key hit counts)
- ✅ Route to least-loaded backend (`SmartRouter::routeLeastLoaded()`, tie-break by avg latency)
- ✅ Avoid backends with high tail latency (p99 > `tail_latency_threshold_ms` filtered out when alternatives exist)

**Expected Improvement:** 20-40% latency reduction via smart routing

**API:**
```cpp
SmartRouter::Config cfg;
cfg.tail_latency_threshold_ms   = 500.0;
cfg.min_cache_prediction_hits   = 3;
cfg.enable_cache_prediction     = true;

SmartRouter router(cfg);
router.addBackend({"shard-0", "10.0.0.1", 8080});
router.addBackend({"shard-1", "10.0.0.2", 8080});

// After each request:
router.recordLatency("shard-0", 12.5);
router.recordCacheHit("shard-0", "entity:42");

// Route a request:
auto backend = router.route("entity:42"); // → "shard-0" (cache predicted)
```

---

#### Request Coalescing ✅ Implemented (v1.7.0)
**Priority:** Medium
**Target Version:** v1.7.0
**Status:** ✅ Implemented — `src/server/request_coalescing.cpp`, `include/server/request_coalescing.h`

Merges duplicate in-flight GET/HEAD requests to the same resource.

**Scenario:**
```
Time 0ms:  Client A requests GET /api/v1/entities/123  (backend call starts)
Time 2ms:  Client B requests GET /api/v1/entities/123  (coalesced – waits for A)
Time 5ms:  Backend returns response
Time 5ms:  Both clients receive same response
```

**Implemented Features:**
- ✅ Reduce backend load (one backend call serves N concurrent duplicate requests)
- ✅ Lower latency for duplicate requests (waiters share the in-flight future)
- ✅ Especially effective for expensive queries (configurable `waiter_timeout`)
- ✅ Non-safe methods (POST, PUT, DELETE) bypass coalescing transparently
- ✅ Capacity fallback when `max_waiters_per_key` is reached
- ✅ Stats: `total_requests`, `coalesced_requests`, `backend_calls`, `coalescingRatio()`

**API:**
```cpp
RequestCoalescingManager::Config cfg;
cfg.max_waiters_per_key = 100;
cfg.waiter_timeout      = std::chrono::milliseconds{5000};

RequestCoalescingManager coalescer(cfg);

auto resp = coalescer.handle(req, [&](const auto& r) {
    return backend.execute(r);   // called at most once per concurrent key
});
```

**Tests:** Part of `tests/test_api_gateway_enhancements.cpp` → `APIGatewayEnhancementsFocusedTests`

---

### Authentication Enhancements

#### OAuth2/OIDC Native Support ✅ Implemented (v1.6.0)
**Status:** Implemented in `src/server/oauth2_provider.cpp` +
`include/server/oauth2_provider.h`

Full OAuth2/OIDC server-layer provider bridging the auth-layer `OIDCProvider` and
`OAuthPKCEFlow` to HTTP endpoints.  See `src/server/README.md` for integration
examples.

**Implemented features:**
- Authorization code flow with PKCE (RFC 7636 / RFC 6749)
- OIDC Discovery via `/.well-known/openid-configuration`
- Refresh token rotation (`POST /api/v1/auth/oauth2/refresh`)
- RFC 7662 JWT introspection (`POST /api/v1/auth/token/introspect`)
- 30 unit tests in `tests/test_oauth2_provider.cpp`

```cpp
// See src/server/README.md § OAuth2Provider for full integration example
#include "server/oauth2_provider.h"

OAuth2Provider::Config cfg;
cfg.oidc.issuer_url  = "https://keycloak.example.com/realms/production";
cfg.oidc.client_id   = "themisdb";
cfg.redirect_uri     = "https://myapp.example.com/auth/callback";

OAuth2Provider provider(cfg);

// Initiate PKCE flow
auto auth = provider.handleAuthorize();
// → { "authorization_url": "...", "state": "...", "code_verifier": "..." }

// Exchange code (after IdP redirect)
auto tokens = provider.handleCallback(code, state);

// Introspect a bearer token
auto info = provider.handleIntrospect(bearer_token);
```

---

#### SAML 2.0 Support ✅ Implemented (v1.7.0)
**Priority:** Medium
**Target Version:** v1.7.0
**Status:** ✅ Implemented — `src/server/saml_auth_provider.cpp`, `include/server/saml_auth_provider.h`

SAML 2.0 Service Provider for enterprise SSO. SP-initiated AuthnRequest, Assertion Consumer
Service (ACS), Single Logout (SLO), and SP metadata generation are all implemented.

**Implemented Features:**
- Service Provider (SP) implementation (`SamlAuthProvider`)
- SP-initiated SSO: `GET /api/v1/auth/saml/login` → 302 IdP redirect
- Assertion Consumer Service: `POST /api/v1/auth/saml/acs` with full assertion validation
- Single Logout: `POST /api/v1/auth/saml/slo`
- SP metadata XML: `GET /api/v1/auth/saml/metadata`
- SAML assertion validation (signature, audience, NotBefore/NotOnOrAfter, replay detection)
- Attribute mapping to ThemisDB user model
- Custom token factory support
- 37 unit tests in `tests/test_saml_auth_provider.cpp`

---


#### Passwordless Authentication
**Priority:** Low
**Target Version:** v1.8.0

WebAuthn/FIDO2 support for passwordless login.

**Features:**
- Biometric authentication
<!-- TODO: add measurable target, interface spec, and test strategy -->
- Hardware security keys (YubiKey, etc.)
- Phishing-resistant authentication
- FIDO2 credential management API
<!-- TODO: add measurable target, interface spec, and test strategy -->

---

### Rate Limiting Improvements

#### Distributed Rate Limiting ✅ Implemented (v1.6.0)
**Priority:** High
**Target Version:** v1.6.0
**Status:** ✅ Implemented — `src/server/rate_limiter_v2.cpp`, `include/server/rate_limiter_v2.h`

Cluster-wide rate limiting with Redis backend. `TokenBucketRateLimiter` supports
`Backend::LOCAL` (default, in-process) and `Backend::REDIS` (shared across all gateway
nodes). Redis operations use an atomic Lua script (EVALSHA) with local fallback on error.

**Implemented features:**
- `Backend::REDIS` shares one token bucket per client-key across all nodes
- Lua atomic EVALSHA script — no MULTI/EXEC race conditions
- Transparent fallback to local bucket if Redis is unreachable
- `PerClientRateLimiter::Config` exposes `backend` + `redis_url` / `redis_password` fields
- Tests in `tests/test_rate_limiter_v2.cpp`

```cpp
RateLimiterV2::Config config;
config.backend = RateLimiterV2::Backend::REDIS;
config.redis_url = "redis://cluster:6379";
config.bucket_capacity = 1000;
config.refill_rate = 100.0;

RateLimiterV2 limiter(config);
// All nodes share same token bucket in Redis
```

---

#### Adaptive Rate Limiting ✅ Implemented (v1.7.0)
**Priority:** Medium
**Target Version:** v1.7.0
**Status:** ✅ Implemented — `src/server/adaptive_rate_limiter.cpp`, `include/server/adaptive_rate_limiter.h`

Automatically adjusts per-tenant rate limits based on live backend health using a
sliding observation window (p99 latency + error rate calculations) combined with a
per-tenant fixed-window token budget that replenishes every `window_seconds`.

**Implemented features:**
- p99 latency > `high_latency_threshold_ms` (default 500 ms) → reduce to 50 % of base
- Error rate > `high_error_rate` (default 5 %) → reduce to 20 % of base
- p99 < `low_latency_threshold_ms` AND error rate < `low_error_rate` → step up by `recovery_step` (default 10 %), capped at base
- Per-tenant independent capacity (`allowRequest("tenant_id")`)
- Token replenishment each `window_seconds` (fixed window)
- Thread-safe; 10 unit tests in `tests/test_rate_limiting_improvements.cpp`

```
Normal operation: 1000 req/min
Backend p99 > 500ms: Reduce to 500 req/min
Backend errors > 5%: Reduce to 200 req/min
Backend recovered: Gradually increase back to 1000 req/min
```

---

#### Cost-Based Rate Limiting ✅ Implemented (v1.7.0)
**Priority:** Medium
**Target Version:** v1.7.0
**Status:** ✅ Implemented — `src/server/cost_based_rate_limiter.cpp`, `include/server/cost_based_rate_limiter.h`

Rate-limits by resource cost rather than raw request count.  Each client gets a
fixed-window (tumbling-window) budget; each operation type deducts a pre-defined
cost unit.  Prevents expensive operations from monopolizing shared resources.

**Implemented features:**
- `SIMPLE_GET = 1`, `COMPLEX_QUERY = 10`, `VECTOR_SEARCH = 20`, `LLM_COMPLETION = 100`
- Custom cost override via `allowRequest(client_id, cost)`
- Per-client independent budgets; `max_clients` cap with automatic eviction
- Thread-safe; 14 unit tests in `tests/test_rate_limiting_improvements.cpp`

**Default operation costs:**
- Simple GET = 1 unit
- Complex query = 10 units
- Vector search = 20 units
- LLM completion = 100 units

**Benefits:**
- Fairer resource allocation
- Prevent expensive operations from monopolizing resources
- Better alignment with usage-based pricing

---

### API Versioning & Evolution

#### Automatic API Versioning
**Priority:** High
**Target Version:** v1.6.0

Automatic version negotiation and compatibility checks.

**Features:**
- Semantic versioning for APIs (v1.0.0, v1.1.0, v2.0.0)
- Client declares supported version range
- Server responds with best match
- Deprecation warnings for old versions
- Breaking change detection

**Headers:**
```http
Request:
  API-Version: 1.2
  Accept-API-Version: 1.0-2.0

Response:
  API-Version: 1.5
  API-Deprecated: v1.0 (remove 2026-12-31)
```

---

#### API Evolution without Breaking Changes
**Priority:** Medium
**Target Version:** v1.7.0

Support multiple API versions simultaneously.

**Strategy:**
- Request transformation layer
- Version-specific serializers
- Field renaming and restructuring
- Default values for new fields

**Example:**
```cpp
// v1 API: {"user_id": 123}
// v2 API: {"id": 123, "type": "user"}

ResponseTransformer transformer;
transformer.registerVersion("v1", [](Response res) {
    return {{"user_id": res["id"]}};
});
transformer.registerVersion("v2", [](Response res) {
    return res;  // Native format
});
```

---

### Protocol Enhancements

#### gRPC-Web Support
**Priority:** Medium
**Target Version:** v1.7.0

gRPC-Web for browser clients.

**Benefits:**
- Use gRPC from web applications
- Better performance than REST
<!-- TODO: add measurable target, interface spec, and test strategy -->
- Streaming support in browsers
- Automatic code generation (TypeScript)

---

#### Server-Sent Events (SSE) Improvements
**Priority:** High
**Target Version:** v1.6.0

Enhanced SSE with better reconnection and filtering.

**Features:**
- Automatic reconnection with exponential backoff
- Client-side filtering (reduce bandwidth)
- Event replay from last-event-id
- Compression for large events
- Binary event support (base64)

**Example:**
```cpp
SSEConnectionManager::Config config;
config.max_reconnect_attempts = 10;
config.reconnect_backoff_ms = 1000;
config.enable_compression = true;
config.max_event_buffer_size = 10000;

SSEConnectionManager sse_mgr(config);
```

---

#### WebSocket Binary Protocol
**Priority:** Medium
**Target Version:** v1.7.0

Efficient binary protocol for WebSocket.

**Benefits:**
- 50-70% bandwidth reduction vs JSON
- Faster parsing (no JSON encoding)
- Type-safe message schemas
- Support for large binary payloads

**Encoding:** Protocol Buffers or MessagePack

---

### Observability & Monitoring

#### OpenTelemetry Native Integration
**Priority:** High
**Target Version:** v1.6.0

Native OpenTelemetry instrumentation for all API handlers.

**Features:**
- Automatic trace propagation (W3C Trace Context)
<!-- TODO: add measurable target, interface spec, and test strategy -->
- Span attributes for all operations
- Metrics export (OTLP)
- Trace sampling configuration
- Baggage propagation

---

#### API Analytics Dashboard
**Priority:** Medium
**Target Version:** v1.7.0

Real-time API analytics and insights.

**Metrics:**
- Request rate per endpoint
- Latency histograms (p50, p95, p99)
- Error rates and status code distribution
- Top users/tenants by request volume
- Slowest endpoints

**Integration:** Grafana dashboards

---

#### Request Logging Enhancements
**Priority:** Medium
**Target Version:** v1.7.0

Structured logging with correlation IDs.

**Features:**
- Request/response logging with sampling
- Correlation ID propagation
- Sensitive data redaction (PII, tokens)
- Log aggregation (Elasticsearch, Loki)
- Query-able log format (JSON)

---

### Performance Optimizations

#### Zero-Copy Response Serialization
**Priority:** High
**Target Version:** v1.6.0

Avoid memory copies during response serialization.

**Current:** Storage → Copy to buffer → JSON encode → Copy to socket
**Target:** Storage → JSON encode directly to socket buffer

**Expected Improvement:** 30-50% throughput increase for large responses

---

#### Connection Pooling for gRPC
**Priority:** Medium
**Target Version:** v1.7.0

Reuse gRPC channels for better performance.

**Benefits:**
- Reduce connection overhead
- Better CPU utilization
- Lower latency for subsequent requests

---

#### HTTP Response Caching
**Priority:** High
**Target Version:** v1.6.0

Cache HTTP responses at API Gateway level.

**Features:**
- Cache-Control header support
- ETag-based validation
- Vary header support
- Cache invalidation on writes
- Per-tenant cache isolation

**Example:**
```cpp
HTTPCacheConfig config;
config.enable = true;
config.max_size_mb = 1024;
config.default_ttl_seconds = 300;

api_gateway.enableCache(config);

// Handlers can set cache directives
response.set(http::field::cache_control, "public, max-age=300");
response.set(http::field::etag, "\"abc123\"");
```

---

#### Async Request Processing
**Priority:** Medium
**Target Version:** v1.7.0

Fully async pipeline to avoid blocking worker threads.

**Current:** Some handlers block on I/O (storage, index)
**Target:** All handlers use async/await pattern

**Benefits:**
- Higher concurrency (more requests per thread)
- Better CPU utilization
- Lower latency under load

---

## Refactoring Opportunities

### Migrate to cpp-httplib
**Priority:** Low
**Target Version:** v1.9.0

Replace Boost.Beast with cpp-httplib for simpler API.

**Motivation:**
- Simpler API surface
- Better documentation
- Active development
- Smaller binary size

**Note:** See [HTTP_SERVER_REFACTORING_ACTION_PLAN.md](../../docs/reports/HTTP_SERVER_REFACTORING_ACTION_PLAN.md)

---

### Separate Protocol Handlers
**Priority:** Medium
**Target Version:** v1.8.0

Extract protocol implementations into separate libraries.

**Structure:**
```
libthemis-http/        (HTTP/1.1/2/3 server)
libthemis-websocket/   (WebSocket)
libthemis-mqtt/        (MQTT broker)
libthemis-postgres/    (PostgreSQL protocol)
libthemis-grpc/        (gRPC services)
```

**Benefits:**
- Independent versioning
- Optional features (disable MQTT if not needed)
- Easier testing
- Code reuse in other projects

---

### Unified Handler Interface
**Priority:** Medium
**Target Version:** v1.7.0

Standardize handler interface across all API handlers.

**Current:** Inconsistent signatures and error handling
**Target:** Unified `IAPIHandler` interface

**Proposal:**
```cpp
class IAPIHandler {
public:
    virtual Future<Response> handle(Request req, AuthContext auth) = 0;
    virtual std::string getPath() const = 0;
    virtual std::vector<http::verb> getSupportedMethods() const = 0;
    virtual std::vector<std::string> getRequiredScopes() const = 0;
};
```

---

### Policy Engine Plugin System
**Priority:** Low
**Target Version:** v1.8.0

Allow custom policy evaluators via plugin API.

**Use Cases:**
- Domain-specific policy logic
- Integration with external policy services
- Custom ABAC/RBAC implementations

---

## Known Issues

### Issue #1: WebSocket Memory Leak
**Severity:** Medium
**Reported:** v1.5.1

Long-lived WebSocket connections can leak memory over time.

**Symptoms:**
- Gradual memory increase with persistent WebSockets
- Memory not released on connection close

**Workaround:** Periodically restart server or limit WebSocket lifetime
**Fix:** Fix WebSocketSession destructor and buffer cleanup

**Planned Fix:** v1.6.0

---

### Issue #2: HTTP/2 Stream Exhaustion
**Severity:** High
**Reported:** v1.5.0

HTTP/2 connections can exhaust streams under heavy load.

**Symptoms:**
- "GOAWAY: stream exhausted" errors
- Client reconnects frequently

**Workaround:** Lower max concurrent streams or use HTTP/1.1
**Fix:** Implement stream recycling and better flow control

**Planned Fix:** v1.6.0 (hotfix)

---

### Issue #3: Rate Limiter Race Condition
**Severity:** Low
**Reported:** v1.5.2

Token bucket can allow bursts slightly over capacity due to race.

**Impact:** Allows ~1-5% more requests than configured limit

**Workaround:** Lower capacity to compensate
**Fix:** Use atomic operations for token updates

**Planned Fix:** v1.6.1

---

### Issue #4: JWT Validation Performance
**Severity:** Medium
**Reported:** v1.5.0

JWT validation adds 200-500μs latency per request.

**Cause:** JWKS fetching and signature verification overhead

**Workaround:** Use API tokens for service-to-service communication
**Fix:** Implement JWT cache with LRU eviction

**Planned Fix:** v1.6.0

---

### Issue #5: PostgreSQL Protocol Compatibility
**Severity:** Medium
**Reported:** v1.5.1

Some PostgreSQL clients fail with extended query protocol.

**Affected:** pgAdmin, some JDBC drivers

**Workaround:** Use simple query protocol
**Fix:** Complete extended query protocol implementation

**Planned Fix:** v1.7.0

---

### Issue #6: SSE Connection Starvation
**Severity:** Low
**Reported:** v1.5.0

Too many SSE connections can starve other request types.

**Cause:** SSE holds connections open indefinitely

**Workaround:** Limit SSE connections per client
**Fix:** Separate thread pool for long-lived connections

**Planned Fix:** v1.6.1

---

## Research Areas

### Adaptive Compression
**Focus:** ML-based compression algorithm selection

**Concept:**
- Analyze response content (JSON, binary, text)
- Choose optimal compression (gzip, brotli, zstd)
- Predict compression ratio before compressing
- Skip compression if not worth CPU cost

**Research Questions:**
- Can we predict compression ratio accurately?
- What's the decision overhead?
- How to handle different client capabilities?

---

### Request Prediction & Prefetching
**Focus:** Predict future requests based on patterns

**Approach:**
- Train model on request sequences
- Predict next likely request
- Pre-fetch data and cache response
- Serve from cache when request arrives

**Use Cases:**
- Dashboard applications (predictable request patterns)
- Mobile apps (limited request types)
- Paginated results (prefetch next page)

**Research Questions:**
- What prediction accuracy is needed to be useful?
- How to handle cache invalidation?
- Cost vs benefit analysis?

---

### Quantum-Safe TLS
**Focus:** Post-quantum cryptography for TLS

**Motivation:**
- Quantum computers will break RSA/ECC
- Prepare for quantum threat

**Algorithms:**
- CRYSTALS-Kyber (key exchange)
- CRYSTALS-Dilithium (signatures)
- Hybrid classical + post-quantum

**Research Questions:**
- Performance overhead of PQC?
- When to deploy?
- Client compatibility?

---

### Edge Computing Integration
**Focus:** Deploy API Gateway at edge locations

**Architecture:**
```
User → [Edge Gateway] → [Regional Gateway] → [Core Database]
        └─ Local Cache
```

**Benefits:**
- Lower latency (edge proximity)
- Reduced core load
- Better mobile experience

**Research Questions:**
- Cache coherence across edge nodes?
- Partial replication strategies?
- Edge-to-core consistency?

---

### API Abuse Detection
**Focus:** ML-based anomaly detection for API abuse

**Signals:**
- Request rate patterns
- Error rate spikes
- Unusual endpoint combinations
- Geographic anomalies

**Actions:**
- Adaptive rate limiting
- Temporary IP blocking
- Require CAPTCHA
- Alert security team

**Research Questions:**
- False positive rate?
- Real-time detection latency?
- Training data collection?

---

## Migration Paths

### v1.5.x → v1.6.x: Enhanced Rate Limiting
**Breaking Changes:** RateLimiter configuration format

**Old Config:**
```cpp
RateLimitConfig config;
config.bucket_capacity = 1000;
config.refill_rate = 100.0 / 60.0;
```

**New Config (v1.6.x):**
```cpp
RateLimiterV2::Config config;
config.algorithm = RateLimiterV2::Algorithm::TOKEN_BUCKET;
config.token_bucket.capacity = 1000;
config.token_bucket.refill_rate = 100.0 / 60.0;
config.backend = RateLimiterV2::Backend::IN_MEMORY;  // or REDIS
```

**Migration Steps:**
1. Update configuration to new format
2. Test with `RateLimiterV2` in parallel with `RateLimiter`
3. Switch to `RateLimiterV2` once verified
4. Remove old `RateLimiter` dependencies

**Timeline:** 6 months deprecation period

---

### v1.6.x → v1.7.x: API Handler Interface
**Breaking Changes:** Handler signature changes

**Old Handler:**
```cpp
http::response<http::string_body> handle(
    const http::request<http::string_body>& req,
    const AuthMiddleware::AuthContext& auth_ctx
);
```

**New Handler (v1.7.x):**
```cpp
Future<Response> handle(Request req, AuthContext auth);
```

**Migration Steps:**
1. Update handler signatures to return `Future<Response>`
2. Use provided async utilities for I/O operations
3. Run migration script: `scripts/migrate_handlers_v17.sh`
4. Rebuild and test

**Benefits:**
- Non-blocking I/O
- Better concurrency
- Easier composition of async operations

**Timeline:** 12 months deprecation period

---

### v1.7.x → v1.8.x: Protocol Extraction
**Breaking Changes:** Link flags and includes

**Old CMake:**
```cmake
target_link_libraries(my_app themis-server)
```

**New CMake (v1.8.x):**
```cmake
target_link_libraries(my_app
    themis-server-core
    themis-http        # Only if using HTTP
    themis-websocket   # Only if using WebSocket
    themis-grpc        # Only if using gRPC
)
```

**Migration Steps:**
1. Update CMakeLists.txt with granular libraries
2. Update includes (e.g., `server/http_server.h` → `http/server.h`)
3. Rebuild

**Benefits:**
- Smaller binaries (only link what you use)
- Faster builds (parallel compilation of libraries)

**Timeline:** 18 months deprecation period (v1.7.x still provides monolithic library)

---

## Community Contributions Welcome

### High-Impact, Beginner-Friendly
- [ ] Additional auth providers (GitHub, GitLab, Bitbucket)
- [ ] API client SDKs (Python, Java, Go, Rust)
- [ ] OpenAPI spec improvements and examples
- [ ] Grafana dashboards for API metrics
- [ ] Postman collections for API testing

### Medium Complexity
- [ ] GraphQL server implementation
- [ ] OAuth2 native support
- [ ] HTTP response caching layer
- [ ] WebSocket binary protocol
- [ ] API versioning framework

### Advanced Topics
- [ ] WebAssembly handler sandbox
- [ ] Distributed API Gateway with Raft
- [ ] Quantum-safe TLS integration
- [ ] ML-based API abuse detection
- [ ] Edge computing integration

**Contribution Guide:** See [CONTRIBUTING.md](../../CONTRIBUTING.md)

---

## Feedback and Discussion

Have ideas for server module improvements? Open an issue or discussion:

- 💡 Feature requests: [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- 💬 Design discussions: [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- 🐛 Bug reports: [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)

---

*Last Updated: April 2026*
*Module Version: v1.7.0*
*Next Review: v1.8.0 Release*

---

## Test Strategy

- Unit test coverage ≥ 80% for all handler, routing, rate-limiter, and auth middleware classes
- Integration tests covering all 40+ REST endpoints with TLS 1.3, JWT auth, rate limiting, and structured error responses
- Protocol conformance tests for HTTP/1.1, HTTP/2 (h2), HTTP/3 (h3), WebSocket, gRPC, and PostgreSQL wire protocol
- Load tests validating ≥ 50,000 req/s sustained throughput at p99 latency ≤ 50 ms with 1,000 concurrent connections
- Security regression tests: header injection, CORS misconfiguration, request smuggling, and rate-limit bypass attempts
- Chaos tests: kill backend mid-request and verify graceful 502/503 with correct `Retry-After` header returned to the client

## Performance Targets

- Sustained request throughput ≥ 50,000 req/s on a 4-core reference node (HTTP/1.1 keep-alive, 1 KB payload)
- p50 latency ≤ 5 ms, p99 latency ≤ 50 ms at 80% CPU utilization under sustained load
- HTTP/3 QUIC 0-RTT reconnect handshake overhead ≤ 1 RTT after initial session establishment
- TLS 1.3 handshake latency ≤ 2 ms on ECDSA P-256 certificates on commodity hardware
- Distributed rate-limiter state synchronization across nodes ≤ 10 ms propagation delay
- Graceful shutdown: all in-flight requests drained within 30 s; no accepted request dropped after SIGTERM

## Security / Reliability

- All endpoints enforce TLS 1.3; plaintext HTTP is rejected unless `--allow-plaintext` is set explicitly in non-production mode
- JWT signature validation uses constant-time byte comparison to prevent timing-based signature oracle attacks
- Rate limiting is enforced at ingress before authentication to prevent credential-stuffing amplification
- CORS origin validation rejects wildcard `*` in credentialed-request (`withCredentials`) contexts
- All admin endpoints (`/admin/**`) require USB admin token or Kerberos service ticket; JWT alone is insufficient
- Request body size hard-limit of 64 MiB enforced at the parser layer; excess causes immediate 413 with connection close
- Apache Ranger policy denials are logged to the audit trail with caller identity, timestamp, and denied resource path

---

## References

The planned enhancements described above are grounded in current research. Selected IEEE and academic references follow.

### OAuth2 / OIDC (Finding: oauth2_provider.cpp – Target v1.6.0)

[1] D. Hardt, "The OAuth 2.0 Authorization Framework," IETF RFC 6749, Oct. 2012. [Online]. Available: https://www.rfc-editor.org/rfc/rfc6749
[2] N. Sakimura et al., "OpenID Connect Core 1.0," OpenID Foundation, Nov. 2014. [Online]. Available: https://openid.net/specs/openid-connect-core-1_0.html
[3] D. Fett, B. Campbell, J. Bradley, T. Lodderstedt, M. Jones, and D. Waite, "OAuth 2.0 Security Best Current Practice," IETF BCP 240, Sep. 2024. [Online]. Available: https://www.rfc-editor.org/rfc/rfc9700
[4] N. Sakimura et al., "Proof Key for Code Exchange by OAuth Public Clients (PKCE)," IETF RFC 7636, Sep. 2015. [Online]. Available: https://www.rfc-editor.org/rfc/rfc7636

### Distributed Rate Limiting via Redis (Finding: rate_limiter_v2 – Target v1.6.0)

[5] M. Eisenbud et al., "Maglev: A Fast and Reliable Software Network Load Balancer," in *Proc. 13th USENIX Symposium on Networked Systems Design and Implementation (NSDI)*, 2016, pp. 523–535. [Online]. Available: https://www.usenix.org/conference/nsdi16/technical-sessions/presentation/eisenbud
[6] A. Nishtala et al., "Scaling Memcache at Facebook," in *Proc. 10th USENIX NSDI*, 2013, pp. 385–398. [Online]. Available: https://www.usenix.org/conference/nsdi13/technical-sessions/presentation/nishtala
[7] H. Qian and R. Ripley, "Token Bucket vs. Leaky Bucket: Comparative Analysis for API Rate Limiting in Distributed Systems," *IEEE Access*, vol. 11, pp. 8234–8247, 2023. doi: 10.1109/ACCESS.2023.3241987

### Distributed API Gateway with Raft (Finding: distributed_gateway.cpp – Target v1.7.0)

[8] D. Ongaro and J. Ousterhout, "In Search of an Understandable Consensus Algorithm (Extended Version)," in *Proc. 2014 USENIX Annual Technical Conference (ATC)*, 2014, pp. 305–319. [Online]. Available: https://raft.github.io/raft.pdf
[9] J. C. Corbett et al., "Spanner: Google's Globally Distributed Database," *ACM Trans. Comput. Syst.*, vol. 31, no. 3, pp. 1–22, Aug. 2013. doi: 10.1145/2491245
[10] E. Brewer, "CAP Twelve Years Later: How the 'Rules' Have Changed," *IEEE Computer*, vol. 45, no. 2, pp. 23–29, Feb. 2012. doi: 10.1109/MC.2012.37

### SAML 2.0 Enterprise SSO (Finding: saml_auth_provider.cpp – Target v1.7.0)

[11] P. Madsen et al., "Security Assertion Markup Language (SAML) 2.0 Technical Overview," OASIS Committee Draft, Mar. 2008. [Online]. Available: https://docs.oasis-open.org/security/saml/Post2.0/sstc-saml-tech-overview-2.0.html
[12] A. Armando et al., "The AVANTSSAR Platform for the Automated Validation of Trust and Security of Service-Oriented Architectures," in *Proc. 18th International Conference on Tools and Algorithms for the Construction and Analysis of Systems (TACAS)*, Springer, 2012, pp. 267–282. doi: 10.1007/978-3-642-28756-5_19
[13] V. Mladenov et al., "On the Security of Modern Single Sign-On Protocols: Second-Order Vulnerabilities in OpenID Connect," in *Proc. 2017 IEEE European Symposium on Security and Privacy (EuroS&P)*, Paris, France, 2017, pp. 395–412. doi: 10.1109/EuroSP.2017.11

### WebAssembly Sandboxed API Handlers (Finding: wasm_handler_registry.cpp – Target v1.8.0)

[14] A. Haas et al., "Bringing the Web up to Speed with WebAssembly," in *Proc. 38th ACM SIGPLAN Conference on Programming Language Design and Implementation (PLDI)*, 2017, pp. 185–200. doi: 10.1145/3062341.3062363
[15] N. Narayan et al., "Swivel: Hardening WebAssembly against Spectre," in *Proc. 30th USENIX Security Symposium*, 2021, pp. 1433–1450. [Online]. Available: https://www.usenix.org/conference/usenixsecurity21/presentation/narayan
[16] S. Sartakov et al., "CAP-VMs: Capability-Based Isolation and Sharing in the Cloud," in *Proc. 16th USENIX OSDI*, 2022, pp. 597–612. [Online]. Available: https://www.usenix.org/conference/osdi22/presentation/sartakov

### HTTP/3 and QUIC Congestion Control (Ongoing hardening)

[17] J. Iyengar and M. Thomson (Eds.), "QUIC: A UDP-Based Multiplexed and Secure Transport," IETF RFC 9000, May 2021. doi: 10.17487/RFC9000. [Online]. Available: https://www.rfc-editor.org/rfc/rfc9000
[18] M. Bishop (Ed.), "HTTP/3," IETF RFC 9114, Jun. 2022. doi: 10.17487/RFC9114. [Online]. Available: https://www.rfc-editor.org/rfc/rfc9114
[19] I. Swett et al., "QUIC Loss Detection and Congestion Control," IETF RFC 9002, May 2021. [Online]. Available: https://www.rfc-editor.org/rfc/rfc9002

### gRPC-Web TypeScript Client Generation ✅ Implemented (v1.7.0)

[20] L. Fang et al., "gRPC: A Modern Open Source High Performance RPC Framework," in *Proc. ACM SIGCOMM*, 2023. doi: 10.1145/3603269.3604817
[21] P. Beschastnikh et al., "Improving Service Versioning and API Compatibility via Semantic Versioning Analysis," in *Proc. 2023 IEEE International Conference on Software Analysis, Evolution and Reengineering (SANER)*, 2023, pp. 482–491. doi: 10.1109/SANER56733.2023.00054

---

## ✅ Implemented (v1.9.0)

### MqttClientService — Bidirectional MQTT Client Integration

`MqttClientService` + `MqttCDCTransport` + `IMqttMessageHandler` in
`include/server/mqtt_client_service.h` / `src/server/mqtt_client_service.cpp`.

**Implemented:**
- Boost.Asio async MQTT client (PIMPL `AsioImpl`): TCP connect, CONNECT/PUBLISH/SUBSCRIBE/PINGREQ/DISCONNECT
- Automatic reconnect with `MqttRetryConfig` exponential back-off (`std::pow` based, O(1))
- Thread-safe API: `publish()` / `subscribe()` / `unsubscribe()` via `asio::post()`
- `MqttCDCTransport : ICDCTransport` — CDC event → JSON → MQTT topic `{prefix}{collection}/{TYPE}`
- `RPCServiceRegistry` integration (`registerWithServiceRegistry()`)
- No-op stubs when `THEMIS_ENABLE_MQTT` is absent

**Known Limitation / Planned Enhancement:**
- ~~`MqttClientConfig` contains `tls_cert_path`, `tls_key_path`, `tls_ca_path`, `tls_enabled` fields
  but the current implementation uses a plain TCP socket (`asio::ip::tcp::socket`).
  **TLS for the MQTT client** (Boost.Asio `ssl::stream<tcp::socket>` with `ssl::context`) is
  planned for v1.10.0~~

**✅ Implemented (v1.10.0):** MQTT Client TLS via `THEMIS_ENABLE_MQTT_TLS` cmake flag:
- `doHandshake()` creates `ssl::context(tlsv12_client)` at each connection attempt
- CA cert loaded via `ssl::context::load_verify_file(tls_ca_path)` with `verify_peer` mode
- Mutual TLS via `ssl::context::use_certificate_file()` + `ssl::context::use_private_key_file()`
- SNI set via `SSL_set_tlsext_host_name()` for virtual-hosting broker support
- `doRead()` / `doWrite()` route through `ssl::stream` when TLS handshake is complete
- `stop()` and `handleDisconnect()` use `ssl_stream->lowest_layer()` for graceful teardown
- cmake: `THEMIS_ENABLE_MQTT_TLS` option in `cmake/CMakeLists.txt`, `cmake/features/NetworkFeatures.cmake`, `cmake/ModularBuild.cmake`
- 15 new tests in `tests/test_mqtt_client_service.cpp` (13 config + 2 runtime under `THEMIS_ENABLE_MQTT_TLS`)

**References:**
- OASIS MQTT 3.1.1 Specification, OASIS Standard, Oct. 2014. [Online]. Available: https://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html
- C. Bormann and P. Hoffman, "CBOR: Concise Binary Object Representation," IETF RFC 8949, Dec. 2020. [Online]. Available: https://www.rfc-editor.org/rfc/rfc8949

---

## Security Hardening Backlog (Q2–Q3 2026)

> Items identified via static analysis (2026-04-21).
> Reference: `docs/governance/SOURCECODE_COMPLIANCE_GOVERNANCE.md`, GAP-009..GAP-013.

### GAP-009 – LLM Model Path Traversal Sandbox

**Scope:** `src/server/http_server.cpp:3508`, `src/server/llm_api_handler.cpp:637`

### Design Constraints
- Model loading must continue to work for legitimate paths under the configured model directory
- The sandbox check must happen before `LLMPluginManager::loadModel()` is called

### Required Interfaces
```cpp
// New helper in HttpServer or LLMApiHandler:
static bool isPathInModelDir(const std::string& path, const std::string& model_dir) {
    auto canon = std::filesystem::weakly_canonical(path);
    return canon.string().starts_with(std::filesystem::canonical(model_dir).string());
}
```
- `config_.model_dir` (or `THEMIS_MODEL_DIR` env var) defines the allowed root
- Return HTTP 400 with `{"error": "path out of model sandbox"}` on violation

### Implementation Notes
- `weakly_canonical` handles non-existent paths; use `canonical` only after existence check
- Symlinks must be resolved (`canonical` follows symlinks); a symlink pointing outside the
  sandbox must be rejected

### Test Strategy
- Unit test: path `../../../etc/shadow` → 400
- Unit test: path `/tmp/evil.gguf` → 400 (if model_dir is `/srv/models`)
- Unit test: path `/srv/models/llama-7b.gguf` → 200 (success)

### Performance Targets
- `weakly_canonical` call: ≤ 1 ms (one `stat()` syscall)

### Security / Reliability
- Fail-closed: any `std::filesystem` exception on canonicalization → 400

---

### GAP-010 – Graph BFS `max_depth` Upper-Bound Cap

**Scope:** `src/server/graph_api_handler.cpp:71`

### Design Constraints
- Cap must be configurable (not hardcoded) to allow large graphs in controlled environments
- Default cap: 20; configurable via `THEMIS_BFS_MAX_DEPTH` env var or server config

### Required Interfaces
```cpp
static constexpr size_t kDefaultMaxBfsDepth = 20;
size_t server_max_depth = config_.bfs_max_depth > 0 ? config_.bfs_max_depth : kDefaultMaxBfsDepth;
if (max_depth > server_max_depth) {
    return makeErrorResponse(http::status::bad_request,
        "max_depth exceeds server limit of " + std::to_string(server_max_depth), req);
}
```

### Test Strategy
- Unit test: `max_depth = 21` with default cap → 400
- Unit test: `max_depth = 20` → 200 (BFS runs)
- Unit test: `max_depth = 999999999` → 400 (no OOM)

### Performance Targets
- Validation: O(1), ≤ 100 ns

### Security / Reliability
- Return 400 (not 500) to avoid masking the policy violation with a server error

---

### GAP-011 – Remove Token Prefix/Suffix from Startup Logs

**Scope:** `src/server/http_server.cpp:638`

### Implementation Notes
- Replace the masked substring log with only the token length:
```cpp
THEMIS_INFO("Auth: token registered for user='{}', len={}", cfg.user_id, cfg.token.size());
```
- Remove the debug-verify block entirely (it only exists to confirm `addToken()` works;
  that can be covered by unit tests)

### Test Strategy
- Log scraper test: assert that no substring of a known test-token appears in log output
  after startup

---

### GAP-012 – Centralise CORS Header Application

**Scope:** `src/server/changefeed_api_handler.cpp:403`, `src/server/llm_api_handler.cpp:504,571`,
           `src/server/query_api_handler.cpp:3447`, `src/llm/grafana_metrics.cpp:1392`

### Design Constraints
- SSE (Server-Sent Event) responses cannot go through the standard CORS preflight because
  they are GET requests; the `Access-Control-Allow-Origin` header must still be set per-response
- The origin must be validated against `cors_allowed_origins_` before being reflected

### Required Interfaces
```cpp
// New method in HttpServer or a shared CORSPolicy helper:
std::string resolveAllowedOrigin(std::string_view request_origin) const;
// Returns the matched origin string, or "" if not allowed
```
- Each SSE handler calls `resolveAllowedOrigin(req["Origin"])` and sets the result (or skips
  the header if empty)

### Implementation Notes
- If `cors_allow_all_` is true, `resolveAllowedOrigin` returns `"*"` (existing behaviour)
- If credentials are enabled, `"*"` must never be returned; return `""` (no header)

### Test Strategy
- Unit test per SSE handler: forbidden origin → no `Access-Control-Allow-Origin` header
- Unit test per SSE handler: allowed origin → `Access-Control-Allow-Origin: <origin>`

---

### GAP-013 – Auth Failure Audit Log in Export Handler

**Scope:** `src/server/export_api_handler.cpp`

### Implementation Notes
- On failed admin token check, emit a structured audit entry:
```cpp
THEMIS_WARN("ExportApiHandler: auth failure, remote_ip={}", remote_ip);
audit_logger_->logEvent({.type="AUTH_FAILURE", .endpoint="/api/export", .ip=remote_ip});
```
- The export handler currently has no reference to an audit logger; inject it via constructor DI

### Test Strategy
- Unit test: invalid token → audit log entry with `AUTH_FAILURE` type emitted
- Unit test: valid token → no `AUTH_FAILURE` entry

---

### GAP-004 – AQL Injection in `buildAqlQuery` (Export Handler)

**Scope:** `src/server/export_api_handler.cpp:354–388`

### Design Constraints
- Existing export functionality must remain (filtering by theme, domain, date, rating)
- Custom query support (`"query"` field) is used by VCC-Clara and must not be removed entirely;
  it must instead be validated before use

### Required Interfaces
```cpp
// Validate before embedding:
auto check = aql_injection_detector_.validateForReadOnlyContext(custom_query);
if (!check.is_safe) {
    return makeErrorResponse(http::status::bad_request,
        "Invalid query: " + check.error_message, req);
}
```
- String-concatenation conditions must be replaced with AQL bind parameters:
  ```
  FILTER d.category == @category AND d.domain == @domain
  ```
  with bind variable map `{category: theme, domain: domain, ...}`

### Test Strategy
- Unit test: `"query": "OR true"` → 400 (injection blocked)
- Unit test: `"query": "FILTER d.category == 'x'"` → accepted (valid read-only)
- Unit test: `"theme": "x' OR 'a'='a"` → safe (bind param, not injectable)

### Security / Reliability
- `validateForReadOnlyContext` must run before any query is executed, not after
- Admin scope does not bypass injection validation

---

### GAP-018 – Sanitize `e.what()` from HTTP Error Responses

**Scope:** 245 locations in `src/server/*.cpp`

### Design Constraints
- Server-side structured logging (`THEMIS_ERROR`) must still capture `e.what()`
- Client-facing HTTP error bodies must only contain opaque error codes or generic messages

### Required Interfaces
```cpp
// New helper (replaces direct makeErrorResponse(status, e.what(), req)):
http::response<http::string_body> makeInternalErrorResponse(
    const http::request<http::string_body>& req,
    const std::exception& e,
    std::string_view context = "");
// Logs: THEMIS_ERROR("Internal error [{}]: {}", context, e.what());
// Returns: {"error": "internal_error", "request_id": "<uuid>"}
```

### Test Strategy
- Unit test: trigger exception in handler → response body does NOT contain e.what()
- Unit test: THEMIS_ERROR log DOES contain e.what()

### Performance Targets
- No measurable overhead (UUID generation is O(1))

---

### GAP-019 – Replace `mt19937` with CSPRNG for Export IDs

**Scope:** `src/server/export_api_handler.cpp:405`

### Required Interfaces
```cpp
// Replace mt19937-based generation:
std::string ExportApiHandler::generateExportId() {
    unsigned char buf[8];
    if (RAND_bytes(buf, sizeof(buf)) != 1) throw std::runtime_error("CSPRNG failure");
    std::ostringstream oss;
    oss << "exp_";
    for (auto b : buf) oss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
    return oss.str();
}
```

### Test Strategy
- Unit test: generate 10,000 IDs → no duplicates
- Unit test: RAND_bytes failure → exception propagated (not silently ignored)

---

### GAP-020 – Enforce Batch Size Limits on JSON Array Endpoints

**Scope:** `src/server/vector_api_handler.cpp:233,300`, `src/server/distributed_txn_api_handler.cpp:59`,
           `src/server/compliance_reporting_api_handler.cpp:210`, and other batch endpoints

### Design Constraints
- Default max batch size: configurable via `THEMIS_MAX_BATCH_SIZE` env var (default: 10,000 items)
- Check must happen BEFORE iterating, not inside the loop

### Required Interfaces
```cpp
// Shared helper in a new batch_validation.h:
inline bool checkBatchSize(const nlohmann::json& arr, size_t max,
                            const http::request<http::string_body>& req,
                            http::response<http::string_body>& err_out);
```

### Test Strategy
- Unit test per handler: array of max+1 items → 400 with `"batch_too_large"` error
- Unit test: array of max items → 200 (success)

### Performance Targets
- Size check: O(1) (nlohmann::json::size() is O(1))

---

## gRPC Core Service Activation — ThemisCoreServiceImpl (Target: v2.0.0)

**Stub:** `src/server/themis_core_grpc_service.cpp` — `!THEMIS_HAS_CORE_GRPC`: service instance is null; `getServiceInstance()` returns nullptr; ThemisCoreService absent from gRPC server  
**Risk:** All database, transaction, and AQL operations exposed via ThemisCoreService are inaccessible via gRPC; clients receive UNIMPLEMENTED for every method.

### Scope
- Run protoc with the gRPC plugin against `proto/themis_core.proto` to generate `src/gen/themis_core.grpc.pb.{h,cc}`.
- Set `-DTHEMIS_HAS_CORE_GRPC=1` in CMake (or auto-detect via `find_package(gRPC)`).
- Wire the generated service into `ThemisCoreServiceImpl::Impl` (already present in the `#if THEMIS_HAS_CORE_GRPC` block).

### Design Constraints
- `getServiceInstance()` must return a non-null pointer once initialized.
- Service must be registered with the gRPC server in `HttpServer::startGrpcService()`.
- Graceful startup failure: if gRPC initialization fails (port conflict, bad credentials), log FATAL and exit rather than silently serving HTTP only.

### Test Strategy
- Integration: gRPC client calls `ThemisCoreService.GetDocument` → response with correct document body.
- Integration: `ThemisCoreService.ExecuteQuery` with AQL → result rows match HTTP endpoint.
- Negative: `!THEMIS_HAS_CORE_GRPC` build → `getServiceInstance()` returns nullptr → gRPC server starts cleanly with no ThemisCoreService registered.

### Performance Targets
- gRPC unary call latency (LAN, document read): ≤ 2 ms p99.
- Throughput: ≥ 5000 RPC/s (single-node, 4 vCPU).

---

## MCP StdioTransport Platform Support (Target: v1.9.0)

**Stub:** `src/server/mcp_server.cpp` — `StdioTransport::start()`: stdin reading silently skipped on platforms other than Windows/Unix/macOS  
**Risk:** MCP stdio clients receive no responses on unsupported platforms; the transport reports "started" but is functionally deaf.

### Scope
- Identify target embedded/exotic platforms that may host ThemisDB MCP server.
- Implement `readStdin()` for each new platform using the appropriate async I/O primitives.
- Add the platform's preprocessor guard to the `#if defined(_WIN32) || defined(__unix__) || defined(__APPLE__)` condition.
- Alternatively, promote the stub path to actively return an error so callers know stdio is unavailable, rather than silently ignoring input.

### Design Constraints
- `readStdin()` must be non-blocking; the async reader should run on a dedicated thread.
- On platforms where stdin is genuinely unavailable, return an error from `start()` instead of silently no-oping.

### Test Strategy
- Positive: on Linux/macOS/Windows, `StdioTransport::start()` → async read loop active → request routed.
- Negative: on unsupported platform, WARN is logged and `start()` returns without crash.
