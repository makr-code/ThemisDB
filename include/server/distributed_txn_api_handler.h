/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            distributed_txn_api_handler.h                      ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:46:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     139                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
     * Returns: { "transaction_id": "&lt;id&gt;", "status": "active" }
     */
    http::response<http::string_body> handleBegin(
        const http::request<http::string_body>& req
    );

    /**
     * POST /dtxn/operation
     * Body: { "transaction_id": "&lt;id&gt;", "shard_id": "&lt;shard&gt;", "operation": {...} }
     * Returns: { "transaction_id": "&lt;id&gt;", "status": "ok" }
     */
    http::response<http::string_body> handleOperation(
        const http::request<http::string_body>& req
    );

    /**
     * POST /dtxn/commit
     * Body: { "transaction_id": "&lt;id&gt;" }
     * Returns: { "transaction_id": "&lt;id&gt;", "status": "committed" | "aborted" }
     */
    http::response<http::string_body> handleCommit(
        const http::request<http::string_body>& req
    );

    /**
     * POST /dtxn/abort
     * Body: { "transaction_id": "&lt;id&gt;" }
     * Returns: { "transaction_id": "&lt;id&gt;", "status": "aborted" }
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
     * Returns: { "transaction_id": "&lt;id&gt;", "state": "ACTIVE|PREPARING|..." }
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
