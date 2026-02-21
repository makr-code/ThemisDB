/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            transaction_api_handler.cpp                        ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 11:01:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   93.0/100                                       ║
    • Total Lines:     224                                            ║
    • Open Issues:     TODOs: 1, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 86839235d  2026-01-13  Add documentation archival system and new issue templates ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "server/transaction_api_handler.h"
#include "storage/rocksdb_wrapper.h"
#include "transaction/transaction_manager.h"
#include "server/auth_middleware.h"
#include "utils/logger.h"
#include "utils/tracing.h"

namespace themis {
namespace server {

using json = nlohmann::json;

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
    // Implementation moved from http_server.cpp handleTransaction()
    try {
        auto body_json = json::parse(req.body());
        
        // TODO: Implement transaction endpoint
        json response = {
            {"message", "Transaction endpoint not yet fully implemented"},
            {"request", body_json}
        };
        return makeResponse(http::status::not_implemented, response.dump(), req);

    } catch (const json::exception& e) {
        return makeErrorResponse(http::status::bad_request,
            "Invalid JSON: " + std::string(e.what()), req);
    }
}

http::response<http::string_body> TransactionApiHandler::handleBegin(
    const http::request<http::string_body>& req
) {
    // Implementation moved from http_server.cpp handleTransactionBegin()
    try {
        // Parse optional isolation level from request body
        IsolationLevel isolation = IsolationLevel::ReadCommitted;
        
        if (!req.body().empty()) {
            json body = json::parse(req.body());
            if (body.contains("isolation")) {
                std::string isolation_str = body["isolation"];
                if (isolation_str == "snapshot") {
                    isolation = IsolationLevel::Snapshot;
                } else if (isolation_str != "read_committed") {
                    return makeErrorResponse(http::status::bad_request, 
                        "Invalid isolation level. Use 'read_committed' or 'snapshot'", req);
                }
            }
        }
        
        auto txn_id = tx_manager_->beginTransaction(isolation);
        
        json response = {
            {"transaction_id", txn_id},
            {"isolation", isolation == IsolationLevel::ReadCommitted ? "read_committed" : "snapshot"},
            {"status", "active"}
        };
        
        return makeResponse(http::status::ok, response.dump(2), req);
    } catch (const json::exception& e) {
        return makeErrorResponse(http::status::bad_request, "Invalid JSON: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, "Error: " + std::string(e.what()), req);
    }
}

http::response<http::string_body> TransactionApiHandler::handleCommit(
    const http::request<http::string_body>& req
) {
    // Implementation moved from http_server.cpp handleTransactionCommit()
    try {
        json body = json::parse(req.body());
        
        if (!body.contains("transaction_id")) {
            return makeErrorResponse(http::status::bad_request, "Missing 'transaction_id'", req);
        }
        
        TransactionManager::TransactionId txn_id = body["transaction_id"];
        
        auto status = tx_manager_->commitTransaction(txn_id);
        
        if (status.ok) {
            json response = {
                {"transaction_id", txn_id},
                {"status", "committed"},
                {"message", "Transaction committed successfully"}
            };
            return makeResponse(http::status::ok, response.dump(2), req);
        } else {
            json response = {
                {"transaction_id", txn_id},
                {"status", "failed"},
                {"error", status.message}
            };
            return makeResponse(http::status::internal_server_error, response.dump(2), req);
        }
    } catch (const json::exception& e) {
        return makeErrorResponse(http::status::bad_request, "Invalid JSON: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, "Error: " + std::string(e.what()), req);
    }
}

http::response<http::string_body> TransactionApiHandler::handleRollback(
    const http::request<http::string_body>& req
) {
    // Implementation moved from http_server.cpp handleTransactionRollback()
    try {
        json body = json::parse(req.body());
        
        if (!body.contains("transaction_id")) {
            return makeErrorResponse(http::status::bad_request, "Missing 'transaction_id'", req);
        }
        
        TransactionManager::TransactionId txn_id = body["transaction_id"];
        
        tx_manager_->rollbackTransaction(txn_id);
        
        json response = {
            {"transaction_id", txn_id},
            {"status", "rolled_back"},
            {"message", "Transaction rolled back successfully"}
        };
        
        return makeResponse(http::status::ok, response.dump(2), req);
    } catch (const json::exception& e) {
        return makeErrorResponse(http::status::bad_request, "Invalid JSON: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, "Error: " + std::string(e.what()), req);
    }
}

http::response<http::string_body> TransactionApiHandler::handleStats(
    const http::request<http::string_body>& req
) {
    // Implementation moved from http_server.cpp handleTransactionStats()
    try {
        auto stats = tx_manager_->getStats();
        
        json response = {
            {"total_begun", stats.total_begun},
            {"total_committed", stats.total_committed},
            {"total_aborted", stats.total_aborted},
            {"active_count", stats.active_count},
            {"avg_duration_ms", stats.avg_duration_ms},
            {"max_duration_ms", stats.max_duration_ms},
            {"success_rate", stats.total_begun > 0 
                ? static_cast<double>(stats.total_committed) / stats.total_begun 
                : 0.0}
        };
        
        return makeResponse(http::status::ok, response.dump(2), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, "Error: " + std::string(e.what()), req);
    }
}

http::response<http::string_body> TransactionApiHandler::makeErrorResponse(
    http::status status, const std::string& message, const http::request<http::string_body>& req
) {
    // Helper implementation following http_server.cpp pattern
    json error_body = {
        {"error", true},
        {"message", message},
        {"status_code", static_cast<int>(status)}
    };
    return makeResponse(status, error_body.dump(), req);
}

http::response<http::string_body> TransactionApiHandler::makeResponse(
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
