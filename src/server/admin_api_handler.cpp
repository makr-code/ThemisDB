/**
 * @file admin_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: admin_api_handler.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 116
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #460 Refactor: Extract WAL repli... (2026-03-11) | #459 Refactor vector operations ... (2026-03-11) | #458 REFACTOR: Extract transacti... (2026-03-11) | #457 REFACTOR: Extract time seri... (2026-03-11) | #456 REFACTOR: Extract spatial o... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "server/admin_api_handler.h"
#include "storage/rocksdb_wrapper.h"
#include "server/auth_middleware.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include <ctime>

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
    auto span = Tracer::startSpan("POST /admin/backup");
    // Implementation moved from http_server.cpp handleAdminBackup()
    try {
        nlohmann::json body = nlohmann::json::parse(req.body());
        std::string dir = body.value("directory", std::string("./data/backup_") + std::to_string(std::time(nullptr)));
        span.setAttribute("admin.backup.directory", dir);
        bool ok = storage_->createCheckpoint(dir);
        if (!ok) {
            span.setStatus(false, "Failed to create checkpoint");
            return makeErrorResponse(http::status::internal_server_error, std::string("Failed to create checkpoint at ") + dir, req);
        }
        nlohmann::json resp = {{"status", "ok"}, {"directory", dir}};
        span.setStatus(true);
        return makeResponse(http::status::ok, resp.dump(), req);
    } catch (const nlohmann::json::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, "Invalid JSON");
        return makeErrorResponse(http::status::bad_request, std::string("Invalid JSON: ") + e.what(), req);
    } catch (const std::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> AdminApiHandler::handleRestore(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("POST /admin/restore");
    // Implementation moved from http_server.cpp handleAdminRestore()
    try {
        nlohmann::json body = nlohmann::json::parse(req.body());
        if (!body.contains("directory") || !body["directory"].is_string()) {
            span.setStatus(false, "Missing required field: directory");
            return makeErrorResponse(http::status::bad_request, "Missing required field: directory", req);
        }
        std::string dir = body["directory"].get<std::string>();
        span.setAttribute("admin.restore.directory", dir);
        bool ok = storage_->restoreFromCheckpoint(dir);
        if (!ok) {
            span.setStatus(false, "Failed to restore from checkpoint");
            return makeErrorResponse(http::status::internal_server_error, std::string("Failed to restore from checkpoint ") + dir, req);
        }
        nlohmann::json resp = {{"status", "ok"}, {"restored_from", dir}};
        span.setStatus(true);
        return makeResponse(http::status::ok, resp.dump(), req);
    } catch (const nlohmann::json::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, "Invalid JSON");
        return makeErrorResponse(http::status::bad_request, std::string("Invalid JSON: ") + e.what(), req);
    } catch (const std::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> AdminApiHandler::makeErrorResponse(
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

http::response<http::string_body> AdminApiHandler::makeResponse(
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

