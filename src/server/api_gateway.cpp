/**
 * @file api_gateway.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/api_gateway.h"
#include "server/api_version_config.h"
#include "server/rate_limiter_v2.h"
#include "sharding/urn.h"
#include <chrono>
#include <ctime>
#include <mutex>
#include <regex>
#include <sstream>
#include <spdlog/spdlog.h>

// Portable wrappers for tm <-> time_t conversions
static inline void portable_gmtime_r_impl(const time_t* t, std::tm* out) {
#ifdef _WIN32
    gmtime_s(out, t);  // Windows: gmtime_s(tm*, time_t*)
#else
    gmtime_r(t, out);  // POSIX: gmtime_r(time_t*, tm*)
#endif
}

// Helper to create error responses
namespace {
    class Error : public std::runtime_error {
    public:
        Error(int code, const std::string& msg) 
            : std::runtime_error(msg), code_(code) {}
        int code() const { return code_; }
    private:
        int code_;
    };
    
    enum class ErrorCode {
        FeatureDisabled = 1,
        ConfigurationError = 2
    };

    // Trim leading and trailing ASCII whitespace from a string in-place.
    inline void trimInPlace(std::string& s) {
        auto first = s.find_first_not_of(" \t");
        if (first == std::string::npos) { s.clear(); return; }
        s.erase(0, first);
        s.erase(s.find_last_not_of(" \t") + 1);
    }
}

namespace themis::server {

// Prefix used to identify entity-by-key paths (for shard routing)
static constexpr std::string_view kEntitiesPrefix = "/entities/";

APIGateway::APIGateway(
    const Config& config,
    std::shared_ptr<AuthMiddleware> auth,
    std::shared_ptr<RateLimiter> rate_limiter,
    std::shared_ptr<LoadShedder> load_shedder,
    std::shared_ptr<sharding::ShardRouter> shard_router,
    std::shared_ptr<sharding::PrometheusMetrics> metrics
) : config_(config),
    auth_(std::move(auth)),
    rate_limiter_(std::move(rate_limiter)),
    rate_limiter_v2_(nullptr),
    load_shedder_(std::move(load_shedder)),
    shard_router_(std::move(shard_router)),
    metrics_(std::move(metrics)),
    version_manager_(std::make_shared<APIVersionManager>())
{
    spdlog::info("APIGateway initialized: id={}, datacenter={}, sharding={}, federation={}, versioning={}, rate_limiter=V1",
                 config_.gateway_id, config_.datacenter, 
                 config_.enable_sharding, config_.enable_query_federation,
                 config_.enable_api_versioning);
    
    // Verify configuration
    if (config_.enable_sharding && !shard_router_) {
        spdlog::warn("Sharding enabled but no shard router provided");
    }
    if (config_.enable_query_federation && !shard_router_) {
        spdlog::warn("Query federation enabled but no shard router provided");
    }
}

APIGateway::APIGateway(
    const Config& config,
    std::shared_ptr<AuthMiddleware> auth,
    std::shared_ptr<PerClientRateLimiter> rate_limiter_v2,
    std::shared_ptr<LoadShedder> load_shedder,
    std::shared_ptr<sharding::ShardRouter> shard_router,
    std::shared_ptr<sharding::PrometheusMetrics> metrics
) : config_(config),
    auth_(std::move(auth)),
    rate_limiter_(nullptr),
    rate_limiter_v2_(std::move(rate_limiter_v2)),
    load_shedder_(std::move(load_shedder)),
    shard_router_(std::move(shard_router)),
    metrics_(std::move(metrics)),
    version_manager_(std::make_shared<APIVersionManager>())
{
    spdlog::info("APIGateway initialized: id={}, datacenter={}, sharding={}, federation={}, versioning={}, rate_limiter=V2",
                 config_.gateway_id, config_.datacenter, 
                 config_.enable_sharding, config_.enable_query_federation,
                 config_.enable_api_versioning);
    
    // Verify configuration (same validation as V1 constructor)
    if (config_.enable_sharding && !shard_router_) {
        spdlog::warn("Sharding enabled but no shard router provided");
    }
    if (config_.enable_query_federation && !shard_router_) {
        spdlog::warn("Query federation enabled but no shard router provided");
    }
}

http::response<http::string_body> APIGateway::handleRequest(
    const http::request<http::string_body>& req,
    std::function<http::response<http::string_body>(const http::request<http::string_body>&)> local_handler
) {
    auto start_time = std::chrono::steady_clock::now();
    total_requests_++;
    
    try {
        // 1. Authentication check
        if (auth_) {
            auto& auth = *auth_;
            if (auth.isEnabled()) {
                bool auth_ok = false;
                const auto auth_header = req[http::field::authorization];
                if (!auth_header.empty()) {
                    auto token = AuthMiddleware::extractBearerToken(std::string_view(auth_header.data(), auth_header.size()));
                    if (token) {
                        auto result = auth.validateToken(*token);
                        auth_ok = result.authorized;
                    }
                }
                if (!auth_ok) {
                    rate_limited_requests_++;
                    return makeErrorResponse(http::status::unauthorized,
                                            "Authentication failed", req);
                }
            }
        }
        
        // 2. Rate limiting check
        if (config_.enable_rate_limiting && !checkRateLimit(req)) {
            rate_limited_requests_++;
            return makeErrorResponse(http::status::too_many_requests, 
                                    "Rate limit exceeded", req);
        }
        
        // 3. Load shedding check
        if (config_.enable_load_shedding && !checkLoadShedding(req)) {
            load_shed_requests_++;
            return makeErrorResponse(http::status::service_unavailable, 
                                    "Service temporarily unavailable due to high load", req);
        }

        // Build a path-normalized request: strip /v{N}/ prefix so downstream
        // handlers and route-target logic work with unversioned paths.
        // The original request is preserved for auth/rate-limit/versioning checks.
        http::request<http::string_body> normalized_req = req;
        if (config_.enable_api_versioning) {
            std::string raw_target = std::string(req.target());
            auto qpos = raw_target.find('?');
            std::string path = (qpos != std::string::npos) ? raw_target.substr(0, qpos) : raw_target;
            std::string query = (qpos != std::string::npos) ? raw_target.substr(qpos) : "";
            std::string stripped = stripVersionPrefix(path);
            if (stripped != path) {
                normalized_req.target(stripped + query);
                spdlog::debug("APIGateway: normalized versioned path '{}' to '{}'",
                              path, stripped);
            }
        }
        
        // 4. Determine routing target (uses normalized path)
        RouteTarget target = determineRouteTarget(normalized_req);
        
        // 5. Execute request based on target
        http::response<http::string_body> response;
        
        switch (target) {
            case RouteTarget::LOCAL:
                local_requests_++;
                response = executeLocal(normalized_req, local_handler);
                break;
                
            case RouteTarget::SHARD:
                distributed_requests_++;
                if (shard_router_) {
                    auto urn = extractUrnFromPath(std::string(normalized_req.target()));
                    if (urn) {
                        response = dispatchShardOperation(*urn, normalized_req);
                    } else {
                        // No valid URN — fall back to local execution
                        response = executeLocal(normalized_req, local_handler);
                    }
                } else {
                    response = executeLocal(normalized_req, local_handler);
                }
                break;
                
            case RouteTarget::SCATTER_GATHER:
                distributed_requests_++;
                response = executeScatterGather(normalized_req);
                break;
                
            case RouteTarget::FEDERATION:
                federated_queries_++;
                // Federation is handled separately via executeFederatedQuery
                response = makeErrorResponse(http::status::bad_request,
                    "Federation queries should use executeFederatedQuery", req);
                break;
        }
        
        // 6. Process API versioning (use original req for version header extraction)
        if (config_.enable_api_versioning) {
            APIVersion version = processVersionHeaders(req, response);
            addDeprecationHeaders(req, response, version);
        }
        
        // 7. Record metrics
        auto end_time = std::chrono::steady_clock::now();
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time).count();
        recordMetrics(req, response, duration_ms, target);
        
        const auto status_code = static_cast<unsigned>(response.result_int());
        if (status_code >= 500) {
            failed_requests_++;
        } else {
            successful_requests_++;
        }
        return response;
        
    } catch (const std::exception& e) {
        failed_requests_++;
        spdlog::error("APIGateway::handleRequest error: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, 
                                std::string("Internal error: ") + e.what(), req);
    }
}

nlohmann::json APIGateway::executeFederatedQuery(
    const std::string& query,
    const AuthContext& auth_context
) {
    federated_queries_++;
    
    if (!config_.enable_query_federation) {
        throw Error(static_cast<int>(ErrorCode::FeatureDisabled), 
                   "Query federation is not enabled");
    }
    
    if (!shard_router_) {
        throw Error(static_cast<int>(ErrorCode::ConfigurationError), 
                   "Shard router not configured for query federation");
    }
    auto& shard_router = *shard_router_;
    
    try {
        spdlog::info("Executing federated query: user={}", auth_context.user_id);
        
        // Use shard router to execute the query
        // The shard router handles:
        // - Query analysis
        // - Routing to appropriate shards
        // - Result aggregation
        auto result = shard_router.executeQuery(query);
        
        return result;
        
    } catch (const std::exception& e) {
        spdlog::error("Federated query execution failed: {}", e.what());
        throw;
    }
}

nlohmann::json APIGateway::getStatistics() const {
    nlohmann::json stats;
    
    stats["gateway_id"] = config_.gateway_id;
    stats["datacenter"] = config_.datacenter;
    
    // Request statistics
    stats["requests"] = {
        {"total", total_requests_.load()},
        {"successful", successful_requests_.load()},
        {"failed", failed_requests_.load()},
        {"rate_limited", rate_limited_requests_.load()},
        {"load_shed", load_shed_requests_.load()},
        {"circuit_breaker_rejections", circuit_breaker_rejections_.load()}
    };
    
    // Routing statistics
    stats["routing"] = {
        {"local", local_requests_.load()},
        {"distributed", distributed_requests_.load()},
        {"federated_queries", federated_queries_.load()}
    };
    
    // Feature status
    stats["features"] = {
        {"sharding", config_.enable_sharding},
        {"rate_limiting", config_.enable_rate_limiting},
        {"load_shedding", config_.enable_load_shedding},
        {"circuit_breaker", config_.enable_circuit_breaker},
        {"query_federation", config_.enable_query_federation}
    };
    
    return stats;
}

nlohmann::json APIGateway::getHealthStatus() const {
    nlohmann::json health;
    
    health["status"] = "healthy";
    health["gateway_id"] = config_.gateway_id;
    
    // Component health
    health["components"] = nlohmann::json::object();
    
    if (auth_) {
        health["components"]["auth"] = "healthy";
    }
    
    if (rate_limiter_ || rate_limiter_v2_) {
        health["components"]["rate_limiter"] = rate_limiter_v2_ ? "healthy_v2" : "healthy_v1";
    }
    
    if (load_shedder_) {
        health["components"]["load_shedder"] = "healthy";
    }
    
    if (shard_router_) {
        health["components"]["shard_router"] = "healthy";
        // Could check shard router health here
    }
    
    // Check if any requests are failing
    uint64_t total = total_requests_.load();
    uint64_t failed = failed_requests_.load();
    if (total > 0) {
        double error_rate = static_cast<double>(failed) / total;
        health["error_rate"] = error_rate;
        
        if (error_rate > 0.5) {
            health["status"] = "unhealthy";
        } else if (error_rate > 0.1) {
            health["status"] = "degraded";
        }
    }
    
    return health;
}

void APIGateway::updateConfig(const Config& config) {
    spdlog::info("Updating APIGateway configuration");
    config_ = config;
}

void APIGateway::registerHandler(
    const std::string& pattern,
    std::function<http::response<http::string_body>(const http::request<http::string_body>&)> handler
) {
    spdlog::info("Registering handler for pattern: {}", pattern);
    handlers_[pattern] = std::move([[maybe_unused]] handler);
}

void APIGateway::registerDeprecation(
    const std::string& endpoint,
    const APIDeprecationInfo& info
) {
    if (version_manager_) {
        auto& version_manager = *version_manager_;
        version_manager.registerDeprecation(endpoint, info);
    }
}

std::optional<sharding::URN> APIGateway::extractUrnFromPath(const std::string& path) const {
    // Check for /entities/{urn} convention first
    const std::string entities_prefix = "/entities/";
    auto urn_pos = path.find(entities_prefix);
    if (urn_pos != std::string::npos) {
        std::string urn_str = path.substr(urn_pos + entities_prefix.size());
        auto qpos = urn_str.find('?');
        if (qpos != std::string::npos) {
            urn_str = urn_str.substr(0, qpos);
        }
        return sharding::URN::parse(urn_str);
    }

    // Fallback: look for any urn:themis: segment anywhere in the path
    const std::string urn_marker = "urn:themis:";
    auto marker_pos = path.find(urn_marker);
    if (marker_pos != std::string::npos) {
        std::string urn_str = path.substr(marker_pos);
        auto qpos = urn_str.find('?');
        if (qpos != std::string::npos) {
            urn_str = urn_str.substr(0, qpos);
        }
        return sharding::URN::parse(urn_str);
    }

    return std::nullopt;
}

http::response<http::string_body> APIGateway::dispatchShardOperation(
    const sharding::URN& urn,
    const http::request<http::string_body>& req
) {
    if (!shard_router_) {
        return makeErrorResponse(http::status::service_unavailable,
            "Shard router not available", req);
    }
    auto& shard_router = *shard_router_;
    nlohmann::json result_body;
    bool ok = false;

    if (req.method() == http::verb::get) {
        auto data = shard_router.get(urn);
        if (data) {
            result_body = *data;
            ok = true;
        }
    } else if (req.method() == http::verb::put ||
               req.method() == http::verb::post) {
        nlohmann::json body;
        if (!req.body().empty()) {
            try {
                body = nlohmann::json::parse(req.body());
            } catch (const nlohmann::json::parse_error& e) {
                spdlog::warn("APIGateway: invalid JSON in request body: {}", e.what());
                return makeErrorResponse(http::status::bad_request,
                    std::string("Invalid JSON: ") + e.what(), req);
            }
        }
        ok = shard_router.put(urn, body);
        if (ok) result_body = {{"status", "ok"}};
    } else if (req.method() == http::verb::delete_) {
        ok = shard_router.del(urn);
        if (ok) result_body = {{"status", "deleted"}};
    }

    if (ok) {
        http::response<http::string_body> response{http::status::ok, req.version()};
        response.set(http::field::content_type, "application/json");
        response.body() = result_body.dump();
        response.prepare_payload();
        return response;
    }
    return makeErrorResponse(http::status::not_found, "Entity not found or shard error", req);
}

APIGateway::RouteTarget APIGateway::determineRouteTarget(
    const http::request<http::string_body>& req
) {
    std::string path = std::string(req.target());
    
    // If sharding is not enabled, always use local
    if (!config_.enable_sharding) {
        return RouteTarget::LOCAL;
    }
    
    // Query endpoints that might need federation
    if (path.find("/query/aql") != std::string::npos ||
        path.find("/query/federated") != std::string::npos) {
        if (config_.enable_query_federation) {
            return RouteTarget::FEDERATION;
        }
    }
    
    // Entity operations by URN - route to specific shard
    if (path.find("/entities/") != std::string::npos) {
        return RouteTarget::SHARD;
    }
    
    // Collection scans - scatter-gather
    if (path.find("/collections/") != std::string::npos && 
        req.method() == http::verb::get) {
        return RouteTarget::SCATTER_GATHER;
    }
    
    // Default to local execution
    if (config_.prefer_local_execution) {
        return RouteTarget::LOCAL;
    }
    
    return RouteTarget::LOCAL;
}

bool APIGateway::checkRateLimit(const http::request<http::string_body>& req) {
    // Build a client ID derived from the originating IP when no auth token is
    // available. Respects trusted gateway headers (X-Real-IP, X-Forwarded-For)
    // when enable_trusted_proxy_headers is true.
    auto ipClientId = [this, &req]() -> std::string {
        std::string ip = extractClientIp(req);
        return ip.empty() ? "anonymous" : ("ip:" + ip);
    };

    // Prefer V2 rate limiter if available
    if (rate_limiter_v2_) {
        auto& rate_limiter_v2 = *rate_limiter_v2_;
        // Extract client ID from request (prefer user ID from JWT)
        std::string client_id = "anonymous";
        
        // Check if Authorization header exists for user-based rate limiting
        const auto auth_header = req[http::field::authorization];
        if (!auth_header.empty()) {
            std::string auth_value = std::string(auth_header.data(), auth_header.size());
            
            // Extract JWT subject if possible (via AuthMiddleware)
            if (auth_ && auth_value.size() > 7 && auth_value.substr(0, 7) == "Bearer ") {
                auto& auth = *auth_;
                std::string token = auth_value.substr(7);
                auto ctx = auth.extractContext(token);
                if (ctx && !ctx->user_id.empty()) {
                    client_id = ctx->user_id;  // Use JWT subject as client ID
                } else {
                    // Fallback: use a stable hash of the token (not the full token)
                    // Convert hash to hex string to avoid collisions from modulo
                    size_t hash_val = std::hash<std::string>{}(token);
                    std::ostringstream oss = {};
                    oss << "token_" << std::hex << hash_val;
                    client_id = oss.str();
                }
            } else {
                // Non-bearer auth or malformed: use generic ID
                client_id = "authenticated";
            }
        } else {
            // No auth header: fall back to client IP (respects trusted proxy headers)
            client_id = ipClientId();
        }
        
        // Determine priority based on client attributes
        // In production, this could check JWT claims for premium users
        auto priority = TokenBucketRateLimiter::Priority::NORMAL;
        // Future: Extract priority from JWT claims (e.g., ctx->premium = true)
        
        return rate_limiter_v2.allowRequest(client_id, 1, priority);
    }
    
    // Fallback to V1 rate limiter
    if (!rate_limiter_) {
        return true;
    }
    auto& rate_limiter = *rate_limiter_;
    
    // Extract client ID from request (could be IP, user ID, etc.)
    std::string client_id = "default";
    
    // Check if Authorization header exists
    const auto auth_header = req[http::field::authorization];
    if (!auth_header.empty()) {
        // Use auth header as client ID
        client_id = std::string(auth_header.data(), auth_header.size());
    } else {
        // Fall back to IP or other identifier
        client_id = ipClientId();
    }
    
    return rate_limiter.allowRequest(client_id);
}

bool APIGateway::checkLoadShedding(const http::request<http::string_body>& /*req*/) {
    if (!load_shedder_) {
        return true;
    }
    auto& load_shedder = *load_shedder_;
    
    return !load_shedder.shouldReject(LoadShedder::Priority::NORMAL);
}

