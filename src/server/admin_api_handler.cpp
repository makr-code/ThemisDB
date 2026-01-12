#include "server/admin_api_handler.h"
#include "storage/rocksdb_wrapper.h"
#include "server/auth_middleware.h"
#include "utils/logger.h"
#include "utils/tracing.h"

namespace themis {
namespace server {

AdminApiHandler::AdminApiHandler(
    std::shared_ptr<RocksDBWrapper> storage,
    std::shared_ptr<AuthMiddleware> auth
)
    : storage_(std::move(storage))
    , auth_(std::move(auth))
{
}

http::response<http::string_body> AdminApiHandler::handleBackup(
    const http::request<http::string_body>& req
) {
    // TODO: Implementation to be moved from http_server.cpp handleAdminBackup()
    return makeErrorResponse(http::status::not_implemented, "Not yet implemented", req);
}

http::response<http::string_body> AdminApiHandler::handleRestore(
    const http::request<http::string_body>& req
) {
    // TODO: Implementation to be moved from http_server.cpp handleAdminRestore()
    return makeErrorResponse(http::status::not_implemented, "Not yet implemented", req);
}

http::response<http::string_body> AdminApiHandler::makeErrorResponse(
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

http::response<http::string_body> AdminApiHandler::makeResponse(
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
