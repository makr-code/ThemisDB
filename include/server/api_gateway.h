/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            api_gateway.h                                      ║
  Version:         0.0.22                                             ║
  Last Modified:   2026-02-21 19:28:53                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     431                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
     * @param pattern Path pattern (e.g., "/api/v1/entities/*")
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
