# HTTP Server Refactoring - Implementation Action Plan
**Date:** 2026-01-13  
**Status:** Draft  
**Priority:** High (Type mismatch prevents compilation)  
**Estimated Effort:** 4-6 weeks (large refactoring)

## Problem Statement

Based on the review in `HTTP_SERVER_REFACTORING_REVIEW.md`, the HTTP server refactoring to cpp-httplib was **not completed**. The codebase has incompatible code:

1. **Critical:** New handlers (DiffApiHandler, SnapshotApiHandler) expect `httplib::Request/Response` types
2. **Critical:** Main server calls these handlers with Beast `http::request/response` types
3. **Critical:** cpp-httplib is not in dependencies and headers don't exist
4. **Issue:** This code **cannot compile** as written

## Immediate Actions (Week 1)

### Action 1.1: Verify Compilation Status
**Priority:** Critical  
**Owner:** TBD  
**Effort:** 1 day

**Tasks:**
- [ ] Attempt to compile the project
- [ ] Document all compilation errors related to type mismatch
- [ ] Check if there's a workaround currently in place
- [ ] Verify which handlers are affected

**Commands:**
```bash
mkdir -p build
cd build
cmake ..
cmake --build . 2>&1 | tee build_errors.log
```

### Action 1.2: Decision Gate
**Priority:** Critical  
**Owner:** Project Lead  
**Effort:** 2 days

Choose one of three paths:

#### **Option A: Complete cpp-httplib Migration** (Recommended)
- ✅ Aligns with original plan
- ✅ Modern, lightweight HTTP library
- ✅ Better separation of concerns
- ⚠️ Requires 4-6 weeks
- ⚠️ Breaking change for existing code

#### **Option B: Revert to Beast Only**
- ✅ Quick fix (1 week)
- ✅ Maintains consistency
- ❌ Keeps monolithic 7,410 LOC file
- ❌ Misses modernization benefits

#### **Option C: Create Adapter Bridge**
- ⚠️ Medium term (2-3 weeks)
- ✅ Allows incremental migration
- ❌ Adds complexity
- ❌ Technical debt remains

**Deliverable:** Decision document with chosen option

### Action 1.3: Create Type Adapter (Quick Fix)
**Priority:** Critical  
**Owner:** Backend Developer  
**Effort:** 2 days

**Immediate fix to restore compilation:**

Create adapter functions to bridge Beast ↔ cpp-httplib types:

```cpp
// src/server/http_type_adapter.h
#pragma once

#include <boost/beast.hpp>
#include <httplib.h>

namespace themis::server {

/**
 * Temporary adapter to bridge Beast and cpp-httplib types
 * TODO: Remove after full migration to cpp-httplib
 */
class HttpTypeAdapter {
public:
    // Convert Beast request to cpp-httplib request
    static httplib::Request beastToHttplib(
        const boost::beast::http::request<boost::beast::http::string_body>& beast_req
    );
    
    // Convert cpp-httplib response to Beast response
    static boost::beast::http::response<boost::beast::http::string_body> httplibToBeast(
        const httplib::Response& httplib_res
    );
};

} // namespace themis::server
```

**Tasks:**
- [ ] Create `http_type_adapter.h` and `.cpp`
- [ ] Implement conversion functions
- [ ] Update main server to use adapter when calling new handlers
- [ ] Add unit tests for adapter
- [ ] Document as temporary solution

**Modified Code:**
```cpp
// In http_server.cpp
case Route::SnapshotsTagsPost:
    if (snapshot_api_handler_) {
        auto httplib_req = HttpTypeAdapter::beastToHttplib(req);
        httplib::Response httplib_res;
        snapshot_api_handler_->handleCreateTag(httplib_req, httplib_res);
        response = HttpTypeAdapter::httplibToBeast(httplib_res);
    }
    break;
```

## Phase 1: Foundation (Week 2-3) - If Option A Chosen

### Action 2.1: Add cpp-httplib Dependency
**Priority:** High  
**Effort:** 1 day

**Tasks:**
- [ ] Add cpp-httplib to `vcpkg.json`:
  ```json
  { "name": "cpp-httplib" }
  ```
- [ ] Run `vcpkg install cpp-httplib`
- [ ] Update CMakeLists.txt to link cpp-httplib
- [ ] Verify headers are accessible

### Action 2.2: Create Directory Structure
**Priority:** High  
**Effort:** 1 day

**Tasks:**
- [ ] Create `src/server/core/` directory
- [ ] Create `src/server/middleware/` directory
- [ ] Create `src/server/routing/` directory
- [ ] Create `src/server/protocols/` directory
- [ ] Create `src/server/handlers/` subdirectories
- [ ] Update CMakeLists.txt to include new directories

