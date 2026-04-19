# Server Module Headers - Future Enhancements

## Scope

- API-level enhancements to `include/server/` headers
- GraphQL schema interface (`GraphQLSchemaBuilder`, generated from data model headers)
- SSE endpoint API (`ISSEHandler`, non-blocking server-sent event push)
- Rate limiter interface (`IRateLimiter`, per-connection and per-tenant enforcement)
- Middleware chain API (`IMiddleware`, `MiddlewareChain`, ordered composition)
- gRPC bridge interface (`IRPCHandler<Req,Res>`, `RPCContext` abstraction)

## Design Constraints

- [ ] Middleware chain is strictly ordered — insertion order determines execution sequence
- [ ] GraphQL schema is generated from data model headers at startup (≤ 100 ms)
- [ ] SSE API is non-blocking — push operations must not stall the I/O thread
- [ ] Rate limiter is per-connection and per-tenant — both scopes enforced simultaneously
- [ ] No Boost.Beast types exposed in new public interfaces
- [ ] All async APIs use `Future<T>` / C++20 coroutine-compatible return types

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `IMiddleware` / `MiddlewareChain` | Auth, rate-limiting, CORS, logging | Ordered; each step may short-circuit |
| `IRateLimiter` | API gateway, per-tenant quota enforcement | Algorithm-agnostic; pluggable backend |
| `ISSEHandler` | Event streaming, live query updates | Non-blocking; backpressure via `cancel()` |
| `IRPCHandler<Req,Res>` | gRPC service bridge | Template-based; `RPCContext` carries metadata |
| `GraphQLSchemaBuilder` | Admin/developer API surface | Generated from data model headers at startup |

## Planned Features

### Unified API Handler Interface
**Priority:** High
**Target Version:** v1.7.0

Standardize all API handler interfaces with consistent signatures and async support.

**Current Interface:**
```cpp
class IAPIHandler {
public:
    virtual http::response<http::string_body> handle(
        const http::request<http::string_body>& req,
        const AuthMiddleware::AuthContext& auth_ctx
    ) = 0;
};
```

**Proposed Interface:**
```cpp
class IAPIHandler {
public:
    // Async handler returning Future
    virtual Future<Response> handleAsync(Request req, AuthContext auth) = 0;

    // Handler metadata
    virtual std::string getPath() const = 0;
    virtual std::vector<http::verb> getSupportedMethods() const = 0;
    virtual std::vector<std::string> getRequiredScopes() const = 0;
    virtual std::optional<RateLimitConfig> getRateLimit() const = 0;
    virtual Priority getPriority() const = 0;

    // Request validation
    virtual Result<void> validateRequest(const Request& req) const = 0;

    // Response transformation
    virtual Response transformResponse(Response res, const Request& req) const = 0;
};
```

**Benefits:**
- Consistent interface across all handlers
- Built-in async support
- Declarative metadata (scopes, rate limits, priority)
- Easier testing and mocking
- Better tooling support

---

### Protocol-Agnostic Request/Response Types
**Priority:** High
**Target Version:** v1.7.0

Abstract away protocol-specific types (Boost.Beast) for easier testing and migration.

**Proposed Types:**
```cpp
// Generic request type
class Request {
public:
    http::verb method() const;
    std::string path() const;
    std::string query_string() const;
    std::unordered_map<std::string, std::string> headers() const;
    std::string_view body() const;

    // Protocol-specific access
    template<typename Protocol>
    const Protocol::Request& as() const;
};

// Generic response type
class Response {
public:
    void setStatus(http::status status);
    void setHeader(std::string_view name, std::string_view value);
    void setBody(std::string body);
    void setBody(std::vector<uint8_t> binary_body);

    // Streaming support
    void setStream(std::shared_ptr<IStreamSource> stream);

    // Protocol-specific conversion
    template<typename Protocol>
    typename Protocol::Response to() const;
};
```

**Benefits:**
- Protocol-independent handler code
- Easier migration to different HTTP libraries
- Simpler testing (no Boost.Beast mocks needed)
- Support for multiple protocols simultaneously

