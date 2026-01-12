---
name: HTTP Server Refactoring - Migrate to cpp-httplib
about: Refactor HTTP server from Boost.Beast to cpp-httplib and split http_server.cpp
title: "[REFACTORING] Migrate HTTP Server to cpp-httplib and Modularize"
labels: type:refactoring, area:api, priority:P2, effort:x-large, technical-debt
assignees: ''
---

## Beschreibung / Description

**Deutsch:**
Refaktorierung des HTTP-Servers von Boost.Beast zu cpp-httplib und Aufteilung der großen `http_server.cpp` Datei (aktuell ~12.500 Zeilen) in kleinere, wartbare Module nach Best Practices.

**English:**
Refactor the HTTP server from Boost.Beast to cpp-httplib and split the large `http_server.cpp` file (currently ~12,500 lines) into smaller, maintainable modules following best practices.

## Motivation

1. **Library-Inkompatibilität**: Neue API-Handler nutzen cpp-httplib, Server nutzt Boost.Beast
2. **Code-Wartbarkeit**: 12.482 Zeilen sind schwer zu warten und testen
3. **Best Practices**: Moderne, robuste HTTP-Server-Architektur implementieren
4. **Separation of Concerns**: Klare Trennung der Verantwortlichkeiten

---

# 🏗️ Best Practice Design für Robusten HTTP-Server

## Architektur-Prinzipien

### 1. Multi-Protocol Layered Architecture (Mehrschichtige Multi-Protokoll-Architektur)

```
┌───────────────────────────────────────────────────────────────────────────────────────┐
│                        Protocol Abstraction Layer                                     │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────────┐    │
│  │ HTTP/1.1   │  │ HTTP/2     │  │ HTTP/3     │  │   gRPC     │  │   MCP      │    │
│  │ (cpp-      │  │ (nghttp2)  │  │ (nghttp3   │  │(protobuf)  │  │(model ctx) │    │
│  │  httplib)  │  │  + ALPN    │  │ + ngtcp2)  │  │            │  │            │    │
│  └─────┬──────┘  └─────┬──────┘  └─────┬──────┘  └─────┬──────┘  └─────┬──────┘    │
│        │               │               │               │               │            │
│        └───────────────┴───────────────┴───────────────┴───────────────┘            │
│                                        │                                             │
└────────────────────────────────────────┼─────────────────────────────────────────────┘
                                         │
┌────────────────────────────────────────▼─────────────────────────────────────────────┐
│                        Protocol Adapter Layer (NEW)                                   │
│  • Unified Request/Response Interface                                                 │
│  • Protocol-specific handling (HTTP/2 streams, gRPC bidirectional, HTTP/3 QUIC)     │
│  • Header normalization across protocols                                             │
│  • Connection multiplexing and pooling                                               │
│  • Protocol-specific features (Server Push, 0-RTT, Streaming)                       │
└────────────────────────────────────────┬─────────────────────────────────────────────┘
                                         │
┌────────────────────────────────────────▼─────────────────────────────────────────────┐
│                           Middleware Layer                                            │
│  • Authentication & Authorization                                                     │
│  • Rate Limiting & Throttling (per-protocol adaptive)                                │
│  • CORS, Compression (Brotli for HTTP/2+, gzip fallback)                            │
│  • Request Validation & Sanitization                                                 │
│  • Protocol-aware logging (include protocol version, stream IDs)                     │
└────────────────────────────────────────┬─────────────────────────────────────────────┘
                                         │
┌────────────────────────────────────────▼─────────────────────────────────────────────┐
│                            Routing Layer                                              │
│  • Unified route registration for all protocols                                      │
│  • Path matching & parameter extraction                                              │
│  • Method routing (GET/POST/PUT/DELETE for HTTP, RPC methods for gRPC)              │
│  • Protocol-specific route optimization                                              │
└────────────────────────────────────────┬─────────────────────────────────────────────┘
                                         │
┌────────────────────────────────────────▼─────────────────────────────────────────────┐
│                            Handler Layer                                              │
│  • Protocol-agnostic business logic                                                  │
│  • Request processing using unified interface                                        │
│  • Response formatting (JSON, Protobuf, MessagePack)                                 │
└────────────────────────────────────────┬─────────────────────────────────────────────┘
                                         │
┌────────────────────────────────────────▼─────────────────────────────────────────────┐
│                            Service Layer                                              │
│  • Business services (CRUD, Query, Transaction)                                      │
│  • Domain logic                                                                      │
│  • Transaction & MVCC management                                                     │
└────────────────────────────────────────┬─────────────────────────────────────────────┘
                                         │
┌────────────────────────────────────────▼─────────────────────────────────────────────┐
│                         Data Access Layer                                             │
│  • RocksDB access                                                                    │
│  • Changefeed & MVCC                                                                 │
│  • Cache management                                                                  │
│  • Index operations                                                                  │
└───────────────────────────────────────────────────────────────────────────────────────┘
```

### Unterstützte Protokolle

| Protokoll | Status | Use Case | Library | Port |
|-----------|--------|----------|---------|------|
| **HTTP/1.1** | ✅ Production | REST API, Debugging | cpp-httplib | 8080 |
| **HTTP/2** | ✅ Production | Multiplexing, Server Push, CDC | nghttp2 + ALPN | 8443 (TLS) |
| **HTTP/3** | 🧪 Experimental | Low latency, Mobile, 0-RTT | nghttp3 + ngtcp2 | 8443 (QUIC/UDP) |
| **gRPC** | ✅ Production | Inter-shard, High throughput | gRPC (protobuf) | 50051 |
| **MCP** | 🚧 Planned | LLM context, Model serving | Custom/SSE | 8080 (HTTP/1.1) |

---

## 📁 Vorgeschlagene Verzeichnisstruktur (Multi-Protokoll)

