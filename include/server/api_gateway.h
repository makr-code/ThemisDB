/**
 * @file api_gateway.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "server/auth_middleware.h"
#include "server/rate_limiter.h"
#include "server/rate_limiter_v2.h"
#include "server/load_shedder.h"
#include "server/api_version.h"
#include "sharding/shard_router.h"
#include "sharding/circuit_breaker.h"
#include "sharding/prometheus_metrics.h"
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace themis::server {

namespace beast = boost::beast;
namespace http = beast::http;

// Bring nested AuthMiddleware types into scope for use in this namespace
using AuthContext = AuthMiddleware::AuthContext;

/**
 * @brief API Gateway - Unified entry point for all API requests
 * 
 * The API Gateway provides:
 * - Request routing to appropriate backend services/shards
 * - Load balancing across multiple backends
 * - Authentication and authorization
 * - Rate limiting and load shedding
 * - Circuit breaking for failing backends
 * - Request/response transformation
 * - Metrics and monitoring
 * - Query federation for distributed queries
 * 
 * Architecture:
 * ```
 * Client → API Gateway → [Auth] → [Rate Limit] → [Load Shedding] 
 *                           ↓
 *                      Route Decision
 *                           ↓
 *         ┌────────────────┴────────────────┐
 *         ↓                                   ↓
 *   Local Handler                      Shard Router
 *   (Single Node)                      (Distributed)
 *         ↓                                   ↓
 *   Response Aggregation ←──────────────────┘
 *         ↓
 *     Client
 * ```
 */
class APIGateway {
public:
    /**
     * @brief Configuration for API Gateway
     */
    struct Config {
        // Gateway identity
        std::string gateway_id = "gateway-001";
        std::string datacenter = "dc1";
        
        // Feature flags
        bool enable_sharding = false;          // Enable distributed routing
        bool enable_rate_limiting = true;      // Enable rate limiting
        bool enable_load_shedding = true;      // Enable load shedding
        bool enable_circuit_breaker = true;    // Enable circuit breaking
        bool enable_query_federation = false;  // Enable cross-shard queries
        
        // Routing configuration
        bool prefer_local_execution = true;    // Prefer local over remote
        uint32_t max_concurrent_requests = 1000;
        uint32_t request_timeout_ms = 30000;
        
        // Load shedding thresholds
        double load_shedding_threshold = 0.9;  // Shed at 90% capacity
        
        // Circuit breaker settings
        sharding::CircuitBreaker::Config circuit_breaker_config;
        
        // Metrics
        bool enable_metrics = true;
        std::string metrics_prefix = "themis_gateway_";
        
        // API Versioning
        bool enable_api_versioning = true;      // Enable API version negotiation
        bool enforce_version_check = false;     // Enforce version compatibility

        // External gateway integration (Kong, Nginx)
        bool enable_trusted_proxy_headers = false; // Trust X-Forwarded-For / X-Real-IP headers
        std::vector<std::string> trusted_proxies;  // Trusted proxy IPs (empty = trust all)
    };
    
    /**
     * @brief Route target for requests
     */
    enum class RouteTarget {
        LOCAL,          // Execute on local node
        SHARD,          // Route to specific shard
        SCATTER_GATHER, // Scatter to all shards and gather results
        FEDERATION      // Execute federated query across shards
    };
    
    /**
     * @brief Construct API Gateway
     * 
     * @param config Gateway configuration
     * @param auth Authentication middleware
     * @param rate_limiter Rate limiter instance (V1 - deprecated, prefer V2)
     * @param load_shedder Load shedder instance
     * @param shard_router Shard router for distributed requests (optional)
     * @param metrics Prometheus metrics collector (optional)
     */
    APIGateway(
        const Config& config,
        std::shared_ptr<AuthMiddleware> auth,
        std::shared_ptr<RateLimiter> rate_limiter,
        std::shared_ptr<LoadShedder> load_shedder,
        std::shared_ptr<sharding::ShardRouter> shard_router = nullptr,
        std::shared_ptr<sharding::PrometheusMetrics> metrics = nullptr
    );
    
    /**
     * @brief Construct API Gateway with V2 rate limiter (preferred)
     * 
     * @param config Gateway configuration
     * @param auth Authentication middleware
     * @param rate_limiter_v2 Rate limiter V2 with priority lanes
     * @param load_shedder Load shedder instance
     * @param shard_router Shard router for distributed requests (optional)
     * @param metrics Prometheus metrics collector (optional)
     */
    APIGateway(
        const Config& config,
        std::shared_ptr<AuthMiddleware> auth,
        std::shared_ptr<PerClientRateLimiter> rate_limiter_v2,
        std::shared_ptr<LoadShedder> load_shedder,
        std::shared_ptr<sharding::ShardRouter> shard_router = nullptr,
        std::shared_ptr<sharding::PrometheusMetrics> metrics = nullptr
    );
    