---

### Async Handler Support
**Priority:** High
**Target Version:** v1.7.0

Native coroutine support for async I/O in handlers.

**Proposed API:**
```cpp
class AsyncAPIHandler : public IAPIHandler {
public:
    virtual std::coroutine_handle<> handleAsync(
        Request req,
        AuthContext auth,
        std::function<void(Response)> callback
    ) = 0;

    // Or using C++20 coroutines:
    virtual task<Response> handle(Request req, AuthContext auth) = 0;
};
```

**Usage Example:**
```cpp
task<Response> EntityAPIHandler::handle(Request req, AuthContext auth) {
    auto trace = co_await tracer->startSpan("entity.create");

    auto entity = co_await parseRequest(req);
    auto validated = co_await validateEntity(entity);
    auto stored = co_await storage->put(validated);

    trace->end();

    co_return Response::ok(stored.toJSON());
}
```

**Benefits:**
- No blocking I/O in handlers
- Higher concurrency
- Better resource utilization
- Cleaner error handling

---

### Streaming Response Interface
**Priority:** Medium
**Target Version:** v1.8.0

Native streaming support for large responses.

**Proposed Interface:**
```cpp
class IStreamSource {
public:
    virtual Future<std::optional<Chunk>> nextChunk() = 0;
    virtual void cancel() = 0;
    virtual size_t estimatedSize() const = 0;
};

// Usage
class QueryResultStream : public IStreamSource {
public:
    QueryResultStream(std::shared_ptr<QueryCursor> cursor) : cursor_(cursor) {}

    Future<std::optional<Chunk>> nextChunk() override {
        if (auto batch = cursor_->fetchBatch(1000)) {
            return Chunk{serialize(*batch)};
        }
        return std::nullopt;  // EOF
    }
};

// In handler
Response res;
res.setStream(std::make_shared<QueryResultStream>(query_cursor));
return res;
```

**Benefits:**
- Stream large query results without buffering
- Reduced memory usage
- Lower latency to first byte
- Backpressure support

---

### Middleware Chain Interface
**Priority:** Medium
**Target Version:** v1.7.0

Composable middleware for request/response processing.

**Proposed Interface:**
```cpp
class IMiddleware {
public:
    virtual Future<MiddlewareResult> process(
        Request& req,
        Response& res,
        std::function<Future<void>()> next
    ) = 0;
};

enum class MiddlewareResult {
    CONTINUE,      // Continue to next middleware
    SHORT_CIRCUIT  // Stop processing, return response
};

// Middleware chain
class MiddlewareChain {
public:
    void use(std::shared_ptr<IMiddleware> middleware);
    Future<Response> process(Request req);
};
```

**Built-in Middleware:**
- Authentication
- Rate limiting
- Load shedding
- Policy enforcement
- Compression
- CORS
- Request logging
- Response caching

**Usage Example:**
```cpp
MiddlewareChain chain;
chain.use(auth_middleware);
chain.use(rate_limit_middleware);
chain.use(policy_middleware);
chain.use(handler_middleware);

auto response = co_await chain.process(request);
```

---

### WebSocket Interface Improvements
**Priority:** Medium
**Target Version:** v1.7.0

Enhanced WebSocket interface with better lifecycle management.

**Proposed Interface:**
```cpp
class IWebSocketHandler {
public:
    virtual Future<void> onConnect(WebSocketConnection conn) = 0;
    virtual Future<void> onMessage(WebSocketConnection conn, Message msg) = 0;
    virtual Future<void> onClose(WebSocketConnection conn, CloseCode code) = 0;
    virtual Future<void> onError(WebSocketConnection conn, Error error) = 0;
};

class WebSocketConnection {
public:
    Future<void> send(Message msg);
    Future<void> close(CloseCode code = CloseCode::NORMAL);
    bool isOpen() const;
    std::string id() const;
    AuthContext auth() const;
};
```

