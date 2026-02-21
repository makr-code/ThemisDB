/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            admin_api_handler.cpp                              ║
  Version:         0.0.19                                             ║
  Last Modified:   2026-02-21 18:59:51                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     115                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
    // Implementation moved from http_server.cpp handleAdminBackup()
    try {
        nlohmann::json body = nlohmann::json::parse(req.body());
        std::string dir = body.value("directory", std::string("./data/backup_") + std::to_string(std::time(nullptr)));
        bool ok = storage_->createCheckpoint(dir);
        if (!ok) {
            return makeErrorResponse(http::status::internal_server_error, std::string("Failed to create checkpoint at ") + dir, req);
        }
        nlohmann::json resp = {{"status", "ok"}, {"directory", dir}};
        return makeResponse(http::status::ok, resp.dump(), req);
    } catch (const nlohmann::json::exception& e) {
        return makeErrorResponse(http::status::bad_request, std::string("Invalid JSON: ") + e.what(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> AdminApiHandler::handleRestore(
    const http::request<http::string_body>& req
) {
    // Implementation moved from http_server.cpp handleAdminRestore()
    try {
        nlohmann::json body = nlohmann::json::parse(req.body());
        if (!body.contains("directory") || !body["directory"].is_string()) {
            return makeErrorResponse(http::status::bad_request, "Missing required field: directory", req);
        }
        std::string dir = body["directory"].get<std::string>();
        bool ok = storage_->restoreFromCheckpoint(dir);
        if (!ok) {
            return makeErrorResponse(http::status::internal_server_error, std::string("Failed to restore from checkpoint ") + dir, req);
        }
        nlohmann::json resp = {{"status", "ok"}, {"restored_from", dir}};
        return makeResponse(http::status::ok, resp.dump(), req);
    } catch (const nlohmann::json::exception& e) {
        return makeErrorResponse(http::status::bad_request, std::string("Invalid JSON: ") + e.what(), req);
    } catch (const std::exception& e) {
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