    /**
     * @brief Route and handle incoming HTTP request
     * 
     * This is the main entry point for all requests. It:
     * 1. Authenticates the request
     * 2. Checks rate limits
     * 3. Applies load shedding if needed
     * 4. Determines routing target
     * 5. Executes request (local or distributed)
     * 6. Returns response
     * 
     * @param req HTTP request
     * @param local_handler Function to handle local requests
     * @return HTTP response
     */
    http::response<http::string_body> handleRequest(
        const http::request<http::string_body>& req,
        std::function<http::response<http::string_body>(const http::request<http::string_body>&)> local_handler
    );
    
    /**
     * @brief Execute federated query across shards
     * 
     * Executes a query that spans multiple shards:
     * 1. Analyzes query to determine sharding strategy
     * 2. Distributes query to relevant shards
     * 3. Merges and aggregates results
     * 4. Applies global ordering/pagination
     * 
     * @param query Query string (AQL format)
     * @param auth_context Authentication context
     * @return Query results
     */
    nlohmann::json executeFederatedQuery(
        const std::string& query,
        const AuthContext& auth_context
    );
    
    /**
     * @brief Get gateway statistics
     * 
     * @return Statistics including request counts, latencies, errors
     */
    nlohmann::json getStatistics() const;
    
    /**
     * @brief Get gateway health status
     * 
     * @return Health status including component health
     */
    nlohmann::json getHealthStatus() const;
    
    /**
     * @brief Update gateway configuration
     * 
     * @param config New configuration
     */
    void updateConfig(const Config& config);
    
    /**
     * @brief Register local request handler for a path pattern
     * 
     * @param pattern Path pattern (e.g., "/api/v1/entities/{name}")
     * @param handler Handler function
     */
    void registerHandler(
        const std::string& pattern,
        std::function<http::response<http::string_body>(const http::request<http::string_body>&)> handler
    );

    /**
     * @brief Register a deprecated API endpoint
     * 
     * Delegates to the internal APIVersionManager so callers can register
     * endpoint deprecations that will cause the gateway to emit Deprecation,
     * Sunset, and Link headers for the affected versions.
     * 
     * @param endpoint Endpoint path (e.g., "/api/v1/old-endpoint")
     * @param info Deprecation details
     */
    void registerDeprecation(
        const std::string& endpoint,
        const APIDeprecationInfo& info
    );

private:
    Config config_;
    std::shared_ptr<AuthMiddleware> auth_;
    std::shared_ptr<RateLimiter> rate_limiter_;  // V1 (legacy)
    std::shared_ptr<PerClientRateLimiter> rate_limiter_v2_;  // V2 (preferred)
    std::shared_ptr<LoadShedder> load_shedder_;
    std::shared_ptr<sharding::ShardRouter> shard_router_;
    std::shared_ptr<sharding::PrometheusMetrics> metrics_;
    std::shared_ptr<APIVersionManager> version_manager_;
    
    // Circuit breakers per backend
    std::unordered_map<std::string, std::shared_ptr<sharding::CircuitBreaker>> circuit_breakers_;
    mutable std::mutex circuit_breakers_mutex_;
    
    // Registered handlers
    std::unordered_map<std::string, 
        std::function<http::response<http::string_body>(const http::request<http::string_body>&)>> handlers_;
    
    // Statistics
    std::atomic<uint64_t> total_requests_{0};
    std::atomic<uint64_t> successful_requests_{0};
    std::atomic<uint64_t> failed_requests_{0};
    std::atomic<uint64_t> rate_limited_requests_{0};
    std::atomic<uint64_t> load_shed_requests_{0};
    std::atomic<uint64_t> circuit_breaker_rejections_{0};
    std::atomic<uint64_t> local_requests_{0};
    std::atomic<uint64_t> distributed_requests_{0};
    std::atomic<uint64_t> federated_queries_{0};
    
    /**
     * @brief Determine routing target for request
     * 
     * @param req HTTP request
     * @return Route target
     */
    RouteTarget determineRouteTarget(const http::request<http::string_body>& req);
    
    /**
     * @brief Check if request should be rate limited
     * 
     * @param req HTTP request
     * @return true if request should be allowed
     */
    bool checkRateLimit(const http::request<http::string_body>& req);
    
    /**
     * @brief Check if request should be load shed
     * 
     * @param req HTTP request
     * @return true if request should be allowed
     */
    bool checkLoadShedding(const http::request<http::string_body>& req);
    
    /**
     * @brief Get circuit breaker for a backend
     * 
     * @param backend_id Backend identifier
     * @return Circuit breaker instance
     */
    std::shared_ptr<sharding::CircuitBreaker> getCircuitBreaker(const std::string& backend_id);
    
    /**
     * @brief Execute request on local node
     * 
     * @param req HTTP request
     * @param handler Local request handler
     * @return HTTP response
     */
    http::response<http::string_body> executeLocal(
        const http::request<http::string_body>& req,
        std::function<http::response<http::string_body>(const http::request<http::string_body>&)> handler
    );
    
