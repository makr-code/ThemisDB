/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            api_gateway.cpp                                    ║
  Version:         0.0.23                                             ║
  Last Modified:   2026-02-21 19:43:07                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   94.0/100                                       ║
    • Total Lines:     835                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "server/api_gateway.h"
#include "server/api_version_config.h"
#include "server/rate_limiter_v2.h"
#include "sharding/urn.h"
#include <chrono>
#include <ctime>
#include <mutex>
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
        if (auth_ && auth_->isEnabled()) {
            bool auth_ok = false;
            auto auth_header = req.find(http::field::authorization);
            if (auth_header != req.end()) {
                auto token = AuthMiddleware::extractBearerToken(auth_header->value());
                if (token) {
                    auto result = auth_->validateToken(*token);
                    auth_ok = result.authorized;
                }
            }
            if (!auth_ok) {
                rate_limited_requests_++;
                return makeErrorResponse(http::status::unauthorized,
                                        "Authentication failed", req);
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
        
        // 4. Determine routing target
        RouteTarget target = determineRouteTarget(req);
        
        // 5. Execute request based on target
        http::response<http::string_body> response;
        
        switch (target) {
            case RouteTarget::LOCAL:
                local_requests_++;
                response = executeLocal(req, local_handler);
                break;
                
            case RouteTarget::SHARD:
                distributed_requests_++;
                if (shard_router_) {
                    auto urn = extractUrnFromPath(std::string(req.target()));
                    if (urn) {
                        response = dispatchShardOperation(*urn, req);
                    } else {
                        // No valid URN — fall back to local execution
                        response = executeLocal(req, local_handler);
                    }
                } else {
                    response = executeLocal(req, local_handler);
                }
                break;
                
            case RouteTarget::SCATTER_GATHER:
                distributed_requests_++;
                response = executeScatterGather(req);
                break;
                
            case RouteTarget::FEDERATION:
                federated_queries_++;
                // Federation is handled separately via executeFederatedQuery
                response = makeErrorResponse(http::status::bad_request,
                    "Federation queries should use executeFederatedQuery", req);
                break;
        }
        
        // 6. Process API versioning
        if (config_.enable_api_versioning) {
            APIVersion version = processVersionHeaders(req, response);
            addDeprecationHeaders(req, response, version);
        }
        
        // 7. Record metrics
        auto end_time = std::chrono::steady_clock::now();
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time).count();
        recordMetrics(req, response, duration_ms, target);
        
        successful_requests_++;
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
    
    try {
        spdlog::info("Executing federated query: user={}", auth_context.user_id);
        
        // Use shard router to execute the query
        // The shard router handles:
        // - Query analysis
        // - Routing to appropriate shards
        // - Result aggregation
        auto result = shard_router_->executeQuery(query);
        
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
    handlers_[pattern] = std::move(handler);
}

void APIGateway::registerDeprecation(
    const std::string& endpoint,
    const APIDeprecationInfo& info
) {
    if (version_manager_) {
        version_manager_->registerDeprecation(endpoint, info);
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
    nlohmann::json result_body;
    bool ok = false;

    if (req.method() == http::verb::get) {
        auto data = shard_router_->get(urn);
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
        ok = shard_router_->put(urn, body);
        if (ok) result_body = {{"status", "ok"}};
    } else if (req.method() == http::verb::delete_) {
        ok = shard_router_->del(urn);
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
    // Prefer V2 rate limiter if available
    if (rate_limiter_v2_) {
        // Extract client ID from request (prefer user ID from JWT)
        std::string client_id = "anonymous";
        
        // Check if Authorization header exists for user-based rate limiting
        auto auth_header = req.find(http::field::authorization);
        if (auth_header != req.end()) {
            std::string auth_value = std::string(auth_header->value());
            
            // Extract JWT subject if possible (via AuthMiddleware)
            if (auth_ && auth_value.size() > 7 && auth_value.substr(0, 7) == "Bearer ") {
                std::string token = auth_value.substr(7);
                auto ctx = auth_->extractContext(token);
                if (ctx && !ctx->user_id.empty()) {
                    client_id = ctx->user_id;  // Use JWT subject as client ID
                } else {
                    // Fallback: use a stable hash of the token (not the full token)
                    // Convert hash to hex string to avoid collisions from modulo
                    size_t hash_val = std::hash<std::string>{}(token);
                    std::ostringstream oss;
                    oss << "token_" << std::hex << hash_val;
                    client_id = oss.str();
                }
            } else {
                // Non-bearer auth or malformed: use generic ID
                client_id = "authenticated";
            }
        }
        
        // Determine priority based on client attributes
        // In production, this could check JWT claims for premium users
        auto priority = TokenBucketRateLimiter::Priority::NORMAL;
        // Future: Extract priority from JWT claims (e.g., ctx->premium = true)
        
        return rate_limiter_v2_->allowRequest(client_id, 1, priority);
    }
    
    // Fallback to V1 rate limiter
    if (!rate_limiter_) {
        return true;
    }
    
    // Extract client ID from request (could be IP, user ID, etc.)
    std::string client_id = "default";
    
    // Check if Authorization header exists
    auto auth_header = req.find(http::field::authorization);
    if (auth_header != req.end()) {
        // Use auth header as client ID
        client_id = std::string(auth_header->value());
    } else {
        // Fall back to IP or other identifier
        client_id = "anonymous";
    }
    
    return rate_limiter_->allowRequest(client_id);
}

bool APIGateway::checkLoadShedding(const http::request<http::string_body>& req) {
    if (!load_shedder_) {
        return true;
    }
    
    return !load_shedder_->shouldReject(LoadShedder::Priority::NORMAL);
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
    auto cb = std::make_shared<sharding::CircuitBreaker>(
        backend_id,
        config_.circuit_breaker_config
    );
    circuit_breakers_[backend_id] = cb;
    return cb;
}

http::response<http::string_body> APIGateway::executeLocal(
    const http::request<http::string_body>& req,
    std::function<http::response<http::string_body>(const http::request<http::string_body>&)> handler
) {
    try {
        return handler(req);
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
    
    // Check circuit breaker
    if (config_.enable_circuit_breaker) {
        auto cb = getCircuitBreaker(shard_id);
        if (cb->isOpen()) {
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
        auto query_result = shard_router_->executeQuery(std::string(req.target()));
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
        auto results = shard_router_->scatterGather(query);
        
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
    std::string target_str;
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
    
    // Parse Accept-Version header
    std::string version_header;
    auto it = req.find(APIHeaders::ACCEPT_VERSION);
    if (it != req.end()) {
        version_header = std::string(it->value());
    }
    
    // Resolve version
    APIVersion version = version_manager_->resolveVersion(version_header);
    
    // Add API-Version response header
    response.set(APIHeaders::API_VERSION, version.toString());
    
    // Check if version is supported
    if (!version_manager_->isVersionSupported(version)) {
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
    
    // Extract endpoint path — strip query string so ?page=1 doesn't break lookup
    std::string endpoint = std::string(req.target());
    auto qpos = endpoint.find('?');
    if (qpos != std::string::npos) {
        endpoint = endpoint.substr(0, qpos);
    }
    
    // Check if endpoint is deprecated
    auto deprecation = version_manager_->getDeprecationInfo(endpoint, version);
    if (!deprecation) {
        return;
    }
    
    // Add Deprecation header (RFC draft)
    response.set(APIHeaders::DEPRECATION_WARNING, 
                 "true; deprecated-version=\"" + deprecation->deprecated_in.toString() + 
                 "\"; removal-version=\"" + deprecation->removed_in.toString() + "\"");
    
    // Add Sunset header (RFC 8594) with removal date
    auto removal_time = std::chrono::system_clock::to_time_t(deprecation->removal_date);
    std::tm removal_tm;
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

} // namespace themis::server