```bash
mkdir -p src/server/{core,middleware,routing,protocols,handlers}
mkdir -p src/server/handlers/{health,crud,query,transaction,mvcc}
mkdir -p src/server/protocols/{http1,http2,http3,grpc}
```

### Action 2.3: Implement Core Components
**Priority:** High  
**Effort:** 5 days

**2.3.1: ServerContext**
```cpp
// src/server/core/server_context.h
namespace themis::server {
class ServerContext {
    // Shared resources (DB, managers, config)
    std::shared_ptr<RocksDBWrapper> db_;
    std::shared_ptr<SecondaryIndexManager> secondary_index_;
    // ... other managers
};
}
```

**2.3.2: RequestContext**
```cpp
// src/server/core/request_context.h
class RequestContext {
    const httplib::Request& req_;
    httplib::Response& res_;
    std::shared_ptr<ServerContext> server_ctx_;
    // Timing, attributes, auth info
};
```

**2.3.3: HandlerBase**
```cpp
// src/server/handlers/handler_base.h
class HandlerBase {
    virtual void registerRoutes(httplib::Server& server) = 0;
    virtual void processRequest(RequestContext& ctx) = 0;
};
```

**Tasks:**
- [ ] Implement ServerContext
- [ ] Implement RequestContext
- [ ] Implement HandlerBase interface
- [ ] Add unit tests

## Phase 2: Middleware Layer (Week 4)

### Action 3.1: Implement Middleware Architecture
**Priority:** High  
**Effort:** 5 days

**3.1.1: Middleware Base**
```cpp
// src/server/middleware/middleware_base.h
class MiddlewareBase {
    virtual bool process(
        RequestContext& ctx,
        std::function<void()> next
    ) = 0;
};
```

**3.1.2: Middleware Chain**
```cpp
// src/server/middleware/middleware_chain.h
class MiddlewareChain {
    void add(std::shared_ptr<MiddlewareBase> middleware);
    void execute(RequestContext& ctx, std::function<void()> handler);
};
```

**3.1.3: Core Middlewares**
- [ ] LoggingMiddleware
- [ ] AuthMiddleware (refactor existing)
- [ ] CorsMiddleware
- [ ] RateLimitMiddleware
- [ ] CompressionMiddleware
- [ ] ValidationMiddleware

**Tasks:**
- [ ] Implement middleware base and chain
- [ ] Migrate existing auth_middleware to new pattern
- [ ] Implement missing middlewares
- [ ] Add unit tests for each middleware

## Phase 3: Routing System (Week 5)

### Action 4.1: Implement Router
**Priority:** High  
**Effort:** 5 days

**4.1.1: Route Matcher**
```cpp
// src/server/routing/route_matcher.h
class RouteMatcher {
    bool match(const std::string& path, httplib::Params& params);
};
```

**4.1.2: Router**
```cpp
// src/server/routing/router.h
class Router {
    void addRoute(
        const std::string& method,
        const std::string& pattern,
        RouteHandler handler
    );
    bool route(const httplib::Request& req, httplib::Response& res);
};
```

**Tasks:**
- [ ] Implement path pattern matching with parameters
- [ ] Implement route registration system
- [ ] Implement router dispatcher
- [ ] Add comprehensive tests

## Phase 4: Migrate Main Server (Week 6)

### Action 5.1: Refactor http_server.cpp
**Priority:** Critical  
**Effort:** 7 days

**Goal:** Reduce from 7,410 LOC to < 500 LOC

**5.1.1: Create New Server Class**
```cpp
// src/server/http_server.cpp (NEW)
namespace themis::server {

HttpServer::HttpServer(const Config& config)
    : config_(config)
    , server_(std::make_unique<httplib::Server>())
    , context_(std::make_shared<ServerContext>(config))
    , router_(std::make_unique<Router>(context_))
{
    initializeMiddleware();
    initializeHandlers();
    configureServer();
}

void HttpServer::initializeHandlers() {
    // Register all handlers
    auto health = std::make_shared<HealthHandler>(context_);
    health->registerRoutes(*server_);
    
    auto diff = std::make_shared<DiffApiHandler>(context_->diff_engine());
    diff->registerRoutes(*server_);
    
    // ... register all handlers
}

bool HttpServer::start() {
    return server_->listen(config_.host, config_.port);
}

} // namespace themis::server
```

**5.1.2: Migration Strategy**
- [ ] Extract handler logic from http_server.cpp
- [ ] Move each endpoint to appropriate handler class
- [ ] Update handlers to use new HandlerBase interface
- [ ] Register handlers with cpp-httplib server
- [ ] Remove old Beast-based code