    /**
     * @brief Execute request on remote shard
     * 
     * @param req HTTP request
     * @param shard_id Target shard ID
     * @return HTTP response
     */
    http::response<http::string_body> executeRemote(
        const http::request<http::string_body>& req,
        const std::string& shard_id
    );
    
    /**
     * @brief Execute scatter-gather request across all shards
     * 
     * @param req HTTP request
     * @return HTTP response with aggregated results
     */
    http::response<http::string_body> executeScatterGather(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief Process API version headers and add response headers
     * 
     * @param req Request to parse version from
     * @param response Response to add version headers to
     * @return Resolved API version
     */
    APIVersion processVersionHeaders(
        const http::request<http::string_body>& req,
        http::response<http::string_body>& response
    );
    
    /**
     * @brief Add deprecation headers if endpoint is deprecated
     * 
     * @param req Request
     * @param response Response to add headers to
     * @param version API version being used
     */
    void addDeprecationHeaders(
        const http::request<http::string_body>& req,
        http::response<http::string_body>& response,
        const APIVersion& version
    );
    
    /**
     * @brief Make error response
     * 
     * @param status HTTP status code
     * @param message Error message
     * @param req Original request
     * @return HTTP error response
     */
    http::response<http::string_body> makeErrorResponse(
        http::status status,
        const std::string& message,
        const http::request<http::string_body>& req
    );

    /**
     * @brief Make generic response
     *
     * @param status HTTP status code
     * @param body Response body
     * @param req Original request
     * @return HTTP response
     */
    http::response<http::string_body> makeResponse(
        http::status status,
        const std::string& body,
        const http::request<http::string_body>& req
    );

    /**
     * @brief Extract the raw version string from a URL path prefix (e.g., /v1/ or /v2/)
     *
     * Recognises paths starting with /v{major}/, /v{major}.{minor}/,
     * or /v{major}.{minor}.{patch}/ at the beginning of the path.
     * Returns the raw version token as it appears in the URL (e.g., "v1", "v1.4",
     * "v1.4.1") so that it can be passed directly to APIVersionManager::resolveVersion()
     * for proper partial-version resolution (major-only → latest minor.patch).
     * Query strings must already be stripped by the caller.
     *
     * @param path URL path without query string (e.g., "/v1/entities/123")
     * @return Raw version string (e.g., "v1") if a version prefix is found, std::nullopt otherwise
     */
    std::optional<std::string> extractVersionFromPath(const std::string& path) const;

    /**
     * @brief Strip version prefix from URL path
     *
     * Removes the leading /v{N}/ (or /v{N}.{M}/ etc.) segment so that
     * downstream handlers and URN extractors see a clean, unversioned path.
     *
     * Examples:
     *  - "/v1/entities/foo"  → "/entities/foo"
     *  - "/v2/query/aql"     → "/query/aql"
     *  - "/entities/foo"     → "/entities/foo"   (no-op)
     *
     * @param path URL path (e.g., "/v1/entities/123")
     * @return Path without version prefix (e.g., "/entities/123")
     */
    std::string stripVersionPrefix(const std::string& path) const;

    /**
     * @brief Extract a urn:themis: URN from an HTTP request path
     *
     * Looks for either:
     *  - /entities/{urn}   (entity endpoints)
     *  - any path segment that starts with urn:themis:
     *
     * Query strings are stripped before returning.
     *
     * @param path HTTP target path (e.g. "/entities/urn:themis:...")
     * @return Parsed URN if found and valid, std::nullopt otherwise
     */
    std::optional<sharding::URN> extractUrnFromPath(const std::string& path) const;

    /**
     * @brief Extract the real client IP address from a request
     *
     * When ThemisDB is deployed behind an external API gateway (Kong, Nginx),
     * the actual client IP is propagated via the X-Real-IP or X-Forwarded-For
     * headers.  This method returns the leftmost (originating) IP from those
     * headers when enable_trusted_proxy_headers is set, otherwise falls back to
     * a fixed placeholder (real peer address is not available at this layer).
     *
     * @param req HTTP request
     * @return Client IP string, or empty string if unavailable
     */
    std::string extractClientIp(const http::request<http::string_body>& req) const;

    /**
     * @brief Dispatch a shard operation using the shard router
     *
     * Performs GET/PUT/POST/DELETE on the shard router using the resolved URN,
     * then wraps the result in an HTTP response. Falls back to a 404 response
     * when the operation fails.
     *
     * @param urn     Resolved URN for the target entity
     * @param req     Incoming HTTP request (method + body used)
     * @return HTTP response (200 on success, 404/400 on failure)
     */
    http::response<http::string_body> dispatchShardOperation(
        const sharding::URN& urn,
        const http::request<http::string_body>& req
    );

    /**
     * @brief Record request metrics
     * 
     * @param req Request
     * @param response Response
     * @param duration_ms Request duration in milliseconds
     * @param target Route target used
     */
    void recordMetrics(
        const http::request<http::string_body>& req,
        const http::response<http::string_body>& response,
        uint64_t duration_ms,
        RouteTarget target
    );
};

} // namespace themis::server
