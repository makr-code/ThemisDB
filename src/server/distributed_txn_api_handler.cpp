/**
 * @file distributed_txn_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "server/distributed_txn_api_handler.h"
#include "sharding/distributed_transaction.h"
#include "utils/input_validator.h"
#include "utils/logger.h"
#include <cstdlib>
#include <string_view>
#include "utils/tracing.h"

namespace themis::server {

using json = nlohmann::json;

namespace {

constexpr size_t kMaxDistributedTxnIdentifierLength = 256;
constexpr std::string_view kSnapshotIsolationWarning =
    "snapshot_isolation may permit write-skew and phantom-read anomalies; "
    "use 'serializable' for strict invariant safety.";
constexpr std::string_view kDefaultIsolationEnvVar = "THEMIS_DTXN_DEFAULT_ISOLATION";

bool isValidDistributedTxnIdentifier(const std::string& value) {
    themis::utils::InputValidator validator;
    return !value.empty() &&
           validator.validateStringLength(value, kMaxDistributedTxnIdentifierLength) &&
           validator.validatePathSegment(value) &&
           validator.validateHeaderValue(value);
}

sharding::DistributedIsolationLevel getConfiguredDefaultIsolationLevel() {
    const char* configured_default = std::getenv(kDefaultIsolationEnvVar.data());
    if (configured_default == nullptr) {
        return sharding::DistributedIsolationLevel::SERIALIZABLE;
    }

    const std::string configured(configured_default);
    if (configured == "serializable") {
        return sharding::DistributedIsolationLevel::SERIALIZABLE;
    }
    if (configured == "snapshot_isolation") {
        return sharding::DistributedIsolationLevel::SNAPSHOT_ISOLATION;
    }

    THEMIS_WARN("Ignoring invalid {}='{}'; using serializable",
                kDefaultIsolationEnvVar, configured);
    return sharding::DistributedIsolationLevel::SERIALIZABLE;
}

} // namespace

DistributedTxnApiHandler::DistributedTxnApiHandler(
    std::shared_ptr<sharding::DistributedTransactionCoordinator> coordinator
)
    : coordinator_(std::move(coordinator))
{}

// ─────────────────────────────────────────────────────────────────────────────
// POST /dtxn/begin
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body>
DistributedTxnApiHandler::handleBegin([[maybe_unused]] const http::request<http::string_body>& req) {
    try {
    auto span = Tracer::startSpan("handleBegin");
        auto body = json::parse(req.body());

        if (!body.contains("shards") || !body["shards"].is_array()) {
            return error(http::status::bad_request,
                         "Missing or invalid 'shards' array", req);
        }

        std::vector<std::string> shard_ids;
        shard_ids.reserve(body["shards"].size());
        for (const auto& s : body["shards"]) {
            if (!s.is_string()) {
                return error(http::status::bad_request,
                             "Each element of 'shards' must be a string", req);
            }
            auto shard_id = s.get<std::string>();
            if (!isValidDistributedTxnIdentifier(shard_id)) {
                return error(http::status::bad_request,
                             "Invalid shard identifier", req);
            }
            shard_ids.push_back(std::move(shard_id));
        }

        if (shard_ids.empty()) {
            return error(http::status::bad_request,
                         "'shards' must contain at least one shard ID", req);
        }

        // Optional isolation_level: "snapshot_isolation" or "serializable"
        // Default is serializable unless overridden by THEMIS_DTXN_DEFAULT_ISOLATION.
        auto isolation = getConfiguredDefaultIsolationLevel();
        if (body.contains("isolation_level")) {
            const std::string isolation_level_str = body["isolation_level"].get<std::string>();
            if (isolation_level_str == "serializable") {
                isolation = sharding::DistributedIsolationLevel::SERIALIZABLE;
            } else if (isolation_level_str != "snapshot_isolation") {
                return error(http::status::bad_request,
                             "Unknown isolation_level; use 'snapshot_isolation' or 'serializable'",
                             req);
            }
        }

        std::string txn_id = coordinator_->beginTransaction(shard_ids, isolation);

        json response = {
            {"transaction_id",  txn_id},
            {"status",          "active"},
            {"shards",          shard_ids},
            {"isolation_level", isolation == sharding::DistributedIsolationLevel::SERIALIZABLE
                                    ? "serializable" : "snapshot_isolation"}
        };
        if (isolation == sharding::DistributedIsolationLevel::SNAPSHOT_ISOLATION) {
            response["isolation_warning"] = kSnapshotIsolationWarning;
        }
        return ok(response, req);

    } catch (const json::exception& e) {
        return error(http::status::bad_request,
                     std::string("Invalid JSON: ") + e.what(), req);
    } catch (const std::exception& e) {
        return error(http::status::internal_server_error,
                     std::string("Error: ") + e.what(), req);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// POST /dtxn/operation
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body>
DistributedTxnApiHandler::handleOperation([[maybe_unused]] const http::request<http::string_body>& req) {
    try {
    auto span = Tracer::startSpan("handleOperation");
        auto body = json::parse(req.body());

        if (!body.contains("transaction_id")) {
            return error(http::status::bad_request, "Missing 'transaction_id'", req);
        }
        if (!body.contains("shard_id")) {
            return error(http::status::bad_request, "Missing 'shard_id'", req);
        }
        if (!body.contains("operation")) {
            return error(http::status::bad_request, "Missing 'operation'", req);
        }

        std::string txn_id  = body["transaction_id"];
        std::string shard   = body["shard_id"];
        auto        op      = body["operation"];

        if (!isValidDistributedTxnIdentifier(txn_id)) {
            return error(http::status::bad_request, "Invalid 'transaction_id'", req);
        }
        if (!isValidDistributedTxnIdentifier(shard)) {
            return error(http::status::bad_request, "Invalid 'shard_id'", req);
        }

        if (!coordinator_->addOperation(txn_id, shard, op)) {
            return error(http::status::unprocessable_entity,
                         "Failed to add operation – transaction not active or not found", req);
        }

        return ok({{"transaction_id", txn_id}, {"status", "ok"}}, req);

    } catch (const json::exception& e) {
        return error(http::status::bad_request,
                     std::string("Invalid JSON: ") + e.what(), req);
    } catch (const std::exception& e) {
        return error(http::status::internal_server_error,
                     std::string("Error: ") + e.what(), req);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// POST /dtxn/commit
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body>
DistributedTxnApiHandler::handleCommit([[maybe_unused]] const http::request<http::string_body>& req) {
    try {
    auto span = Tracer::startSpan("handleCommit");
        auto body = json::parse(req.body());

        if (!body.contains("transaction_id")) {
            return error(http::status::bad_request, "Missing 'transaction_id'", req);
        }

        std::string txn_id = body["transaction_id"];
        if (!isValidDistributedTxnIdentifier(txn_id)) {
            return error(http::status::bad_request, "Invalid 'transaction_id'", req);
        }
        bool committed = coordinator_->commit(txn_id);

        if (committed) {
            return ok({{"transaction_id", txn_id}, {"status", "committed"}}, req);
        } else {
            return ok({{"transaction_id", txn_id}, {"status", "aborted"},
                       {"message", "Transaction was aborted (prepare phase failed or participant error)"}},
                      req);
        }

    } catch (const json::exception& e) {
        return error(http::status::bad_request,
                     std::string("Invalid JSON: ") + e.what(), req);
    } catch (const std::exception& e) {
        return error(http::status::internal_server_error,
                     std::string("Error: ") + e.what(), req);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// POST /dtxn/abort
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body>
DistributedTxnApiHandler::handleAbort([[maybe_unused]] const http::request<http::string_body>& req) {
    try {
    auto span = Tracer::startSpan("handleAbort");
        auto body = json::parse(req.body());

        if (!body.contains("transaction_id")) {
            return error(http::status::bad_request, "Missing 'transaction_id'", req);
        }

        std::string txn_id = body["transaction_id"];
        if (!isValidDistributedTxnIdentifier(txn_id)) {
            return error(http::status::bad_request, "Invalid 'transaction_id'", req);
        }
        coordinator_->abort(txn_id);

        return ok({{"transaction_id", txn_id}, {"status", "aborted"}}, req);

    } catch (const json::exception& e) {
        return error(http::status::bad_request,
                     std::string("Invalid JSON: ") + e.what(), req);
    } catch (const std::exception& e) {
        return error(http::status::internal_server_error,
                     std::string("Error: ") + e.what(), req);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// POST /dtxn/readonly
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body>
DistributedTxnApiHandler::handleReadOnly([[maybe_unused]] const http::request<http::string_body>& req) {
    try {
    auto span = Tracer::startSpan([[maybe_unused]] "handleReadOnly");
        auto body = json::parse(req.body());

        if (!body.contains("shards") || !body["shards"].is_array()) {
            return error(http::status::bad_request,
                         "Missing or invalid 'shards' array", req);
        }

        std::vector<std::string> shard_ids;
        for (const auto& s : body["shards"]) {
            if (!s.is_string()) {
                return error(http::status::bad_request,
                             "Each element of 'shards' must be a string", req);
            }
            auto shard_id = s.get<std::string>();
            if (!isValidDistributedTxnIdentifier(shard_id)) {
                return error(http::status::bad_request,
                             "Invalid shard identifier", req);
            }
            shard_ids.push_back(std::move(shard_id));
        }

        auto operations = body.value("operations", json::object());

        auto results = coordinator_->executeReadOnly(shard_ids, operations);

        return ok({{"results", results}}, req);

    } catch (const json::exception& e) {
        return error(http::status::bad_request,
                     std::string("Invalid JSON: ") + e.what(), req);
    } catch (const std::exception& e) {
        return error(http::status::internal_server_error,
                     std::string("Error: ") + e.what(), req);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// GET /dtxn/status/{txn_id}
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body>
DistributedTxnApiHandler::handleStatus([[maybe_unused]] const http::request<http::string_body>& req) {
    try {
    auto span = Tracer::startSpan("handleStatus");
        // Extract transaction ID from the URL path: /dtxn/status/{txn_id}
        std::string_view path  = req.target();
        const std::string_view prefix = "/dtxn/status/";
        if (path.size() <= prefix.size()) {
            return error(http::status::bad_request,
                         "Missing transaction ID in path", req);
        }
        std::string txn_id(path.substr(prefix.size()));
        if (!isValidDistributedTxnIdentifier(txn_id)) {
            return error(http::status::bad_request,
                         "Invalid transaction ID in path", req);
        }

        auto state = coordinator_->getTransactionState(txn_id);
        if (!state) {
            return error(http::status::not_found,
                         "Transaction not found: " + txn_id, req);
        }

        return ok({
            {"transaction_id", txn_id},
            {"state",          stateToString(*state)}
        }, req);

    } catch (const std::exception& e) {
        return error(http::status::internal_server_error,
                     std::string("Error: ") + e.what(), req);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// GET /dtxn/stats
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body>
DistributedTxnApiHandler::handleStats([[maybe_unused]] const http::request<http::string_body>& req) {
    try {
    auto span = Tracer::startSpan("handleStats");
        return ok(coordinator_->getStatistics(), req);
    } catch (const std::exception& e) {
        return error(http::status::internal_server_error,
                     std::string("Error: ") + e.what(), req);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body>
DistributedTxnApiHandler::ok(const json& body,
                              const http::request<http::string_body>& req) const {
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server,       "THEMIS");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = body.dump(2);
    res.prepare_payload();
    return res;
}

http::response<http::string_body>
DistributedTxnApiHandler::error(http::status        status,
                                 const std::string&  message,
                                 const http::request<http::string_body>& req) const {
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::server,       "THEMIS");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = json{{"error", true}, {"message", message},
                      {"status_code", static_cast<int>(status)}}.dump();
    res.prepare_payload();
    return res;
}

std::string
DistributedTxnApiHandler::stateToString([[maybe_unused]] sharding::TransactionState state) {
    using TS = sharding::TransactionState;
    switch (state) {
        case TS::ACTIVE:     return "ACTIVE";
        case TS::PREPARING:  return "PREPARING";
        case TS::PREPARED:   return "PREPARED";
        case TS::COMMITTING: return "COMMITTING";
        case TS::COMMITTED:  return "COMMITTED";
        case TS::ABORTING:   return "ABORTING";
        case TS::ABORTED:    return "ABORTED";
        default:             return "UNKNOWN";
    }
}

} // namespace themis::server