**Benefits:**
- Clear lifecycle hooks
- Async message sending
- Better error handling
- Connection metadata access

---

### gRPC Interface Abstraction
**Priority:** Low
**Target Version:** v1.8.0

Abstract gRPC-specific types for easier testing.

**Proposed Interface:**
```cpp
template<typename Request, typename Response>
class IRPCHandler {
public:
    virtual Future<Result<Response>> handle(
        const Request& req,
        const RPCContext& ctx
    ) = 0;
};

class RPCContext {
public:
    std::string method() const;
    std::unordered_map<std::string, std::string> metadata() const;
    AuthContext auth() const;
    void setStatus(RPCStatus status);
};
```

---

### Enhanced Rate Limiter Interface
**Priority:** High
**Target Version:** v1.6.0

Unified rate limiter interface supporting multiple algorithms and backends.

**Proposed Interface:**
```cpp
class IRateLimiter {
public:
    enum class Algorithm {
        TOKEN_BUCKET,
        SLIDING_WINDOW,
        LEAKY_BUCKET,
        FIXED_WINDOW
    };

    enum class Backend {
        IN_MEMORY,
        REDIS,
        MEMCACHED
    };

    struct CheckResult {
        bool allowed;
        uint64_t retry_after_ms;
        uint64_t remaining;
        uint64_t limit;
    };

    virtual Future<CheckResult> checkLimit(
        const std::string& key,
        uint64_t cost = 1
    ) = 0;

    virtual Future<void> reset(const std::string& key) = 0;
};

// Factory
class RateLimiterFactory {
public:
    static std::shared_ptr<IRateLimiter> create(
        Algorithm algo,
        Backend backend,
        const Config& config
    );
};
```

**Benefits:**
- Algorithm-agnostic interface
- Pluggable backends
- Consistent API across implementations
- Easy testing with mock implementations

---

### Policy Engine Interface Extensions
**Priority:** Medium
**Target Version:** v1.7.0

Enhanced policy engine with more flexible evaluation.

**Proposed Extensions:**
```cpp
class IPolicyEngine {
public:
    // Existing
    virtual Decision evaluate(const EvalContext& ctx) const = 0;

    // New: Batch evaluation
    virtual std::vector<Decision> evaluateBatch(
        const std::vector<EvalContext>& contexts
    ) const = 0;

    // New: Explain decision
    virtual DecisionExplanation explain(const EvalContext& ctx) const = 0;

    // New: Policy testing
    virtual std::vector<PolicyTestResult> testPolicies(
        const std::vector<EvalContext>& test_cases
    ) const = 0;

    // New: Dynamic policy loading
    virtual Future<void> reloadPolicies() = 0;
};

struct DecisionExplanation {
    Decision decision;
    std::vector<std::string> matched_policies;
    std::vector<std::string> evaluation_steps;
    std::chrono::microseconds evaluation_time;
};
```

---

### Connection Pool Interface
**Priority:** Medium
**Target Version:** v1.7.0

Abstract connection pooling for reuse across protocols.

**Proposed Interface:**
```cpp
template<typename Connection>
class IConnectionPool {
public:
    virtual Future<Connection> acquire() = 0;
    virtual void release(Connection conn) = 0;
    virtual void invalidate(Connection conn) = 0;

    virtual size_t size() const = 0;
    virtual size_t available() const = 0;

    struct Stats {
        size_t total_connections;
        size_t active_connections;
        size_t idle_connections;
        uint64_t total_acquires;
        uint64_t total_releases;
        std::chrono::milliseconds avg_acquire_time;
    };
    virtual Stats getStats() const = 0;
};
```

---

## Performance Optimizations

### Zero-Copy Header Access
**Priority:** High
**Target Version:** v1.6.0

Avoid string copies when accessing HTTP headers.

**Current:**
```cpp
std::string auth_header = request.headers()["Authorization"];
```

**Proposed:**
```cpp
std::string_view auth_header = request.header("Authorization");
```

**Expected Improvement:** 20-30% reduction in header parsing overhead