std::shared_ptr<sharding::CircuitBreaker> APIGateway::getCircuitBreaker(
    const std::string& backend_id
) {
    std::lock_guard<std::mutex> lock(circuit_breakers_mutex_);
    auto it = circuit_breakers_.find(backend_id);
    if (it != circuit_breakers_.end()) {
        return it->second;
    }
    
    // Create new circuit breaker for this backend
    auto cb = std::make_shared<sharding::CircuitBreaker>(config_.circuit_breaker_config);
    circuit_breakers_[backend_id] = cb;
    return cb;
}

http::response<http::string_body> APIGateway::executeLocal(
    const http::request<http::string_body>& req,
    std::function<http::response<http::string_body>(const http::request<http::string_body>&)> handler
) {
    try {
        return handler([[maybe_unused]] req);
    } catch (const std::exception& e) {
        spdlog::error("Local execution failed: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, 
                                std::string("Local execution error: ") + e.what(), req);
    }
}

http::response<http::string_body> APIGateway::executeRemote(
    const http::request<http::string_body>& req,
    const std::string& shard_id
) {
    if (!shard_router_) {
        return makeErrorResponse(http::status::service_unavailable,
                                "Shard router not available", req);
    }
    auto& shard_router = *shard_router_;
    
    // Check circuit breaker
    if (config_.enable_circuit_breaker) {
        auto cb = getCircuitBreaker(shard_id);
        if (!cb->allowRequest()) {
            circuit_breaker_rejections_++;
            return makeErrorResponse(http::status::service_unavailable,
                                    "Circuit breaker open for shard: " + shard_id, req);
        }
    }
    
    try {
        // Extract URN from path and dispatch via shard router
        auto urn = extractUrnFromPath(std::string(req.target()));
        if (urn) {
            auto response = dispatchShardOperation(*urn, req);
            bool success = (response.result() == http::status::ok);
            if (config_.enable_circuit_breaker) {
                auto cb = getCircuitBreaker(shard_id);
                if (success) {
                    cb->recordSuccess();
                } else {
                    cb->recordFailure();
                }
            }
            return response;
        }

        // No URN found — forward as a generic query
        auto query_result = shard_router.executeQuery(std::string(req.target()));
        if (config_.enable_circuit_breaker) {
            getCircuitBreaker(shard_id)->recordSuccess();
        }
        http::response<http::string_body> response{http::status::ok, req.version()};
        response.set(http::field::content_type, "application/json");
        response.body() = query_result.dump();
        response.prepare_payload();
        return response;

    } catch (const std::exception& e) {
        spdlog::error("Remote execution failed for shard '{}': {}", shard_id, e.what());
        
        if (config_.enable_circuit_breaker) {
            getCircuitBreaker(shard_id)->recordFailure();
        }
        
        return makeErrorResponse(http::status::service_unavailable,
                                std::string("Remote execution error: ") + e.what(), req);
    }
}

