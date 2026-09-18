/**
 * @file distributed_txn_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include "sharding/distributed_transaction.h"
#include "sharding/truetime.h"
#include <boost/beast/http.hpp>
#include <memory>
#include <string>
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace http  = beast::http;

namespace themis::server {

/**
 * @brief HTTP handler for the distributed (cross-shard) 2PC transaction coordinator
 *
 * Exposes the DistributedTransactionCoordinator over REST:
 *
 *   POST /dtxn/begin       – begin a distributed transaction
 *   POST /dtxn/operation   – append an operation to an active transaction
 *   POST /dtxn/commit      – commit (runs full 2PC)
 *   POST /dtxn/abort       – abort the transaction
 *   POST /dtxn/readonly    – execute a read-only (snapshot) query
 *   GET  /dtxn/status/{id} – query transaction state
 *   GET  /dtxn/stats       – coordinator statistics
 */
class DistributedTxnApiHandler {
public:
    /**
     * @brief TBD: Describe DistributedTxnApiHandler.
     * @param[in] coordinator Input parameter.
     * @return Return value.
     */
    explicit DistributedTxnApiHandler(
        std::shared_ptr<sharding::DistributedTransactionCoordinator> coordinator
    );

    // ── Route handlers ───────────────────────────────────────────────────────

    /**
     * POST /dtxn/begin
     * Body: { "shards": ["shard1", "shard2", ...] }
     * Returns: { "transaction_id": "<id>", "status": "active" }
     * @brief TBD: Describe handleBegin.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleBegin(
        const http::request<http::string_body>& req
    );

    /**
     * POST /dtxn/operation
     * Body: { "transaction_id": "<id>", "shard_id": "<shard>", "operation": {...} }
     * Returns: { "transaction_id": "<id>", "status": "ok" }
     * @brief TBD: Describe handleOperation.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleOperation(
        const http::request<http::string_body>& req
    );

    /**
     * POST /dtxn/commit
     * Body: { "transaction_id": "<id>" }
     * Returns: { "transaction_id": "<id>", "status": "committed" | "aborted" }
     * @brief TBD: Describe handleCommit.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleCommit(
        const http::request<http::string_body>& req
    );

    /**
     * POST /dtxn/abort
     * Body: { "transaction_id": "<id>" }
     * Returns: { "transaction_id": "<id>", "status": "aborted" }
     * @brief TBD: Describe handleAbort.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleAbort(
        const http::request<http::string_body>& req
    );

    /**
     * POST /dtxn/readonly
     * Body: { "shards": ["shard1", ...], "operations": {...} }
     * Returns: { "results": { "shard1": {...}, ... } }
     * @brief TBD: Describe handleReadOnly.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleReadOnly(
        const http::request<http::string_body>& req
    );

    /**
     * GET /dtxn/status/{txn_id}
     * Returns: { "transaction_id": "<id>", "state": "ACTIVE|PREPARING|..." }
     * @brief TBD: Describe handleStatus.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleStatus(
        const http::request<http::string_body>& req
    );

    /**
     * GET /dtxn/stats
     * Returns coordinator statistics JSON
     * @brief TBD: Describe handleStats.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleStats(
        const http::request<http::string_body>& req
    );

private:
    std::shared_ptr<sharding::DistributedTransactionCoordinator> coordinator_;

    /**
     * @brief TBD: Describe ok.
     * @param[in] body Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> ok(
        const nlohmann::json& body,
        const http::request<http::string_body>& req
    ) const;

    /**
     * @brief TBD: Describe error.
     * @param[in] status Input parameter.
     * @param[in] message Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> error(
        http::status status,
        const std::string& message,
        const http::request<http::string_body>& req
    ) const;

    /**
     * @brief TBD: Describe stateToString.
     * @param[in] state Input parameter.
     * @return Return value.
     */
    static std::string stateToString(sharding::TransactionState state);
};

} // namespace themis::server

