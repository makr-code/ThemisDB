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

struct HttpRequest {
    std::string method;

    std::string path;

    std::string query_string;

    std::unordered_map<std::string, std::string> headers;

    std::string body;

    std::string tenant_id;

    std::string correlation_id;

    bool hasAuth() const noexcept {
        return headers.count("authorization") > 0 || headers.count("Authorization") > 0;
    }

    std::string_view header(std::string_view name) const noexcept {
        auto it = headers.find(std::string(name));
        if (it != headers.end()) {
          return it->second;
        }
        return {};
    }
};

// ---------------------------------------------------------------------------
// HttpResponse — plain-data value type representing an outbound HTTP response
// ---------------------------------------------------------------------------

struct HttpResponse {
    int status_code = 200;

    std::unordered_map<std::string, std::string> headers;

    std::string body = {};

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

    /**
     * @brief No Content.
     * @return Return value.
     * @details Implements noContent without additional internal calls.
     */
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

    /**
     * @brief Unauthorized.
     * @return Return value.
     * @details Implements unauthorized without additional internal calls.
     */
    static HttpResponse unauthorized() {
        HttpResponse r;
        r.status_code = 401;
        r.body = "{\"error\":\"Unauthorized\"}";
        r.headers["Content-Type"] = "application/json";
        return r;
    }

    /**
     * @brief Forbidden.
     * @return Return value.
     * @details Implements forbidden without additional internal calls.
     */
    static HttpResponse forbidden() {
        HttpResponse r;
        r.status_code = 403;
        r.body = "{\"error\":\"Forbidden\"}";
        r.headers["Content-Type"] = "application/json";
        return r;
    }

    /**
     * @brief Not Found.
     * @return Return value.
     * @details Implements notFound without additional internal calls.
     */
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

class IHttpHandler {
public:
    /**
     * @brief IHttp Handler.
     * @return Return value.
     */
    virtual ~IHttpHandler() = default;

    [[nodiscard]] virtual themis::Result<HttpResponse> handle(const HttpRequest& request) = 0;

    virtual bool requiresAuthentication() const noexcept { return true; }

    [[nodiscard]] virtual std::string_view handlerName() const noexcept = 0;
};

// ---------------------------------------------------------------------------
// MiddlewareChain — composes a sequence of IHttpHandler middlewares
// ---------------------------------------------------------------------------

class MiddlewareChain final : public IHttpHandler {
public:
    /**
     * @brief Append.
     * @param[in] handler Input parameter.
     * @return Return value.
     * @details Calls: push_back(), std::move().
     */
    MiddlewareChain& append(std::shared_ptr<IHttpHandler> handler) {
        links_.push_back(std::move(handler));
        return *this;
    }

    themis::Result<HttpResponse> handle(const HttpRequest& request) override {
        return invokeAt(request, 0);
    }

    bool requiresAuthentication() const noexcept override {
        // Require auth if ANY link in the chain requires it.
        for (const auto& link : links_) {
            if (link->requiresAuthentication()) {
              return true;
            }
        }
        return false;
    }

    std::string_view handlerName() const noexcept override { return "MiddlewareChain"; }

    std::size_t size() const noexcept { return links_.size(); }

private:
    /**
     * @brief Invoke At.
     * @param[in] request Input parameter.
     * @param[in] idx Input parameter.
     * @return Return value.
     * @details Calls: size(), tl::unexpected(), themis::Error(), handle(), has_value().
     */
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