http::response<http::string_body> APIGateway::executeScatterGather(
    const http::request<http::string_body>& req
) {
    if (!shard_router_) {
        return makeErrorResponse(http::status::service_unavailable,
                                "Shard router not available", req);
    }
    auto& shard_router = *shard_router_;
    
    try {
        // Parse request body as query — body is required for scatter-gather
        if (req.body().empty()) {
            return makeErrorResponse(http::status::bad_request,
                                    "Request body with 'query' field required for scatter-gather", req);
        }
        nlohmann::json req_body;
        try {
            req_body = nlohmann::json::parse(req.body());
        } catch (const nlohmann::json::parse_error&) {
            return makeErrorResponse(http::status::bad_request,
                                    "Request body must be valid JSON", req);
        }
        std::string query = req_body.value("query", "");
        
        if (query.empty()) {
            return makeErrorResponse(http::status::bad_request,
                                    "Query parameter required", req);
        }
        
        // Execute scatter-gather via shard router
        auto results = shard_router.scatterGather(query);
        
        // Aggregate results
        nlohmann::json response_body;
        response_body["results"] = nlohmann::json::array();
        
        for (const auto& result : results) {
            if (result.success) {
                response_body["results"].push_back(result.data);
            } else {
                spdlog::warn("Shard {} failed: {}", result.shard_id, result.error_msg);
            }
        }
        
        http::response<http::string_body> response{http::status::ok, req.version()};
        response.set(http::field::content_type, "application/json");
        response.body() = response_body.dump();
        response.prepare_payload();
        return response;
        
    } catch (const std::exception& e) {
        spdlog::error("Scatter-gather failed: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error,
                                std::string("Scatter-gather error: ") + e.what(), req);
    }
}