---

### Compile-Time Handler Registration
**Priority:** Medium
**Target Version:** v1.7.0

Use template metaprogramming for zero-cost handler registration.

**Proposed:**
```cpp
template<typename Handler, const char* Path>
class RegisteredHandler {
    static_assert(std::is_base_of_v<IAPIHandler, Handler>);
    // ... compile-time registration
};

// Usage
constexpr char entity_path[] = "/api/v1/entities";
using EntityHandler = RegisteredHandler<EntityAPIHandler, entity_path>;
```

**Benefits:**
- Zero runtime registration overhead
- Compile-time path validation
- Better code generation

---

### Header-Only Implementations
**Priority:** Low
**Target Version:** v1.8.0

Move small classes to header-only for better inlining.

**Candidates:**
- `AuthContext`
- `RateLimitConfig`
- `LoadShedder::Config`
- Small utility classes

**Expected Improvement:** 5-10% throughput increase via inlining

---

## Refactoring Opportunities

### Separate Protocol Headers
**Priority:** Medium
**Target Version:** v1.8.0

Extract protocol-specific headers into separate directories.

**Current Structure:**
```
include/server/
├── http_server.h
├── http2_session.h
├── websocket_session.h
├── mqtt_session.h
└── ...
```

**Proposed Structure:**
```
include/
├── server/
│   ├── api_handler_interface.h
│   ├── api_gateway.h
│   └── ...
├── protocols/
│   ├── http/
│   │   ├── http_server.h
│   │   ├── http2_session.h
│   │   └── http3_session.h
│   ├── websocket/
│   │   └── websocket_session.h
│   ├── mqtt/
│   │   └── mqtt_session.h
│   └── grpc/
│       └── grpc_service.h
```

**Benefits:**
- Clearer organization
- Optional protocol support
- Better modularity

---

### Handler Traits System
**Priority:** Low
**Target Version:** v1.8.0

Use traits to declare handler capabilities at compile-time.

**Proposed:**
```cpp
template<typename Handler>
struct HandlerTraits {
    static constexpr bool supports_streaming = false;
    static constexpr bool requires_auth = true;
    static constexpr bool cacheable = false;
    static constexpr LoadShedder::Priority priority = LoadShedder::Priority::NORMAL;
};

// Specialization
template<>
struct HandlerTraits<QueryAPIHandler> {
    static constexpr bool supports_streaming = true;
    static constexpr bool requires_auth = true;
    static constexpr bool cacheable = true;
    static constexpr LoadShedder::Priority priority = LoadShedder::Priority::NORMAL;
};
```

**Benefits:**
- Compile-time capability checks
- Zero-cost abstractions
- Better documentation

---

### Concept-Based Handler Validation
**Priority:** Low
**Target Version:** v1.9.0

Use C++20 concepts for handler validation.

**Proposed:**
```cpp
template<typename T>
concept APIHandler = requires(T handler, Request req, AuthContext auth) {
    { handler.handle(req, auth) } -> std::convertible_to<Future<Response>>;
    { handler.getPath() } -> std::convertible_to<std::string>;
    { handler.getSupportedMethods() } -> std::convertible_to<std::vector<http::verb>>;
};

// Usage
template<APIHandler Handler>
void registerHandler(Handler handler) {
    // Compile-time validation
}
```

---

## Known Issues

### Issue #1: Forward Declaration Complexity
**Severity:** Low
**Reported:** v1.5.0

Complex forward declaration chains in headers cause compilation issues.

**Symptoms:**
- Incomplete type errors
- Circular dependency warnings

**Workaround:** Include full definitions
**Fix:** Refactor to reduce interdependencies

**Planned Fix:** v1.7.0 - Header dependency audit and cleanup

---

### Issue #2: Boost.Beast Type Leakage
**Severity:** Medium
**Reported:** v1.5.0

Boost.Beast types exposed in public interfaces.

**Impact:** Difficult to test, hard to migrate to other libraries

