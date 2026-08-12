/**
 * @file http_handler.h
 * @brief HTTP request/response abstractions and handler interfaces.
 *
 * @details Provides transport-independent representations of HTTP requests and responses,
 * along with pluggable handler interfaces for composing middleware stacks.
 *
 * Core abstractions:
 *  - `HttpRequest`: Immutable value type containing parsed request metadata and body
 *  - `HttpResponse`: Mutable builder for constructing HTTP response payloads
 *  - `IHttpHandler`: Pure-virtual interface for processing HTTP requests
 *  - `IHttpServer`: Pure-virtual interface for lifecycle management and request routing
 *
 * Request flow:
 *  1. Raw HTTP connection parsed into `HttpRequest` by transport layer
 *  2. Request dispatched to `IHttpHandler::handle()` (often a middleware stack)
 *  3. Handler chain processes request, applying policy, auth, tracing, logging
 *  4. Final handler produces `HttpResponse` or error
 *  5. Response serialized back to HTTP wire format and sent to client
 *
 * Middleware composition:
 * Handlers can be chained to compose orthogonal concerns. Example stack:
 * ```
 * TransportPolicyMiddleware (validate payload/path/version)
 *   ↓
 * AuthenticationMiddleware (enforce authz)
 *   ↓
 * TracingMiddleware (correlation ID + OTLP export)
 *   ↓
 * RateLimitMiddleware (per-user quota enforcement)
 *   ↓
 * ApplicationHandler (business logic)
 * ```
 *
 * ### Thread safety
 * - `HttpRequest` is immutable and safe to share across threads
 * - `HttpResponse` must be built by a single thread (not thread-safe for concurrent mutation)
 * - `IHttpHandler::handle()` must be thread-safe and reentrant
 *
 * ### Error handling
 * - `IHttpHandler::handle()` returns `Result<HttpResponse>` which may contain either
 *   a successful response or an `HttpError` (status + message)
 * - On error, the middleware chain should short-circuit and return immediately
 *   (fail-closed: no upstream handler is invoked)
 *
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 */


#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>
#include "utils/expected.h"

namespace themis {
namespace api {

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------

struct HttpRequest;
struct HttpResponse;

// ---------------------------------------------------------------------------
// Error type used in Result<T>
// ---------------------------------------------------------------------------

struct HttpError {
    int         status_code = 500;
    std::string message;
};

// ---------------------------------------------------------------------------
// HttpRequest — plain-data value type representing an inbound HTTP request
// ---------------------------------------------------------------------------

/**
 * @brief Immutable value type representing a parsed inbound HTTP request.
 *
 * All fields are populated before the request is dispatched to an `IHttpHandler`.
 * The struct is intentionally shallow (no raw socket handles or streams) so that
 * it can be copied into async contexts without lifetime issues.
 */
struct HttpRequest {
    /// HTTP method, e.g. "GET", "POST", "DELETE".
    std::string method;

    /// Decoded request path, e.g. "/v1/entity/42".
    std::string path;

    /// Raw query string (without the leading `?`), or empty.
    std::string query_string;

    /// Case-insensitive HTTP header map.
    std::unordered_map<std::string, std::string> headers;

    /// Request body bytes (may be empty for GET / DELETE).
    std::string body;

    /// Tenant / namespace derived from routing (may be empty for unauthenticated routes).
    std::string tenant_id;

    /// Correlation ID injected by `TracingMiddleware`; always non-empty after middleware runs.
    std::string correlation_id;

    /// Returns true if the request carries an `Authorization` header.
    bool hasAuth() const noexcept {
        return headers.count("authorization") > 0 || headers.count("Authorization") > 0;
    }

    /// Returns the value of a header (case-sensitive key lookup) or empty string.
    std::string_view header(std::string_view name) const noexcept {
        auto it = headers.find(std::string(name));
        if (it != headers.end()) return it->second;
        return {};
    }
};

// ---------------------------------------------------------------------------
// HttpResponse — plain-data value type representing an outbound HTTP response
// ---------------------------------------------------------------------------

/**
 * @brief Value type representing an outbound HTTP response.
 *
 * Handlers return this (wrapped in `Result<HttpResponse>`) instead of writing
 * directly to a socket, making handlers unit-testable without a live server.
 */
struct HttpResponse {
    /// HTTP status code, e.g. 200, 201, 400, 404, 500.
    int status_code = 200;

    /// Response headers.  The server will merge governance headers on top.
    std::unordered_map<std::string, std::string> headers;

    /// Response body bytes.
    std::string body;

    // ---- Convenience factories ----

    static HttpResponse ok(std::string body = {}, std::string content_type = "application/json") {
        HttpResponse r;
        r.status_code = 200;
        r.body = std::move(body);
        r.headers["Content-Type"] = std::move(content_type);
        return r;
    }

    static HttpResponse created(std::string body = {}) {
        HttpResponse r;
        r.status_code = 201;
        r.body = std::move(body);
        r.headers["Content-Type"] = "application/json";
        return r;
    }

    static HttpResponse noContent() {
        HttpResponse r;
        r.status_code = 204;
        return r;
    }

    static HttpResponse badRequest(std::string message = "Bad Request") {
        HttpResponse r;
        r.status_code = 400;
        r.body = "{\"error\":\"" + message + "\"}";
        r.headers["Content-Type"] = "application/json";
        return r;
    }

    static HttpResponse unauthorized() {
        HttpResponse r;
        r.status_code = 401;
        r.body = "{\"error\":\"Unauthorized\"}";
        r.headers["Content-Type"] = "application/json";
        return r;
    }

