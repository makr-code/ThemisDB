/**
 * @file cache_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/cache_api_handler.h"
#include "cache/semantic_cache.h"
#include "server/auth_middleware.h"
#include "utils/logger.h"
#include "utils/tracing.h"

namespace themis {
namespace server {

CacheApiHandler::CacheApiHandler(
    std::shared_ptr<SemanticCache> semantic_cache,
    std::shared_ptr<AuthMiddleware> auth
)
    : semantic_cache_(std::move(semantic_cache))
    , auth_(std::move(auth))
{
}

http::response<http::string_body> CacheApiHandler::handleQuery(
    const http::request<http::string_body>& req
) {
    // Implementation moved from http_server.cpp handleCacheQuery()
    auto span = Tracer::startSpan("handleCacheQuery");
    span.setAttribute("http.path", "/cache/query");
    
    if (!semantic_cache_) {
        span.setStatus(false, "cache_not_initialized");
        return makeErrorResponse(http::status::internal_server_error, "Semantic cache not initialized", req);
    }
    auto& semantic_cache = *semantic_cache_;
    try {
        nlohmann::json body = nlohmann::json::parse(req.body());
        
        if (!body.contains("prompt")) {
            span.setStatus(false, "missing_prompt");
            return makeErrorResponse(http::status::bad_request, "Missing 'prompt' field", req);
        }
        
        std::string prompt = body["prompt"].get<std::string>();
        nlohmann::json params = body.value("params", nlohmann::json::object());
        
        span.setAttribute("prompt.length", static_cast<int64_t>(prompt.size()));
        
        auto result = semantic_cache.query(prompt, params);
        
        if (result) {
            // Cache hit
            span.setAttribute("cache.hit", true);
            nlohmann::json response = {
                {"hit", true},
                {"response", result->response},
                {"metadata", result->metadata},
                {"timestamp_ms", result->timestamp_ms}
            };
            span.setStatus(true);
            return makeResponse(http::status::ok, response.dump(), req);
        } else {
            // Cache miss
            span.setAttribute("cache.hit", false);
            nlohmann::json response = {
                {"hit", false}
            };
            span.setStatus(true);
            return makeResponse(http::status::ok, response.dump(), req);
        }
        
    } catch (const nlohmann::json::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, "json_parse_error");
        return makeErrorResponse(http::status::bad_request, std::string("JSON error: ") + e.what(), req);
    } catch (const std::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, "internal_error");
        return makeErrorResponse(http::status::internal_server_error, std::string("Error: ") + e.what(), req);
    }
}

http::response<http::string_body> CacheApiHandler::handlePut(
    const http::request<http::string_body>& req
) {
    // Implementation moved from http_server.cpp handleCachePut()
    auto span = Tracer::startSpan("handleCachePut");
    span.setAttribute("http.path", "/cache/put");
    
    if (!semantic_cache_) {
        span.setStatus(false, "cache_not_initialized");
        return makeErrorResponse(http::status::internal_server_error, "Semantic cache not initialized", req);
    }
    auto& semantic_cache = *semantic_cache_;
    try {
        nlohmann::json body = nlohmann::json::parse(req.body());
        
        if (!body.contains("prompt") || !body.contains("response")) {
            span.setStatus(false, "missing_fields");
            return makeErrorResponse(http::status::bad_request, "Missing 'prompt' or 'response' field", req);
        }
        
        std::string prompt = body["prompt"].get<std::string>();
        std::string response = body["response"].get<std::string>();
        nlohmann::json params = body.value("params", nlohmann::json::object());
        nlohmann::json metadata = body.value("metadata", nlohmann::json::object());
        int ttl_seconds = body.value("ttl_seconds", 0);
        
        span.setAttribute("prompt.length", static_cast<int64_t>(prompt.size()));
        span.setAttribute("response.length", static_cast<int64_t>(response.size()));
        
        bool success = semantic_cache.put(prompt, params, response, metadata, ttl_seconds);
        
        if (success) {
            nlohmann::json result = {
                {"success", true},
                {"message", "Response cached successfully"}
            };
            span.setStatus(true);
            return makeResponse(http::status::ok, result.dump(), req);
        } else {
            span.setStatus(false, "cache_put_failed");
            return makeErrorResponse(http::status::internal_server_error, "Failed to cache response", req);
        }
        
    } catch (const nlohmann::json::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, "json_parse_error");
        return makeErrorResponse(http::status::bad_request, std::string("JSON error: ") + e.what(), req);
    } catch (const std::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, "internal_error");
        return makeErrorResponse(http::status::internal_server_error, std::string("Error: ") + e.what(), req);
    }
}

http::response<http::string_body> CacheApiHandler::handleStats(
    const http::request<http::string_body>& req
) {
    // Implementation moved from http_server.cpp handleCacheStats()
    auto span = Tracer::startSpan("handleCacheStats");
    span.setAttribute("http.path", "/cache/stats");
    
    if (!semantic_cache_) {
        span.setStatus(false, "cache_not_initialized");
        return makeErrorResponse(http::status::internal_server_error, "Semantic cache not initialized", req);
    }
    auto& semantic_cache = *semantic_cache_;
    try {
        auto stats = semantic_cache.getStats();
        nlohmann::json response = stats.toJson();
        
        span.setAttribute("cache.hit_count", static_cast<int64_t>(stats.hit_count));
        span.setAttribute("cache.miss_count", static_cast<int64_t>(stats.miss_count));
        span.setAttribute("cache.hit_rate", stats.hit_rate);
        
        span.setStatus(true);
        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const std::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, "internal_error");
        return makeErrorResponse(http::status::internal_server_error, std::string("Error: ") + e.what(), req);
    }
}

http::response<http::string_body> CacheApiHandler::makeErrorResponse(
    http::status status, const std::string& message, const http::request<http::string_body>& req
) {
    // Helper implementation following http_server.cpp pattern
    nlohmann::json error_body = {
        {"error", true},
        {"message", message},
        {"status_code", static_cast<int>(status)}
    };
    return makeResponse(status, error_body.dump(), req);
}

http::response<http::string_body> CacheApiHandler::makeResponse(
    http::status status, const std::string& body, const http::request<http::string_body>& req
) {
    // Helper implementation following http_server.cpp pattern
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::server, "THEMIS/0.1.0");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = body;
    res.prepare_payload();
    return res;
}

} // namespace server
} // namespace themis

