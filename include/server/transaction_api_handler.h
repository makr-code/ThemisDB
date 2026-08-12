/**
 * @file transaction_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
 * - POST /transaction            - Execute a list of operations atomically
 * - POST /transaction/begin      - Begin a multi-statement transaction
 * - POST /transaction/commit     - Commit an active transaction
 * - POST /transaction/rollback   - Rollback an active transaction
 * - GET  /transaction/stats      - Get transaction statistics
 * - GET  /transaction/version    - Get OCC entity version (optimistic locking)
 *
 * Features:
 * - ACID transaction support
 * - Isolation levels: read_committed, snapshot, serializable
 * - Multi-statement transactions
 * - Optimistic Concurrency Control (OCC) via optimistic_put / optimistic_erase
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

    /**
     * @brief Handle GET /transaction/version request
     *
     * Returns the current OCC version of an entity without acquiring a lock.
     * The caller supplies the active transaction ID, table name, and primary
     * key via query parameters or request body.
     *
     * Request body:
     * @code{.json}
     * {
     *   "transaction_id": 42,
     *   "table": "users",
     *   "key": "u1"
     * }
     * @endcode
     *
     * Response body:
     * @code{.json}
     * { "transaction_id": 42, "table": "users", "key": "u1", "version": 3 }
     * @endcode
     *
     * Returns version 0 when the entity does not exist.
     *
     * @param req HTTP request
     * @return HTTP response with entity version
     */
    http::response<http::string_body> handleGetVersion(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /transaction/{id}/explain request
     *
     * Returns the locks currently held and the write set (MVCC version chain
     * entries) accumulated by the transaction with the given ID.
     *
     * @param req HTTP request; the transaction ID is extracted from the URL path.
     * @return HTTP response with the explain report as JSON, or 404 if not found.
     */
    http::response<http::string_body> handleExplain(const http::request<http::string_body>& req);

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<TransactionManager> tx_manager_;
    std::shared_ptr<themis::AuthMiddleware> auth_;

    http::response<http::string_body> makeErrorResponse(
        http::status status, const std::string& message, const http::request<http::string_body>& req);
    http::response<http::string_body> makeResponse(
        http::status status, const std::string& body, const http::request<http::string_body>& req);
};

} // namespace server
} // namespace themis