```
src/server/
├── http_server.h                      # Main unified server interface
├── http_server.cpp                    # Main server orchestrator (300-500 LOC)
│
├── protocols/                         # Protocol-specific implementations
│   ├── protocol_adapter.h             # Unified protocol adapter interface
│   │
│   ├── http1/                         # HTTP/1.1 (cpp-httplib)
│   │   ├── http1_server.h/.cpp        # HTTP/1.1 server
│   │   └── http1_adapter.h/.cpp       # HTTP/1.1 → Unified adapter
│   │
│   ├── http2/                         # HTTP/2 (nghttp2)
│   │   ├── http2_session.h/.cpp       # HTTP/2 session handler (existing)
│   │   ├── http2_server.h/.cpp        # HTTP/2 server wrapper
│   │   ├── http2_adapter.h/.cpp       # HTTP/2 → Unified adapter
│   │   └── http2_push.h/.cpp          # Server Push für CDC
│   │
│   ├── http3/                         # HTTP/3 (nghttp3 + ngtcp2)
│   │   ├── http3_session.h/.cpp       # HTTP/3 session handler (existing)
│   │   ├── http3_server.h/.cpp        # HTTP/3/QUIC server wrapper
│   │   ├── http3_adapter.h/.cpp       # HTTP/3 → Unified adapter
│   │   └── quic_connection.h/.cpp     # QUIC connection management
│   │
│   ├── grpc/                          # gRPC (existing)
│   │   ├── grpc_service_impl.h/.cpp   # gRPC service implementations
│   │   ├── grpc_adapter.h/.cpp        # gRPC → Unified adapter
│   │   └── streaming.h/.cpp           # Bidirectional streaming
│   │
│   └── mcp/                           # Model Context Protocol
│       ├── mcp_server.h/.cpp          # MCP server (existing stub)
│       ├── mcp_adapter.h/.cpp         # MCP → Unified adapter
│       └── sse_stream.h/.cpp          # Server-Sent Events
│
├── core/                              # Core server components
│   ├── server_config.h/.cpp           # Configuration management
│   ├── server_context.h/.cpp          # Shared server context
│   ├── request_context.h/.cpp         # Protocol-agnostic request context
│   ├── response_builder.h/.cpp        # Protocol-agnostic response builder
│   ├── error_handler.h/.cpp           # Centralized error handling
│   └── connection_pool.h/.cpp         # Multi-protocol connection pooling
│
├── middleware/                        # Protocol-agnostic middleware
│   ├── middleware_base.h              # Base middleware interface
│   ├── auth_middleware.h/.cpp         # Authentication
│   ├── cors_middleware.h/.cpp         # CORS handling
│   ├── compression_middleware.h/.cpp  # Brotli/Gzip compression (adaptive)
│   ├── rate_limit_middleware.h/.cpp   # Rate limiting (per-protocol adaptive)
│   ├── logging_middleware.h/.cpp      # Protocol-aware logging
│   ├── validation_middleware.h/.cpp   # Input validation
│   ├── cache_middleware.h/.cpp        # Response caching
│   └── middleware_chain.h/.cpp        # Middleware chain executor
│
├── routing/                           # Routing system
│   ├── router.h/.cpp                  # Main router
│   ├── route.h/.cpp                   # Route definition
│   ├── route_matcher.h/.cpp           # Path matching logic
│   ├── route_registry.h/.cpp          # Route registration
│   └── path_params.h/.cpp             # Path parameter extraction
│
├── handlers/                          # Request handlers
│   ├── handler_base.h                 # Base handler interface
│   ├── handler_factory.h/.cpp         # Handler factory
│   │
│   ├── health/                        # Health & monitoring
│   │   ├── health_handler.h/.cpp
│   │   ├── metrics_handler.h/.cpp
│   │   └── status_handler.h/.cpp
│   │
│   ├── crud/                          # CRUD operations
│   │   ├── get_handler.h/.cpp
│   │   ├── post_handler.h/.cpp
│   │   ├── put_handler.h/.cpp
│   │   ├── delete_handler.h/.cpp
│   │   └── batch_handler.h/.cpp
│   │
│   ├── query/                         # Query & search
│   │   ├── aql_handler.h/.cpp
│   │   ├── sql_handler.h/.cpp
│   │   ├── vector_search_handler.h/.cpp
│   │   └── fulltext_search_handler.h/.cpp
│   │
│   ├── transaction/                   # Transaction management
│   │   ├── transaction_handler.h/.cpp
│   │   ├── mvcc_handler.h/.cpp
│   │   └── changefeed_handler.h/.cpp
│   │
│   ├── index/                         # Index management
│   │   ├── index_handler.h/.cpp
│   │   ├── secondary_index_handler.h/.cpp
│   │   └── vector_index_handler.h/.cpp
│   │
│   ├── graph/                         # Graph operations
│   │   ├── graph_handler.h/.cpp
│   │   ├── traversal_handler.h/.cpp
│   │   └── shortest_path_handler.h/.cpp
│   │
│   ├── geo/                           # Geospatial
│   │   ├── geo_handler.h/.cpp
│   │   ├── spatial_query_handler.h/.cpp
│   │   └── geocoding_handler.h/.cpp
│   │
│   ├── content/                       # Content processing
│   │   ├── content_handler.h/.cpp
│   │   ├── upload_handler.h/.cpp
│   │   └── processing_handler.h/.cpp
│   │
│   ├── voice/                         # Voice features
│   │   ├── voice_handler.h/.cpp
│   │   ├── stt_handler.h/.cpp
│   │   └── tts_handler.h/.cpp
│   │
│   ├── mvcc/                          # MVCC features (Phase 1 & 2) ✨
│   │   ├── diff_handler.h/.cpp        # Diff API
│   │   ├── snapshot_handler.h/.cpp    # Named Snapshots
│   │   └── pitr_handler.h/.cpp        # Point-in-time recovery (future)
│   │
│   └── admin/                         # Admin operations
│       ├── admin_handler.h/.cpp
│       ├── backup_handler.h/.cpp
│       └── config_handler.h/.cpp
│
├── services/                          # Business services
│   ├── crud_service.h/.cpp
│   ├── query_service.h/.cpp
│   ├── transaction_service.h/.cpp
│   └── index_service.h/.cpp
│
└── utils/                             # Server utilities
    ├── json_helper.h/.cpp             # JSON utilities
    ├── url_parser.h/.cpp              # URL parsing
    ├── content_negotiation.h/.cpp     # Content-type negotiation
    └── async_executor.h/.cpp          # Async task execution
```

---

## 💎 Best Practice Code Examples

### 1. Handler Base Class (Interface)