http::response<http::string_body> APIGateway::makeErrorResponse(
    http::status status,
    const std::string& message,
    const http::request<http::string_body>& req
) {
    nlohmann::json error_body;
    error_body["error"] = message;
    error_body["status"] = static_cast<int>(status);
    
    http::response<http::string_body> response{status, req.version()};
    response.set(http::field::content_type, "application/json");
    response.body() = error_body.dump();
    response.prepare_payload();
    response.keep_alive(req.keep_alive());
    
    return response;
}

http::response<http::string_body> APIGateway::makeResponse(
    http::status status,
    const std::string& body,
    const http::request<http::string_body>& req
) {
    http::response<http::string_body> response{status, req.version()};
    response.set(http::field::content_type, "application/json");
    response.body() = body;
    response.prepare_payload();
    response.keep_alive(req.keep_alive());
    return response;
}

void APIGateway::recordMetrics(
    const http::request<http::string_body>& req,
    const http::response<http::string_body>& response,
    uint64_t duration_ms,
    RouteTarget target
) {
    if (!metrics_) {
        return;
    }
    
    // Record request count
    std::string target_str = {};
    switch (target) {
        case RouteTarget::LOCAL: target_str = "local"; break;
        case RouteTarget::SHARD: target_str = "shard"; break;
        case RouteTarget::SCATTER_GATHER: target_str = "scatter_gather"; break;
        case RouteTarget::FEDERATION: target_str = "federation"; break;
    }
    
    // Record metrics using Prometheus format
    std::string metric_name = config_.metrics_prefix + "requests_total";
    std::map<std::string, std::string> labels = {
        {"method", std::string(req.method_string())},
        {"target", target_str},
        {"status", std::to_string(static_cast<int>(response.result()))}
    };
    
    // This would integrate with actual Prometheus metrics
    // For now, just log
    spdlog::debug("Metric: {} {} duration_ms={}", metric_name, 
                 nlohmann::json(labels).dump(), duration_ms);
}

