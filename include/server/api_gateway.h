/**
 * @file api_gateway.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class APIGateway {
public:
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
    
    enum class RouteTarget {
        LOCAL,          // Execute on local node
        SHARD,          // Route to specific shard
        SCATTER_GATHER, // Scatter to all shards and gather results
        FEDERATION      // Execute federated query across shards
    };
    
    APIGateway(
        const Config& config,
        std::shared_ptr<AuthMiddleware> auth,
        std::shared_ptr<RateLimiter> rate_limiter,
        std::shared_ptr<LoadShedder> load_shedder,
        std::shared_ptr<sharding::ShardRouter> shard_router = nullptr,
        std::shared_ptr<sharding::PrometheusMetrics> metrics = nullptr
    );
    
    APIGateway(
        const Config& config,
        std::shared_ptr<AuthMiddleware> auth,
        std::shared_ptr<PerClientRateLimiter> rate_limiter_v2,
        std::shared_ptr<LoadShedder> load_shedder,
        std::shared_ptr<sharding::ShardRouter> shard_router = nullptr,
        std::shared_ptr<sharding::PrometheusMetrics> metrics = nullptr
    );
    
    http::response<http::string_body> handleRequest(
        const http::request<http::string_body>& req,
        std::function<http::response<http::string_body>(const http::request<http::string_body>&)> local_handler
    );
    
    /**
     * @brief Execute Federated Query.
     * @param[in] query Input parameter.
     * @param[in] auth_context Input parameter.
     * @return Return value.
     */
    nlohmann::json executeFederatedQuery(
        const std::string& query,
        const AuthContext& auth_context
    );
    
    /**
     * @brief Return access control statistics.
     * @return Access control statistics.
     */
    nlohmann::json getStatistics() const;
    
    /**
     * @brief Get Health Status.
     * @return Return value.
     */
    nlohmann::json getHealthStatus() const;
    
    /**
     * @brief Update the access control configuration.
     * @param[in] config New access control configuration.
     */
    void updateConfig(const Config& config);
    
    void registerHandler(
        const std::string& pattern,
        std::function<http::response<http::string_body>(const http::request<http::string_body>&)> handler
    );

    /**
     * @brief Register Deprecation.
     * @param[in] endpoint Input parameter.
     * @param[in] info Input parameter.
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
     * @brief Determine Route Target.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    RouteTarget determineRouteTarget(const http::request<http::string_body>& req);
    
    /**
     * @brief Check whether a user exceeds the current rate limit.
     * @param[in] req Input parameter.
     * @return True when the user remains within the configured limit.
     */
    bool checkRateLimit(const http::request<http::string_body>& req);
    
    /**
     * @brief Check Load Shedding.
     * @param[in] req Input parameter.
     * @return True when the operation succeeds.
     */
    bool checkLoadShedding(const http::request<http::string_body>& req);
    
    /**
     * @brief Get Circuit Breaker.
     * @param[in] backend_id Identifier of the backend.
     * @return Return value.
     */
    std::shared_ptr<sharding::CircuitBreaker> getCircuitBreaker(const std::string& backend_id);
    
    http::response<http::string_body> executeLocal(
        const http::request<http::string_body>& req,
        std::function<http::response<http::string_body>(const http::request<http::string_body>&)> handler
    );
    
    /**
     * @brief Execute Remote.
     * @param[in] req Input parameter.
     * @param[in] shard_id Identifier of the shard.
     * @return Return value.
     */
    http::response<http::string_body> executeRemote(
        const http::request<http::string_body>& req,
        const std::string& shard_id
    );
    
    /**
     * @brief Execute Scatter Gather.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> executeScatterGather(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief Process Version Headers.
     * @param[in] req Input parameter.
     * @param[in,out] response Input/output parameter.
     * @return Return value.
     */
    APIVersion processVersionHeaders(
        const http::request<http::string_body>& req,
        http::response<http::string_body>& response
    );
    
    /**
     * @brief Add Deprecation Headers.
     * @param[in] req Input parameter.
     * @param[in,out] response Input/output parameter.
     * @param[in] version Input parameter.
     */
    void addDeprecationHeaders(
        const http::request<http::string_body>& req,
        http::response<http::string_body>& response,
        const APIVersion& version
    );
    
    /**
     * @brief Make Error Response.
     * @param[in] status Input parameter.
     * @param[in] message Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeErrorResponse(
        http::status status,
        const std::string& message,
        const http::request<http::string_body>& req
    );

    /**
     * @brief Make Response.
     * @param[in] status Input parameter.
     * @param[in] body Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeResponse(
        http::status status,
        const std::string& body,
        const http::request<http::string_body>& req
    );

    /**
     * @brief Extract Version From Path.
     * @param[in] path Input parameter.
     * @return Return value.
     */
    std::optional<std::string> extractVersionFromPath(const std::string& path) const;

    /**
     * @brief Strip Version Prefix.
     * @param[in] path Input parameter.
     * @return Return value.
     */
    std::string stripVersionPrefix(const std::string& path) const;

    /**
     * @brief Extract Urn From Path.
     * @param[in] path Input parameter.
     * @return Return value.
     */
    std::optional<sharding::URN> extractUrnFromPath(const std::string& path) const;

    /**
     * @brief Extract Client Ip.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    std::string extractClientIp(const http::request<http::string_body>& req) const;

    /**
     * @brief Dispatch Shard Operation.
     * @param[in] urn Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> dispatchShardOperation(
        const sharding::URN& urn,
        const http::request<http::string_body>& req
    );

    /**
     * @brief Record Metrics.
     * @param[in] req Input parameter.
     * @param[in] response Input parameter.
     * @param[in] duration_ms Input parameter.
     * @param[in] target Input parameter.
     */
    void recordMetrics(
        const http::request<http::string_body>& req,
        const http::response<http::string_body>& response,
        uint64_t duration_ms,
        RouteTarget target
    );
};

} // namespace themis::server
