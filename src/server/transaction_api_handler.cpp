/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            transaction_api_handler.cpp                        ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:58:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     347                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a9a9edcf2  2026-02-21  server: Phase 2 – HTTP/3 hardening, GraphQL endpoint, API... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "server/transaction_api_handler.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "transaction/transaction_manager.h"
#include "server/auth_middleware.h"
#include "utils/logger.h"
#include "utils/tracing.h"

namespace themis {
namespace server {

using json = nlohmann::json;

// ── Internal helpers ─────────────────────────────────────────────────────────

static constexpr const char* INVALID_ISOLATION_MSG =
    "Invalid isolation level. Use 'read_committed', 'snapshot', or 'serializable'";

/// Parse the "isolation" field from a JSON body.
/// Returns ReadCommitted when the field is absent.
/// Returns Status::Error(...) via the out-param on invalid value.
static IsolationLevel parseIsolationLevel(const json& body, bool* valid,
                                           std::string* error_msg) {
    *valid = true;
    if (!body.contains("isolation")) return IsolationLevel::ReadCommitted;
    const std::string iso_str = body["isolation"].get<std::string>();
    if (iso_str == "read_committed")  return IsolationLevel::ReadCommitted;
    if (iso_str == "snapshot")        return IsolationLevel::Snapshot;
    if (iso_str == "serializable")    return IsolationLevel::SERIALIZABLE;
    *valid = false;
    *error_msg = INVALID_ISOLATION_MSG;
    return IsolationLevel::ReadCommitted;
}

/// Convert an IsolationLevel to its JSON string representation.
static const char* isolationLevelToString(IsolationLevel iso) noexcept {
    switch (iso) {
        case IsolationLevel::SERIALIZABLE: return "serializable";
        case IsolationLevel::Snapshot:     return "snapshot";
        default:                           return "read_committed";
    }
}

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
    // POST /transaction – execute a list of operations atomically.
    //
    // Expected request body:
    // {
    //   "isolation": "read_committed" | "snapshot",  // optional, default read_committed
    //   "operations": [
    //     { "type": "put",    "table": "...", "key": "...", "data": { ... } },
    //     { "type": "delete", "table": "...", "key": "..." },
    //     ...
    //   ]
    // }
    //
    // Response body:
    // {
    //   "transaction_id": 42,
    //   "status": "committed",
    //   "applied": 2,
    //   "errors": []
    // }
    try {
        if (req.body().empty()) {
            return makeErrorResponse(http::status::bad_request,
                "Request body is required", req);
        }
        json body = json::parse(req.body());

        // --- Parse isolation level ---
        bool iso_valid = true;
        std::string iso_error;
        IsolationLevel isolation = parseIsolationLevel(body, &iso_valid, &iso_error);
        if (!iso_valid) {
            return makeErrorResponse(http::status::bad_request, iso_error, req);
        }

        // --- Validate operations array ---
        if (!body.contains("operations") || !body["operations"].is_array()) {
            return makeErrorResponse(http::status::bad_request,
                "Missing or invalid 'operations' array", req);
        }
        const json& ops = body["operations"];
        if (ops.empty()) {
            return makeErrorResponse(http::status::bad_request,
                "'operations' array must not be empty", req);
        }

        // --- Begin transaction ---
        auto txn_id = tx_manager_->beginTransaction(isolation);
        auto txn = tx_manager_->getTransaction(txn_id);
        if (!txn) {
            return makeErrorResponse(http::status::internal_server_error,
                "Failed to create transaction", req);
        }

        // --- Apply operations ---
        int applied = 0;
        json errors_array = json::array();

        for (size_t i = 0; i < ops.size(); ++i) {
            const json& op = ops[i];

            if (!op.contains("type") || !op["type"].is_string()) {
                errors_array.push_back({{"index", i}, {"error", "Missing 'type' field"}});
                continue;
            }
            if (!op.contains("table") || !op["table"].is_string()) {
                errors_array.push_back({{"index", i}, {"error", "Missing 'table' field"}});
                continue;
            }
            if (!op.contains("key") || !op["key"].is_string()) {
                errors_array.push_back({{"index", i}, {"error", "Missing 'key' field"}});
                continue;
            }

            const std::string op_type = op["type"].get<std::string>();
            const std::string table   = op["table"].get<std::string>();
            const std::string key     = op["key"].get<std::string>();

            TransactionManager::Status status;
            if (op_type == "put") {
                // Build entity from provided data field
                const json& data = op.value("data", json::object());
                BaseEntity entity = BaseEntity::fromJson(key, data.dump());
                status = txn->putEntity(table, entity);
            } else if (op_type == "delete") {
                status = txn->eraseEntity(table, key);
            } else if (op_type == "optimistic_put") {
                // OCC: write entity only if version matches expected_version
                if (!op.contains("expected_version") || !op["expected_version"].is_number_unsigned()) {
                    errors_array.push_back({{"index", i},
                        {"error", "optimistic_put requires 'expected_version' (unsigned int)"}});
                    continue;
                }
                const uint64_t expected_version = op["expected_version"].get<uint64_t>();
                const json& data = op.value("data", json::object());
                BaseEntity entity = BaseEntity::fromJson(key, data.dump());
                status = txn->optimisticPut(table, entity, expected_version);
            } else if (op_type == "optimistic_erase") {
                // OCC: delete entity only if version matches expected_version
                if (!op.contains("expected_version") || !op["expected_version"].is_number_unsigned()) {
                    errors_array.push_back({{"index", i},
                        {"error", "optimistic_erase requires 'expected_version' (unsigned int)"}});
                    continue;
                }
                const uint64_t expected_version = op["expected_version"].get<uint64_t>();
                status = txn->optimisticErase(table, key, expected_version);
            } else {
                errors_array.push_back({{"index", i},
                    {"error", "Unknown op type '" + op_type + "'. Use 'put', 'delete', 'optimistic_put', or 'optimistic_erase'"}});
                continue;
            }

            if (status.ok) {
                ++applied;
            } else {
                errors_array.push_back({{"index", i}, {"error", status.message}});
            }
        }

        // --- Commit or rollback depending on errors ---
        if (!errors_array.empty()) {
            txn->rollback();
            json resp = {
                {"transaction_id", txn_id},
                {"status", "rolled_back"},
                {"applied", 0},
                {"errors", errors_array}
            };
            return makeResponse(http::status::conflict, resp.dump(2), req);
        }

        auto commit_status = tx_manager_->commitTransaction(txn_id);
        if (!commit_status.ok) {
            json resp = {
                {"transaction_id", txn_id},
                {"status", "failed"},
                {"applied", applied},
                {"error", commit_status.message}
            };
            return makeResponse(http::status::internal_server_error, resp.dump(2), req);
        }

        THEMIS_INFO("Transaction {} committed: {} operation(s) applied", txn_id, applied);
        json resp = {
            {"transaction_id", txn_id},
            {"status", "committed"},
            {"applied", applied},
            {"errors", errors_array}
        };
        return makeResponse(http::status::ok, resp.dump(2), req);

    } catch (const json::exception& e) {
        return makeErrorResponse(http::status::bad_request,
            "Invalid JSON: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error,
            "Error: " + std::string(e.what()), req);
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
            bool iso_valid = true;
            std::string iso_error;
            isolation = parseIsolationLevel(body, &iso_valid, &iso_error);
            if (!iso_valid) {
                return makeErrorResponse(http::status::bad_request, iso_error, req);
            }
        }