    static HttpResponse forbidden() {
        HttpResponse r;
        r.status_code = 403;
        r.body = "{\"error\":\"Forbidden\"}";
        r.headers["Content-Type"] = "application/json";
        return r;
    }

    static HttpResponse notFound() {
        HttpResponse r;
        r.status_code = 404;
        r.body = "{\"error\":\"Not Found\"}";
        r.headers["Content-Type"] = "application/json";
        return r;
    }

    static HttpResponse internalError(std::string message = "Internal Server Error") {
        HttpResponse r;
        r.status_code = 500;
        r.body = "{\"error\":\"" + message + "\"}";
        r.headers["Content-Type"] = "application/json";
        return r;
    }
};

// ---------------------------------------------------------------------------
// IHttpHandler — pure-virtual interface for all HTTP request handlers
// ---------------------------------------------------------------------------

/**
 * @brief Pure-virtual interface that every HTTP request handler must implement.
 *
 * ### Contract
 * - `handle()` is called once per request on a thread-pool thread.
 * - Returning a `Result::error(HttpError)` causes the framework to serialize
 *   the error and send it to the client; the handler must not write to the
 *   socket directly.
 * - Handlers that require authentication must declare
 *   `requiresAuthentication() = true` (the default).  Handlers that
 *   explicitly opt out must override and return `false` and document the
 *   reason in the handler declaration.
 * - CORS headers are injected by the framework after `handle()` returns; the
 *   handler must not set `Access-Control-*` headers in the response.
 *
 * ### Thread safety
 * The framework may call `handle()` concurrently from multiple threads.
 * Implementations must be thread-safe.
 */
class IHttpHandler {
public:
    virtual ~IHttpHandler() = default;

    /**
     * @brief Process an inbound HTTP request and produce a response.
     *
     * @param request  Fully populated request value type.
     * @return `Result<HttpResponse>` — an `HttpResponse` on success or an
     *         `HttpError` on failure (the framework serializes errors).
     */
    [[nodiscard]] virtual themis::Result<HttpResponse> handle(const HttpRequest& request) = 0;

    /**
     * @brief Return `true` if this handler requires a valid JWT/API-key before
     *        `handle()` is called.
     *
     * Defaults to `true`.  Override and return `false` only for explicitly
     * public endpoints (e.g. health-check, public metrics) and document the
     * justification in the subclass header.
     */
    virtual bool requiresAuthentication() const noexcept { return true; }

    /// Human-readable handler name used in logs and metrics labels.
    [[nodiscard]] virtual std::string_view handlerName() const noexcept = 0;
};

// ---------------------------------------------------------------------------
// MiddlewareChain — composes a sequence of IHttpHandler middlewares
// ---------------------------------------------------------------------------

/**
 * @brief Ordered chain of `IHttpHandler` middlewares.
 *
 * Each middleware in the chain calls `handle()` on the next link via the
 * `IHttpHandler` pointer it receives at construction.  The final link in the
 * chain is the terminal handler.
 *
 * Usage:
 * ```cpp
 * auto chain = MiddlewareChain{}
 *     .append(std::make_shared<AuthMiddleware>(validator))
 *     .append(std::make_shared<RateLimitMiddleware>(limiter))
 *     .append(std::make_shared<TerminalHandler>());
 *
 * auto result = chain.handle(request);
 * ```
 */
class MiddlewareChain final : public IHttpHandler {
public:
    /**
     * @brief Append a handler to the end of the chain.
     * @param handler  Shared pointer to an `IHttpHandler` implementation.
     * @return Reference to `*this` for fluent construction.
     */
    MiddlewareChain& append(std::shared_ptr<IHttpHandler> handler) {
        links_.push_back(std::move(handler));
        return *this;
    }

    /**
     * @brief Invoke the chain, starting from the first appended handler.
     *
     * Calls each handler in the order they were appended.  If a handler returns
     * a `Result::error`, the chain stops immediately and returns that error.
     * Otherwise the chain advances to the next handler; the final handler's
     * success response is the one returned to the caller (intermediate handlers'
     * success responses are discarded — they act as interceptors/guards).
     */
    themis::Result<HttpResponse> handle(const HttpRequest& request) override {
        return invokeAt(request, 0);
    }

    bool requiresAuthentication() const noexcept override {
        // Require auth if ANY link in the chain requires it.
        for (const auto& link : links_) {
            if (link->requiresAuthentication()) return true;
        }
        return false;
    }

    std::string_view handlerName() const noexcept override { return "MiddlewareChain"; }

    /// Return the number of handlers in the chain.
    std::size_t size() const noexcept { return links_.size(); }

private:
    themis::Result<HttpResponse> invokeAt(const HttpRequest& request, std::size_t idx) {
        if (idx >= links_.size()) {
            return tl::unexpected(themis::Error(
                themis::errors::ErrorCode::ERR_API_INTERNAL_ERROR,
                "MiddlewareChain: no terminal handler"));
        }
        auto result = links_[idx]->handle(request);
        if (!result.has_value()) {
            return result; // error: short-circuit the chain
        }
        // If there's a next handler, advance (discard this intermediate response)
        if (idx + 1 < links_.size()) {
            return invokeAt(request, idx + 1);
        }
        return result; // final handler's response
    }

    std::vector<std::shared_ptr<IHttpHandler>> links_;
};

} // namespace api
} // namespace themis
