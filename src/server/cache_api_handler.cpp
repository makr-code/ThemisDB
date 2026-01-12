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
    // TODO: Implementation to be moved from http_server.cpp handleCacheQuery()
    return makeErrorResponse(http::status::not_implemented, "Not yet implemented", req);
}

http::response<http::string_body> CacheApiHandler::handlePut(
    const http::request<http::string_body>& req
) {
    // TODO: Implementation to be moved from http_server.cpp handleCachePut()
    return makeErrorResponse(http::status::not_implemented, "Not yet implemented", req);
}

http::response<http::string_body> CacheApiHandler::handleStats(
    const http::request<http::string_body>& req
) {
    // TODO: Implementation to be moved from http_server.cpp handleCacheStats()
    return makeErrorResponse(http::status::not_implemented, "Not yet implemented", req);
}

http::response<http::string_body> CacheApiHandler::makeErrorResponse(
    http::status status, const std::string& message, const http::request<http::string_body>& req
) {
    // TODO: Helper implementation
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::content_type, "application/json");
    nlohmann::json body = {{"error", message}};
    res.body() = body.dump();
    res.prepare_payload();
    return res;
}

http::response<http::string_body> CacheApiHandler::makeResponse(
    http::status status, const std::string& body, const http::request<http::string_body>& req
) {
    // TODO: Helper implementation
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::content_type, "application/json");
    res.body() = body;
    res.prepare_payload();
    return res;
}

} // namespace server
} // namespace themis
