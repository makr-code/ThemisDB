#include "server/transaction_api_handler.h"
#include "storage/rocksdb_wrapper.h"
#include "transaction/transaction_manager.h"
#include "server/auth_middleware.h"
#include "utils/logger.h"
#include "utils/tracing.h"

namespace themis {
namespace server {

TransactionApiHandler::TransactionApiHandler(
    std::shared_ptr<RocksDBWrapper> storage,
    std::shared_ptr<TransactionManager> tx_manager,
    std::shared_ptr<AuthMiddleware> auth
)
    : storage_(std::move(storage))
    , tx_manager_(std::move(tx_manager))
    , auth_(std::move(auth))
{
}

http::response<http::string_body> TransactionApiHandler::handleTransaction(
    const http::request<http::string_body>& req
) {
    // TODO: Implementation to be moved from http_server.cpp handleTransaction()
    return makeErrorResponse(http::status::not_implemented, "Not yet implemented", req);
}

http::response<http::string_body> TransactionApiHandler::handleBegin(
    const http::request<http::string_body>& req
) {
    // TODO: Implementation to be moved from http_server.cpp handleTransactionBegin()
    return makeErrorResponse(http::status::not_implemented, "Not yet implemented", req);
}

http::response<http::string_body> TransactionApiHandler::handleCommit(
    const http::request<http::string_body>& req
) {
    // TODO: Implementation to be moved from http_server.cpp handleTransactionCommit()
    return makeErrorResponse(http::status::not_implemented, "Not yet implemented", req);
}

http::response<http::string_body> TransactionApiHandler::handleRollback(
    const http::request<http::string_body>& req
) {
    // TODO: Implementation to be moved from http_server.cpp handleTransactionRollback()
    return makeErrorResponse(http::status::not_implemented, "Not yet implemented", req);
}

http::response<http::string_body> TransactionApiHandler::handleStats(
    const http::request<http::string_body>& req
) {
    // TODO: Implementation to be moved from http_server.cpp handleTransactionStats()
    return makeErrorResponse(http::status::not_implemented, "Not yet implemented", req);
}

http::response<http::string_body> TransactionApiHandler::makeErrorResponse(
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

http::response<http::string_body> TransactionApiHandler::makeResponse(
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