```cpp
// handlers/handler_base.h
#pragma once
#include <httplib.h>
#include <memory>
#include <string>

namespace themis::server {

// Forward declarations
class ServerContext;
class RequestContext;

/**
 * Base interface for all HTTP request handlers.
 * Implements Template Method pattern for common request handling logic.
 */
class HandlerBase {
public:
    explicit HandlerBase(std::shared_ptr<ServerContext> context);
    virtual ~HandlerBase() = default;

    // Delete copy/move (handlers are not copyable)
    HandlerBase(const HandlerBase&) = delete;
    HandlerBase& operator=(const HandlerBase&) = delete;

    /**
     * Register all routes handled by this handler.
     * Called during server initialization.
     */
    virtual void registerRoutes(httplib::Server& server) = 0;

protected:
    /**
     * Template method for handling requests with common pre/post processing.
     */
    void handleRequest(
        const httplib::Request& req,
        httplib::Response& res,
        std::function<void(RequestContext&)> handler
    );

    /**
     * Validate request (authentication, authorization, input validation).
     * Override for custom validation logic.
     */
    virtual bool validateRequest(RequestContext& ctx);

    /**
     * Process the actual business logic.
     * Subclasses must implement this.
     */
    virtual void processRequest(RequestContext& ctx) = 0;

    /**
     * Format and send the response.
     * Override for custom response formatting.
     */
    virtual void sendResponse(RequestContext& ctx, httplib::Response& res);

    /**
     * Handle errors and send appropriate error responses.
     */
    virtual void handleError(
        RequestContext& ctx,
        httplib::Response& res,
        const std::exception& e
    );

    // Helper methods
    std::string getPathParam(const httplib::Request& req, const std::string& key);
    std::string getQueryParam(const httplib::Request& req, const std::string& key);
    bool hasQueryParam(const httplib::Request& req, const std::string& key);
    nlohmann::json parseJsonBody(const httplib::Request& req);

    // Shared context (DB, services, config, etc.)
    std::shared_ptr<ServerContext> context_;
};

} // namespace themis::server
```

### 2. Request Context (Per-Request State)

```cpp
// core/request_context.h
#pragma once
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <chrono>

namespace themis::server {

/**
 * Request-scoped context carrying state throughout request lifecycle.
 * RAII-style resource management for per-request resources.
 */
class RequestContext {
public:
    RequestContext(
        const httplib::Request& req,
        httplib::Response& res,
        std::shared_ptr<ServerContext> server_ctx
    );
    ~RequestContext();

    // Request information
    const httplib::Request& request() const { return req_; }
    httplib::Response& response() { return res_; }
    
    // Path & query parameters
    void setPathParams(httplib::Params params) { path_params_ = std::move(params); }
    std::string getPathParam(const std::string& key) const;
    std::string getQueryParam(const std::string& key) const;
    
    // Request body
    void setRequestBody(nlohmann::json body) { request_body_ = std::move(body); }
    const nlohmann::json& requestBody() const { return request_body_; }
    
    // Response
    void setResponseBody(nlohmann::json body) { response_body_ = std::move(body); }
    const nlohmann::json& responseBody() const { return response_body_; }
    void setStatusCode(int code) { status_code_ = code; }
    int statusCode() const { return status_code_; }
    
    // Error handling
    void setError(const std::string& error, int code = 500);
    bool hasError() const { return has_error_; }
    std::string errorMessage() const { return error_message_; }
    
    // Authentication
    void setUserId(const std::string& user_id) { user_id_ = user_id; }
    std::string userId() const { return user_id_; }
    void setAuthToken(const std::string& token) { auth_token_ = token; }
    bool isAuthenticated() const { return !user_id_.empty(); }
    
    // Custom attributes (middleware can store data here)
    void setAttribute(const std::string& key, std::string value);
    std::string getAttribute(const std::string& key) const;
    bool hasAttribute(const std::string& key) const;
    
    // Timing & metrics
    auto startTime() const { return start_time_; }
    std::chrono::milliseconds elapsedTime() const;
    
    // Server context access
    ServerContext& serverContext() { return *server_context_; }

private:
    const httplib::Request& req_;
    httplib::Response& res_;
    std::shared_ptr<ServerContext> server_context_;
    
    httplib::Params path_params_;
    nlohmann::json request_body_;
    nlohmann::json response_body_;
    int status_code_ = 200;
    
    bool has_error_ = false;
    std::string error_message_;
    
    std::string user_id_;
    std::string auth_token_;
    
    std::unordered_map<std::string, std::string> attributes_;
    std::chrono::steady_clock::time_point start_time_;
};

} // namespace themis::server
```

### 3. Middleware Chain Pattern

```cpp
// middleware/middleware_base.h
#pragma once
#include "core/request_context.h"
#include <functional>
#include <memory>

namespace themis::server {

/**
 * Base interface for middleware components.
 * Middleware can inspect/modify requests before they reach handlers
 * and responses before they're sent to clients.
 */
class MiddlewareBase {
public:
    virtual ~MiddlewareBase() = default;
    
    /**
     * Process the request/response.
     * @param ctx Request context
     * @param next Callback to invoke next middleware/handler
     * @return true if request should continue, false to short-circuit
     */
    virtual bool process(
        RequestContext& ctx,
        std::function<void()> next
    ) = 0;
    
    /**
     * Get middleware name for logging/debugging.
     */
    virtual std::string name() const = 0;
};

/**
 * Middleware chain executor.
 * Implements Chain of Responsibility pattern.
 */
class MiddlewareChain {
public:
    void add(std::shared_ptr<MiddlewareBase> middleware);
    void execute(RequestContext& ctx, std::function<void()> final_handler);
    
private:
    std::vector<std::shared_ptr<MiddlewareBase>> middlewares_;
    
    void executeNext(
        RequestContext& ctx,
        size_t index,
        std::function<void()> final_handler
    );
};

} // namespace themis::server
```

### 4. Example: Authentication Middleware

