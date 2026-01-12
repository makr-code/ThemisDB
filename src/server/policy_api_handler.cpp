#include "server/policy_api_handler.h"
#include "storage/rocksdb_wrapper.h"
#include "server/ranger_adapter.h"
#include "server/auth_middleware.h"
#include "utils/logger.h"
#include "utils/tracing.h"

namespace themis {
namespace server {

PolicyApiHandler::PolicyApiHandler(
    std::shared_ptr<RocksDBWrapper> storage,
    std::shared_ptr<RangerAdapter> ranger_adapter,
    std::shared_ptr<AuthMiddleware> auth
)
    : storage_(std::move(storage))
    , ranger_adapter_(std::move(ranger_adapter))
    , auth_(std::move(auth))
{
}

http::response<http::string_body> PolicyApiHandler::handleImportRanger(
    const http::request<http::string_body>& req
) {
    // TODO: Implementation to be moved from http_server.cpp handlePoliciesImportRanger()
    return makeErrorResponse(http::status::not_implemented, "Not yet implemented", req);
}

http::response<http::string_body> PolicyApiHandler::handleExportRanger(
    const http::request<http::string_body>& req
) {
    // TODO: Implementation to be moved from http_server.cpp handlePoliciesExportRanger()
    return makeErrorResponse(http::status::not_implemented, "Not yet implemented", req);
}

http::response<http::string_body> PolicyApiHandler::makeErrorResponse(
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

http::response<http::string_body> PolicyApiHandler::makeResponse(
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
