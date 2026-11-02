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
 * @note Phase-2 results are acknowledged only when the remote participant
 *       confirms the requested transition.
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
        const std::string& coordinator_shard_id,
        const std::string& transaction_data
    ) override {
        nlohmann::json ops = nlohmann::json::array();
        EpochToken token;
        try {
            auto parsed = nlohmann::json::parse(transaction_data);
            ops = parsed.value("operations", nlohmann::json::array());
            const auto& token_json = parsed.at("fencing_token");
            token.epoch = token_json.at("epoch").get<EpochNumber>();
            token.issuer = token_json.at("issuer").get<NodeId>();
            token.shard_id = token_json.at("shard_id").get<std::string>();
            const auto issued_at_ms = token_json.at("issued_at_ms").get<int64_t>();
            token.issued_at = std::chrono::system_clock::time_point{
                std::chrono::milliseconds{issued_at_ms}};
            if (!ops.is_array() || token.issuer != coordinator_shard_id) {
                return false;
            }
        } catch (...) {
            // Malformed payload → vote ABORT
            return false;
        }

        try {
            return client_.prepare(transaction_id, ops, token);
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
     *
     * Returns false when the remote participant does not acknowledge the
     * abort; idempotency permits a later recovery retry.
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