```cpp
// middleware/auth_middleware.cpp
#include "middleware/auth_middleware.h"
#include "utils/jwt_validator.h"
#include "utils/logger.h"

namespace themis::server {

bool AuthMiddleware::process(RequestContext& ctx, std::function<void()> next) {
    // Skip auth for public endpoints
    if (isPublicEndpoint(ctx.request().path)) {
        next();
        return true;
    }
    
    // Extract token from Authorization header
    auto auth_header = ctx.request().get_header_value("Authorization");
    if (auth_header.empty()) {
        ctx.setError("Missing Authorization header", 401);
        return false;
    }
    
    // Validate Bearer token
    if (auth_header.substr(0, 7) != "Bearer ") {
        ctx.setError("Invalid Authorization format", 401);
        return false;
    }
    
    std::string token = auth_header.substr(7);
    ctx.setAuthToken(token);
    
    // Validate JWT
    try {
        auto claims = jwt_validator_->validate(token);
        ctx.setUserId(claims["sub"]);
        ctx.setAttribute("user_role", claims["role"]);
        
        TRACE("User authenticated: {}", ctx.userId());
        next();
        return true;
        
    } catch (const std::exception& e) {
        ERROR("JWT validation failed: {}", e.what());
        ctx.setError("Invalid or expired token", 401);
        return false;
    }
}

bool AuthMiddleware::isPublicEndpoint(const std::string& path) {
    static const std::vector<std::string> public_paths = {
        "/api/v1/health",
        "/api/v1/status",
        "/api/v1/metrics",
        "/api/v1/auth/login",
        "/api/v1/auth/register"
    };
    
    return std::find(public_paths.begin(), public_paths.end(), path) 
           != public_paths.end();
}

} // namespace themis::server
```

### 5. Example: Rate Limiting Middleware

```cpp
// middleware/rate_limit_middleware.cpp
#include "middleware/rate_limit_middleware.h"
#include <chrono>

namespace themis::server {

bool RateLimitMiddleware::process(RequestContext& ctx, std::function<void()> next) {
    std::string client_id = getClientIdentifier(ctx);
    
    // Check rate limit
    std::lock_guard<std::mutex> lock(mutex_);
    auto& bucket = buckets_[client_id];
    auto now = std::chrono::steady_clock::now();
    
    // Token bucket algorithm
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        now - bucket.last_refill
    ).count();
    
    bucket.tokens = std::min(
        max_tokens_,
        bucket.tokens + elapsed * refill_rate_
    );
    bucket.last_refill = now;
    
    if (bucket.tokens < 1.0) {
        // Rate limit exceeded
        ctx.setError("Rate limit exceeded. Try again later.", 429);
        ctx.response().set_header("Retry-After", "60");
        ctx.response().set_header("X-RateLimit-Limit", std::to_string(max_tokens_));
        ctx.response().set_header("X-RateLimit-Remaining", "0");
        return false;
    }
    
    // Consume one token
    bucket.tokens -= 1.0;
    
    // Add rate limit headers
    ctx.response().set_header("X-RateLimit-Limit", std::to_string(max_tokens_));
    ctx.response().set_header("X-RateLimit-Remaining", 
                              std::to_string(static_cast<int>(bucket.tokens)));
    
    next();
    return true;
}

std::string RateLimitMiddleware::getClientIdentifier(RequestContext& ctx) {
    // Use authenticated user ID if available
    if (ctx.isAuthenticated()) {
        return "user:" + ctx.userId();
    }
    
    // Fall back to IP address
    std::string ip = ctx.request().get_header_value("X-Forwarded-For");
    if (ip.empty()) {
        ip = ctx.request().get_header_value("X-Real-IP");
    }
    if (ip.empty()) {
        ip = "unknown";
    }
    
    return "ip:" + ip;
}

} // namespace themis::server
```

### 6. Router with Pattern Matching

```cpp
// routing/router.cpp
#include "routing/router.h"
#include "routing/route_matcher.h"

namespace themis::server {

void Router::addRoute(
    const std::string& method,
    const std::string& pattern,
    RouteHandler handler,
    std::vector<std::shared_ptr<MiddlewareBase>> middlewares
) {
    Route route{
        .method = method,
        .pattern = pattern,
        .handler = std::move(handler),
        .middlewares = std::move(middlewares),
        .matcher = std::make_unique<RouteMatcher>(pattern)
    };
    
    routes_.push_back(std::move(route));
}

bool Router::route(const httplib::Request& req, httplib::Response& res) {
    // Find matching route
    for (auto& route : routes_) {
        if (route.method != req.method) continue;
        
        httplib::Params params;
        if (!route.matcher->match(req.path, params)) continue;
        
        // Found matching route - create request context
        auto ctx = std::make_shared<RequestContext>(req, res, server_context_);
        ctx->setPathParams(params);
        
        // Execute middleware chain + handler
        auto final_handler = [&route, ctx]() {
            route.handler(*ctx);
        };
        
        middleware_chain_->execute(*ctx, final_handler);
        return true;
    }
    
    // No route found - 404
    res.status = 404;
    res.set_content(R"({"error":"Not Found"})", "application/json");
    return false;
}

// Convenience methods for different HTTP methods
void Router::get(const std::string& pattern, RouteHandler handler) {
    addRoute("GET", pattern, std::move(handler), {});
}

void Router::post(const std::string& pattern, RouteHandler handler) {
    addRoute("POST", pattern, std::move(handler), {});
}

void Router::put(const std::string& pattern, RouteHandler handler) {
    addRoute("PUT", pattern, std::move(handler), {});
}

void Router::del(const std::string& pattern, RouteHandler handler) {
    addRoute("DELETE", pattern, std::move(handler), {});
}

} // namespace themis::server
```

### 7. Example Handler: Diff API (Phase 2)