APIVersion APIGateway::processVersionHeaders(
    const http::request<http::string_body>& req,
    http::response<http::string_body>& response
) {
    if (!version_manager_) {
        // Return current version from config if manager not available
        return APIVersion{
            APIVersionConfig::CURRENT_MAJOR,
            APIVersionConfig::CURRENT_MINOR,
            APIVersionConfig::CURRENT_PATCH
        };
    }
    auto& version_manager = *version_manager_;
    
    // Check for version prefix in URL path first (e.g. "/v1/entities").
    // Strip query string before inspecting the path so that a target like
    // "/v1/entities?page=2" is correctly resolved to version "v1".
    std::string version_header = {};
    std::optional<std::string> url_version_str;
    {
        std::string raw_target = std::string(req.target());
        auto qpos = raw_target.find('?');
        std::string path_only = (qpos != std::string::npos)
                                    ? raw_target.substr(0, qpos)
                                    : raw_target;
        auto path_version = extractVersionFromPath(path_only);
        if (path_version) {
            url_version_str = *path_version;
            version_header = *path_version;
        } else {
            // Priority 1: API-Version request header (client's current version)
            auto it = req.find(APIHeaders::API_VERSION);
            if (it != req.end()) {
                version_header = std::string(it->value());
            }
            // Priority 2: Accept-Version header (legacy)
            if (version_header.empty()) {
                auto it2 = req.find(APIHeaders::ACCEPT_VERSION);
                if (it2 != req.end()) {
                    version_header = std::string(it2->value());
                }
            }
        }
    }

    // Check for Accept-API-Version range header (e.g. "1.0-2.0").
    // When present and no explicit version was determined from the URL or
    // API-Version header, resolve the best matching version within the range.
    APIVersion version = {};
    if (url_version_str) {
        version = version_manager.resolveVersion(*url_version_str);
        spdlog::debug("APIGateway: version resolved from URL path prefix: {}", version.toString());
    } else if (!version_header.empty()) {
        version = version_manager.resolveVersion(version_header);
    } else {
        // Check Accept-API-Version range header as final fallback
        auto range_it = req.find(APIHeaders::ACCEPT_API_VERSION);
        if (range_it != req.end()) {
            auto range = APIVersionRange::parse(std::string(range_it->value()));
            if (range) {
                version = version_manager.resolveVersionRange(*range);
                spdlog::debug("APIGateway: version resolved from Accept-API-Version range '{}': {}",
                              std::string(range_it->value()), version.toString());
            } else {
                spdlog::warn("APIGateway: invalid Accept-API-Version range '{}', using current",
                             std::string(range_it->value()));
                version = version_manager.getCurrentVersion();
            }
        } else {
            version = version_manager.resolveVersion("");
        }
    }

    // Add API-Version response header
    response.set(APIHeaders::API_VERSION, version.toString());
    
    // Check if version is supported
    if (!version_manager.isVersionSupported(version)) {
        spdlog::warn("Unsupported API version requested: {}", version.toString());
        
        if (config_.enforce_version_check) {
            // Could reject the request here if enforcement is enabled
            // For now, just log and proceed with current version
        }
    }
    
    return version;
}