**5.1.3: Handler Migration Checklist**
- [ ] HealthHandler (~300 LOC)
- [ ] EntityApiHandler (existing, update interface)
- [ ] QueryHandler (~1500 LOC)
- [ ] TransactionHandler (~1000 LOC)
- [ ] VectorApiHandler (existing, update interface)
- [ ] GraphApiHandler (existing, update interface)
- [ ] ContentApiHandler (existing, update interface)
- [ ] ChangefeedApiHandler (existing, update interface)
- [ ] DiffApiHandler (already done, remove adapter)
- [ ] SnapshotApiHandler (already done, remove adapter)
- [ ] ... (26 more handlers)

## Phase 5: Protocol Adapter Layer (Week 7-8)

### Action 6.1: Implement Protocol Adapters
**Priority:** Medium  
**Effort:** 10 days

**6.1.1: Protocol Adapter Interface**
```cpp
// src/server/protocols/protocol_adapter.h
class ProtocolAdapter {
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual Protocol getProtocol() const = 0;
};
```

**6.1.2: HTTP/1.1 Adapter**
```cpp
// src/server/protocols/http1/http1_adapter.h
class Http1Adapter : public ProtocolAdapter {
    std::unique_ptr<httplib::Server> server_;
};
```

**6.1.3: HTTP/2 Adapter**
- Wrap existing `http2_session.cpp` with adapter
- Use nghttp2 library

**6.1.4: HTTP/3 Adapter**
- Wrap existing `http3_session.cpp` with adapter
- Use ngtcp2/nghttp3 libraries

**6.1.5: gRPC Adapter**
- Wrap existing gRPC services with adapter

**Tasks:**
- [ ] Implement protocol adapter base
- [ ] Implement HTTP/1.1 adapter
- [ ] Refactor HTTP/2 session into adapter
- [ ] Refactor HTTP/3 session into adapter
- [ ] Refactor gRPC into adapter
- [ ] Create multi-protocol orchestrator

## Phase 6: Testing & Validation (Week 9)

### Action 7.1: Integration Testing
**Priority:** Critical  
**Effort:** 5 days

**Tasks:**
- [ ] Verify all existing endpoints still work
- [ ] Test all HTTP methods (GET, POST, PUT, DELETE)
- [ ] Test authentication and authorization
- [ ] Test middleware chain
- [ ] Test error handling
- [ ] Test concurrent requests
- [ ] Test large payloads
- [ ] Test timeout handling

### Action 7.2: Performance Benchmarking
**Priority:** High  
**Effort:** 3 days

**Metrics to Compare:**
- Request throughput (req/sec)
- Latency (p50, p95, p99)
- Memory usage
- CPU usage
- Connection handling

**Acceptance Criteria:**
- ✅ Performance regression < 5%
- ✅ All tests passing (95%+)
- ✅ No memory leaks
- ✅ No crashes under load

### Action 7.3: Documentation
**Priority:** High  
**Effort:** 2 days

**Tasks:**
- [ ] Update API documentation
- [ ] Document new architecture
- [ ] Create migration guide for developers
- [ ] Update README with new dependencies
- [ ] Document middleware system
- [ ] Document routing system
- [ ] Add code examples

## Success Criteria

- [x] Code compiles without errors
- [ ] http_server.cpp < 500 LOC
- [ ] All handlers in separate files (< 500 LOC each)
- [ ] Middleware chain implemented and working
- [ ] Router with pattern matching working
- [ ] All existing endpoints functional
- [ ] Performance regression < 5%
- [ ] All tests passing (95%+)
- [ ] Documentation complete
- [ ] Code review approved

## Risk Mitigation

### Risk 1: Breaking Changes
**Mitigation:** 
- Create feature branch
- Extensive testing before merge
- Gradual rollout with monitoring

### Risk 2: Performance Regression
**Mitigation:**
- Benchmark at each phase
- Profile and optimize hot paths
- Load test before production

### Risk 3: Timeline Overrun
**Mitigation:**
- Start with critical path items
- Parallel workstreams where possible
- Regular progress reviews

## Rollback Plan

If issues arise after deployment:
1. Keep old Beast-based code in separate branch
2. Have feature flag to switch between implementations
3. Monitor error rates and performance metrics
4. Be ready to rollback within 1 hour

## Appendix A: File Size Targets

| Component | Current | Target | Status |
|-----------|---------|--------|--------|
| http_server.cpp | 7,410 | < 500 | ❌ |
| Handler files | Mixed | < 500 each | ⚠️ |
| Middleware files | 1 | 6+ | ❌ |
| Router files | 0 | 4 | ❌ |
| Protocol adapters | 0 | 4+ | ❌ |

## Appendix B: Dependencies

### Add to vcpkg.json:
```json
{
  "name": "cpp-httplib"
}
```

### Update CMakeLists.txt:
```cmake
find_package(httplib CONFIG REQUIRED)
target_link_libraries(themis-server PRIVATE httplib::httplib)
```

---

**Document Version:** 1.0  
**Last Updated:** 2026-04-06  
**Next Review:** After Action 1.2 decision
