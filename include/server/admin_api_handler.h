/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            admin_api_handler.h                                ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-02-21 11:48:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     99                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once
#include "server/auth_middleware.h"

#include <memory>
#include <string>
#include <boost/beast/http.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
#include <nlohmann/json.hpp>

namespace themis {

// Forward declarations
class RocksDBWrapper;

namespace server {

/**
 * @brief Handler for Administrative Operations
 * 
 * This handler manages all administrative endpoints:
 * - POST /admin/backup - Create a backup
 * - POST /admin/restore - Restore from a backup
 * 
 * Features:
 * - Database backup creation
 * - Point-in-time recovery
 * - Incremental backups
 * - Backup verification
 * 
 * Extracted from http_server.cpp (~300 lines) to improve maintainability.
 */
class AdminApiHandler {
public:
    /**
     * @brief Construct a new Admin API Handler
     * 
     * @param storage Storage backend
     * @param auth Authentication/authorization middleware
     */
    AdminApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<themis::AuthMiddleware> auth
    );

    /**
     * @brief Handle POST /admin/backup request
     * @param req HTTP request with backup configuration
     * @return HTTP response with backup status and location
     */
    http::response<http::string_body> handleBackup(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /admin/restore request
     * @param req HTTP request with restore configuration
     * @return HTTP response with restore status
     */
    http::response<http::string_body> handleRestore(const http::request<http::string_body>& req);

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<themis::AuthMiddleware> auth_;

    // Helper methods (to be implemented)
    http::response<http::string_body> makeErrorResponse(
        http::status status, const std::string& message, const http::request<http::string_body>& req);
    http::response<http::string_body> makeResponse(
        http::status status, const std::string& body, const http::request<http::string_body>& req);
};

} // namespace server
} // namespace themis