```cpp
// handlers/mvcc/diff_handler.cpp
#include "handlers/mvcc/diff_handler.h"
#include "analytics/diff_engine.h"

namespace themis::server {

DiffHandler::DiffHandler(std::shared_ptr<ServerContext> context)
    : HandlerBase(context)
    , diff_engine_(std::make_unique<DiffEngine>(
          context->db(),
          context->changefeed()
      ))
{}

void DiffHandler::registerRoutes(httplib::Server& server) {
    // GET /api/v1/diff - Compute diff
    server.Get("/api/v1/diff", [this](const auto& req, auto& res) {
        handleRequest(req, res, [this](RequestContext& ctx) {
            processDiffRequest(ctx);
        });
    });
    
    // GET /api/v1/diff/cache/stats - Cache statistics
    server.Get("/api/v1/diff/cache/stats", [this](const auto& req, auto& res) {
        handleRequest(req, res, [this](RequestContext& ctx) {
            processCacheStatsRequest(ctx);
        });
    });
    
    // DELETE /api/v1/diff/cache - Clear cache
    server.Delete("/api/v1/diff/cache", [this](const auto& req, auto& res) {
        handleRequest(req, res, [this](RequestContext& ctx) {
            processClearCacheRequest(ctx);
        });
    });
}

void DiffHandler::processRequest(RequestContext& ctx) {
    // Implemented by specific process methods above
}

void DiffHandler::processDiffRequest(RequestContext& ctx) {
    const auto& req = ctx.request();
    
    // Parse parameters
    DiffOptions options;
    
    // Determine diff mode (sequence, timestamp, or tag)
    if (hasQueryParam(req, "from_tag") && hasQueryParam(req, "to_tag")) {
        // Tag-based diff
        options.from_tag = getQueryParam(req, "from_tag");
        options.to_tag = getQueryParam(req, "to_tag");
        
    } else if (hasQueryParam(req, "from") && hasQueryParam(req, "to")) {
        std::string from = getQueryParam(req, "from");
        std::string to = getQueryParam(req, "to");
        
        // Check if ISO 8601 timestamp or sequence number
        if (from.find('T') != std::string::npos) {
            // Timestamp-based diff
            options.from_timestamp = parseTimestamp(from);
            options.to_timestamp = parseTimestamp(to);
        } else {
            // Sequence-based diff
            options.from_sequence = std::stoull(from);
            options.to_sequence = std::stoull(to);
        }
    } else {
        ctx.setError("Missing required parameters: from/to or from_tag/to_tag", 400);
        return;
    }
    
    // Optional filters
    if (hasQueryParam(req, "table")) {
        options.table_filter = getQueryParam(req, "table");
    }
    if (hasQueryParam(req, "key_prefix")) {
        options.key_prefix_filter = getQueryParam(req, "key_prefix");
    }
    
    // Pagination
    if (hasQueryParam(req, "limit")) {
        options.limit = std::stoi(getQueryParam(req, "limit"));
    }
    if (hasQueryParam(req, "offset")) {
        options.offset = std::stoi(getQueryParam(req, "offset"));
    }
    
    // Compute diff
    try {
        DiffResult result = diff_engine_->computeDiff(options);
        ctx.setResponseBody(result.toJson());
        ctx.setStatusCode(200);
        
    } catch (const std::exception& e) {
        ERROR("Diff computation failed: {}", e.what());
        ctx.setError("Failed to compute diff: " + std::string(e.what()), 500);
    }
}

void DiffHandler::processCacheStatsRequest(RequestContext& ctx) {
    auto stats = diff_engine_->getCacheStats();
    ctx.setResponseBody(stats);
    ctx.setStatusCode(200);
}

void DiffHandler::processClearCacheRequest(RequestContext& ctx) {
    diff_engine_->clearCache();
    ctx.setResponseBody({{"message", "Cache cleared successfully"}});
    ctx.setStatusCode(200);
}

} // namespace themis::server
```

### 8. Main Server Class (Clean & Minimal)

```cpp
// http_server.cpp (NEW - ~300-500 LOC instead of 12,482)
#include "server/http_server.h"
#include "server/core/server_context.h"
#include "server/routing/router.h"
#include "server/middleware/middleware_chain.h"

// Middleware
#include "server/middleware/logging_middleware.h"
#include "server/middleware/auth_middleware.h"
#include "server/middleware/cors_middleware.h"
#include "server/middleware/rate_limit_middleware.h"
#include "server/middleware/compression_middleware.h"

// Handlers
#include "server/handlers/health/health_handler.h"
#include "server/handlers/crud/crud_handler.h"
#include "server/handlers/query/query_handler.h"
#include "server/handlers/mvcc/diff_handler.h"
#include "server/handlers/mvcc/snapshot_handler.h"
// ... more handlers

namespace themis::server {

HttpServer::HttpServer(std::shared_ptr<ServerConfig> config)
    : config_(config)
    , server_(std::make_unique<httplib::Server>())
    , context_(std::make_shared<ServerContext>(config))
    , router_(std::make_unique<Router>(context_))
    , middleware_chain_(std::make_unique<MiddlewareChain>())
{
    initializeMiddleware();
    initializeHandlers();
    configureServer();
}

void HttpServer::initializeMiddleware() {
    // Order matters! Middleware is executed in the order added
    
    // 1. Logging (first - log everything)
    middleware_chain_->add(
        std::make_shared<LoggingMiddleware>(context_)
    );
    
    // 2. CORS (early - needed for preflight)
    middleware_chain_->add(
        std::make_shared<CorsMiddleware>(config_->cors_config())
    );
    
    // 3. Rate limiting (before auth to prevent auth brute force)
    middleware_chain_->add(
        std::make_shared<RateLimitMiddleware>(config_->rate_limit_config())
    );
    
    // 4. Authentication (after rate limit, before business logic)
    middleware_chain_->add(
        std::make_shared<AuthMiddleware>(context_)
    );
    
    // 5. Compression (last - compress responses)
    middleware_chain_->add(
        std::make_shared<CompressionMiddleware>()
    );
}

void HttpServer::initializeHandlers() {
    // Health & monitoring
    auto health_handler = std::make_shared<HealthHandler>(context_);
    health_handler->registerRoutes(*server_);
    handlers_.push_back(health_handler);
    
    // CRUD operations
    auto crud_handler = std::make_shared<CrudHandler>(context_);
    crud_handler->registerRoutes(*server_);
    handlers_.push_back(crud_handler);
    
    // Query & search
    auto query_handler = std::make_shared<QueryHandler>(context_);
    query_handler->registerRoutes(*server_);
    handlers_.push_back(query_handler);
    
    // MVCC features (Phase 1 & 2) ✨
    auto diff_handler = std::make_shared<DiffHandler>(context_);
    diff_handler->registerRoutes(*server_);
    handlers_.push_back(diff_handler);
    
    auto snapshot_handler = std::make_shared<SnapshotHandler>(context_);
    snapshot_handler->registerRoutes(*server_);
    handlers_.push_back(snapshot_handler);
    
    // ... register more handlers
}

void HttpServer::configureServer() {
    // Thread pool
    server_->new_task_queue = [this] {
        return new httplib::ThreadPool(config_->thread_pool_size());
    };
    
    // Timeouts
    server_->set_read_timeout(config_->read_timeout_seconds());
    server_->set_write_timeout(config_->write_timeout_seconds());
    server_->set_idle_interval(config_->idle_interval_seconds());
    
    // Keep-alive
    server_->set_keep_alive_max_count(config_->keep_alive_max_count());
    server_->set_keep_alive_timeout(config_->keep_alive_timeout_seconds());
    
    // Payload limits
    server_->set_payload_max_length(config_->max_payload_size());
    
    // SSL/TLS configuration
    if (config_->ssl_enabled()) {
        server_->set_ssl_context(createSslContext());
    }
    
    // Error handler
    server_->set_error_handler([](const auto& req, auto& res) {
        res.set_content(
            R"({"error":"Internal Server Error"})",
            "application/json"
        );
    });
    
    // Exception handler
    server_->set_exception_handler([](const auto& req, auto& res, const std::exception& e) {
        ERROR("Unhandled exception: {}", e.what());
        res.status = 500;
        res.set_content(
            fmt::format(R"({{"error":"{}"}}")", e.what()),
            "application/json"
        );
    });
}

bool HttpServer::start() {
    INFO("Starting HTTP server on {}:{}", config_->host(), config_->port());
    
    bool success = server_->listen(config_->host(), config_->port());
    
    if (success) {
        INFO("HTTP server started successfully");
    } else {
        ERROR("Failed to start HTTP server");
    }
    
    return success;
}

void HttpServer::stop() {
    INFO("Stopping HTTP server...");
    server_->stop();
    INFO("HTTP server stopped");
}

} // namespace themis::server
```

