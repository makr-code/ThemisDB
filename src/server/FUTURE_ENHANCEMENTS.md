# Server Module - Future Enhancements

## Planned Features

### GraphQL API Support
**Priority:** High  
**Target Version:** v1.7.0

Add GraphQL endpoint alongside REST API for flexible client queries.

**Features:**
- Full GraphQL schema generation from data model
- Query, mutation, subscription support
- DataLoader for batch loading and caching
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

### WebAssembly API Handlers
**Priority:** Medium  
**Target Version:** v1.8.0

Execute user-defined API handlers in WebAssembly sandbox.

**Use Cases:**
- Custom business logic without C++ compilation
- User-defined data transformations
- Multi-tenant custom endpoints
- Language-agnostic handler development (Rust, Go, C, etc. → Wasm)

**API:**
```cpp
WasmHandlerRegistry registry;
registry.loadWasm("tenant-001", "custom-validator.wasm");

// Route requests to Wasm handler
server.registerHandler("/api/v1/tenants/001/custom", 
                       WasmHandler(registry, "tenant-001"));
```

**Isolation:**
- Memory sandboxing via WASI
- CPU time limits
- Controlled I/O (no direct storage access)
- Versioning and rollback

---

### HTTP/3 Production Readiness
**Priority:** High  
**Target Version:** v1.6.0

Move HTTP/3 from experimental to production-ready.

**Improvements Needed:**
- Connection migration stability
- Better QUIC congestion control
- 0-RTT handshake optimization
- Fallback to HTTP/2 on QUIC failure
- Performance benchmarking vs HTTP/2

**Expected Benefits:**
- 30-50% latency reduction
- Better mobile network performance
- Faster connection establishment
- Built-in encryption (no plaintext HTTP)

---

### API Gateway Enhancements

#### Distributed API Gateway
**Priority:** High  
**Target Version:** v1.7.0

Deploy API Gateway in distributed mode with Raft consensus.

**Features:**
- Multi-node gateway cluster
- Raft-based configuration sync
- Automatic failover
- Session affinity for WebSocket/SSE
- Distributed rate limiting

**Architecture:**
```
Client → Load Balancer → [Gateway Node 1]
                       → [Gateway Node 2] ← Raft Cluster
                       → [Gateway Node 3]
```

---

#### Smart Routing
**Priority:** Medium  
**Target Version:** v1.8.0

ML-based routing decisions for optimal performance.

**Approach:**
- Learn query patterns and latencies
- Predict which shard has cached data
- Route to least-loaded backend
- Avoid backends with high tail latency

**Expected Improvement:** 20-40% latency reduction via smart routing

---

#### Request Coalescing
**Priority:** Medium  
**Target Version:** v1.7.0

Merge duplicate in-flight requests to same resource.

**Scenario:**
```
Time 0ms:  Client A requests GET /api/v1/entities/123
Time 2ms:  Client B requests GET /api/v1/entities/123
Time 5ms:  Backend returns response
Time 5ms:  Both clients receive same response
```

**Benefits:**
- Reduce backend load
- Lower latency for duplicate requests
- Especially effective for expensive queries

---

### Authentication Enhancements

#### OAuth2/OIDC Native Support
**Priority:** High  
**Target Version:** v1.6.0

First-class OAuth2 and OpenID Connect support.

**Features:**
- Authorization code flow
- PKCE for mobile apps
- Refresh token rotation
- Token introspection endpoint
- Discovery endpoint (.well-known/openid-configuration)

**Configuration:**
```cpp
OAuth2Config config;
config.authorization_endpoint = "https://auth.example.com/authorize";
config.token_endpoint = "https://auth.example.com/token";
config.client_id = "themisdb";
config.client_secret = "secret";

auth_middleware.enableOAuth2(config);
```

---

#### SAML 2.0 Support
**Priority:** Medium  
**Target Version:** v1.7.0

SAML 2.0 integration for enterprise SSO.

**Features:**
- Service Provider (SP) implementation
- SAML assertion validation
- Attribute mapping to ThemisDB user model
- Single Sign-On (SSO) and Single Logout (SLO)

---

#### Passwordless Authentication
**Priority:** Low  
**Target Version:** v1.8.0

WebAuthn/FIDO2 support for passwordless login.

**Features:**
- Biometric authentication
- Hardware security keys (YubiKey, etc.)
- Phishing-resistant authentication
- FIDO2 credential management API

---

### Rate Limiting Improvements

#### Distributed Rate Limiting
**Priority:** High  
**Target Version:** v1.6.0

Cluster-wide rate limiting with Redis backend.

**Current:** Per-node rate limiting (can be bypassed with multiple nodes)  
**Target:** Shared rate limit state across all gateway nodes

**Implementation:**
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

#### Adaptive Rate Limiting
**Priority:** Medium  
**Target Version:** v1.7.0

Automatically adjust rate limits based on backend health.

**Logic:**
- Monitor backend latency and error rates
- Reduce rate limits when backends struggle
- Increase rate limits during low load
- Per-tenant adaptive limits

**Example:**
```
Normal operation: 1000 req/min
Backend p99 > 500ms: Reduce to 500 req/min
Backend errors > 5%: Reduce to 200 req/min
Backend recovered: Gradually increase back to 1000 req/min
```

---

#### Cost-Based Rate Limiting
**Priority:** Medium  
**Target Version:** v1.7.0

Rate limit by resource cost rather than request count.

**Concept:**
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

**Note:** See [HTTP_SERVER_REFACTORING_ACTION_PLAN.md](../../HTTP_SERVER_REFACTORING_ACTION_PLAN.md)

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

*Last Updated: February 2026*  
*Module Version: v1.5.x*  
*Next Review: v1.6.0 Release*
