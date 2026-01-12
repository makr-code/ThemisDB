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

### 1. Layered Architecture (Schichtenarchitektur)

```
┌─────────────────────────────────────────────────────────────┐
│                     HTTP Layer (cpp-httplib)                │
│  • Request/Response Handling                                │
│  • SSL/TLS Termination                                      │
│  • Connection Management                                    │
└──────────────────────┬──────────────────────────────────────┘
                       │
┌──────────────────────▼──────────────────────────────────────┐
│                   Middleware Layer                          │
│  • Authentication & Authorization                           │
│  • Rate Limiting & Throttling                              │
│  • CORS, Compression, Logging                              │
│  • Request Validation                                       │
└──────────────────────┬──────────────────────────────────────┘
                       │
┌──────────────────────▼──────────────────────────────────────┐
│                    Routing Layer                            │
│  • Route Registration                                       │
│  • Path Matching & Parameters                              │
│  • Method Routing (GET/POST/PUT/DELETE)                    │
└──────────────────────┬──────────────────────────────────────┘
                       │
┌──────────────────────▼──────────────────────────────────────┐
│                    Handler Layer                            │
│  • Business Logic                                           │
│  • Request Processing                                       │
│  • Response Formatting                                      │
└──────────────────────┬──────────────────────────────────────┘
                       │
┌──────────────────────▼──────────────────────────────────────┐
│                    Service Layer                            │
│  • Business Services                                        │
│  • Domain Logic                                             │
│  • Transaction Management                                   │
└──────────────────────┬──────────────────────────────────────┘
                       │
┌──────────────────────▼──────────────────────────────────────┐
│                   Data Access Layer                         │
│  • RocksDB Access                                           │
│  • Cache Management                                         │
│  • Index Operations                                         │
└─────────────────────────────────────────────────────────────┘
```

---

## 📁 Vorgeschlagene Verzeichnisstruktur

```
src/server/
├── http_server.h                      # Main server interface
├── http_server.cpp                    # Main server implementation (300-500 LOC)
│
├── core/                              # Core server components
│   ├── server_config.h/.cpp           # Configuration management
│   ├── server_context.h/.cpp          # Shared server context
│   ├── request_context.h/.cpp         # Per-request context
│   ├── response_builder.h/.cpp        # Response builder utility
│   └── error_handler.h/.cpp           # Centralized error handling
│
├── middleware/                        # Middleware components
│   ├── middleware_base.h              # Base middleware interface
│   ├── auth_middleware.h/.cpp         # Authentication
│   ├── cors_middleware.h/.cpp         # CORS handling
│   ├── compression_middleware.h/.cpp  # Gzip/Brotli compression
│   ├── rate_limit_middleware.h/.cpp   # Rate limiting
│   ├── logging_middleware.h/.cpp      # Request/response logging
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

**Labels**: `type:refactoring`, `area:api`, `priority:P2`, `effort:x-large`, `technical-debt`