        auto txn_id = tx_manager_->beginTransaction(isolation);

        json response = {
            {"transaction_id", txn_id},
            {"isolation", isolationLevelToString(isolation)},
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

http::response<http::string_body> TransactionApiHandler::handleGetVersion(
    const http::request<http::string_body>& req
) {
    // GET /transaction/version
    //
    // Request body:
    // {
    //   "transaction_id": 42,
    //   "table": "users",
    //   "key": "u1"
    // }
    //
    // Response body:
    // { "transaction_id": 42, "table": "users", "key": "u1", "version": 3 }
    //
    // Returns version 0 when the entity does not exist.
    try {
        if (req.body().empty()) {
            return makeErrorResponse(http::status::bad_request,
                "Request body is required", req);
        }
        json body = json::parse(req.body());

        if (!body.contains("transaction_id")) {
            return makeErrorResponse(http::status::bad_request, "Missing 'transaction_id'", req);
        }
        if (!body.contains("table") || !body["table"].is_string()) {
            return makeErrorResponse(http::status::bad_request, "Missing 'table'", req);
        }
        if (!body.contains("key") || !body["key"].is_string()) {
            return makeErrorResponse(http::status::bad_request, "Missing 'key'", req);
        }

        TransactionManager::TransactionId txn_id = body["transaction_id"];
        const std::string table = body["table"].get<std::string>();
        const std::string key   = body["key"].get<std::string>();

        auto txn = tx_manager_->getTransaction(txn_id);
        if (!txn) {
            return makeErrorResponse(http::status::not_found,
                "Transaction " + std::to_string(txn_id) + " not found or already completed", req);
        }

        auto version = txn->getEntityVersion(table, key);
        if (!version.has_value()) {
            return makeErrorResponse(http::status::gone,
                "Transaction " + std::to_string(txn_id) + " is no longer active", req);
        }

        json response = {
            {"transaction_id", txn_id},
            {"table", table},
            {"key", key},
            {"version", *version}
        };
        return makeResponse(http::status::ok, response.dump(2), req);

    } catch (const json::exception& e) {
        return makeErrorResponse(http::status::bad_request,
            "Invalid JSON: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error,
            "Error: " + std::string(e.what()), req);
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