**Workaround:** Wrap in internal types
**Fix:** Protocol-agnostic Request/Response types

**Planned Fix:** v1.7.0

---

### Issue #3: Missing noexcept Specifications
**Severity:** Low
**Reported:** v1.5.1

Many interfaces lack noexcept specifications.

**Impact:** Suboptimal code generation, unclear exception guarantees

**Workaround:** None
**Fix:** Audit and add noexcept where appropriate

**Planned Fix:** v1.6.1

---

### Issue #4: Virtual Function Overhead
**Severity:** Low
**Reported:** v1.5.0

Virtual function calls in hot paths add latency.

**Impact:** ~5-10ns per virtual call

**Workaround:** None
**Fix:** Use CRTP or templates for hot paths

**Planned Fix:** v1.8.0

---

### Issue #5: Header Include Bloat
**Severity:** Medium
**Reported:** v1.5.2

Including `http_server.h` pulls in 100+ transitive headers.

**Impact:** Slow compilation times

**Workaround:** Forward declare when possible
**Fix:** Header include cleanup, use pimpl where appropriate

**Planned Fix:** v1.7.0

---

## Research Areas

### Compile-Time API Specification
**Focus:** Generate API code from specifications

**Concept:**
- Define API in DSL or structured format
- Generate handler interfaces at compile-time
- Automatic validation and documentation

**Example:**
```cpp
constexpr auto entity_api = API{
    .path = "/api/v1/entities",
    .methods = {GET, POST, PUT, DELETE},
    .scopes = {"read:entities", "write:entities"},
    .rate_limit = {.capacity = 1000, .rate = 100.0}
};

// Compiler generates handler interface from spec
```

---

### Type-Safe Routing
**Focus:** Compile-time route validation

**Approach:**
- Use template magic for type-safe paths
- Validate path parameters at compile-time
- Generate route table at compile-time

**Example:**
```cpp
Route<"/api/v1/entities/{id}">::Handler auto handler =
    [](int id, AuthContext auth) -> Response {
        // id is guaranteed to be int
    };
```

---

### Header-Only Server
**Priority:** Research
**Focus:** Experimental header-only HTTP server

**Goal:**
- Zero-dependency HTTP server
- Header-only implementation
- Minimal footprint

**Use Cases:**
- Embedded systems
- WASM compilation
- Rapid prototyping

---

## Migration Paths

### v1.5.x → v1.6.x: Rate Limiter Interface
**Breaking Changes:** IRateLimiter interface introduction

**Old Code:**
```cpp
#include "server/rate_limiter.h"
RateLimiter limiter(config);
```

**New Code (v1.6.x):**
```cpp
#include "server/rate_limiter_interface.h"
#include "server/rate_limiter_factory.h"

auto limiter = RateLimiterFactory::create(
    IRateLimiter::Algorithm::TOKEN_BUCKET,
    IRateLimiter::Backend::IN_MEMORY,
    config
);
```

**Migration Steps:**
1. Update includes
2. Use factory instead of direct construction
3. Update tests to use mock interface

**Timeline:** 6 months deprecation period

---

### v1.6.x → v1.7.x: Async Handler Interface
**Breaking Changes:** Handler signatures return Future<Response>

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
1. Update handler signatures
2. Convert blocking I/O to async
3. Use co_await for async operations
4. Run migration tool: `scripts/migrate_handlers_async_v17.sh`

**Benefits:**
- Non-blocking handlers
- Better concurrency
- Easier composition

**Timeline:** 12 months deprecation period

---

### v1.7.x → v1.8.x: Protocol Separation
**Breaking Changes:** Header paths changed

**Old Includes:**
```cpp
#include "server/http_server.h"
#include "server/websocket_session.h"
#include "server/mqtt_session.h"
```

**New Includes (v1.8.x):**
```cpp
#include "protocols/http/http_server.h"
#include "protocols/websocket/websocket_session.h"
#include "protocols/mqtt/mqtt_session.h"
```