---

## 🔧 Implementation Plan

### Phase 1: Foundation (Week 1)
- [ ] Create core directory structure
- [ ] Implement `ServerContext`, `RequestContext`
- [ ] Implement `HandlerBase` interface
- [ ] Implement `MiddlewareBase` and `MiddlewareChain`
- [ ] Implement `Router` with pattern matching

### Phase 2: Middleware (Week 2)
- [ ] Implement `LoggingMiddleware`
- [ ] Implement `AuthMiddleware`
- [ ] Implement `CorsMiddleware`
- [ ] Implement `RateLimitMiddleware`
- [ ] Implement `CompressionMiddleware`

### Phase 3: Extract Handlers (Week 3-4)
- [ ] Extract `HealthHandler` (~300 LOC)
- [ ] Extract `CrudHandler` (~2000 LOC)
- [ ] Extract `QueryHandler` (~1500 LOC)
- [ ] Extract `TransactionHandler` (~1000 LOC)
- [ ] Extract `IndexHandler` (~800 LOC)
- [ ] Extract `GraphHandler` (~800 LOC)
- [ ] Extract `GeoHandler` (~600 LOC)

### Phase 4: Integration (Week 5)
- [ ] Integrate new `DiffHandler` ✨
- [ ] Integrate new `SnapshotHandler` ✨
- [ ] Update main `http_server.cpp`
- [ ] Test all endpoints
- [ ] Performance benchmarks

---

## 📊 Success Metrics

| Metric | Current | Target | Status |
|--------|---------|--------|--------|
| `http_server.cpp` LOC | 12,482 | < 500 | 🎯 Target |
| Number of handler files | 1 | 15+ | 🎯 Target |
| Test coverage | ? | ≥ 90% | 🎯 Target |
| Max handler file size | N/A | < 500 LOC | 🎯 Target |
| Performance regression | 0% | < 5% | 🎯 Target |
| Build time improvement | 0% | +20% | 🎯 Target |

---

## ✅ Acceptance Criteria

- [ ] `http_server.cpp` < 500 LOC
- [ ] All handlers in separate files (< 500 LOC each)
- [ ] Middleware chain working correctly
- [ ] Router pattern matching working
- [ ] All existing endpoints functional
- [ ] New MVCC handlers integrated ✨
- [ ] All tests passing (95%+)
- [ ] Performance benchmarks passed
- [ ] Documentation complete
- [ ] Code review approved

---

## 🌐 Multi-Protokoll-Support Integration

### Protocol Adapter Interface

```cpp
// protocols/protocol_adapter.h
#pragma once
#include <memory>
#include <string>
#include <functional>
#include "core/request_context.h"

namespace themis::server {

enum class Protocol {
    HTTP1_1,
    HTTP2,
    HTTP3,
    GRPC,
    MCP
};

/**
 * Unified interface for all protocol adapters.
 * Allows protocol-agnostic request handling.
 */
class ProtocolAdapter {
public:
    virtual ~ProtocolAdapter() = default;
    
    /**
     * Start the protocol server
     */
    virtual bool start() = 0;
    
    /**
     * Stop the protocol server gracefully
     */
    virtual void stop() = 0;
    
    /**
     * Get protocol type
     */
    virtual Protocol getProtocol() const = 0;
    
    /**
     * Get protocol-specific port
     */
    virtual uint16_t getPort() const = 0;
    
    /**
     * Check if protocol is secure (TLS/QUIC)
     */
    virtual bool isSecure() const = 0;
    
    /**
     * Convert protocol-specific request to unified RequestContext
     */
    virtual std::unique_ptr<RequestContext> createRequestContext(
        const void* native_request,
        void* native_response
    ) = 0;
    
    /**
     * Register route handler (protocol-specific implementation)
     */
    virtual void registerRoute(
        const std::string& method,
        const std::string& path,
        std::function<void(RequestContext&)> handler
    ) = 0;
    
    /**
     * Get protocol-specific statistics
     */
    virtual nlohmann::json getStats() const = 0;
};

} // namespace themis::server
```

### HTTP/2 Adapter Implementation