void APIGateway::addDeprecationHeaders(
    const http::request<http::string_body>& req,
    http::response<http::string_body>& response,
    const APIVersion& version
) {
    if (!version_manager_) {
        return;
    }
    auto& version_manager = *version_manager_;
    
    // Extract endpoint path — strip query string so ?page=1 doesn't break lookup
    std::string endpoint = std::string(req.target());
    auto qpos = endpoint.find('?');
    if (qpos != std::string::npos) {
        endpoint = endpoint.substr(0, qpos);
    }

    // Normalize: strip version prefix so registrations like "/entities" match
    // both "/entities" and "/v1/entities" requests
    endpoint = stripVersionPrefix(endpoint);
    
    // Check if endpoint is deprecated
    auto deprecation = version_manager.getDeprecationInfo(endpoint, version);
    if (!deprecation) {
        return;
    }
    
    // Add Deprecation header (RFC draft)
    response.set(APIHeaders::DEPRECATION_WARNING, 
                 "true; deprecated-version=\"" + deprecation->deprecated_in.toString() + 
                 "\"; removal-version=\"" + deprecation->removed_in.toString() + "\"");

    // Add API-Deprecated header (issue-specified format: "v1.0 (remove YYYY-MM-DD)")
    // Use major.minor format (no patch) to match the documented "v1.0" style.
    auto removal_time_t = std::chrono::system_clock::to_time_t(deprecation->removal_date);
    std::tm removal_tm_api = {};
    portable_gmtime_r_impl(&removal_time_t, &removal_tm_api);
    char api_deprecated_buf[64];
    std::strftime(api_deprecated_buf, sizeof(api_deprecated_buf), "%Y-%m-%d", &removal_tm_api);
    std::string deprecated_version_str = "v" +
        std::to_string(deprecation->deprecated_in.major) + "." +
        std::to_string(deprecation->deprecated_in.minor);
    response.set(APIHeaders::API_DEPRECATED,
                 deprecated_version_str + " (remove " +
                 std::string(api_deprecated_buf) + ")");
    
    // Add Sunset header (RFC 8594) with removal date
    auto removal_time = std::chrono::system_clock::to_time_t(deprecation->removal_date);
    std::tm removal_tm = {};
    portable_gmtime_r_impl(&removal_time, &removal_tm);
    char sunset_buf[100];
    std::strftime(sunset_buf, sizeof(sunset_buf), "%a, %d %b %Y %H:%M:%S GMT", &removal_tm);
    response.set(APIHeaders::SUNSET, sunset_buf);
    
    // Add Link header to migration guide
    if (!deprecation->migration_guide_url.empty()) {
        response.set(APIHeaders::LINK, 
                     "<" + deprecation->migration_guide_url + ">; rel=\"deprecation\"");
    }
    
    // Log deprecation warning
    spdlog::warn("Deprecated endpoint accessed: {} (version {}), will be removed in version {}", 
                 endpoint, version.toString(), deprecation->removed_in.toString());
}

