/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            transaction_api_handler.h                          ║
  Version:         0.0.21                                             ║
  Last Modified:   2026-02-21 19:20:03                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     127                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
class TransactionManager;

namespace server {

/**
 * @brief Handler for Transaction Operations
 * 
 * This handler manages all transaction-related endpoints:
 * - POST /transaction - Execute a single-statement transaction
 * - POST /transaction/begin - Begin a multi-statement transaction
 * - POST /transaction/commit - Commit an active transaction
 * - POST /transaction/rollback - Rollback an active transaction
 * - GET /transaction/stats - Get transaction statistics
 * 
 * Features:
 * - ACID transaction support
 * - Snapshot isolation
 * - Multi-statement transactions
 * - Transaction statistics and monitoring
 * 
 * Extracted from http_server.cpp (~250 lines) to improve maintainability.
 */
class TransactionApiHandler {
public:
    /**
     * @brief Construct a new Transaction API Handler
     * 
     * @param storage Storage backend
     * @param tx_manager Transaction manager
     * @param auth Authentication/authorization middleware
     */
    TransactionApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<TransactionManager> tx_manager,
        std::shared_ptr<themis::AuthMiddleware> auth
    );

    /**
     * @brief Handle POST /transaction request
     * @param req HTTP request with transaction operations
     * @return HTTP response with transaction results
     */
    http::response<http::string_body> handleTransaction(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /transaction/begin request
     * @param req HTTP request
     * @return HTTP response with transaction ID
     */
    http::response<http::string_body> handleBegin(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /transaction/commit request
     * @param req HTTP request with transaction ID
     * @return HTTP response with commit status
     */
    http::response<http::string_body> handleCommit(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /transaction/rollback request
     * @param req HTTP request with transaction ID
     * @return HTTP response with rollback status
     */
    http::response<http::string_body> handleRollback(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /transaction/stats request
     * @param req HTTP request
     * @return HTTP response with transaction statistics
     */
    http::response<http::string_body> handleStats(const http::request<http::string_body>& req);

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<TransactionManager> tx_manager_;
    std::shared_ptr<themis::AuthMiddleware> auth_;

    // Helper methods (to be implemented)
    http::response<http::string_body> makeErrorResponse(
        http::status status, const std::string& message, const http::request<http::string_body>& req);
    http::response<http::string_body> makeResponse(
        http::status status, const std::string& body, const http::request<http::string_body>& req);
};

} // namespace server
} // namespace themis