```cpp
// protocols/http2/http2_adapter.cpp
#include "protocols/http2/http2_adapter.h"
#include "protocols/http2/http2_session.h"

namespace themis::server {

Http2Adapter::Http2Adapter(const ServerConfig& config)
    : config_(config)
    , session_(std::make_unique<Http2Handler>(
          config.ioc,
          config.host,
          config.https_port,
          nullptr  // http_server pointer
      ))
{}

bool Http2Adapter::start() {
    THEMIS_INFO("Starting HTTP/2 server on port {}", config_.https_port);
    
    // Configure ALPN for h2 negotiation
    Http2Handler::configureAlpn(ssl_ctx_);
    
    // Start listening
    session_->start();
    
    THEMIS_INFO("HTTP/2 server started successfully");
    return true;
}

void Http2Adapter::stop() {
    THEMIS_INFO("Stopping HTTP/2 server");
    session_->stop();
}

Protocol Http2Adapter::getProtocol() const {
    return Protocol::HTTP2;
}

std::unique_ptr<RequestContext> Http2Adapter::createRequestContext(
    const void* native_request,
    void* native_response
) {
    // Convert nghttp2 frame to RequestContext
    auto* frame = static_cast<const nghttp2_frame*>(native_request);
    
    auto ctx = std::make_unique<RequestContext>();
    ctx->setProtocol(Protocol::HTTP2);
    ctx->setStreamId(frame->hd.stream_id);
    
    // Extract headers from HEADERS frame
    if (frame->hd.type == NGHTTP2_HEADERS) {
        for (auto& header : frame->headers.nva) {
            std::string name(reinterpret_cast<const char*>(header.name), header.namelen);
            std::string value(reinterpret_cast<const char*>(header.value), header.valuelen);
            
            if (name == ":method") ctx->setMethod(value);
            else if (name == ":path") ctx->setPath(value);
            else if (name == ":scheme") ctx->setScheme(value);
            else ctx->setHeader(name, value);
        }
    }
    
    return ctx;
}

void Http2Adapter::registerRoute(
    const std::string& method,
    const std::string& path,
    std::function<void(RequestContext&)> handler
) {
    // Register route with HTTP/2 session
    routes_[method + ":" + path] = std::move(handler);
}

nlohmann::json Http2Adapter::getStats() const {
    return {
        {"protocol", "HTTP/2"},
        {"port", config_.https_port},
        {"secure", true},
        {"active_streams", session_->getActiveStreamCount()},
        {"max_concurrent_streams", config_.http2_max_concurrent_streams},
        {"bytes_sent", session_->getBytesSent()},
        {"bytes_received", session_->getBytesReceived()}
    };
}

} // namespace themis::server
```

### gRPC Adapter Implementation

```cpp
// protocols/grpc/grpc_adapter.cpp
#include "protocols/grpc/grpc_adapter.h"
#include "server/rpc_service_impl.h"

namespace themis::server {

GrpcAdapter::GrpcAdapter(const ServerConfig& config)
    : config_(config)
    , server_address_(config.host + ":" + std::to_string(config.grpc_port))
{}

bool GrpcAdapter::start() {
    THEMIS_INFO("Starting gRPC server on {}", server_address_);
    
    grpc::ServerBuilder builder;
    
    // Listen on the specified address without authentication (or with TLS)
    if (config_.enable_tls) {
        grpc::SslServerCredentialsOptions ssl_opts;
        ssl_opts.pem_root_certs = "";  // Load from config
        ssl_opts.pem_key_cert_pairs.push_back({
            config_.tls_key,
            config_.tls_cert
        });
        builder.AddListeningPort(server_address_, grpc::SslServerCredentials(ssl_opts));
    } else {
        builder.AddListeningPort(server_address_, grpc::InsecureServerCredentials());
    }
    
    // Register gRPC services
    builder.RegisterService(&themis_service_);
    builder.RegisterService(&llm_service_);
    builder.RegisterService(&wal_service_);
    
    // Build and start server
    server_ = builder.BuildAndStart();
    
    if (!server_) {
        THEMIS_ERROR("Failed to start gRPC server");
        return false;
    }
    
    THEMIS_INFO("gRPC server started successfully");
    return true;
}

void GrpcAdapter::stop() {
    THEMIS_INFO("Stopping gRPC server");
    if (server_) {
        server_->Shutdown();
        server_->Wait();
    }
}

Protocol GrpcAdapter::getProtocol() const {
    return Protocol::GRPC;
}

std::unique_ptr<RequestContext> GrpcAdapter::createRequestContext(
    const void* native_request,
    void* native_response
) {
    // gRPC uses different request model (ServerContext + Request/Response messages)
    // This adapter focuses on registering services rather than individual routes
    auto ctx = std::make_unique<RequestContext>();
    ctx->setProtocol(Protocol::GRPC);
    return ctx;
}

void GrpcAdapter::registerRoute(
    const std::string& method,
    const std::string& path,
    std::function<void(RequestContext&)> handler
) {
    // gRPC uses service registration, not individual route registration
    THEMIS_WARN("gRPC adapter does not support individual route registration");
}

nlohmann::json GrpcAdapter::getStats() const {
    return {
        {"protocol", "gRPC"},
        {"address", server_address_},
        {"secure", config_.enable_tls},
        {"services_registered", 3}  // themis_core, llm, wal
    };
}

} // namespace themis::server
```

### Unified Server Orchestrator

```cpp
// http_server.cpp (simplified main implementation)
#include "server/http_server.h"
#include "protocols/http1/http1_adapter.h"
#include "protocols/http2/http2_adapter.h"
#include "protocols/http3/http3_adapter.h"
#include "protocols/grpc/grpc_adapter.h"
#include "protocols/mcp/mcp_adapter.h"

namespace themis::server {

HttpServer::HttpServer(const Config& config)
    : config_(config)
    , context_(std::make_shared<ServerContext>())
{
    // Initialize protocol adapters based on configuration
    if (config_.enable_http1) {
        adapters_.push_back(std::make_unique<Http1Adapter>(config_));
    }
    
    if (config_.enable_http2) {
        adapters_.push_back(std::make_unique<Http2Adapter>(config_));
    }
    
    if (config_.enable_http3) {
        adapters_.push_back(std::make_unique<Http3Adapter>(config_));
    }
    
    if (config_.enable_grpc) {
        adapters_.push_back(std::make_unique<GrpcAdapter>(config_));
    }
    
    if (config_.enable_mcp) {
        adapters_.push_back(std::make_unique<McpAdapter>(config_));
    }
    
    THEMIS_INFO("Initialized {} protocol adapter(s)", adapters_.size());
}

bool HttpServer::start() {
    THEMIS_INFO("Starting ThemisDB Multi-Protocol Server");
    
    // Start all configured protocol adapters
    for (auto& adapter : adapters_) {
        if (!adapter->start()) {
            THEMIS_ERROR("Failed to start {} adapter", 
                         protocolToString(adapter->getProtocol()));
            return false;
        }
        
        THEMIS_INFO("{} adapter started on port {}",
                    protocolToString(adapter->getProtocol()),
                    adapter->getPort());
    }
    
    THEMIS_INFO("All protocol adapters started successfully");
    return true;
}

void HttpServer::stop() {
    THEMIS_INFO("Stopping all protocol adapters");
    
    for (auto& adapter : adapters_) {
        adapter->stop();
    }
    
    THEMIS_INFO("All protocol adapters stopped");
}

void HttpServer::registerHandler(std::unique_ptr<HandlerBase> handler) {
    // Register handler routes with all compatible protocol adapters
    for (auto& adapter : adapters_) {
        // Each handler registers its routes with the adapter
        handler->registerRoutes(*adapter);
    }
    
    handlers_.push_back(std::move(handler));
}

nlohmann::json HttpServer::getStats() const {
    nlohmann::json stats;
    stats["server"] = "ThemisDB Multi-Protocol Server";
    stats["version"] = THEMIS_VERSION;
    stats["adapters"] = nlohmann::json::array();
    
    for (const auto& adapter : adapters_) {
        stats["adapters"].push_back(adapter->getStats());
    }
    
    return stats;
}

std::string HttpServer::protocolToString(Protocol proto) const {
    switch (proto) {
        case Protocol::HTTP1_1: return "HTTP/1.1";
        case Protocol::HTTP2: return "HTTP/2";
        case Protocol::HTTP3: return "HTTP/3";
        case Protocol::GRPC: return "gRPC";
        case Protocol::MCP: return "MCP";
        default: return "Unknown";
    }
}

} // namespace themis::server
```