**Migration Steps:**
1. Update include paths
2. Update CMakeLists.txt (granular protocol libraries)
3. Rebuild

**Timeline:** 18 months deprecation period (v1.7.x still supports old paths)

---

## Community Contributions Welcome

### High-Impact, Beginner-Friendly
- [ ] Add noexcept specifications to interfaces
- [ ] Improve Doxygen comments
- [ ] Add usage examples to headers
- [ ] Create handler templates for common patterns
- [ ] Document exception safety guarantees

### Medium Complexity
- [ ] Implement protocol-agnostic Request/Response types
- [ ] Create async handler base class
- [ ] Implement streaming response interface
- [ ] Design middleware chain interface
- [ ] Create rate limiter factory

### Advanced Topics
- [ ] Compile-time handler registration system
- [ ] Type-safe routing framework
- [ ] Header-only HTTP server implementation
- [ ] Concept-based interface validation
- [ ] Zero-copy header processing

**Contribution Guide:** See [CONTRIBUTING.md](../../CONTRIBUTING.md)

---

## Feedback and Discussion

Have ideas for server header improvements? Open an issue or discussion:

- 💡 Feature requests: [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- 💬 Design discussions: [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- 🐛 Bug reports: [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)

---

*Last Updated: April 2026*
*Module Version: v1.7.0*
*Next Review: v1.7.0 Release*

## Test Strategy

- Unit tests for `MiddlewareChain` ordering and short-circuit behaviour under mocked middleware
- Integration tests for `IRateLimiter` with in-memory backend verifying per-tenant quota isolation
- SSE push tests confirming non-blocking delivery and correct `Content-Type: text/event-stream` headers
- GraphQL schema generation tests validating output against known data model headers
- gRPC bridge smoke tests using a loopback channel with `RPCContext` metadata round-trip assertions
- Load tests verifying rate limiter rejects excess requests at ≥ 10,000 req/s saturation

## Performance Targets

- Middleware dispatch overhead ≤ 5 µs per hop (p99)
- SSE push latency ≤ 1 ms from event emit to wire delivery
- GraphQL schema generation ≤ 100 ms at server startup
- Rate limiter admission check ≤ 10 µs per request (in-memory backend)
- gRPC bridge handler dispatch ≤ 20 µs overhead vs direct handler call

## Security / Reliability

- All server API endpoints require authentication by default — opt-out requires explicit annotation
- Rate limiter prevents DoS by enforcing per-connection and per-tenant request budgets
- CORS policy enforced at the API gateway level before handler dispatch
- SSE connections authenticated at upgrade time; token re-validated on reconnect
- Middleware chain failures result in 500 responses with no partial or unauthenticated output

---

## Paper 2 — Layer 8: WorkloadFingerprintEngine (IMPL-B8)

> Research paper: `docs/en/research/LLM_OPTIMIZATION_LAYERS_MATRIX.md` §Layer 8
> Issue: `docs/issues/optimization_layers/IMPL-B8-workload-fingerprint.md`

### Scope
- Per-tenant workload fingerprinting based on rolling query-window metrics (QPS, read/write ratio, vector fraction)
- `WorkloadPattern`: `OLTP_WRITE_HEAVY`, `OLTP_READ_HEAVY`, `ANALYTICAL`, `VECTOR_SEARCH`, `MIXED`, `BURST_INGEST`, `UNKNOWN`
- `fingerprintHash()` — 64-bit deterministic hash for cross-shard Jaccard similarity comparison

### Integration Notes
- `SmartRouter` uses fingerprint to select optimal shard for new tenant sessions
- `distributed_knowledge` Layer 11D `CrossShardFeedbackSync` propagates fingerprint changes across shards
- `AIDecisionAuditor` receives `DecisionRecord` on dominant-pattern change
- GDPR: fingerprint contains only metrics; no query content

### Performance Targets
- Fingerprint computation ≤ 1 ms for 1 000-query sliding window
- Cross-shard Jaccard distance: identical workloads → 0.0; orthogonal workloads → 1.0