std::optional<std::string> APIGateway::extractVersionFromPath(const std::string& path) const {
    // Match leading /v{N}[.{M}[.{P}]] segment (semver-style: major, major.minor,
    // or major.minor.patch only — no more than 2 additional dot-separated components).
    static const std::regex kVersionPrefixRegex(
        R"(^/v(\d+(?:\.\d+){0,2})(?=/|$))"
    );

    std::smatch m = {};
    if (!std::regex_search(path, m, kVersionPrefixRegex)) {
        return std::nullopt;
    }

    // Return full version token including 'v' prefix (e.g. "v1" or "v1.4")
    return "v" + m[1].str();
}

std::string APIGateway::stripVersionPrefix(const std::string& path) const {
    // Remove leading /v{N}[.{M}[.{P}]]/ (semver-style, max 3 components) segment.
    // "/v1/entities/foo"  → "/entities/foo"
    // "/v2"               → "/"
    // "/entities/foo"     → "/entities/foo"  (no-op)
    static const std::regex kVersionPrefixStripRegex(
        R"(^/v\d+(?:\.\d+){0,2}(?=/|$))"
    );

    std::smatch m = {};
    if (!std::regex_search(path, m, kVersionPrefixStripRegex)) {
        return path;
    }

    std::string stripped = path.substr(m[0].length());
    if (stripped.empty()) {
        stripped = "/";
    }
    return stripped;
}

std::string APIGateway::extractClientIp(
    const http::request<http::string_body>& req
) const {
    if (!config_.enable_trusted_proxy_headers) {
        return {};
    }

    // X-Real-IP is set by Nginx and Kong to the originating client IP.
    // Prefer it over X-Forwarded-For when present because it is always a
    // single IP address with no list parsing required.
    auto real_ip_it = req.find("X-Real-IP");
    if (real_ip_it != req.end()) {
        std::string ip = std::string(real_ip_it->value());
        trimInPlace(ip);
        if (!ip.empty()) {
            return ip;
        }
    }

    // X-Forwarded-For is a comma-separated list: "client, proxy1, proxy2".
    // The leftmost entry is the originating client.
    auto xff_it = req.find("X-Forwarded-For");
    if (xff_it != req.end()) {
        std::string xff = std::string(xff_it->value());
        auto comma = xff.find(',');
        std::string ip = (comma != std::string::npos) ? xff.substr(0, comma) : xff;
        trimInPlace(ip);
        if (!ip.empty()) {
            return ip;
        }
    }

    return {};
}

} // namespace themis::server
