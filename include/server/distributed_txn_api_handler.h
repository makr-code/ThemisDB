/**
 * @file distributed_txn_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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
    explicit DistributedTxnApiHandler(
        std::shared_ptr<sharding::DistributedTransactionCoordinator> coordinator
    );

    // ── Route handlers ───────────────────────────────────────────────────────

    /**
     * POST /dtxn/begin
     * Body: { "shards": ["shard1", "shard2", ...] }
     * Returns: { "transaction_id": "<id>", "status": "active" }
     */
    http::response<http::string_body> handleBegin(
        const http::request<http::string_body>& req
    );

    /**
     * POST /dtxn/operation
     * Body: { "transaction_id": "<id>", "shard_id": "<shard>", "operation": {...} }
     * Returns: { "transaction_id": "<id>", "status": "ok" }
     */
    http::response<http::string_body> handleOperation(
        const http::request<http::string_body>& req
    );

    /**
     * POST /dtxn/commit
     * Body: { "transaction_id": "<id>" }
     * Returns: { "transaction_id": "<id>", "status": "committed" | "aborted" }
     */
    http::response<http::string_body> handleCommit(
        const http::request<http::string_body>& req
    );

    /**
     * POST /dtxn/abort
     * Body: { "transaction_id": "<id>" }
     * Returns: { "transaction_id": "<id>", "status": "aborted" }
     */
    http::response<http::string_body> handleAbort(
        const http::request<http::string_body>& req
    );

    /**
     * POST /dtxn/readonly
     * Body: { "shards": ["shard1", ...], "operations": {...} }
     * Returns: { "results": { "shard1": {...}, ... } }
     */
    http::response<http::string_body> handleReadOnly(
        const http::request<http::string_body>& req
    );

    /**
     * GET /dtxn/status/{txn_id}
     * Returns: { "transaction_id": "<id>", "state": "ACTIVE|PREPARING|..." }
     */
    http::response<http::string_body> handleStatus(
        const http::request<http::string_body>& req
    );

    /**
     * GET /dtxn/stats
     * Returns coordinator statistics JSON
     */
    http::response<http::string_body> handleStats(
        const http::request<http::string_body>& req
    );

private:
    std::shared_ptr<sharding::DistributedTransactionCoordinator> coordinator_;

    http::response<http::string_body> ok(
        const nlohmann::json& body,
        const http::request<http::string_body>& req
    ) const;

    http::response<http::string_body> error(
        http::status status,
        const std::string& message,
        const http::request<http::string_body>& req
    ) const;

    static std::string stateToString(sharding::TransactionState state);
};

} // namespace themis::server

