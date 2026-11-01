/**
 * @file shard_rpc_client_adapter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.34
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// ShardRPCClientAdapter
//
// Bridges the ShardRPCClient (network/gRPC) to the ShardRPCServer::RequestHandler
// interface so that a TwoPhaseCommitCoordinator can drive remote shards over the
// network exactly the same way it drives in-process participants.
//
//   TwoPhaseCommitCoordinator
//          │ calls onPrepare / onCommit / onAbort
//          ▼
//   ShardRPCClientAdapter          ← this file
//          │ translates to network calls
//          ▼
//   ShardRPCClient  ──gRPC/HTTP──▶  remote TwoPhaseCommitParticipant

#pragma once

#include "sharding/shard_rpc_client.h"
#include "sharding/shard_rpc_server.h"
#include <chrono>
#include <memory>
#include <string>
#include <utility>
#include <nlohmann/json.hpp>

namespace themis::sharding {

/**
 * @brief Adapts a ShardRPCClient to the ShardRPCServer::RequestHandler interface.
 *
 * This allows a TwoPhaseCommitCoordinator to treat a remote shard
 * (accessed via gRPC/HTTP through ShardRPCClient) identically to an
 * in-process TwoPhaseCommitParticipant.
 *
 * Thread-safe: the underlying ShardRPCClient handles its own locking.
 *
 * @note Phase-two acknowledgements are propagated. A failed remote ABORT
 *       returns false so the coordinator retains the decision as in-doubt.
 */
class ShardRPCClientAdapter final : public ShardRPCServer::RequestHandler {
public:
    /**
     * @brief Construct an adapter wrapping a remote shard.
     * @param config  ShardRPCClient configuration (endpoint, TLS, timeouts)
     */
    explicit ShardRPCClientAdapter(const ShardRPCClient::Config& config)
        : client_(config)
    {}

    /**
     * @brief Construct an adapter with a response handler for explicit inproc endpoints.
     * @param config ShardRPCClient configuration.
     * @param handler Test/custom response handler; used only for inproc:// or loopback://.
     */
    ShardRPCClientAdapter(
        const ShardRPCClient::Config& config,
        ShardRPCClient::InProcessResponseHandler handler)
        : client_(config) {
        client_.setInProcessResponseHandler(std::move(handler));
    }

    ~ShardRPCClientAdapter() override = default;

    // Non-copyable, non-movable (owns a ShardRPCClient)
    ShardRPCClientAdapter(const ShardRPCClientAdapter&)            = delete;
    ShardRPCClientAdapter& operator=(const ShardRPCClientAdapter&) = delete;

    // ── ShardRPCServer::RequestHandler ────────────────────────────────────────

    /**
     * @brief Translate PREPARE call to a ShardRPCClient::prepare() RPC.
     *
     * Parses the coordinator-serialised @p transaction_data to extract
     * the "operations" array, then sends it over the wire.
     */
    bool onPrepare(
        const std::string& transaction_id,
        const std::string& /*coordinator_shard_id*/,
        const std::string& transaction_data
    ) override {
        nlohmann::json ops = nlohmann::json::array();
        try {
            auto parsed = nlohmann::json::parse(transaction_data);
            ops = parsed.value("operations", nlohmann::json::array());
        } catch (...) {
            // Malformed payload → vote ABORT
            return false;
        }

        try {
            return client_.prepare(transaction_id, ops);
        } catch (...) {
            return false;
        }
    }

    /**
     * @brief Translate COMMIT call to a ShardRPCClient::commit() RPC.
     *
     * Generates a commit timestamp from the coordinator side
     * (wall-clock nanoseconds) to achieve external consistency.
     */
    bool onCommit(const std::string& transaction_id) override {
        const int64_t commit_ts = static_cast<int64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()
        );
        try {
            return client_.commit(transaction_id, commit_ts);
        } catch (...) {
            return false;
        }
    }

    /**
     * @brief Translate ABORT call to a ShardRPCClient::abort() RPC.
     * @return true only when the remote shard acknowledges ABORT; false on a
     *         negative response or transport failure.
     * @note A false result keeps the coordinator's selected ABORT decision
     *       unresolved; retry/recovery policy remains coordinator-specific.
     */
    bool onAbort(const std::string& transaction_id) override {
        try {
            return client_.abort(transaction_id);
        } catch (...) {
            return false;
        }
    }

    /**
     * @brief Perform a liveness check against the remote shard.
     */
    ShardRPCServer::HealthInfo onHealthCheck() override {
        ShardRPCServer::HealthInfo info;
        try {
            info.is_healthy = client_.ping();
        } catch (...) {
            info.is_healthy = false;
        }
        return info;
    }

private:
    ShardRPCClient client_;
};

} // namespace themis::sharding