### Configuration Example

```yaml
# config/server.yaml
server:
  host: "0.0.0.0"
  
  # Protocol enablement (explicit opt-in)
  protocols:
    http1:
      enabled: true
      port: 8080
      
    http2:
      enabled: true              # Requires TLS
      port: 8443
      max_concurrent_streams: 128
      initial_window_size: 65535
      server_push_enabled: true  # For CDC
      
    http3:
      enabled: false             # Experimental
      port: 8443                 # QUIC on UDP
      max_idle_timeout_ms: 30000
      max_streams_bidi: 100
      
    grpc:
      enabled: true
      port: 50051
      max_concurrent_calls: 1000
      
    mcp:
      enabled: false             # Planned
      port: 8080                 # Reuses HTTP/1.1
      sse_heartbeat_interval: 30
      
  # TLS configuration (required for HTTP/2, optional for gRPC)
  tls:
    enabled: true
    cert_file: "/etc/themisdb/certs/server.crt"
    key_file: "/etc/themisdb/certs/server.key"
    ca_file: "/etc/themisdb/certs/ca.crt"
    
  # Performance tuning
  thread_pool_size: 16
  max_connections: 10000
```

---

## Protocol-Specific Features

### HTTP/2 Server Push für CDC

```cpp
// protocols/http2/http2_push.cpp
namespace themis::server {

void Http2PushHandler::enableServerPush(
    int32_t stream_id,
    const std::string& changefeed_id
) {
    // Subscribe to changefeed events
    changefeed_subscriptions_[stream_id] = changefeed_id;
    
    // Register callback for new events
    changefeed_->subscribe(changefeed_id, [this, stream_id](const Event& event) {
        // Push event to client via HTTP/2 Server Push
        pushEvent(stream_id, event);
    });
}

void Http2PushHandler::pushEvent(int32_t stream_id, const Event& event) {
    // Create PUSH_PROMISE frame
    nghttp2_nv headers[] = {
        {":method", "GET"},
        {":path", "/api/v1/changefeed/events"},
        {":scheme", "https"},
        {":authority", "themisdb.local"}
    };
    
    // Submit push promise
    session_->submitPushPromise(stream_id, headers, 4);
    
    // Send event data
    std::string json_data = event.toJson().dump();
    session_->submitData(promised_stream_id, json_data);
}

} // namespace themis::server
```

### HTTP/3 0-RTT Support

```cpp
// protocols/http3/http3_adapter.cpp
bool Http3Adapter::handle0RTT(const QuicConnection& conn) {
    // Check if client sent early data
    if (!conn.hasEarlyData()) {
        return false;
    }
    
    // Validate 0-RTT ticket
    if (!validateEarlyDataTicket(conn.getTicket())) {
        THEMIS_WARN("Invalid 0-RTT ticket, rejecting early data");
        conn.rejectEarlyData();
        return false;
    }
    
    // Accept 0-RTT and process early data
    THEMIS_DEBUG("Accepting 0-RTT connection");
    conn.acceptEarlyData();
    
    // Process early data requests
    processEarlyData(conn);
    return true;
}
```

### gRPC Bidirectional Streaming

```cpp
// protocols/grpc/grpc_adapter.cpp
grpc::Status GrpcAdapter::StreamChangefeed(
    grpc::ServerContext* context,
    grpc::ServerReaderWriter<ChangefeedEvent, ChangefeedRequest>* stream
) {
    ChangefeedRequest request;
    
    // Read client requests
    while (stream->Read(&request)) {
        std::string changefeed_id = request.changefeed_id();
        
        // Subscribe to changefeed
        auto subscription = changefeed_->subscribe(changefeed_id);
        
        // Stream events back to client
        for (const auto& event : subscription) {
            ChangefeedEvent response;
            response.set_sequence(event.sequence);
            response.set_key(event.key);
            response.set_value(event.value);
            response.set_operation_type(event.op_type);
            
            if (!stream->Write(response)) {
                // Client disconnected
                break;
            }
        }
    }
    
    return grpc::Status::OK;
}
```

---

## 📚 References

- [cpp-httplib Documentation](https://github.com/yhirose/cpp-httplib)
- [nghttp2 - HTTP/2 C Library](https://nghttp2.org/)
- [nghttp3/ngtcp2 - HTTP/3 Implementation](https://github.com/ngtcp2/nghttp3)
- [gRPC C++ Guide](https://grpc.io/docs/languages/cpp/)
- [HTTP/2 RFC 7540](https://tools.ietf.org/html/rfc7540)
- [HTTP/3 RFC 9114](https://www.rfc-editor.org/rfc/rfc9114.html)
- [QUIC RFC 9000](https://www.rfc-editor.org/rfc/rfc9000.html)
- [REST API Best Practices](https://restfulapi.net/rest-architectural-constraints/)
- [Clean Code Principles](https://clean-code-developer.com/)
- [SOLID Principles](https://en.wikipedia.org/wiki/SOLID)

---

**Created**: 2026-01-12  
**Last Updated**: 2026-01-12  
**Version**: 2.0 (Multi-Protocol)

---

**Labels**: `type:refactoring`, `area:api`, `priority:P2`, `effort:x-large`, `technical-debt`, `multi-protocol`
